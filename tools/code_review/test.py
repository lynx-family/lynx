import argparse
import code
import json
import fnmatch
import subprocess
import sys
from github_utils import *

APP_ID = 372156

def get_changed_files():
    command = ["git", "diff-tree", "--no-commit-id", "--name-only", "-r", "HEAD"]
    # Execute the command and capture the output
    result = subprocess.run(command, capture_output=True, text=True)
    # Check if the command succeeded
    if result.returncode == 0:
        return result.stdout.splitlines()
    
    return None

def parse_codeowners(code_owners_list: list):
    code_owners = {}
    for line in code_owners_list:
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        parts = line.split(" ")
        pattern = parts[0]
        owners = [owner.replace('@', '') for owner in parts[1:]]
        code_owners[pattern] = owners
    return code_owners

def is_fnmatch_pattern(pattern):
    fnmatch_special_chars = r'*?[]'
    return any(char in pattern for char in fnmatch_special_chars)

def match_codeowners(file_paths, codeowners):
    matches = {}
    for file_path in file_paths:
        matched_rules = {}
        for pattern, owners in codeowners.items():
            if is_fnmatch_pattern(pattern):
                if fnmatch.fnmatch(file_path, pattern):
                    matched_rules[pattern] = owners
            else:
                if file_path.startswith(pattern):
                    matched_rules[pattern] = owners
        if matched_rules:
            sorted_patterns = sorted(matched_rules.keys(), key=len, reverse=True)
            matches[file_path] = matched_rules[sorted_patterns[0]]
        else:
            matches[file_path] = []
    return matches

def main():
    parser = argparse.ArgumentParser()
    # parser.add_argument('--private_key', type=str, required=True)
    # parser.add_argument('--client_id', type=str, required=True)
    # parser.add_argument('--app_id', type=str, required=True)
    parser.add_argument('--pr_number', type=str, required=True)
    parser.add_argument('--repo_name', type=str, required=True)
    parser.add_argument('--change_files', type=str, required=True)
    parser.add_argument('--code_owners', type=str, required=True)

    args = parser.parse_args()

    # private_key = args.private_key
    # client_id = args.client_id
    # app_id = args.app_id
    code_owners_list = args.code_owners.split("\n")
    changed_files_list = ['/' + line for line in args.change_files.split('\n')]
    # jwt_token = generate_jwt_token(private_key, client_id)
    # access_token = generate_access_token(jwt_token, app_id)
    reviewers = set()
    match_result = match_codeowners(changed_files_list, parse_codeowners(code_owners_list))
    for _, owners in match_result.items():
        if owners:
            reviewers.update(owners)
    print(f"reviewers: {list(reviewers)} repo_name: {args.repo_name} pr_number: {args.pr_number}")
    # add_reviewers(access_token, args.repo_name, args.pr_number, list(reviewers))

if __name__ == "__main__":
    sys.exit(main())