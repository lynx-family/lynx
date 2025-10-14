# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import argparse

def main():
  parser = argparse.ArgumentParser(description='Write content to a file if it has changed.')
  parser.add_argument('--output', required=True, help='Path to the output file')
  parser.add_argument('--content', required=True, help='Content to write to the file')
  args = parser.parse_args()

  content = args.content.replace('\\n', '\n')

  if os.path.exists(args.output):
    with open(args.output, 'r') as file:
      existing_content = file.read()
    if existing_content == content:
      print(f"Content in {args.output} is already up to date. No changes made.")
      return

  with open(args.output, 'w') as file:
    file.write(content)
  print(f"Content written to {args.output}")

if __name__ == "__main__":
  main()
