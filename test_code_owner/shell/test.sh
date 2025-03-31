changed_files=$(git diff-tree --no-commit-id --name-only -r HEAD)

code_owners=$(cat CODE_OWNERS)

python3 tools/code_review/test.py --code_owners "$code_owners" --change_files "$changed_files" --repo_name "111" --pr_number "222"