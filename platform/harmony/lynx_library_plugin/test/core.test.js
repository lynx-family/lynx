// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const test = require('node:test');
const JSON5 = require('json5');

const {
  REGISTRY_PACKAGE_NAME,
  discoverHarmonyLibraries,
  generateRegistrySource,
  setupHarmonyAutolink,
} = require('../src/core.js');

const tempDirs = [];

test.afterEach(() => {
  for (const dir of tempDirs.splice(0)) {
    fs.rmSync(dir, { force: true, recursive: true });
  }
});

test('loads the public plugin entry through CommonJS', async () => {
  const plugin = require('..');

  assert.equal(typeof plugin, 'function');
  assert.equal(typeof plugin.enableHarmonyLynxAutolink, 'function');
  assert.equal(plugin.default, plugin.enableHarmonyLynxAutolink);

  const importedPlugin = await import('../src/index.js');
  assert.equal(importedPlugin.default, plugin);
  assert.equal(importedPlugin.enableHarmonyLynxAutolink, plugin);
});

test('requires the public Hvigor JSON5 parser API', () => {
  const plugin = require('..');

  assert.throws(() => plugin({}), /requires @ohos\/hvigor\.parseJsonFile/);
});

test('discovers scoped libraries and parses JSON5 Harmony metadata', () => {
  const project = createProject();
  createLibrary(project, '@example/demo', '@example/demo_harmony', {
    ohPackageSource: `{
      name: '@example/demo_harmony',
      main: 'Index.ets',
    }`,
  });

  const libraries = discoverHarmonyLibraries(
    path.join(project, 'entry'),
    parseJson5File
  );

  assert.equal(libraries.length, 1);
  assert.equal(libraries[0].npmPackageName, '@example/demo');
  assert.equal(libraries[0].ohPackageName, '@example/demo_harmony');
});

test('discovers Node-API addons without requiring a provider export', () => {
  const project = createProject();
  createLibrary(project, '@example/addon', '@example/addon_harmony', {
    harmonyManifest: {
      providerExportName: null,
      nodeApiAddons: [
        {
          name: 'demo_addon',
          libraryName: 'demo_addon',
          initializerExportName: 'initializeNodeApiAddon',
          required: false,
        },
      ],
    },
  });

  const library = discoverHarmonyLibraries(
    path.join(project, 'entry'),
    parseJson5File
  )[0];

  assert.equal(library.providerExportName, null);
  assert.deepEqual(library.nodeApiAddons, [
    {
      name: 'demo_addon',
      libraryName: 'demo_addon',
      initializerExportName: 'initializeNodeApiAddon',
      required: false,
    },
  ]);
});

test('validates Harmony provider and Node-API addon metadata', () => {
  const invalidCases = [
    {
      harmonyManifest: { providerExportName: 'not-qualified.name' },
      expected: /providerExportName must be a valid ArkTS identifier/,
    },
    {
      harmonyManifest: { nodeApiAddons: {} },
      expected: /nodeApiAddons must be an array/,
    },
    {
      harmonyManifest: { nodeApiAddons: [{ name: '../bad' }] },
      expected: /nodeApiAddons\[0\]\.name must match/,
    },
    {
      harmonyManifest: {
        nodeApiAddons: [
          { name: 'demo', initializerExportName: 'bad-name' },
        ],
      },
      expected: /initializerExportName must be a valid ArkTS identifier/,
    },
    {
      harmonyManifest: {
        nodeApiAddons: [
          {
            name: 'demo',
            initializerExportName: 'initializeNodeApiAddon',
            required: 'yes',
          },
        ],
      },
      expected: /required must be a boolean/,
    },
  ];

  for (const [index, invalidCase] of invalidCases.entries()) {
    const project = createProject();
    createLibrary(
      project,
      `@example/invalid-${index}`,
      `@example/invalid_${index}_harmony`,
      { harmonyManifest: invalidCase.harmonyManifest }
    );

    assert.throws(
      () =>
        discoverHarmonyLibraries(
          path.join(project, 'entry'),
          parseJson5File
        ),
      invalidCase.expected
    );
  }
});

test('rejects Harmony package paths that escape the npm package', () => {
  const project = createProject();
  const packageRoot = createLibrary(
    project,
    '@example/escape',
    '@example/escape_harmony'
  );
  writeJson(path.join(packageRoot, 'lynx.lib.json'), {
    platforms: { harmony: { packageDir: '../outside' } },
  });

  assert.throws(
    () => discoverHarmonyLibraries(path.join(project, 'entry'), parseJson5File),
    /platforms\.harmony\.packageDir escapes/
  );
});

test('rejects duplicate npm package names', () => {
  const project = createProject();
  createLibrary(project, '@example/one', '@example/one_harmony');
  const secondRoot = createLibrary(
    project,
    '@example/two',
    '@example/two_harmony'
  );
  writeJson(path.join(secondRoot, 'package.json'), {
    name: '@example/one',
    version: '1.0.0',
  });

  assert.throws(
    () => discoverHarmonyLibraries(path.join(project, 'entry'), parseJson5File),
    /Duplicate npm package name/
  );
});

test('rejects duplicate OHPM package names', () => {
  const project = createProject();
  createLibrary(project, '@example/one', '@example/shared', {
    moduleName: 'one_module',
  });
  createLibrary(project, '@example/two', '@example/shared', {
    moduleName: 'two_module',
  });

  assert.throws(
    () => discoverHarmonyLibraries(path.join(project, 'entry'), parseJson5File),
    /Duplicate OHPM package name/
  );
});

test('rejects duplicate Harmony module names', () => {
  const project = createProject();
  createLibrary(project, '@example/one', '@example/one_harmony', {
    moduleName: 'shared_module',
  });
  createLibrary(project, '@example/two', '@example/two_harmony', {
    moduleName: 'shared_module',
  });

  assert.throws(
    () => discoverHarmonyLibraries(path.join(project, 'entry'), parseJson5File),
    /Duplicate Harmony module name/
  );
});

test('rejects the generated Registry Harmony module name', () => {
  const project = createProject();
  createLibrary(project, '@example/reserved', '@example/reserved_harmony', {
    moduleName: 'lynx_autolink_registry',
  });

  assert.throws(
    () => discoverHarmonyLibraries(path.join(project, 'entry'), parseJson5File),
    /Harmony module name lynx_autolink_registry is reserved/
  );
});

test('generates registry imports in stable npm package order', () => {
  const source = generateRegistrySource([
    libraryDescriptor('@example/z', '@example/z_harmony'),
    libraryDescriptor('@example/a', '@example/a_harmony'),
  ]);

  assert.ok(
    source.indexOf("from '@example/a_harmony'") <
      source.indexOf("from '@example/z_harmony'")
  );
  assert.match(source, /LynxLibraryRegistry\.setupGlobal\(PROVIDERS\)/);
});

test('initializes Node-API addons before optional providers', () => {
  const source = generateRegistrySource([
    {
      ...libraryDescriptor('@example/required', '@example/required_harmony'),
      providerExportName: null,
      nodeApiAddons: [
        {
          name: 'required_addon',
          initializerExportName: 'initializeRequiredAddon',
          required: true,
        },
      ],
    },
    {
      ...libraryDescriptor('@example/optional', '@example/optional_harmony'),
      providerExportName: 'CustomProvider',
      nodeApiAddons: [
        {
          name: 'optional_addon',
          initializerExportName: 'initializeOptionalAddon',
          required: false,
        },
      ],
    },
  ]);

  assert.doesNotMatch(
    source,
    /LynxLibraryProviderImpl as Provider0.*required_harmony/
  );
  assert.match(
    source,
    /import \{ CustomProvider as Provider0 \} from '@example\/optional_harmony'/
  );
  assert.match(
    source,
    /import \{ initializeOptionalAddon as InitializeNodeApiAddon0 \}/
  );
  assert.match(
    source,
    /import \{ initializeRequiredAddon as InitializeNodeApiAddon1 \}/
  );
  assert.ok(
    source.indexOf('InitializeNodeApiAddon1();') <
      source.indexOf('LynxLibraryRegistry.setupGlobal(PROVIDERS);')
  );
  assert.match(source, /try \{\n    InitializeNodeApiAddon0\(\);/);
  assert.match(
    source,
    /Failed to initialize optional Lynx Node-API addon optional_addon/
  );
});

test('does not load Lynx registry APIs for Node-API-only libraries', () => {
  const source = generateRegistrySource([
    {
      ...libraryDescriptor('@example/addon', '@example/addon_harmony'),
      providerExportName: null,
      nodeApiAddons: [
        {
          name: 'required_addon',
          initializerExportName: 'initializeRequiredAddon',
          required: true,
        },
      ],
    },
  ]);

  assert.doesNotMatch(source, /from '@lynx\/lynx'/);
  assert.doesNotMatch(source, /LynxLibraryRegistry/);
  assert.doesNotMatch(source, /PROVIDERS/);
  assert.match(source, /InitializeNodeApiAddon0\(\);/);
});

test('does not depend on Lynx registry APIs for Node-API-only libraries', () => {
  const project = createProject();
  createLibrary(project, '@example/addon', '@example/addon_harmony', {
    harmonyManifest: {
      providerExportName: null,
      nodeApiAddons: [
        {
          name: 'required_addon',
          initializerExportName: 'initializeRequiredAddon',
          required: true,
        },
      ],
    },
  });
  const state = createModuleState();

  const { result } = configureAutolink(project, state);
  const registryPackage = JSON.parse(
    fs.readFileSync(path.join(result.registryDir, 'oh-package.json5'), 'utf8')
  );

  assert.equal(registryPackage.dependencies['@lynx/lynx'], undefined);
  assert.match(
    registryPackage.dependencies['@example/addon_harmony'],
    /^file:/
  );
});

test('adds HAR nodes before configuring the HAP through model setters', () => {
  const project = createProject();
  createLibrary(project, '@example/demo', '@example/demo_harmony');
  const originalModuleJson = {
    module: { name: 'entry', type: 'entry' },
  };
  const state = {
    dependencies: { '@lynx/lynx': '^3.5.0' },
    buildProfile: { targets: [{ name: 'default' }] },
    moduleJson: originalModuleJson,
  };
  const { descriptors, lifecycle, result } = configureAutolink(project, state);

  assert.equal(result.libraries.length, 1);
  assert.match(
    state.dependencies[REGISTRY_PACKAGE_NAME],
    /^file:\.\.\/\.hvigor\/lynx-autolink\/entry\/registry$/
  );
  assert.equal(
    state.moduleJson.module.appStartup,
    '$profile:lynx_autolink_startup'
  );
  assert.deepEqual(
    descriptors.map((descriptor) => descriptor.name),
    ['entry', 'lynx_autolink_registry', 'demo']
  );
  assert.equal(
    descriptors[1].srcPath,
    './.hvigor/lynx-autolink/entry/registry'
  );
  assert.equal(descriptors[2].srcPath, './node_modules/@example/demo/harmony');
  assert.deepEqual(descriptors[1].extraOptions.targets, [
    { name: 'default', applyToProducts: ['default'] },
  ]);
  assert.equal(state.buildProfile.targets[0].source, undefined);
  assert.deepEqual(state.buildProfile.targets[0].resource.directories, [
    './src/main/resources',
    './build/generated/lynx-autolink/src/main/resources',
  ]);
  const registrySource = fs.readFileSync(
    path.join(result.registryDir, 'Index.ets'),
    'utf8'
  );
  assert.match(registrySource, /new Provider0\(\)/);
  const startupTask = fs.readFileSync(
    path.join(
      project,
      'entry',
      'src',
      'main',
      'ets',
      'lynx_autolink',
      'LynxAutolinkStartupTask.ets'
    ),
    'utf8'
  );
  assert.match(startupTask, /setupGlobal\(\);/);
  assert.doesNotMatch(
    startupTask,
    /registerBehavior|registerModule|registerService/
  );
  const startupProfile = JSON.parse(
    fs.readFileSync(
      path.join(
        result.generatedSourceRoot,
        'resources',
        'base',
        'profile',
        'lynx_autolink_startup.json'
      ),
      'utf8'
    )
  );
  assert.equal(
    startupProfile.startupTasks[0].srcEntry,
    './ets/lynx_autolink/LynxAutolinkStartupTask.ets'
  );
  assert.equal(
    startupProfile.configEntry,
    './ets/lynx_autolink/LynxAutolinkStartupConfig.ets'
  );
  assert.ok(
    fs.existsSync(
      path.join(
        project,
        'entry',
        'src',
        'main',
        'ets',
        'lynx_autolink',
        'LynxAutolinkStartupConfig.ets'
      )
    )
  );
  assert.equal(
    fs.readFileSync(
      path.join(
        project,
        'entry',
        'src',
        'main',
        'ets',
        'lynx_autolink',
        '.gitignore'
      ),
      'utf8'
    ),
    '*\n'
  );
  assert.deepEqual(originalModuleJson, {
    module: { name: 'entry', type: 'entry' },
  });

  fs.rmSync(result.generatedSourceRoot, { recursive: true });
  lifecycle.runTask('generateLynxAutolink');
  assert.ok(
    fs.existsSync(
      path.join(
        result.generatedSourceRoot,
        'resources',
        'base',
        'profile',
        'lynx_autolink_startup.json'
      )
    )
  );
  assert.deepEqual(lifecycle.tasks[0].postDependencies, ['default@PreBuild']);
});

test('preserves existing startup tasks and config entry', () => {
  const project = createProject();
  const modulePath = path.join(project, 'entry');
  const profileDir = path.join(
    modulePath,
    'src',
    'main',
    'resources',
    'base',
    'profile'
  );
  fs.mkdirSync(profileDir, { recursive: true });
  writeJson(path.join(profileDir, 'startup.json'), {
    startupTasks: [
      { name: 'ExistingTask', srcEntry: './ets/ExistingTask.ets' },
    ],
    configEntry: './ets/ExistingConfig.ets',
  });
  const state = {
    dependencies: { '@lynx/lynx': '^3.5.0' },
    buildProfile: { targets: [{ name: 'default' }] },
    moduleJson: {
      module: {
        name: 'entry',
        type: 'entry',
        appStartup: '$profile:startup',
      },
    },
  };

  const { result } = configureAutolink(project, state);
  const generatedProfile = JSON.parse(
    fs.readFileSync(
      path.join(
        result.generatedSourceRoot,
        'resources',
        'base',
        'profile',
        'lynx_autolink_startup.json'
      ),
      'utf8'
    )
  );

  assert.equal(generatedProfile.configEntry, './ets/ExistingConfig.ets');
  assert.deepEqual(
    generatedProfile.startupTasks.map((task) => task.name),
    ['ExistingTask', 'LynxAutolinkStartupTask']
  );
});

test('rebases a local Lynx SDK dependency for the generated Registry HAR', () => {
  const project = createProject();
  createLibrary(project, '@example/provider', '@example/provider_harmony');
  setLynxDependency(project, 'entry', 'file:../sdk');
  const state = {
    dependencies: { '@lynx/lynx': 'file:../sdk' },
    buildProfile: { targets: [{ name: 'default' }] },
    moduleJson: { module: { name: 'entry', type: 'entry' } },
  };

  const { result } = configureAutolink(project, state);
  const registryPackage = JSON.parse(
    fs.readFileSync(path.join(result.registryDir, 'oh-package.json5'), 'utf8')
  );

  assert.equal(
    registryPackage.dependencies['@lynx/lynx'],
    'file:../../../../sdk'
  );
});

test('accepts an existing project module that resolves through a symlink', () => {
  const project = createProject();
  const packageRoot = createLibrary(
    project,
    '@example/demo',
    '@example/demo_harmony'
  );
  const linkedHarmonyDir = path.join(project, 'linked-demo-harmony');
  fs.symlinkSync(path.join(packageRoot, 'harmony'), linkedHarmonyDir, 'dir');
  const state = {
    dependencies: { '@lynx/lynx': '^3.5.0' },
    buildProfile: { targets: [{ name: 'default' }] },
    moduleJson: { module: { name: 'entry', type: 'entry' } },
  };

  const { descriptors } = configureAutolink(project, state, {
    descriptors: [
      { name: 'entry', srcPath: './entry' },
      { name: 'demo', srcPath: './linked-demo-harmony' },
    ],
    options: { moduleName: 'entry' },
  });

  assert.equal(
    descriptors.filter((descriptor) => descriptor.name === 'demo').length,
    1
  );
});

test('rejects a HAP module source path outside the project', () => {
  const project = createProject();
  const outsideProject = fs.mkdtempSync(
    path.join(os.tmpdir(), 'lynx-harmony-outside-')
  );
  tempDirs.push(outsideProject);
  createHapModule(outsideProject, 'entry', 'entry');

  assert.throws(
    () =>
      setupHarmonyAutolink(
        createHvigorConfig(project, [
          {
            name: 'entry',
            srcPath: path.relative(project, path.join(outsideProject, 'entry')),
          },
        ]),
        createLifecycle(),
        { moduleName: 'entry' },
        parseJson5File
      ),
    /Hvigor module entry source path escapes/
  );
});

test('rejects a HAP module symlink that resolves outside the project', () => {
  const project = createProject();
  const outsideProject = fs.mkdtempSync(
    path.join(os.tmpdir(), 'lynx-harmony-outside-')
  );
  tempDirs.push(outsideProject);
  createHapModule(outsideProject, 'entry', 'entry');
  fs.symlinkSync(
    path.join(outsideProject, 'entry'),
    path.join(project, 'linked-entry'),
    'dir'
  );

  assert.throws(
    () =>
      setupHarmonyAutolink(
        createHvigorConfig(project, [
          { name: 'entry', srcPath: './linked-entry' },
        ]),
        createLifecycle(),
        { moduleName: 'entry' },
        parseJson5File
      ),
    /Hvigor module entry source path escapes/
  );
});

test('rejects a dynamic HAR node that conflicts with an existing module', () => {
  const project = createProject();
  createLibrary(project, '@example/demo', '@example/demo_harmony');

  assert.throws(
    () =>
      setupHarmonyAutolink(
        createHvigorConfig(project, [
          { name: 'entry', srcPath: './entry' },
          { name: 'demo', srcPath: './another-demo' },
        ]),
        createLifecycle(),
        { moduleName: 'entry' },
        parseJson5File
      ),
    /Harmony module conflict for demo/
  );
});

test('isolates generated output by HAP module in the Hvigor cache', () => {
  const project = createProject();
  const hvigorConfig = createHvigorConfig(project);
  const lifecycle = createLifecycle();

  const result = setupHarmonyAutolink(
    hvigorConfig,
    lifecycle,
    { moduleName: 'entry' },
    parseJson5File
  );

  assert.equal(
    result.outputRoot,
    path.join(fs.realpathSync(project), '.hvigor', 'lynx-autolink', 'entry')
  );
});

test('infers the only HAP module that depends on Lynx', () => {
  const project = createProject();
  const state = createModuleState();

  const { result } = configureAutolink(project, state, { options: {} });

  assert.equal(result.moduleName, 'entry');
});

test('requires moduleName when multiple HAP modules depend on Lynx', () => {
  const project = createProject();
  createHapModule(project, 'feature', 'feature');
  const hvigorConfig = createHvigorConfig(project, [
    { name: 'entry', srcPath: './entry' },
    { name: 'feature', srcPath: './feature' },
  ]);

  assert.throws(
    () =>
      setupHarmonyAutolink(hvigorConfig, createLifecycle(), {}, parseJson5File),
    /found multiple Lynx HAP modules \(entry, feature\); set moduleName explicitly/
  );
});

function createProject() {
  const project = fs.mkdtempSync(
    path.join(os.tmpdir(), 'lynx-harmony-autolink-')
  );
  tempDirs.push(project);
  writeJson(path.join(project, 'package.json'), { name: 'app', private: true });
  writeJson(path.join(project, 'build-profile.json5'), {
    app: { products: [{ name: 'default' }] },
    modules: [{ name: 'entry', srcPath: './entry' }],
  });
  createHapModule(project, 'entry', 'entry');
  return project;
}

function createHapModule(project, moduleName, moduleType) {
  const modulePath = path.join(project, moduleName);
  writeJson(path.join(modulePath, 'oh-package.json5'), {
    name: moduleName,
    version: '1.0.0',
    dependencies: { '@lynx/lynx': '^3.5.0' },
  });
  writeJson(path.join(modulePath, 'build-profile.json5'), {
    apiType: 'stageMode',
    targets: [{ name: 'default' }],
  });
  writeJson(path.join(modulePath, 'src', 'main', 'module.json5'), {
    module: { name: moduleName, type: moduleType },
  });
}

function setLynxDependency(project, moduleName, dependency) {
  writeJson(path.join(project, moduleName, 'oh-package.json5'), {
    name: moduleName,
    version: '1.0.0',
    dependencies: { '@lynx/lynx': dependency },
  });
}

function createLibrary(project, npmName, ohPackageName, options = {}) {
  const parts = npmName.split('/');
  const packageRoot = path.join(project, 'node_modules', ...parts);
  const harmonyDir = path.join(packageRoot, 'harmony');
  fs.mkdirSync(path.join(harmonyDir, 'src', 'main'), { recursive: true });
  writeJson(path.join(packageRoot, 'package.json'), {
    name: npmName,
    version: '1.0.0',
  });
  writeJson(path.join(packageRoot, 'lynx.lib.json'), {
    platforms: {
      harmony: {
        packageDir: 'harmony',
        ...(options.harmonyManifest ?? {}),
      },
    },
  });
  if (options.ohPackageSource !== undefined) {
    fs.writeFileSync(
      path.join(harmonyDir, 'oh-package.json5'),
      options.ohPackageSource
    );
  } else {
    writeJson(path.join(harmonyDir, 'oh-package.json5'), {
      name: ohPackageName,
      main: 'Index.ets',
    });
  }
  fs.writeFileSync(
    path.join(harmonyDir, 'Index.ets'),
    'export class LynxLibraryProviderImpl {}\n'
  );
  writeJson(path.join(harmonyDir, 'src', 'main', 'module.json5'), {
    module: {
      name:
        options.moduleName ?? npmName.split('/').at(-1).replaceAll('-', '_'),
      type: 'har',
    },
  });
  writeJson(path.join(harmonyDir, 'build-profile.json5'), {
    apiType: 'stageMode',
    targets: [{ name: 'default' }],
  });
  fs.writeFileSync(
    path.join(harmonyDir, 'hvigorfile.ts'),
    "import { harTasks } from '@ohos/hvigor-ohos-plugin';\n\n" +
      'export default { system: harTasks, plugins: [] };\n'
  );
  return packageRoot;
}

function createContext(modulePath, state) {
  return {
    getModulePath: () => modulePath,
    getModuleType: () => 'entry',
    targets: (callback) => {
      for (const targetName of state.targetNames ?? ['default']) {
        callback({ getTargetName: () => targetName });
      }
    },
    getDependenciesOpt: () => state.dependencies,
    setDependenciesOpt: (value) => {
      state.dependencies = value;
    },
    getBuildProfileOpt: () => state.buildProfile,
    setBuildProfileOpt: (value) => {
      state.buildProfile = value;
    },
    getModuleJsonOpt: () => state.moduleJson,
    setModuleJsonOpt: (value) => {
      state.moduleJson = value;
    },
  };
}

function createModuleState() {
  return {
    dependencies: { '@lynx/lynx': '^3.5.0' },
    buildProfile: { targets: [{ name: 'default' }] },
    moduleJson: { module: { name: 'entry', type: 'entry' } },
  };
}

function createHvigorConfig(
  projectPath,
  initialDescriptors = [{ name: 'entry', srcPath: './entry' }]
) {
  const descriptors = initialDescriptors.map((descriptor) => ({
    ...descriptor,
    extraOptions: descriptor.extraOptions ?? {},
  }));
  return {
    descriptors,
    getRootNodeDescriptor: () => ({ name: 'app', srcPath: projectPath }),
    getAllNodeDescriptor: () => descriptors,
    getNodeDescriptorByName: (name) =>
      descriptors.find((descriptor) => descriptor.name === name),
    includeNode: (name, srcPath, extraOptions) => {
      descriptors.push({ name, srcPath, extraOptions });
    },
  };
}

function createLifecycle() {
  let afterNodeEvaluate;
  let nodesEvaluated;
  const tasks = [];
  return {
    tasks,
    afterNodeEvaluate(callback) {
      afterNodeEvaluate = callback;
    },
    nodesEvaluated(callback) {
      nodesEvaluated = callback;
    },
    evaluate(moduleName, context) {
      afterNodeEvaluate({
        getNodeName: () => moduleName,
        getContext: () => context,
        registerTask: (task) => tasks.push(task),
      });
      nodesEvaluated();
    },
    runTask(name) {
      const task = tasks.find((candidate) => candidate.name === name);
      assert.ok(task, `Missing task ${name}`);
      task.run();
    },
  };
}

function configureAutolink(project, state, config = {}) {
  const hvigorConfig = createHvigorConfig(project, config.descriptors);
  const lifecycle = createLifecycle();
  const options = config.options ?? { moduleName: 'entry' };
  const result = setupHarmonyAutolink(
    hvigorConfig,
    lifecycle,
    options,
    parseJson5File
  );
  lifecycle.evaluate(
    result.moduleName,
    createContext(path.join(project, result.moduleName), state)
  );
  return { descriptors: hvigorConfig.descriptors, lifecycle, result };
}

function libraryDescriptor(npmPackageName, ohPackageName) {
  return { npmPackageName, ohPackageName };
}

function writeJson(filePath, value) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, `${JSON.stringify(value, null, 2)}\n`);
}

function parseJson5File(filePath) {
  return JSON5.parse(fs.readFileSync(filePath, 'utf8'));
}
