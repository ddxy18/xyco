#![feature(assert_matches)]

mod pre_commit;

use self::pre_commit::SubCheck;

use anyhow::Result;

#[derive(structopt::StructOpt)]
enum Command {
    Check(SubCheck),
}

fn main() -> Result<()> {
    let cmd: Command = <Command as structopt::StructOpt>::from_args();
    match cmd {
        Command::Check(check_args) => check_args.check(),
    }
}
