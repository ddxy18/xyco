use std::{fs, io::Read, path::PathBuf};

use anyhow::Result;
use regex::Regex;

#[derive(structopt::StructOpt)]
pub struct CommitMessage {
    #[structopt(long)]
    path: PathBuf,
}

#[derive(Debug, PartialEq)]
pub enum MsgError {
    General,
    Header,
    Revert,
    LenLimit,
    CloseIssue,
    Subject,
}

impl std::error::Error for MsgError {}

impl std::fmt::Display for MsgError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            MsgError::General => {
                write!(
                    f,
                    "\ncommit message format:\n\
                    <type>(<scope>): <subject>\n\
                    <BLANK LINE>\n\
                    [<body>]\n\
                    <BLANK LINE>\n\
                    [<footer>]"
                )
            }
            MsgError::Header => {
                write!(
                    f,
                    "\n<header> format: <type>[<scope>]: <subject>\n\
                    <type> should be one of the follows:\n\
                    feat (feature)\n\
                    fix (bug fix)\n\
                    docs (documentation)\n\
                    style (formatting, missing semi colons, …)\n\
                    refactor\n\
                    test (when adding missing tests)\n\
                    chore (maintain)"
                )
            }
            MsgError::Revert => {
                write!(
                    f,
                    "\nrevert commit format:\n\
                    revert: <reverted commit header>\n\
                    <BLANK LINE>\n\
                    This reverts commit <commit hash>."
                )
            }
            MsgError::LenLimit => {
                write!(
                    f,
                    "Any line of the commit message cannot be longer 100 characters!"
                )
            }
            MsgError::CloseIssue => {
                write!(
                    f,
                    "\nCloses issue format:\n\
                     Closes #<num> [, #<num>...]"
                )
            }
            MsgError::Subject => {
                write!(
                    f,
                    "\n<subject> must be provided and should satisfy follow requirements:\n\
                    - use imperative, present tense: \"change\" not \"changed\" nor \"changes\"\n\
                    - don't capitalize first letter\n\
                    - no dot (.) at the end"
                )
            }
        }
    }
}

struct Header<'a> {
    header: &'a str,
}

impl CommitMessage {
    pub fn check(&self) -> Result<()> {
        let mut contents = String::new();
        fs::File::open(&self.path)?.read_to_string(&mut contents)?;

        let mut contents = contents
            .lines()
            .filter_map(|line| {
                if !line.starts_with('#') {
                    let mut line = line.to_string();
                    // Replace all line endings wuth '\n' so we don't have to care about the difference.
                    line.push('\n');
                    return Some(line);
                }
                None
            })
            .fold(String::new(), |mut acc, line| {
                acc.push_str(&line);
                acc
            });

        contents
            .lines()
            .try_for_each(|line| (line.len() <= 100).then_some(()).ok_or(MsgError::LenLimit))?;

        let (header, contents) = match contents.split_once("\n\n") {
            Some((header, contents)) => (header, Some(contents)),
            None => {
                // header only
                if contents.lines().count() == 1 {
                    contents.pop();
                    (contents.as_str(), None)
                } else {
                    return Err(MsgError::General.into());
                }
            }
        };

        let header = Header::new(header);
        header.check()?;
        if header.is_revert() {
            Regex::new(r"This reverts commit [[:alnum:]]{40}\.")?
                .is_match(contents.ok_or(MsgError::Revert)?)
                .then_some(())
                .ok_or(MsgError::Revert)?;
        }

        let contents = match contents {
            Some(contents) => contents,
            None => {
                return Ok(());
            }
        };

        let footer = match contents.rsplit_once("\n\n") {
            Some((_, footer)) => footer,
            None => contents, // empty body
        };
        for line in footer.lines().filter_map(|line| {
            let mut lines = line.split_whitespace();
            if lines.next().unwrap_or_default().eq("Closes") {
                return Some(lines.collect::<String>());
            }
            None
        }) {
            // check close issue
            for issue in line.split_terminator(',') {
                Regex::new(r"^#\d+$")?
                    .is_match(issue)
                    .then_some(())
                    .ok_or(MsgError::CloseIssue)?;
            }
        }

        Ok(())
    }
}

impl<'a> Header<'a> {
    fn new(header: &'a str) -> Self {
        Self { header }
    }

    fn check(&self) -> Result<()> {
        let (ty, subject) = self
            .header
            .split_once(':')
            .map(|(ty, subject)| (ty, subject.trim_start()))
            .ok_or(MsgError::Header)?;

        if ty == "revert" {
            return Ok(Self::new(subject).check().map_err(|_| MsgError::Revert)?);
        }
        Regex::new(r"^(?:feat|fix|docs|style|refactor|test|chore)(?:\([[:alpha:]]+\))*$")?
            .is_match(ty)
            .then_some(())
            .ok_or(MsgError::Header)?;

        (!subject.is_empty() && subject.starts_with(char::is_lowercase) && !subject.ends_with('.'))
            .then_some(())
            .ok_or(MsgError::Subject)?;

        Ok(())
    }

    fn is_revert(&self) -> bool {
        self.header
            .split_once(':')
            .map(|(ty, _)| ty)
            .unwrap_or_default()
            == "revert"
    }
}

#[cfg(test)]
mod tests {
    use std::{assert_matches::assert_matches, io::Write};

    use super::*;

    use tempfile::NamedTempFile;

    #[test]
    fn test_break_change() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file
            .write_all("fix: xxxxxx\n\nxxxx\n\nBREAKING CHANGE:xxxx\n".as_bytes())
            .unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert!(msg.check().is_ok());
    }

    #[test]
    fn test_header_only() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all("feat: xxxxxx".as_bytes()).unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert!(msg.check().is_ok());
    }

    #[test]
    fn test_revert() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all(
            "revert: feat: xxxxxx\n\nThis reverts commit 667ecc1654a317a13331b17617d973392f415f02.".as_bytes()
        ).unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert!(msg.check().is_ok());
    }

    #[test]
    fn test_close_issue() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file
            .write_all("fix: xxxxxx\n\nCloses #234\nxxxx".as_bytes())
            .unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert!(msg.check().is_ok());
    }

    #[test]
    fn test_wrong_type() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all("bugfix: xxxxxx".as_bytes()).unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::Header);
    }

    #[test]
    fn test_too_long_header() {
        let content = (0..100).fold(String::from("fix: "), |mut acc, _| {
            acc.push('x');
            acc
        });
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all(content.as_bytes()).unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::LenLimit);
    }

    #[test]
    fn test_subject_end_with_dot() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all("fix: xxx.".as_bytes()).unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::Subject);
    }

    #[test]
    fn test_subject_capitalize_first_letter() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all("fix: Xxx".as_bytes()).unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::Subject);
    }

    #[test]
    fn test_empty_subject() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all("fix: ".as_bytes()).unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::Subject);
    }

    #[test]
    fn test_lack_blank_line() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file
            .write_all("fix: xxx.\nxxxxxxxx".as_bytes())
            .unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::General);
    }

    #[test]
    fn test_invalid_close_issue() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file
            .write_all("fix: xxx\n\nCloses 123".as_bytes())
            .unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::CloseIssue);
    }

    #[test]
    fn test_invalid_revert_subject() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all("revert: bugfix: xxxxxx\n\nThis reverts commit 667ecc1654a317a13331b17617d973392f415f02.".as_bytes()).unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::Revert);
    }

    #[test]
    fn test_revert_lack_footer() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file
            .write_all("revert: fix: xxxxxx".as_bytes())
            .unwrap();

        let msg = CommitMessage {
            path: temp_file.path().to_path_buf(),
        };

        assert_matches!(msg.check().err(), Some(err) if *err.downcast_ref::<MsgError>().unwrap() == MsgError::Revert);
    }
}
