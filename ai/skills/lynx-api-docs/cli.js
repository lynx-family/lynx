#!/usr/bin/env node

'use strict';

const fs = require('fs');
const path = require('path');

const DEFAULT_DEST = '.ai/lynx-api-docs';
const SKILL_SOURCE = 'skills/using-lynx-api-docs';
const SKILL_DEST = '.agents/skills/using-lynx-api-docs';
const SKILL_NAME = 'using-lynx-api-docs';
const PROJECT_AGENT_DIRS = {
  claude: '.claude/skills',
  codex: '.codex/skills',
  trae: '.trae/skills',
};

function main() {
  try {
    const { command, options } = parseArgs(process.argv.slice(2));

    if (options.help) {
      printHelp();
      return;
    }

    if (command !== 'install') {
      throw new Error(`Unsupported command "${command}". Only "install" is available.`);
    }

    const packageRoot = __dirname;
    const projectPath = path.resolve(options.project || process.cwd());
    ensureProjectRoot(projectPath);
    const projectRoot = fs.realpathSync(projectPath);
    const destRelative = normalizeRelativeDest(options.dest || DEFAULT_DEST);
    const destAbsolute = path.resolve(projectRoot, destRelative);
    const skillSourcePath = path.join(packageRoot, SKILL_SOURCE);
    const skillDestPath = path.join(projectRoot, SKILL_DEST);

    ensurePathWithinRoot(projectRoot, destAbsolute, 'Docs destination');
    ensurePathWithinRoot(projectRoot, skillDestPath, 'Skill destination');
    ensureWritableDestination(projectRoot, destAbsolute);
    ensureWritableSkillParent(projectRoot, skillDestPath);

    const ops = [];
    queueCopy(skillSourcePath, destAbsolute, ops, projectRoot);

    const linkTargets = resolveLinkTargets(options, projectRoot);
    const allLinkTargets = [skillDestPath, ...linkTargets];
    const linkResults = [];
    for (const target of allLinkTargets) {
      if (isPathWithinRoot(projectRoot, target)) {
        ensurePathWithinRoot(projectRoot, path.dirname(target), 'Skill link parent');
      }
    }

    if (options.dryRun) {
      printDryRunSummary(projectRoot, destRelative, ops, allLinkTargets);
      return;
    }

    for (const op of ops) {
      writeCopy(op);
    }

    for (const target of allLinkTargets) {
      linkResults.push(linkSkill(destAbsolute, target));
    }

    printSummary(projectRoot, destRelative, ops.some((op) => op.changed), linkResults);
  } catch (error) {
    console.error(`Error: ${error.message}`);
    process.exitCode = 1;
  }
}

function parseArgs(argv) {
  let command = 'install';
  const options = {
    dryRun: false,
    help: false,
    linkAgents: [],
    linkAll: true,
    skillsDir: null,
  };

  const args = [...argv];
  if (args[0] && !args[0].startsWith('-')) {
    command = args.shift();
  }

  for (let i = 0; i < args.length; i += 1) {
    const arg = args[i];
    if (arg === '--help' || arg === '-h') {
      options.help = true;
    } else if (arg === '--dry-run') {
      options.dryRun = true;
    } else if (arg === '--project') {
      options.project = requireValue(args, ++i, '--project');
    } else if (arg.startsWith('--project=')) {
      options.project = arg.slice('--project='.length);
    } else if (arg === '--dest') {
      options.dest = requireValue(args, ++i, '--dest');
    } else if (arg.startsWith('--dest=')) {
      options.dest = arg.slice('--dest='.length);
    } else if (arg === '--link-claude') {
      options.linkAll = false;
      options.linkAgents.push('claude');
    } else if (arg === '--link-codex') {
      options.linkAll = false;
      options.linkAgents.push('codex');
    } else if (arg === '--link-trae') {
      options.linkAll = false;
      options.linkAgents.push('trae');
    } else if (arg === '--link-all') {
      options.linkAll = true;
    } else if (arg === '--no-link') {
      options.linkAll = false;
      options.linkAgents = [];
    } else if (arg === '--skills-dir') {
      options.skillsDir = requireValue(args, ++i, '--skills-dir');
    } else if (arg.startsWith('--skills-dir=')) {
      options.skillsDir = arg.slice('--skills-dir='.length);
    } else {
      throw new Error(`Unknown argument "${arg}".`);
    }
  }

  return { command, options };
}

function requireValue(args, index, flag) {
  const value = args[index];
  if (!value) {
    throw new Error(`Missing value for ${flag}.`);
  }
  return value;
}

function normalizeRelativeDest(dest) {
  const portableDest = dest.replace(/[\\/]+/g, path.sep);
  const normalized = path.normalize(portableDest).replace(/[\\]+/g, '/');

  if (!normalized || normalized === '.') {
    throw new Error('Destination path must not be empty.');
  }

  if (
    path.isAbsolute(dest) ||
    path.win32.isAbsolute(dest) ||
    /^[A-Za-z]:/.test(dest)
  ) {
    throw new Error('Destination path must be relative to the project root.');
  }

  if (normalized.startsWith('../') || normalized === '..') {
    throw new Error('Destination path must stay inside the project root.');
  }

  return normalized;
}

function ensureProjectRoot(projectRoot) {
  if (!fs.existsSync(projectRoot)) {
    throw new Error(`Project path does not exist: ${projectRoot}`);
  }

  const stats = fs.statSync(projectRoot);
  if (!stats.isDirectory()) {
    throw new Error(`Project path is not a directory: ${projectRoot}`);
  }
}

function ensurePathWithinRoot(projectRoot, targetPath, label) {
  if (!isPathWithinRoot(projectRoot, targetPath)) {
    throw new Error(`${label} must stay inside the project root.`);
  }

  const existingPath = nearestExistingPath(targetPath);
  let realExistingPath;
  try {
    realExistingPath = fs.realpathSync(existingPath);
  } catch (error) {
    throw new Error(`${label} contains an unresolved symbolic link: ${existingPath}`);
  }

  const remainingPath = path.relative(existingPath, targetPath);
  const realTargetPath = path.resolve(realExistingPath, remainingPath);
  if (!isPathWithinRoot(projectRoot, realTargetPath)) {
    throw new Error(`${label} resolves outside the project root.`);
  }
}

function isPathWithinRoot(projectRoot, targetPath) {
  const relativePath = path.relative(projectRoot, targetPath);
  return (
    relativePath === '' ||
    (relativePath !== '..' &&
      !relativePath.startsWith(`..${path.sep}`) &&
      !path.isAbsolute(relativePath))
  );
}

function nearestExistingPath(targetPath) {
  let current = targetPath;
  while (true) {
    try {
      fs.lstatSync(current);
      return current;
    } catch (error) {
      if (error.code !== 'ENOENT') {
        throw error;
      }
    }

    const parent = path.dirname(current);
    if (parent === current) {
      throw new Error(`No existing parent found for: ${targetPath}`);
    }
    current = parent;
  }
}

function ensureWritableDestination(projectRoot, destAbsolute) {
  const probeDir = nearestExistingDir(destAbsolute) || projectRoot;
  if (!isWritable(probeDir)) {
    throw new Error(`Project is not writable: ${probeDir}`);
  }
}

function ensureWritableSkillParent(projectRoot, skillDestPath) {
  const skillParent = path.dirname(skillDestPath);
  if (!fs.existsSync(skillParent)) {
    return;
  }
  if (!isWritable(skillParent)) {
    throw new Error(`Cannot write skill in: ${skillParent}`);
  }
}

function nearestExistingDir(targetPath) {
  let current = targetPath;
  while (!fs.existsSync(current)) {
    const parent = path.dirname(current);
    if (parent === current) {
      return null;
    }
    current = parent;
  }

  if (fs.statSync(current).isDirectory()) {
    return current;
  }

  return path.dirname(current);
}

function isWritable(targetPath) {
  try {
    fs.accessSync(targetPath, fs.constants.W_OK);
    return true;
  } catch (error) {
    return false;
  }
}

function queueCopy(sourcePath, targetPath, ops, safetyRoot) {
  const stats = fs.statSync(sourcePath);
  if (stats.isDirectory()) {
    const entries = fs.readdirSync(sourcePath).sort();
    for (const entry of entries) {
      queueCopy(
        path.join(sourcePath, entry),
        path.join(targetPath, entry),
        ops,
        safetyRoot
      );
    }
    return;
  }

  ensurePathWithinRoot(safetyRoot, targetPath, 'Copy target');
  const nextContent = fs.readFileSync(sourcePath);
  const exists = fs.existsSync(targetPath);
  const changed = !exists || !bufferEquals(nextContent, fs.readFileSync(targetPath));
  ops.push({
    sourcePath,
    targetPath,
    content: nextContent,
    changed,
    safetyRoot,
  });
}

function bufferEquals(left, right) {
  if (left.length !== right.length) {
    return false;
  }
  return left.equals(right);
}

function writeCopy(op) {
  ensurePathWithinRoot(op.safetyRoot, op.targetPath, 'Copy target');
  fs.mkdirSync(path.dirname(op.targetPath), { recursive: true });
  if (op.changed) {
    fs.writeFileSync(op.targetPath, op.content);
  }
}

function resolveLinkTargets(options, projectRoot) {
  const targets = [];

  if (options.skillsDir) {
    targets.push(path.join(path.resolve(options.skillsDir), SKILL_NAME));
  }

  if (options.linkAll) {
    for (const dir of Object.values(PROJECT_AGENT_DIRS)) {
      targets.push(path.join(projectRoot, dir, SKILL_NAME));
    }
  } else {
    for (const agent of options.linkAgents) {
      const dir = PROJECT_AGENT_DIRS[agent];
      if (dir) {
        targets.push(path.join(projectRoot, dir, SKILL_NAME));
      }
    }
  }

  return [...new Set(targets)];
}

function linkSkill(sourcePath, targetPath) {
  if (!fs.existsSync(sourcePath)) {
    return { target: targetPath, status: 'skipped', reason: 'source skill not found' };
  }

  if (path.resolve(sourcePath) === path.resolve(targetPath)) {
    return { target: targetPath, status: 'canonical' };
  }

  try {
    fs.mkdirSync(path.dirname(targetPath), { recursive: true });
  } catch (error) {
    return { target: targetPath, status: 'failed', reason: `cannot create parent dir: ${error.message}` };
  }

  try {
    const lstat = fs.lstatSync(targetPath);
    if (lstat.isSymbolicLink() || lstat.isDirectory() || lstat.isFile()) {
      removeEntry(targetPath);
    }
  } catch (error) {
    if (error.code !== 'ENOENT') {
      return { target: targetPath, status: 'failed', reason: `cannot remove existing: ${error.message}` };
    }
  }

  try {
    fs.symlinkSync(path.resolve(sourcePath), targetPath, 'dir');
    return { target: targetPath, status: 'linked' };
  } catch (error) {
    try {
      copyDirectory(sourcePath, targetPath);
      return { target: targetPath, status: 'copied' };
    } catch (copyError) {
      return { target: targetPath, status: 'failed', reason: `symlink failed (${error.message}), copy failed (${copyError.message})` };
    }
  }
}

function removeEntry(targetPath) {
  const stats = fs.lstatSync(targetPath);
  if (stats.isDirectory() && !stats.isSymbolicLink()) {
    for (const entry of fs.readdirSync(targetPath)) {
      removeEntry(path.join(targetPath, entry));
    }
    fs.rmdirSync(targetPath);
    return;
  }

  fs.unlinkSync(targetPath);
}

function copyDirectory(sourcePath, targetPath) {
  const stats = fs.statSync(sourcePath);
  if (stats.isDirectory()) {
    fs.mkdirSync(targetPath, { recursive: true });
    for (const entry of fs.readdirSync(sourcePath).sort()) {
      copyDirectory(path.join(sourcePath, entry), path.join(targetPath, entry));
    }
    return;
  }

  fs.mkdirSync(path.dirname(targetPath), { recursive: true });
  fs.copyFileSync(sourcePath, targetPath);
}

function printDryRunSummary(projectRoot, destRelative, ops, linkTargets) {
  console.log(`Dry run for ${projectRoot}`);
  console.log(`Docs destination: ${destRelative}`);
  console.log(`Docs files to ${ops.some((op) => op.changed) ? 'write/update' : 'keep'}: ${ops.filter((op) => op.changed).length}/${ops.length}`);
  for (const target of linkTargets) {
    console.log(`Skill link: ${target} (dry run)`);
  }
}

function printSummary(projectRoot, destRelative, docsChanged, linkResults) {
  console.log(`Installed Lynx API docs into ${projectRoot}`);
  console.log(`Docs destination: ${destRelative}`);
  console.log(`Docs: ${docsChanged ? 'installed/updated' : 'already up to date'}`);
  for (const result of linkResults) {
    console.log(`Skill link: ${result.target} — ${result.status}${result.reason ? ` (${result.reason})` : ''}`);
  }
}

function printHelp() {
  console.log([
    'Usage:',
    '  lynx-api-docs install [--project <path>] [--dest <path>] [--dry-run]',
    '                        [--no-link] [--link-claude] [--link-codex] [--link-trae]',
    '                        [--link-all] [--skills-dir <path>]',
    '  lynx-api-docs --help',
    '',
    'Defaults:',
    `  command: install`,
    `  destination: ${DEFAULT_DEST}`,
    `  primary skill: ${SKILL_DEST}`,
    `  additional links: all known agent directories (use --no-link to skip)`,
    '',
    'Skill linking:',
    '  --link-claude    Link skill to <project>/.claude/skills/',
    '  --link-codex     Link skill to <project>/.codex/skills/',
    '  --link-trae      Link skill to <project>/.trae/skills/',
    '  --link-all       Link skill to all known agent-specific dirs (default)',
    '  --no-link        Skip additional agent-specific links',
    '  --skills-dir     Link skill to a custom skills directory',
  ].join('\n'));
}

main();
