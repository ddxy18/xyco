use std::{
    io::Write,
    path::PathBuf,
    process::{Command, Stdio},
};

use anyhow::{anyhow, Result};

#[derive(structopt::StructOpt)]
pub enum SubCheck {
    ClangFormat(ClangFmt),
    ClangTidy(ClangTidy),
}

impl SubCheck {
    pub fn check(&self) -> Result<()> {
        match self {
            SubCheck::ClangFormat(fmt) => fmt.check(),
            SubCheck::ClangTidy(tidy) => tidy.check(),
        }
    }
}

pub trait FileCheck {
    fn per_file_check(&self, path: PathBuf) -> Result<()>;

    fn get_root_path(&self) -> PathBuf;

    fn sub_dir_self(&self, path: PathBuf) -> Self;

    fn check(&self) -> Result<()>
    where
        Self: Sized,
    {
        let errs = std::fs::read_dir(&self.get_root_path())?
            .filter_map(|entry| {
                if let Ok(entry) = entry {
                    if entry.file_type().ok()?.is_dir() {
                        return self.sub_dir_self(entry.path()).check().err();
                    } else if matches!(entry.path().extension()?.to_str()?, "cc" | "h") {
                        return self.per_file_check(entry.path()).err();
                    }
                }
                None
            })
            .fold(vec![], |mut acc, e| {
                match e.downcast::<FileError>() {
                    Ok(e) => {
                        acc.push(e);
                    }
                    Err(e) => {
                        acc.push(FileError {
                            path: PathBuf::new(),
                            err: ErrorType::Other(e),
                        });
                    }
                };
                acc
            });
        errs.is_empty()
            .then_some(())
            .ok_or_else(|| DiffsError { errs }.into())
    }
}

#[derive(structopt::StructOpt)]
pub struct ClangFmt {
    #[structopt(short = "s", long = "source")]
    path: PathBuf,
}

#[derive(structopt::StructOpt)]
pub struct ClangTidy {
    #[structopt(short, long = "build")]
    build_path: PathBuf,
    #[structopt(short = "s", long = "source")]
    root_path: PathBuf,
    #[structopt(long)]
    checks: Option<String>,
}

impl FileCheck for ClangFmt {
    fn per_file_check(&self, path: PathBuf) -> Result<()> {
        let clang_format = Command::new("clang-format")
            .arg("--Werror")
            .arg("--style=file")
            .arg(&path)
            .stdout(Stdio::piped())
            .spawn()?;
        let mut diff = Command::new("diff")
            .arg(&path)
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .arg("-")
            .spawn()?;

        let mut stdin = diff
            .stdin
            .take()
            .ok_or_else(|| anyhow!("cannot take stdin from diff"))?;
        std::thread::spawn(move || {
            stdin.write_all(&clang_format.wait_with_output().unwrap().stdout)
        });

        let output = diff.wait_with_output()?;
        output.status.success().then_some(()).ok_or_else(|| {
            FileError {
                path: path.clone(),
                err: ErrorType::Diff(FileDiffError {
                    diffs: String::from_utf8(output.stdout).unwrap_or_default(),
                }),
            }
            .into()
        })
    }

    fn get_root_path(&self) -> PathBuf {
        self.path.clone()
    }

    fn sub_dir_self(&self, path: PathBuf) -> Self {
        Self { path }
    }
}

impl FileCheck for ClangTidy {
    fn per_file_check(&self, path: PathBuf) -> Result<()> {
        let mut cmd = Command::new("clang-tidy");
        #[cfg(test)]
        let mut cmd = cmd.stdout(Stdio::null()).stderr(Stdio::null());
        let cmd = if let Some(checks) = &self.checks {
            cmd.arg(format!("--checks={}", checks))
        } else {
            &mut cmd
        };
        cmd.arg(format!("-p={}", self.build_path.to_string_lossy()))
            .arg("--warnings-as-errors")
            .arg("*")
            .arg(&path)
            .spawn()?
            .wait()?
            .success()
            .then_some(())
            .ok_or_else(|| {
                FileError {
                    path: path.clone(),
                    err: ErrorType::Other(anyhow!("")),
                }
                .into()
            })
    }

    fn get_root_path(&self) -> PathBuf {
        self.root_path.clone()
    }

    fn sub_dir_self(&self, path: PathBuf) -> Self {
        Self {
            build_path: self.build_path.clone(),
            root_path: path,
            checks: self.checks.clone(),
        }
    }
}

#[derive(Debug)]
struct DiffsError {
    errs: Vec<FileError>,
}

#[derive(Debug)]
struct FileError {
    path: PathBuf,
    err: ErrorType,
}

#[derive(Debug)]
enum ErrorType {
    Diff(FileDiffError),
    Other(anyhow::Error),
}

#[derive(Debug)]
struct FileDiffError {
    diffs: String,
}

impl std::error::Error for DiffsError {}

impl std::error::Error for FileError {}

impl std::error::Error for ErrorType {}

impl std::error::Error for FileDiffError {}

impl std::fmt::Display for DiffsError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.errs.iter().try_for_each(|e| write!(f, "\n{}", e))
    }
}

impl std::fmt::Display for FileError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}\n{}", self.path.to_string_lossy(), self.err)
    }
}

impl std::fmt::Display for ErrorType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ErrorType::Diff(e) => {
                write!(f, "{}", e)
            }
            ErrorType::Other(e) => {
                write!(f, "{}", e)
            }
        }
    }
}

impl std::fmt::Display for FileDiffError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use colored::Colorize;

        let colored_diffs = self.diffs.lines().fold(String::new(), |acc, line| {
            let line = if line.starts_with('<') {
                line.red()
            } else if line.starts_with('>') {
                line.green()
            } else {
                line.normal()
            };
            format!("{}\n{}", acc, line)
        });

        writeln!(f, "{}", colored_diffs)
    }
}

#[cfg(test)]
mod tests {
    use std::{assert_matches::assert_matches, fs::File};

    use super::*;

    use tempfile::tempdir;

    #[test]
    fn test_fmt_cc_file() {
        let temp_dir = tempdir().unwrap();
        let file_path = temp_dir.path().join("main.cc");
        let mut file = File::create(file_path).unwrap();
        file.write_all(
            "#include <iostream>\n\nint main(int, char **) {\nstd::cout << \"Hello, world!\\n\";std::cout << \"Hello, world!\\n\";\n}"
                .as_bytes(),
        )
        .unwrap();

        let pre_commit = ClangFmt {
            path: temp_dir.path().to_path_buf(),
        };
        if let ErrorType::Diff(e) = &pre_commit
            .check()
            .err()
            .unwrap()
            .downcast_ref::<DiffsError>()
            .unwrap()
            .errs[0]
            .err
        {
            assert_eq!(e.diffs.lines().count(), 5);
        } else {
            panic!()
        }
    }

    #[test]
    fn test_fmt_h_file() {
        let temp_dir = tempdir().unwrap();
        let file_path = temp_dir.path().join("header.h");
        let mut file = File::create(file_path).unwrap();
        file.write_all("#include <iostream>".as_bytes()).unwrap();

        let pre_commit = ClangFmt {
            path: temp_dir.path().to_path_buf(),
        };

        assert!(pre_commit.check().is_ok());
    }

    #[test]
    fn test_fmt_two_file() {
        let temp_dir = tempdir().unwrap();
        let first_path = temp_dir.path().join("main1.cc");
        let mut file1 = File::create(first_path).unwrap();
        let second_path = temp_dir.path().join("main2.cc");
        let mut file2 = File::create(second_path).unwrap();
        file1.write_all(
            "#include <iostream>\n\nint main(int, char **) {\nstd::cout << \"Hello, world!\\n\";std::cout << \"Hello, world!\\n\";\n}"
                .as_bytes(),
        )
        .unwrap();
        file2.write_all(
            "#include <iostream>\n\nint main(int, char **) {\nstd::cout << \"Hello, world!\\n\";std::cout << \"Hello, world!\\n\";\n}"
                .as_bytes(),
        )
        .unwrap();

        let pre_commit = ClangFmt {
            path: temp_dir.path().to_path_buf(),
        };

        assert_matches!(pre_commit.check().err(),Some(err) if err.downcast_ref::<DiffsError>().unwrap().errs.len() == 2);
    }

    #[test]
    fn test_tidy_cc_file() {
        let temp_dir = tempdir().unwrap();
        let file_path = temp_dir.path().join("main.cc");
        let mut file = File::create(file_path).unwrap();
        let cmake_file_path = temp_dir.path().join("CMakeLists.txt");
        let mut cmake_file = File::create(cmake_file_path).unwrap();

        file.write_all(
            "#include <iostream>\n\nint main(int, char **) {\nstd::cout << \"Hello, world!\\n\";std::cout << \"Hello, world!\\n\";\n}"
                .as_bytes(),
        )
        .unwrap();
        cmake_file
            .write_all("project(test LANGUAGES CXX)\n\nadd_executable(test main.cc)".as_bytes())
            .unwrap();
        Command::new("cmake")
            .arg(temp_dir.path())
            .arg("-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE")
            .arg("-DCMAKE_CXX_COMPILER:FILEPATH=/bin/clang++-11")
            .arg("-S")
            .arg(temp_dir.path())
            .arg("-B")
            .arg(temp_dir.path().join("build"))
            .stdout(Stdio::null())
            .spawn()
            .unwrap()
            .wait()
            .unwrap();

        let pre_commit = ClangTidy {
            root_path: temp_dir.path().to_path_buf(),
            build_path: temp_dir.path().join("build"),
            checks: Some(String::from("modernize-*")),
        };
        assert_matches!(pre_commit.check().err(),Some(e) if e.downcast_ref::<DiffsError>().unwrap().errs.len() == 1);
    }
}
