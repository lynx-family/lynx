'use strict';

const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const packageRoot = __dirname;
const cliPath = path.join(packageRoot, 'cli.js');
const skillRoot = path.join(packageRoot, 'skills', 'using-lynx-api-docs');

run('validates the publishable source layout', () => {
  const result = execNode([path.join(packageRoot, 'verify-package-layout.js')]);
  assert.strictEqual(result.status, 0, result.stderr);

  const packageJson = readJson(path.join(packageRoot, 'package.json'));
  const cli = fs.readFileSync(cliPath, 'utf8');
  assert.strictEqual(packageJson.name, '@lynx-js/lynx-api-docs');
  assert.strictEqual(packageJson.scripts.prepack, 'node ./verify-package-layout.js');
  assert.ok(packageJson.files.includes('verify-package-layout.js'));
  assert.ok(!packageJson.files.includes('test.js'));
  assert.doesNotMatch(cli, /fs\.(?:cpSync|rmSync)/);
});

run('installs public docs and agent skill', () => {
  const projectDir = makeTempDir();
  const result = execCli(['install', '--project', projectDir, '--no-link']);

  assert.strictEqual(result.status, 0, result.stderr);
  assert.ok(fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/elements/page.md')));
  assert.ok(fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/elements/input.md')));
  assert.ok(fs.existsSync(path.join(projectDir, '.agents/skills/using-lynx-api-docs/SKILL.md')));
  assert.ok(!fs.existsSync(path.join(projectDir, '.ai/lynx-api-docs/config')));
  assert.ok(
    !fs.readdirSync(path.join(projectDir, '.ai/lynx-api-docs/elements')).some((name) =>
      /^x-.*\.md$/.test(name)
    )
  );

  const agentsContent = fs.readFileSync(path.join(projectDir, 'AGENTS.md'), 'utf8');
  assert.match(agentsContent, /BEGIN MANAGED BLOCK: @lynx-js\/lynx-api-docs/);
  assert.match(agentsContent, /\[Lynx API Docs Index\]\|root: \.ai\/lynx-api-docs/);
});

run('updates one managed block and preserves project notes', () => {
  const projectDir = makeTempDir();
  const agentsPath = path.join(projectDir, 'AGENTS.md');
  fs.writeFileSync(agentsPath, '# Project notes\n', 'utf8');

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

  const agentsContent = fs.readFileSync(agentsPath, 'utf8');
  assert.match(agentsContent, /^# Project notes/m);
  assert.match(agentsContent, /root: \.docs\/lynx-api/);
  assert.strictEqual(countMatches(agentsContent, /BEGIN MANAGED BLOCK: @lynx-js\/lynx-api-docs/g), 1);
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

run('rejects an AGENTS.md symlink that resolves outside the project', () => {
  const projectDir = makeTempDir();
  const outsideDir = makeTempDir();
  const outsideAgentsPath = path.join(outsideDir, 'AGENTS.md');
  const originalContent = '# Outside project\n';
  fs.writeFileSync(outsideAgentsPath, originalContent, 'utf8');
  if (!tryCreateSymlink(outsideAgentsPath, path.join(projectDir, 'AGENTS.md'), 'file')) {
    return;
  }

  const result = execCli(['install', '--project', projectDir, '--no-link']);
  assert.notStrictEqual(result.status, 0);
  assert.match(result.stderr, /AGENTS\.md resolves outside the project root/);
  assert.strictEqual(fs.readFileSync(outsideAgentsPath, 'utf8'), originalContent);
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
  assert.ok(!fs.existsSync(path.join(skillRoot, 'AGENTS.public.md')));
  assert.ok(!fs.existsSync(path.join(skillRoot, 'README.public.md')));

  for (const filePath of listFiles(packageRoot)) {
    if (!/\.(?:js|json|md)$/.test(filePath)) {
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

run('keeps CSS differences routing complete and consistent', () => {
  const indexPath = path.join(skillRoot, 'lynx-vs-web', 'css-differences.md');
  const indexContent = fs.readFileSync(indexPath, 'utf8');
  const topicSections = {
    'display-and-default-layout.md': ['The `display` Property', 'Default Layout Behavior (Important)'],
    'flex-and-sizing.md': [
      'Flex Shrinking and Content-Based Minimum Sizes',
      'Cyclic Percentage Sizing',
      'Length Values',
    ],
    'box-model.md': ['Box Model'],
    'positioning-and-stacking.md': ['Positioning', 'Floats Are Not Supported', '`z-index`'],
    'text-selectors-and-fonts.md': [
      'Text Handling',
      'Pseudo-elements',
      'Selector Limitations',
      'Custom Fonts',
    ],
    'transforms-and-animations.md': [
      'Subset of 3D Transform Functions',
      'The `steps(...)` Animation Timing Function',
    ],
    'migration-pitfalls.md': [
      'Quick Migration Checklist',
      'Common Pitfalls',
      'Pitfall 1: Omitting the `text` Element',
      'Pitfall 2: Relying on Margin Collapsing',
      'Pitfall 3: Using Unsupported or Partially Supported Units',
      'Pitfall 4: Omitting `white-space`',
      'Pitfall 5: Failing to Copy `body` Styles to the `page` Element',
      'Pitfall 6: Mapping Web `html` and `body` Backgrounds to Ordinary Containers',
      'Pitfall 7: Migrating a `background` Shorthand That Contains `background-attachment`',
    ],
  };

  for (const [fileName, headings] of Object.entries(topicSections)) {
    const topicPath = path.join(skillRoot, 'lynx-vs-web', 'css-differences', fileName);
    assert.ok(fs.existsSync(topicPath), `Missing CSS differences topic: ${fileName}`);
    const content = fs.readFileSync(topicPath, 'utf8');
    for (const heading of headings) {
      assert.ok(content.includes(heading), `Missing ${heading} in ${fileName}`);
    }
  }

  const slugify = (heading) =>
    heading
      .trim()
      .toLowerCase()
      .replace(/`/g, '')
      .replace(/[^\p{L}\p{N} _-]/gu, '')
      .replace(/ /g, '-');
  const headingAnchors = new Map();
  for (const match of indexContent.matchAll(/\]\(([^)]+)\)/g)) {
    const link = match[1];
    if (/^[a-z]+:/i.test(link) || link.startsWith('#')) {
      continue;
    }
    const [relativePath, anchor] = link.split('#');
    const targetPath = path.resolve(path.dirname(indexPath), relativePath);
    assert.ok(fs.existsSync(targetPath), `Broken CSS differences link: ${link}`);
    if (!anchor) {
      continue;
    }
    if (!headingAnchors.has(targetPath)) {
      const content = fs.readFileSync(targetPath, 'utf8');
      headingAnchors.set(
        targetPath,
        new Set(
          [...content.matchAll(/^#{1,6}\s+(.+)$/gm)].map((heading) => slugify(heading[1]))
        )
      );
    }
    assert.ok(headingAnchors.get(targetPath).has(anchor), `Broken CSS differences anchor: ${link}`);
  }

  const supportedProperties = fs.readFileSync(
    path.join(skillRoot, 'css', 'supported-properties.md'),
    'utf8'
  );
  assert.match(supportedProperties, /`transform: matrix, matrix3d`/);
  assert.doesNotMatch(supportedProperties, /scale3d|rotate3d/);
  assert.match(indexContent, /Flex text, numbers, or children shrink, wrap/);
  assert.match(indexContent, /including `0`, creates a stacking context without requiring `position`/);
  assert.match(indexContent, /`left` or `right`, and `top` or `bottom`/);

  const positioningContent = fs.readFileSync(
    path.join(skillRoot, 'lynx-vs-web', 'css-differences', 'positioning-and-stacking.md'),
    'utf8'
  );
  assert.doesNotMatch(positioningContent, /Requires `position`/);
  assert.match(positioningContent, /when the start inset is `auto` and the end inset is definite/);

  const skillContent = fs.readFileSync(path.join(skillRoot, 'SKILL.md'), 'utf8');
  assert.match(skillContent, /## Repair Scope and Output/);
  assert.match(skillContent, /Diagnose before expanding retrieval/);
  assert.match(skillContent, /Preserve unrelated styles, component props and behavior/);
  assert.match(skillContent, /without retrieval notes, status updates, explanations, or other prose/);
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

function countMatches(content, pattern) {
  const matches = content.match(pattern);
  return matches ? matches.length : 0;
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
