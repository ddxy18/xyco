#![feature(assert_matches)]

mod commit_msg;
mod pre_commit;

use self::commit_msg::CommitMessage;
use self::pre_commit::SubCheck;

use anyhow::Result;

#[derive(structopt::StructOpt)]
enum Command {
    CommitMessage(CommitMessage),
    Check(SubCheck),
}

fn main() -> Result<()> {
    let cmd: Command = <Command as structopt::StructOpt>::from_args();
    match cmd {
        Command::CommitMessage(msg) => msg.check(),
        Command::Check(check_args) => check_args.check(),
    }
}
