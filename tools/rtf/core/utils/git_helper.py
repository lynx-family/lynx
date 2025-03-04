# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import re
import subprocess

from core.env.env import RTFEnv
from core.utils.log import Log


class GitHelper:
    @staticmethod
    def GetChangedFiles(commit_count=1):
        current_commit_id = GitHelper.GetCommitID(0)
        base_commit_id = GitHelper.GetCommitID(commit_count)
        outputs = subprocess.check_output(
            f"git diff  {base_commit_id} {current_commit_id} --name-only", shell=True
        )
        return outputs.decode("utf-8").strip().split("\n")

    @staticmethod
    def GetFileChangedLines(file, commit_count=1):
        current_commit_id = GitHelper.GetCommitID(0)
        base_commit_id = GitHelper.GetCommitID(commit_count)
        outputs = subprocess.check_output(
            f"git diff {base_commit_id} {current_commit_id} -U0 {file}",
            shell=True,
            stderr=subprocess.DEVNULL,
        )
        lines = outputs.decode("utf-8").strip().split("\n")
        change_lines = []
        for line in lines:
            if line.startswith("@@"):
                matches = re.match(r"@@ -\d+(,\d+)? \+(\d+),?(\d+)? @@.*", line)
                if not matches:
                    continue
                change_line_start_number = int(matches.groups()[1])
                change_line_end_number = matches.groups()[2]
                if change_line_end_number is None:
                    change_lines.append(change_line_start_number)
                else:
                    change_lines += [
                        i + change_line_start_number
                        for i in range(int(change_line_end_number))
                    ]
        return change_lines

    @staticmethod
    def GetCommitID(index):
        outputs = subprocess.check_output(
            f"git log --format='%H' --max-count={index+1}", shell=True
        )
        format_outputs = outputs.decode("utf-8").strip().split("\n")
        commits = [i.strip() for i in format_outputs]
        if len(commits) == 0:
            Log.fatal(f"Not found commit id for index {index}")
        if index < len(commits):
            return commits[index]
        return commits[-1]


if __name__ == "__main__":
    print(GitHelper.GetChangedFiles(4))
    print(GitHelper.GetFileChangedLines("../../core/checker/coverage_checker.py", 100))
