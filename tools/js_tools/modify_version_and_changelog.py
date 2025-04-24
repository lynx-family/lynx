# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json
import os
import requests
import sys
import subprocess
import tarfile
import io
import re
from packaging import version

def get_package_name(project_dir):
  package_json_path = os.path.join(project_dir, 'package.json')
  try:
    with open(package_json_path, 'r', encoding='utf-8') as f:
      package_info = json.load(f)
      package_name = package_info.get('name')
      if package_name:
        print(f"package_name: {package_name}")
        return package_name
      else:
        print(f"Parse package_name failed: {package_name}")
        sys.exit(1)
  except Exception as e:
    print(f"Parse package_name Error: {e}")
    sys.exit(1)

def get_current_commit_title():
  try:
    result = subprocess.run(['git', 'log', '-1', '--pretty=format:%s'], capture_output=True, text=True, check=True)
    commit_title = result.stdout.strip()
    print(f"commit_title: {commit_title}")
    return commit_title
  except Exception as e:
    print(f"Git execute error: {e.stderr.strip()}")
    sys.exit(1)

def get_major_minor_from_file(version_path):
  try:
    with open(version_path, 'r') as f:
      package_info = json.load(f)
    version = package_info.get('version')
    if version:
      parts = version.split('.')
      version = '.'.join(parts[:2])
      print(f"major_minor: {version}")
      return version
  except Exception as e:
    print(f"Parse major_minor error: {e}")
    sys.exit(1)

def get_latest_matching_version_and_tarball(major_minor, package_name):
  url = f'https://registry.npmjs.org/{package_name}'
  try:
    response = requests.get(url)
    response.raise_for_status()
    data = response.json()
    versions = list(data.get('versions', {}).keys())
    release_versions = []
    for ver_str in versions:
      try:
        ver = version.parse(ver_str)
        if isinstance(ver, version.Version) and not ver.is_prerelease:
          release_versions.append(ver_str)
      except version.InvalidVersion:
        continue    
    matching_versions = [v for v in release_versions if v.startswith(major_minor + '.')]
    if matching_versions:
      matching_versions.sort(key=version.parse)
      latest_version = matching_versions[-1]
    else:
      release_versions.sort(key=version.parse)
      latest_version = release_versions[-1]
    tarball_url = data["versions"][latest_version]["dist"]["tarball"]
    if(latest_version and tarball_url):
      print(f"Get version or tarball link success: latest_version: {latest_version} , tarball_url: {tarball_url}")
      return latest_version, tarball_url
    else:
      print(f"Get version or tarball link failed: latest_version: {latest_version} , tarball_url: {tarball_url}")
      sys.exit(1)
  except Exception as e:
    print(f"Get version or tarball link error: {e}")
    sys.exit(1)

def extract_changelog(tarball_url):
  try:
    response = requests.get(tarball_url)
    response.raise_for_status()
    with tarfile.open(fileobj=io.BytesIO(response.content), mode="r:gz") as tar:
      changelog_files = [
        member for member in tar.getmembers()
        if "CHANGELOG" in member.name.upper()
      ]
      if changelog_files:
        changelog_file = tar.extractfile(changelog_files[0])
        return changelog_file.read().decode("utf-8")
      else:
        return """"#CHANGELOG
        """
  except Exception as e:
    print(f"Extract CHANGELOG Error: {e}")
    sys.exit(1)

def increment_patch_version(version):
    parts = list(map(int, version.split('.')))
    parts[2] += 1
    return '.'.join(map(str, parts))

def append_to_changelog(project_dir, content):
  changelog_path = os.path.join(project_dir, 'CHANGELOG.md')
  with open(changelog_path, 'w', encoding='utf-8') as f:
    f.write(content + '\n')


def write_version_to_package_json(version, package_json_path):
  try:
    with open(package_json_path, 'r', encoding='utf-8') as f:
      data = json.load(f)
    data['version'] = version
    with open(package_json_path, 'w', encoding='utf-8') as f:
      json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"version {version} write to {package_json_path}")
  except Exception as e:
    print(f"write new version failed: {e}")
    sys.exit(1)

def main():
  
  if len(sys.argv) < 3:
    print("Please provide version file and project dir")
    sys.exit(1)
    
  major_minor_version_path = sys.argv[1]
  project_dir = sys.argv[2]
  
  package_name = get_package_name(project_dir)
  major_minor = get_major_minor_from_file(major_minor_version_path)
  
  latest_version, tarball_url = get_latest_matching_version_and_tarball(major_minor, package_name)

  latest_major_minor = '.'.join(latest_version.split('.')[:2])
  if(latest_major_minor != major_minor):
    new_version = f"{major_minor}.0"
  else:
    new_version = increment_patch_version(latest_version)
  changelog = extract_changelog(tarball_url)
  commit_title = get_current_commit_title()
  
  lines = changelog.split('\n' , 1)
  lines_length = len(lines)
  
  if lines_length == 1:
    append_to_changelog(project_dir, f"""
{lines[0]}

## {new_version}

- {commit_title}
""")
  else:
    append_to_changelog(project_dir, f"""
{lines[0]}

## {new_version}

- {commit_title}
 {lines[1]}
""")
    
  write_version_to_package_json(new_version , f"{project_dir}/package.json")

if __name__ == "__main__":
  main()
