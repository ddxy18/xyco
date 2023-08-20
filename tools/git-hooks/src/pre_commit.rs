use std::{
    io::{stdout, Write},
    path::PathBuf,
    process::{Command, Stdio},
    sync::Arc,
    thread,
};

use anyhow::{anyhow, Result};

#[derive(structopt::StructOpt)]
pub enum SubCheck {
    ClangFormat(ClangFmt),
    ClangTidy(ClangTidy),
}

impl SubCheck {
    pub fn check(self) -> Result<()> {
        match self {
            SubCheck::ClangFormat(fmt) => ClangFmt::check(Arc::new(fmt)),
            SubCheck::ClangTidy(tidy) => ClangTidy::check(Arc::new(tidy)),
        }
    }
}

fn get_source_paths(root_path: PathBuf) -> Vec<PathBuf> {
    if let Ok(dir) = std::fs::read_dir(root_path) {
        return dir
            .filter_map(|entry| {
                if let Ok(entry) = entry {
                    if entry.file_type().ok()?.is_dir() {
                        return Some(get_source_paths(entry.path()));
                    } else if matches!(entry.path().extension()?.to_str()?, "cc" | "h") {
                        return Some(vec![entry.path()]);
                    }
                }
                None
            })
            .fold(vec![], |mut acc, mut v| {
                acc.append(&mut v);
                acc
            });
    }
    vec![]
}

pub trait FileCheck {
    fn per_file_check(me: Arc<Self>, path: PathBuf) -> Result<()>;

    fn get_root_path(me: Arc<Self>) -> Vec<PathBuf>;

    fn sub_dir_self(me: Arc<Self>, path: Vec<PathBuf>) -> Self;

    #[allow(clippy::manual_try_fold)]
    fn check(me: Arc<Self>) -> Result<()>
    where
        Self: Sized,
        Self: Sync,
        Self: Send,
        Self: 'static,
    {
        let paths = Self::get_root_path(me.clone())
            .into_iter()
            .map(get_source_paths)
            .fold(vec![], |acc: Vec<PathBuf>, paths| [acc, paths].concat());
        let n = std::cmp::min(paths.len(), num_cpus::get());

        let mut out = vec![];
        let _ = writeln!(out, "run with {} threads", n);

        let _ = writeln!(out, "check files:[");
        for path in paths.iter() {
            let _ = writeln!(out, "{:?}", path);
        }
        let _ = writeln!(out, "]");
        let _ = stdout().write(out.as_slice());

        let mut handles = vec![];

        for i in 0..n {
            let paths = if i == n - 1 {
                paths[i * paths.len() / n..paths.len()].to_vec()
            } else {
                paths[i * paths.len() / n..(i + 1) * paths.len() / n].to_vec()
            };
            let me = me.clone();
            let handle = thread::spawn(move || {
                let mut errs = vec![];
                for path in paths {
                    if let Err(e) = Self::per_file_check(me.clone(), path.to_owned()) {
                        match e.downcast::<FileError>() {
                            Ok(e) => {
                                errs.push(e);
                            }
                            Err(e) => {
                                errs.push(FileError {
                                    path: PathBuf::new(),
                                    err: ErrorType::Other(e),
                                });
                            }
                        };
                    }
                }
                errs.is_empty().then_some(()).ok_or(DiffsError { errs })
            });
            handles.push(handle);
        }
        handles
            .into_iter()
            .map(|handle| handle.join().unwrap())
            .fold(Ok(()), |acc: Result<(), DiffsError>, r| {
                if let Err(mut errs) = r {
                    if let Err(mut acc_err) = acc {
                        errs.errs.append(&mut acc_err.errs);
                        return Err(errs);
                    }
                    return Err(errs);
                }
                acc
            })
            .map_err(|e| e.into())
    }
}

#[derive(structopt::StructOpt)]
pub struct ClangFmt {
    #[structopt(short = "s", long = "source")]
    path: Vec<PathBuf>,
    #[structopt(long)]
    tool_version: Option<String>,
}

#[derive(structopt::StructOpt)]
pub struct ClangTidy {
    #[structopt(short, long = "build")]
    build_path: PathBuf,
    #[structopt(short = "s", long = "source")]
    root_path: Vec<PathBuf>,
    #[structopt(long)]
    checks: Option<String>,
    #[structopt(long)]
    tool_version: Option<String>,
    #[structopt(long)]
    extra_arg: String,
}

impl FileCheck for ClangFmt {
    fn per_file_check(me: Arc<Self>, path: PathBuf) -> Result<()> {
        let clang_format = Command::new(if me.tool_version.is_none() {
            String::from("clang-format")
        } else {
            format!("clang-format-{}", me.tool_version.clone().unwrap())
        })
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

    fn get_root_path(me: Arc<Self>) -> Vec<PathBuf> {
        me.path.clone()
    }

    fn sub_dir_self(me: Arc<Self>, path: Vec<PathBuf>) -> Self {
        Self {
            path,
            tool_version: me.tool_version.clone(),
        }
    }
}

impl FileCheck for ClangTidy {
    fn per_file_check(me: Arc<Self>, path: PathBuf) -> Result<()> {
        let cmd = &mut Command::new(if me.tool_version.is_none() {
            String::from("clang-tidy")
        } else {
            format!("clang-tidy-{}", me.tool_version.clone().unwrap())
        });
        #[cfg(test)]
        let cmd = cmd.stdout(Stdio::null()).stderr(Stdio::null());
        let cmd = if let Some(checks) = &me.checks {
            cmd.arg(format!("--checks={}", checks))
        } else {
            cmd
        };
        cmd.arg(format!("-p={}", me.build_path.to_string_lossy()))
            .arg(format!("-extra-arg={}", me.extra_arg.as_str()))
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

    fn get_root_path(me: Arc<Self>) -> Vec<PathBuf> {
        me.root_path.clone()
    }

    fn sub_dir_self(me: Arc<Self>, path: Vec<PathBuf>) -> Self {
        Self {
            build_path: me.build_path.clone(),
            root_path: path,
            checks: me.checks.clone(),
            tool_version: me.tool_version.clone(),
            extra_arg: me.extra_arg.clone(),
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
            path: vec![temp_dir.path().to_path_buf()],
            tool_version: None,
        };
        if let ErrorType::Diff(e) = &ClangFmt::check(Arc::new(pre_commit))
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
            path: vec![temp_dir.path().to_path_buf()],
            tool_version: None,
        };

        ClangFmt::check(Arc::new(pre_commit)).unwrap();
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
            path: vec![temp_dir.path().to_path_buf()],
            tool_version: None,
        };

        assert_matches!(ClangFmt::check(Arc::new(pre_commit)).err(),Some(err) if err.downcast_ref::<DiffsError>().unwrap().errs.len() == 2);
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
            .arg("-DCMAKE_CXX_COMPILER:FILEPATH=clang++")
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
            root_path: vec![temp_dir.path().to_path_buf()],
            build_path: temp_dir.path().join("build"),
            checks: Some(String::from("modernize-*")),
            tool_version: None,
            extra_arg: String::from("-I/usr/lib/llvm-14/include/c++/v1/"),
        };
        assert_matches!(ClangTidy::check(Arc::new(pre_commit)).err(),Some(e) if e.downcast_ref::<DiffsError>().unwrap().errs.len() == 1);
    }
}
