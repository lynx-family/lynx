'use strict';

const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');
const { readTopics, validateTopics } = require('./generate-index');

const packageRoot = __dirname;
const cliPath = path.join(packageRoot, 'cli.js');
const evaluatePath = path.join(packageRoot, 'evaluate.js');
const skillRoot = path.join(packageRoot, 'skills', 'using-lynx-api-docs');

run('validates the publishable source layout', () => {
  const result = execNode([path.join(packageRoot, 'verify-package-layout.js')]);
  assert.strictEqual(result.status, 0, result.stderr);

  const packageJson = readJson(path.join(packageRoot, 'package.json'));
  const cli = fs.readFileSync(cliPath, 'utf8');
  assert.strictEqual(packageJson.name, '@lynx-js/lynx-api-docs');
  assert.strictEqual(packageJson.scripts.prepack, 'node ./verify-package-layout.js');
  assert.strictEqual(packageJson.scripts.evaluate, 'node ./evaluate.js');
  assert.strictEqual(packageJson.scripts['generate:index'], 'node ./generate-index.js');
  assert.ok(packageJson.files.includes('generate-index.js'));
  assert.ok(packageJson.files.includes('verify-package-layout.js'));
  assert.ok(!packageJson.files.includes('test.js'));
  assert.ok(packageJson.files.includes('evaluate.js'));
  assert.ok(packageJson.files.includes('evaluations'));
  assert.doesNotMatch(cli, /fs\.(?:cpSync|rmSync)/);
  assert.doesNotMatch(cli, /AGENTS\.md/);
});

run('passes routing and claim evaluations', () => {
  const result = execNode([evaluatePath]);
  assert.strictEqual(result.status, 0, result.stderr);
  assert.match(result.stdout, /ok - background-cover-transparent-border/);
});

run('installs public docs and agent skill', () => {
  const projectDir = makeTempDir();
  const result = execCli(['install', '--project', projectDir, '--no-link']);

  assert.strictEqual(result.status, 0, result.stderr);
  assert.ok(fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/elements/page.md')));
  assert.ok(fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/elements/input.md')));
  assert.ok(fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/SKILL.md')));
  assert.ok(fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/INDEX.md')));
  assert.ok(fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/topics.jsonl')));
  assert.ok(fs.existsSync(path.join(projectDir, '.agents/skills/using-lynx-api-docs/SKILL.md')));
  assert.ok(!fs.existsSync(path.join(projectDir, 'AGENTS.md')));
  assert.ok(!fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/config')));
  assert.ok(
    !fs.readdirSync(path.join(projectDir, '.ai/lynx-api-docs/elements')).some((name) =>
      /^x-.*\.md$/.test(name)
    )
  );
});

run('keeps the primary agent skill on the canonical tree when symlinks work', () => {
  const projectDir = makeTempDir();
  const result = execCli(['install', '--project', projectDir, '--no-link']);
  assert.strictEqual(result.status, 0, result.stderr);

  const canonicalRoot = path.join(projectDir, '.ai/lynx-api-docs');
  const primarySkill = path.join(projectDir, '.agents/skills/using-lynx-api-docs');
  if (fs.lstatSync(primarySkill).isSymbolicLink()) {
    assert.strictEqual(fs.realpathSync(primarySkill), fs.realpathSync(canonicalRoot));
  }
});

run('keeps the generated topic index current and complete', () => {
  const result = execNode([path.join(packageRoot, 'generate-index.js'), '--check']);
  assert.strictEqual(result.status, 0, result.stderr);

  const index = fs.readFileSync(path.join(skillRoot, 'INDEX.md'), 'utf8');
  assert.match(index, /Background and border painting/);
  assert.doesNotMatch(index, /Version \| Platforms/);
});

run('uses the package release as the only compatibility baseline', () => {
  const topicsPath = path.join(skillRoot, 'topics.jsonl');
  const topicsSource = fs.readFileSync(topicsPath, 'utf8');
  const topics = readTopics(topicsPath);
  const skill = fs.readFileSync(path.join(skillRoot, 'SKILL.md'), 'utf8');
  const backgroundDoc = fs.readFileSync(
    path.join(skillRoot, 'css/compatibility/backgrounds-and-borders.md'),
    'utf8'
  );

  assert.doesNotMatch(topicsSource, /"(?:version_bands|versions|since|until)"\s*:/);
  assert.throws(
    () =>
      validateTopics(
        [{ ...topics[0], version_bands: ['example'] }, ...topics.slice(1)],
        skillRoot
      ),
    /unsupported fields: version_bands/
  );
  assert.throws(
    () =>
      validateTopics(
        [{ ...topics[0], path: 'css/../../package.json' }, ...topics.slice(1)],
        skillRoot
      ),
    /unsafe path/
  );
  assert.match(skill, /installed skill tree as one coherent documentation baseline/);
  assert.match(backgroundDoc, /standard CSS behavior/);
  assert.match(backgroundDoc, /background-origin: border-box/);

  for (const filePath of listFiles(skillRoot).filter((filePath) => filePath.endsWith('.md'))) {
    const content = fs.readFileSync(filePath, 'utf8');
    assert.doesNotMatch(
      content,
      /\bLynx\s+v?\d+\.\d+(?:\.\d+)?\+?/i,
      `${path.relative(skillRoot, filePath)} contains an engine-version branch`
    );
    assert.doesNotMatch(content, /^#{1,6}\s+Version history\s*$/im);
  }
});

run('leaves an existing AGENTS.md unchanged', () => {
  const projectDir = makeTempDir();
  const agentsPath = path.join(projectDir, 'AGENTS.md');
  const originalContent = [
    '# Project notes',
    '',
    '<!-- BEGIN MANAGED BLOCK: @lynx-js/lynx-api-docs -->',
    'legacy content',
    '<!-- END MANAGED BLOCK: @lynx-js/lynx-api-docs -->',
    '',
  ].join('\n');
  fs.writeFileSync(agentsPath, originalContent, 'utf8');

  let result = execCli(['install', '--project', projectDir, '--no-link']);
  assert.strictEqual(result.status, 0, result.stderr);
  result = execCli([
    'install',
    '--project',
    projectDir,
    '--dest',
    '.docs/lynx-api',
    '--no-link',
  ]);
  assert.strictEqual(result.status, 0, result.stderr);

  assert.strictEqual(fs.readFileSync(agentsPath, 'utf8'), originalContent);
});

run('replaces existing project-local skill links on reinstall', () => {
  const projectDir = makeTempDir();
  let result = execCli(['install', '--project', projectDir]);
  assert.strictEqual(result.status, 0, result.stderr);

  result = execCli(['install', '--project', projectDir]);
  assert.strictEqual(result.status, 0, result.stderr);
  assert.ok(
    fs.existsSync(path.join(projectDir, '.codex/skills/using-lynx-api-docs/SKILL.md'))
  );
});

run('replaces a dangling project-local skill link on reinstall', () => {
  const projectDir = makeTempDir();
  const linkPath = path.join(projectDir, '.codex/skills/using-lynx-api-docs');
  fs.mkdirSync(path.dirname(linkPath), { recursive: true });
  if (!tryCreateSymlink(path.join(projectDir, 'missing-skill'), linkPath, 'dir')) {
    return;
  }

  const result = execCli(['install', '--project', projectDir]);
  assert.strictEqual(result.status, 0, result.stderr);
  assert.ok(fs.existsSync(path.join(linkPath, 'SKILL.md')));
});

run('supports dry-run without writing files', () => {
  const projectDir = makeTempDir();
  const result = execCli(['install', '--project', projectDir, '--dry-run']);

  assert.strictEqual(result.status, 0, result.stderr);
  assert.match(result.stdout, /Dry run/);
  assert.ok(!fs.existsSync(path.join(projectDir, 'AGENTS.md')));
  assert.ok(!fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs')));
});

run('rejects portable paths that can escape the project', () => {
  for (const destination of [
    '../outside',
    'nested/../../outside',
    'nested\\..\\..\\outside',
    '/tmp/outside',
    'C:outside',
    'C:\\outside',
    'C:/outside',
    '\\\\server\\share',
  ]) {
    const projectDir = makeTempDir();
    const result = execCli([
      'install',
      '--project',
      projectDir,
      '--dest',
      destination,
      '--no-link',
    ]);
    assert.notStrictEqual(result.status, 0, `${destination} should be rejected`);
    assert.match(result.stderr, /Destination path must/);
  }
});

run('ignores an AGENTS.md symlink that resolves outside the project', () => {
  const projectDir = makeTempDir();
  const outsideDir = makeTempDir();
  const outsideAgentsPath = path.join(outsideDir, 'AGENTS.md');
  const originalContent = '# Outside project\n';
  fs.writeFileSync(outsideAgentsPath, originalContent, 'utf8');
  if (!tryCreateSymlink(outsideAgentsPath, path.join(projectDir, 'AGENTS.md'), 'file')) {
    return;
  }

  const result = execCli(['install', '--project', projectDir, '--no-link']);
  assert.strictEqual(result.status, 0, result.stderr);
  assert.strictEqual(fs.readFileSync(outsideAgentsPath, 'utf8'), originalContent);
  assert.ok(fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/SKILL.md')));
});

run('rejects an agent directory that resolves outside the project', () => {
  const projectDir = makeTempDir();
  const outsideDir = makeTempDir();
  const linkType = process.platform === 'win32' ? 'junction' : 'dir';
  if (!tryCreateSymlink(outsideDir, path.join(projectDir, '.codex'), linkType)) {
    return;
  }

  const result = execCli(['install', '--project', projectDir]);
  assert.notStrictEqual(result.status, 0);
  assert.match(result.stderr, /Skill link parent resolves outside the project root/);
  assert.deepStrictEqual(fs.readdirSync(outsideDir), []);
  assert.ok(!fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs')));
});

run('copies a skill when symlinks are unavailable', () => {
  const projectDir = makeTempDir();
  const skillsDir = path.join(makeTempDir(), 'skills');
  const preloadPath = path.join(makeTempDir(), 'disable-symlinks.js');
  fs.writeFileSync(
    preloadPath,
    "'use strict'; const fs = require('fs'); fs.symlinkSync = () => { throw new Error('disabled for test'); };\n",
    'utf8'
  );

  const result = execNode([
    '--require',
    preloadPath,
    cliPath,
    'install',
    '--project',
    projectDir,
    '--no-link',
    '--skills-dir',
    skillsDir,
  ]);

  assert.strictEqual(result.status, 0, result.stderr);
  assert.match(result.stdout, /— copied/);
  const copiedSkill = path.join(skillsDir, 'using-lynx-api-docs');
  assert.ok(fs.lstatSync(copiedSkill).isDirectory());
  assert.ok(fs.existsSync(path.join(copiedSkill, 'SKILL.md')));
});

run('keeps public source free of generated and non-public docs', () => {
  assert.ok(!fs.existsSync(path.join(skillRoot, 'config')));
  assert.ok(!fs.existsSync(path.join(skillRoot, 'AGENTS.md')));
  assert.ok(!fs.existsSync(path.join(skillRoot, 'AGENTS.public.md')));
  assert.ok(!fs.existsSync(path.join(skillRoot, 'README.public.md')));

  for (const filePath of listFiles(packageRoot)) {
    if (!/\.(?:js|json|jsonl|md)$/.test(filePath)) {
      continue;
    }
    const content = fs.readFileSync(filePath, 'utf8');
    const packageNames = content.match(/@[A-Za-z0-9_.-]+\/lynx-api-docs/g) || [];
    assert.ok(
      packageNames.every((name) => name === '@lynx-js/lynx-api-docs'),
      `${path.relative(packageRoot, filePath)} has non-public package branding`
    );
  }
});

function execCli(args) {
  return execNode([cliPath, ...args]);
}

function execNode(args) {
  return spawnSync(process.execPath, args, { encoding: 'utf8' });
}

function makeTempDir() {
  return fs.mkdtempSync(path.join(os.tmpdir(), 'lynx-api-docs-'));
}

function tryCreateSymlink(targetPath, linkPath, type) {
  try {
    fs.symlinkSync(targetPath, linkPath, type);
    return true;
  } catch (error) {
    if (error.code === 'EPERM' || error.code === 'EACCES') {
      return false;
    }
    throw error;
  }
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function listFiles(rootDir) {
  const files = [];
  for (const entry of fs.readdirSync(rootDir, { withFileTypes: true })) {
    if (entry.name === 'node_modules') {
      continue;
    }
    const fullPath = path.join(rootDir, entry.name);
    if (entry.isDirectory()) {
      files.push(...listFiles(fullPath));
    } else if (entry.isFile()) {
      files.push(fullPath);
    }
  }
  return files;
}

function run(name, fn) {
  try {
    fn();
    console.log(`ok - ${name}`);
  } catch (error) {
    console.error(`not ok - ${name}`);
    console.error(error.stack);
    process.exitCode = 1;
  }
}
