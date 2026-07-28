// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const fs = require('node:fs');
const path = require('node:path');

const REGISTRY_PACKAGE_NAME = '@lynx/lynx_autolink_registry';
const STARTUP_PROFILE_NAME = 'lynx_autolink_startup';
const STARTUP_TASK_NAME = 'LynxAutolinkStartupTask';
const GENERATION_TASK_NAME = 'generateLynxAutolink';
const REGISTRY_MODULE_NAME = 'lynx_autolink_registry';
const HAP_PLUGIN_ID = 'com.ohos.hap';
const CONFIGURED_HVIGOR_CONFIGS = new WeakSet();

/** Finds and validates all Harmony-enabled Lynx libraries visible to a HAP. */
function discoverHarmonyLibraries(startPath, readJson5File) {
  requireJson5FileReader(readJson5File);
  const packageRoots = [];
  const seenNodeModules = new Set();
  const seenPackages = new Set();

  for (const nodeModulesDir of findAncestorNodeModules(startPath)) {
    collectPackageRoots(
      nodeModulesDir,
      packageRoots,
      seenNodeModules,
      seenPackages
    );
  }

  const libraries = [];
  for (const packageRoot of packageRoots) {
    const manifestPath = path.join(packageRoot, 'lynx.lib.json');
    if (!fs.existsSync(manifestPath)) {
      continue;
    }

    const manifest = readJson(manifestPath);
    const harmony = manifest?.platforms?.harmony;
    if (harmony === undefined) {
      continue;
    }
    if (!isObject(harmony)) {
      throw new Error(`${manifestPath}: platforms.harmony must be an object`);
    }

    libraries.push(readHarmonyLibrary(packageRoot, harmony, readJson5File));
  }

  libraries.sort((left, right) =>
    compareText(left.npmPackageName, right.npmPackageName)
  );
  validateUniqueLibraries(libraries);
  return libraries;
}

/** Registers generated HAR nodes before Hvigor initializes the module graph. */
function setupHarmonyAutolink(
  hvigorConfig,
  lifecycle,
  options = {},
  parseJsonFile
) {
  if (
    hvigorConfig === undefined ||
    typeof hvigorConfig.getRootNodeDescriptor !== 'function' ||
    typeof hvigorConfig.getAllNodeDescriptor !== 'function' ||
    typeof hvigorConfig.includeNode !== 'function'
  ) {
    throw new Error('Harmony Lynx Autolink requires the Hvigor config API');
  }
  if (
    lifecycle === undefined ||
    typeof lifecycle.afterNodeEvaluate !== 'function' ||
    typeof lifecycle.nodesEvaluated !== 'function'
  ) {
    throw new Error('Harmony Lynx Autolink requires the Hvigor lifecycle API');
  }
  if (CONFIGURED_HVIGOR_CONFIGS.has(hvigorConfig)) {
    throw new Error(
      'Harmony Lynx Autolink is already enabled for this project'
    );
  }

  const prepared = prepareHarmonyAutolink(
    hvigorConfig,
    options,
    createJson5FileReader(parseJsonFile)
  );
  CONFIGURED_HVIGOR_CONFIGS.add(hvigorConfig);
  let hapConfigured = false;

  lifecycle.afterNodeEvaluate((node) => {
    if (node.getNodeName() !== prepared.moduleName) {
      return;
    }
    const context = node.getContext(HAP_PLUGIN_ID);
    if (context === undefined || context === null) {
      throw new Error(
        `Harmony Lynx Autolink requires ${prepared.moduleName} to use the HarmonyOS HAP plugin`
      );
    }
    const generation = configureHarmonyAutolinkHap(context, prepared);
    registerHapGenerationTask(node, prepared, generation);
    hapConfigured = true;
  });
  lifecycle.nodesEvaluated(() => {
    if (!hapConfigured) {
      throw new Error(
        `Harmony Lynx Autolink could not configure HAP module ${prepared.moduleName}`
      );
    }
  });

  return prepared;
}

/** Generates the Registry HAR and adds all required dynamic Hvigor nodes. */
function prepareHarmonyAutolink(hvigorConfig, options = {}, readJson5File) {
  requireJson5FileReader(readJson5File);
  const rootDescriptor = hvigorConfig.getRootNodeDescriptor();
  const projectPath = resolveComparablePath(
    requireNonEmptyString(
      rootDescriptor?.srcPath,
      'Hvigor root project source path'
    )
  );
  const target = resolveTargetHap(
    hvigorConfig,
    projectPath,
    options.moduleName,
    readJson5File
  );
  const modulePath = target.modulePath;
  const scanRoot = path.resolve(options.projectRoot ?? modulePath);
  const outputBase = path.join(projectPath, '.hvigor', 'lynx-autolink');
  const outputRoot = path.resolve(outputBase, target.moduleName);
  assertPathInside(outputBase, outputRoot, 'generated output directory');

  const libraries = discoverHarmonyLibraries(scanRoot, readJson5File);
  fs.rmSync(outputRoot, { force: true, recursive: true });
  const registryDir = path.join(outputRoot, 'registry');
  const generatedHapRoot = path.join(
    modulePath,
    'build',
    'generated',
    'lynx-autolink'
  );
  fs.rmSync(generatedHapRoot, { force: true, recursive: true });
  const generatedSourceRoot = path.join(generatedHapRoot, 'src', 'main');
  generateRegistryHar(
    registryDir,
    libraries,
    rebaseFileDependency(modulePath, registryDir, target.lynxDependency)
  );
  addHarNodesToHvigorConfig(
    hvigorConfig,
    projectPath,
    registryDir,
    libraries,
    readJson5File
  );

  return {
    ...target,
    libraries,
    outputRoot,
    registryDir,
    generatedSourceRoot,
    readJson5File,
  };
}

/** Applies generated dependencies and AppStartup configuration to the HAP. */
function configureHarmonyAutolinkHap(context, prepared) {
  const modulePath = resolveComparablePath(context.getModulePath());
  if (
    resolveComparablePath(modulePath) !==
    resolveComparablePath(prepared.modulePath)
  ) {
    throw new Error(
      `Harmony Lynx Autolink expected HAP module ${prepared.modulePath}, got ${modulePath}`
    );
  }
  const moduleType = context.getModuleType();
  if (moduleType !== 'entry' && moduleType !== 'feature') {
    throw new Error(
      `Harmony Lynx Autolink requires an entry or feature HAP module, got ${moduleType}`
    );
  }

  const dependencies = { ...(context.getDependenciesOpt() ?? {}) };
  const lynxDependency = dependencies['@lynx/lynx'];
  if (
    typeof lynxDependency !== 'string' ||
    lynxDependency.trim().length === 0
  ) {
    throw new Error(
      'Harmony Lynx Autolink requires @lynx/lynx in the HAP module dependencies'
    );
  }
  generateRegistryHar(
    prepared.registryDir,
    prepared.libraries,
    rebaseFileDependency(modulePath, prepared.registryDir, lynxDependency)
  );

  const moduleJson = context.getModuleJsonOpt();
  const buildProfile = context.getBuildProfileOpt();
  const existingStartup = readExistingStartup(
    modulePath,
    moduleJson,
    buildProfile,
    prepared.readJson5File
  );
  generateHapStartup(modulePath, prepared.generatedSourceRoot, existingStartup);

  dependencies[REGISTRY_PACKAGE_NAME] = toFileDependency(
    modulePath,
    prepared.registryDir
  );
  context.setDependenciesOpt(dependencies);
  context.setBuildProfileOpt(
    addGeneratedResourceDirectory(
      modulePath,
      buildProfile,
      prepared.generatedSourceRoot
    )
  );
  context.setModuleJsonOpt(setAppStartupProfile(moduleJson));

  return {
    existingStartup,
    targetNames: readHapTargetNames(context),
  };
}

function registerHapGenerationTask(node, prepared, generation) {
  if (typeof node.registerTask !== 'function') {
    throw new Error('Harmony Lynx Autolink requires the Hvigor task API');
  }
  node.registerTask({
    name: GENERATION_TASK_NAME,
    run() {
      generateHapStartup(
        prepared.modulePath,
        prepared.generatedSourceRoot,
        generation.existingStartup
      );
    },
    postDependencies: generation.targetNames.map(
      (targetName) => `${targetName}@PreBuild`
    ),
  });
}

function readHapTargetNames(context) {
  if (typeof context.targets !== 'function') {
    throw new Error('Harmony Lynx Autolink requires the HAP target API');
  }
  const targetNames = [];
  context.targets((target) => {
    targetNames.push(
      requireNonEmptyString(target?.getTargetName?.(), 'HAP target name')
    );
  });
  if (targetNames.length === 0) {
    throw new Error('Harmony Lynx Autolink requires at least one HAP target');
  }
  return [...new Set(targetNames)];
}

/** Emits the deterministic Registry HAR consumed by the generated startup task. */
function generateRegistryHar(outputDir, libraries, lynxDependency) {
  const sortedLibraries = sortLibraries(libraries);
  fs.mkdirSync(path.join(outputDir, 'src', 'main', 'ets'), {
    recursive: true,
  });

  const dependencies = { '@lynx/lynx': lynxDependency };
  for (const library of sortedLibraries) {
    dependencies[library.ohPackageName] = toFileDependency(
      outputDir,
      library.harmonyPackageDir
    );
  }

  writeJson(path.join(outputDir, 'oh-package.json5'), {
    name: REGISTRY_PACKAGE_NAME,
    version: '0.0.0',
    license: 'Apache-2.0',
    main: 'Index.ets',
    dependencies,
  });
  writeJson(path.join(outputDir, 'build-profile.json5'), {
    apiType: 'stageMode',
    buildOption: {
      arkOptions: { byteCodeHar: false },
    },
    targets: [{ name: 'default' }],
  });
  writeJson(path.join(outputDir, 'src', 'main', 'module.json5'), {
    module: {
      name: REGISTRY_MODULE_NAME,
      type: 'har',
      deviceTypes: ['default', 'tablet', '2in1'],
    },
  });
  fs.writeFileSync(
    path.join(outputDir, 'hvigorfile.ts'),
    "import { harTasks } from '@ohos/hvigor-ohos-plugin';\n\n" +
      'export default { system: harTasks, plugins: [] };\n'
  );
  fs.writeFileSync(
    path.join(outputDir, 'Index.ets'),
    generateRegistrySource(sortedLibraries)
  );
}

/** Returns the stable ArkTS provider list for a generated Registry HAR. */
function generateRegistrySource(libraries) {
  const sortedLibraries = sortLibraries(libraries);
  const imports = sortedLibraries.map(
    (library, index) =>
      `import { LynxLibraryProviderImpl as Provider${index} } from '${library.ohPackageName}';`
  );
  const entries = sortedLibraries.map(
    (library, index) =>
      `  { packageName: ${JSON.stringify(library.npmPackageName)}, provider: new Provider${index}() }`
  );

  return `// Generated by @lynx/lynx-library-plugin. Do not edit.
import { LynxLibraryProviderEntry, LynxLibraryRegistry } from '@lynx/lynx';
${imports.length > 0 ? `${imports.join('\n')}\n` : ''}
const PROVIDERS: LynxLibraryProviderEntry[] = [
${entries.join(',\n')}
];

export function setupGlobal(): void {
  LynxLibraryRegistry.setupGlobal(PROVIDERS);
}
`;
}

function sortLibraries(libraries) {
  return [...libraries].sort((left, right) =>
    compareText(left.npmPackageName, right.npmPackageName)
  );
}

function readHarmonyLibrary(packageRoot, harmony, readJson5File) {
  const npmPackageJsonPath = path.join(packageRoot, 'package.json');
  const npmPackageJson = readJson(npmPackageJsonPath);
  const npmPackageName = requireNonEmptyString(
    npmPackageJson.name,
    `${npmPackageJsonPath}: name`
  );
  const packageDirValue = harmony.packageDir ?? 'harmony';
  const packageDir = requireNonEmptyString(
    packageDirValue,
    `${path.join(packageRoot, 'lynx.lib.json')}: platforms.harmony.packageDir`
  );
  const harmonyPackageDir = path.resolve(packageRoot, packageDir);
  assertPathInside(
    packageRoot,
    harmonyPackageDir,
    'platforms.harmony.packageDir'
  );
  if (
    !fs.existsSync(harmonyPackageDir) ||
    !fs.statSync(harmonyPackageDir).isDirectory()
  ) {
    throw new Error(
      `${npmPackageName}: Harmony package directory does not exist: ${harmonyPackageDir}`
    );
  }
  assertRealPathInside(
    packageRoot,
    harmonyPackageDir,
    'Harmony package directory'
  );

  const ohPackagePath = path.join(harmonyPackageDir, 'oh-package.json5');
  const ohPackage = readJson5File(ohPackagePath);
  const ohPackageName = requireNonEmptyString(
    ohPackage.name,
    `${ohPackagePath}: name`
  );
  const entry =
    ohPackage.main === undefined || ohPackage.main === ''
      ? 'Index.ets'
      : requireNonEmptyString(ohPackage.main, `${ohPackagePath}: main`);
  const entryPath = path.resolve(harmonyPackageDir, entry);
  assertPathInside(harmonyPackageDir, entryPath, 'Harmony package main entry');
  if (!fs.existsSync(entryPath) || !fs.statSync(entryPath).isFile()) {
    throw new Error(
      `${npmPackageName}: Harmony package main entry does not exist: ${entryPath}`
    );
  }
  assertRealPathInside(
    harmonyPackageDir,
    entryPath,
    'Harmony package main entry'
  );

  const moduleJsonPath = path.join(
    harmonyPackageDir,
    'src',
    'main',
    'module.json5'
  );
  const moduleJson = readJson5File(moduleJsonPath);
  if (moduleJson?.module?.type !== 'har') {
    throw new Error(`${moduleJsonPath}: module.type must be "har"`);
  }
  const harmonyModuleName = requireNonEmptyString(
    moduleJson.module.name,
    `${moduleJsonPath}: module.name`
  );
  const harmonyBuildProfilePath = path.join(
    harmonyPackageDir,
    'build-profile.json5'
  );
  const harmonyBuildProfile = readJson5File(harmonyBuildProfilePath);
  const targetNames = readHarTargetNames(
    harmonyBuildProfile,
    harmonyBuildProfilePath
  );

  return {
    npmPackageName,
    packageRoot,
    harmonyPackageDir,
    ohPackageName,
    harmonyModuleName,
    targetNames,
  };
}

function readHarTargetNames(buildProfile, buildProfilePath) {
  if (
    !Array.isArray(buildProfile?.targets) ||
    buildProfile.targets.length === 0
  ) {
    throw new Error(`${buildProfilePath}: targets must be a non-empty array`);
  }
  const targetNames = [];
  const seen = new Set();
  for (const target of buildProfile.targets) {
    const name = requireNonEmptyString(
      target?.name,
      `${buildProfilePath}: target.name`
    );
    if (seen.has(name)) {
      throw new Error(`${buildProfilePath}: duplicate target name "${name}"`);
    }
    seen.add(name);
    targetNames.push(name);
  }
  return targetNames;
}

function validateUniqueLibraries(libraries) {
  const npmNames = new Map();
  const ohPackageNames = new Map();
  const moduleNames = new Map();

  for (const library of libraries) {
    assertUniqueLibraryName(
      npmNames,
      library.npmPackageName,
      library.packageRoot,
      'npm package name'
    );
    assertUniqueLibraryName(
      ohPackageNames,
      library.ohPackageName,
      library.packageRoot,
      'OHPM package name'
    );
    assertUniqueLibraryName(
      moduleNames,
      library.harmonyModuleName,
      library.packageRoot,
      'Harmony module name'
    );
    if (library.ohPackageName === REGISTRY_PACKAGE_NAME) {
      throw new Error(
        `${library.npmPackageName}: OHPM package name ${REGISTRY_PACKAGE_NAME} is reserved`
      );
    }
    if (library.harmonyModuleName === REGISTRY_MODULE_NAME) {
      throw new Error(
        `${library.npmPackageName}: Harmony module name ${REGISTRY_MODULE_NAME} is reserved`
      );
    }
  }
}

function assertUniqueLibraryName(values, name, packageRoot, kind) {
  const previousRoot = values.get(name);
  if (previousRoot !== undefined) {
    throw new Error(
      `Duplicate ${kind} "${name}" in ${previousRoot} and ${packageRoot}`
    );
  }
  values.set(name, packageRoot);
}

function findAncestorNodeModules(startPath) {
  const result = [];
  let current = path.resolve(startPath);
  if (fs.existsSync(current) && fs.statSync(current).isFile()) {
    current = path.dirname(current);
  }

  while (true) {
    const candidate = path.join(current, 'node_modules');
    if (fs.existsSync(candidate) && fs.statSync(candidate).isDirectory()) {
      result.push(candidate);
    }
    const parent = path.dirname(current);
    if (parent === current) {
      break;
    }
    current = parent;
  }
  return result;
}

function collectPackageRoots(
  nodeModulesDir,
  packageRoots,
  seenNodeModules,
  seenPackages
) {
  const realNodeModules = fs.realpathSync(nodeModulesDir);
  if (seenNodeModules.has(realNodeModules)) {
    return;
  }
  seenNodeModules.add(realNodeModules);

  for (const entry of fs
    .readdirSync(nodeModulesDir, { withFileTypes: true })
    .sort((left, right) => compareText(left.name, right.name))) {
    if (entry.name.startsWith('.')) {
      continue;
    }
    const entryPath = path.join(nodeModulesDir, entry.name);
    if (entry.name.startsWith('@') && isDirectoryOrSymlink(entryPath)) {
      for (const scopedEntry of fs
        .readdirSync(entryPath, { withFileTypes: true })
        .sort((left, right) => compareText(left.name, right.name))) {
        const packagePath = path.join(entryPath, scopedEntry.name);
        if (isDirectoryOrSymlink(packagePath)) {
          collectOnePackage(
            packagePath,
            packageRoots,
            seenNodeModules,
            seenPackages
          );
        }
      }
    } else if (isDirectoryOrSymlink(entryPath)) {
      collectOnePackage(entryPath, packageRoots, seenNodeModules, seenPackages);
    }
  }
}

function collectOnePackage(
  packagePath,
  packageRoots,
  seenNodeModules,
  seenPackages
) {
  const packageRoot = fs.realpathSync(packagePath);
  if (
    !seenPackages.has(packageRoot) &&
    fs.existsSync(path.join(packageRoot, 'package.json'))
  ) {
    seenPackages.add(packageRoot);
    packageRoots.push(packageRoot);
  }
  const nestedNodeModules = path.join(packageRoot, 'node_modules');
  if (
    fs.existsSync(nestedNodeModules) &&
    fs.statSync(nestedNodeModules).isDirectory()
  ) {
    collectPackageRoots(
      nestedNodeModules,
      packageRoots,
      seenNodeModules,
      seenPackages
    );
  }
}

function isDirectoryOrSymlink(filePath) {
  const stat = fs.lstatSync(filePath);
  return stat.isDirectory() || stat.isSymbolicLink();
}

function resolveTargetHap(
  hvigorConfig,
  projectPath,
  moduleName,
  readJson5File
) {
  const descriptors = hvigorConfig.getAllNodeDescriptor();
  if (!Array.isArray(descriptors)) {
    throw new Error('Hvigor config did not provide module descriptors');
  }

  if (moduleName !== undefined) {
    const targetName = requireNonEmptyString(
      moduleName,
      'Harmony Lynx Autolink moduleName'
    );
    const descriptor =
      hvigorConfig.getNodeDescriptorByName?.(targetName) ??
      descriptors.find((candidate) => candidate?.name === targetName);
    if (descriptor === undefined) {
      throw new Error(
        `Harmony Lynx Autolink cannot find module ${targetName} in the Hvigor project`
      );
    }
    return readLynxHapDescriptor(projectPath, descriptor, true, readJson5File);
  }

  const candidates = descriptors
    .map((descriptor) =>
      readLynxHapDescriptor(projectPath, descriptor, false, readJson5File)
    )
    .filter((candidate) => candidate !== undefined);
  if (candidates.length === 1) {
    return candidates[0];
  }
  if (candidates.length === 0) {
    throw new Error(
      'Harmony Lynx Autolink cannot find an entry or feature HAP module that depends on @lynx/lynx'
    );
  }
  throw new Error(
    `Harmony Lynx Autolink found multiple Lynx HAP modules (${candidates
      .map((candidate) => candidate.moduleName)
      .join(', ')}); set moduleName explicitly`
  );
}

function readLynxHapDescriptor(
  projectPath,
  descriptor,
  required,
  readJson5File
) {
  const moduleName = requireNonEmptyString(
    descriptor?.name,
    'Hvigor module name'
  );
  const srcPath = requireNonEmptyString(
    descriptor?.srcPath,
    `Hvigor module ${moduleName} source path`
  );
  const modulePath = path.resolve(projectPath, srcPath);
  const modulePathLabel = `Hvigor module ${moduleName} source path`;
  assertPathInside(projectPath, modulePath, modulePathLabel);
  const moduleJsonPath = path.join(modulePath, 'src', 'main', 'module.json5');
  if (!fs.existsSync(moduleJsonPath)) {
    if (!required) {
      return undefined;
    }
    throw new Error(
      `Harmony Lynx Autolink cannot find ${moduleJsonPath} for module ${moduleName}`
    );
  }
  assertRealPathInside(projectPath, modulePath, modulePathLabel);

  const moduleType = readJson5File(moduleJsonPath)?.module?.type;
  if (moduleType !== 'entry' && moduleType !== 'feature') {
    if (!required) {
      return undefined;
    }
    throw new Error(
      `Harmony Lynx Autolink requires module ${moduleName} to be an entry or feature HAP module`
    );
  }

  const ohPackagePath = path.join(modulePath, 'oh-package.json5');
  if (!fs.existsSync(ohPackagePath)) {
    if (!required) {
      return undefined;
    }
    throw new Error(
      `Harmony Lynx Autolink cannot find ${ohPackagePath} for module ${moduleName}`
    );
  }
  const lynxDependency =
    readJson5File(ohPackagePath)?.dependencies?.['@lynx/lynx'];
  if (
    typeof lynxDependency !== 'string' ||
    lynxDependency.trim().length === 0
  ) {
    if (!required) {
      return undefined;
    }
    throw new Error(
      `Harmony Lynx Autolink requires @lynx/lynx in module ${moduleName} dependencies`
    );
  }

  return { moduleName, modulePath, moduleType, lynxDependency };
}

function addHarNodesToHvigorConfig(
  hvigorConfig,
  projectPath,
  registryDir,
  libraries,
  readJson5File
) {
  const productNames = readProjectProductNames(projectPath, readJson5File);
  addHvigorNode(
    hvigorConfig,
    projectPath,
    REGISTRY_MODULE_NAME,
    registryDir,
    ['default'],
    productNames
  );
  for (const library of libraries) {
    addHvigorNode(
      hvigorConfig,
      projectPath,
      library.harmonyModuleName,
      library.harmonyPackageDir,
      library.targetNames,
      productNames
    );
  }
}

function readProjectProductNames(projectPath, readJson5File) {
  const buildProfilePath = path.join(projectPath, 'build-profile.json5');
  const buildProfile = readJson5File(buildProfilePath);
  if (
    !Array.isArray(buildProfile?.app?.products) ||
    buildProfile.app.products.length === 0
  ) {
    throw new Error(
      `${buildProfilePath}: app.products must be a non-empty array`
    );
  }

  const result = [];
  const seen = new Set();
  for (const product of buildProfile.app.products) {
    const name = requireNonEmptyString(
      product?.name,
      `${buildProfilePath}: app product name`
    );
    if (seen.has(name)) {
      throw new Error(`${buildProfilePath}: duplicate product name "${name}"`);
    }
    seen.add(name);
    result.push(name);
  }
  return result;
}

function addHvigorNode(
  hvigorConfig,
  projectPath,
  moduleName,
  modulePath,
  targetNames,
  productNames
) {
  const resolvedModulePath = resolveComparablePath(modulePath);
  const conflicts = hvigorConfig.getAllNodeDescriptor().filter((descriptor) => {
    const descriptorPath =
      typeof descriptor?.srcPath === 'string'
        ? resolveComparablePath(projectPath, descriptor.srcPath)
        : undefined;
    return (
      descriptor?.name === moduleName || descriptorPath === resolvedModulePath
    );
  });
  if (conflicts.length > 0) {
    const matches = conflicts.every(
      (descriptor) =>
        descriptor.name === moduleName &&
        resolveComparablePath(projectPath, descriptor.srcPath) ===
          resolvedModulePath
    );
    if (!matches) {
      throw new Error(
        `Harmony module conflict for ${moduleName} at ${resolvedModulePath}`
      );
    }
    return;
  }

  hvigorConfig.includeNode(
    moduleName,
    toModuleRelativePath(projectPath, resolvedModulePath),
    {
      targets: targetNames.map((name) => ({
        name,
        applyToProducts: productNames,
      })),
    }
  );
}

function readExistingStartup(
  modulePath,
  moduleJson,
  buildProfile,
  readJson5File
) {
  const appStartup = moduleJson?.module?.appStartup;
  if (appStartup === undefined) {
    return undefined;
  }
  if (typeof appStartup !== 'string' || !appStartup.startsWith('$profile:')) {
    throw new Error(
      'module.appStartup must use a $profile: resource reference'
    );
  }
  const profileName = appStartup.slice('$profile:'.length);
  const candidates = new Set([
    path.join(modulePath, 'src', 'main', 'resources'),
    ...readConfiguredResourceDirs(modulePath, buildProfile),
  ]);
  const matches = [];
  for (const resourcesDir of candidates) {
    const candidate = path.join(
      resourcesDir,
      'base',
      'profile',
      `${profileName}.json`
    );
    if (fs.existsSync(candidate)) {
      matches.push(candidate);
    }
  }
  if (matches.length === 0) {
    throw new Error(`Cannot resolve existing AppStartup profile ${appStartup}`);
  }
  if (matches.length > 1) {
    throw new Error(
      `AppStartup profile ${appStartup} resolves to multiple resource directories`
    );
  }
  const startup = readJson5File(matches[0]);
  if (!Array.isArray(startup.startupTasks)) {
    throw new Error(`${matches[0]}: startupTasks must be an array`);
  }
  if (
    typeof startup.configEntry !== 'string' ||
    startup.configEntry.length === 0
  ) {
    throw new Error(`${matches[0]}: configEntry must be a non-empty string`);
  }
  return startup;
}

function readConfiguredResourceDirs(modulePath, buildProfile) {
  const result = [];
  for (const target of buildProfile?.targets ?? []) {
    for (const directory of target?.resource?.directories ?? []) {
      result.push(path.resolve(modulePath, directory));
    }
  }
  return result;
}

function generateHapStartup(modulePath, sourceRoot, existingStartup) {
  const etsDir = path.join(sourceRoot, 'ets', 'lynx_autolink');
  const profileDir = path.join(sourceRoot, 'resources', 'base', 'profile');
  const taskEntry = toStartupEntry(
    modulePath,
    path.join(etsDir, `${STARTUP_TASK_NAME}.ets`)
  );
  const configEntry = toStartupEntry(
    modulePath,
    path.join(etsDir, 'LynxAutolinkStartupConfig.ets')
  );
  fs.mkdirSync(etsDir, { recursive: true });
  fs.mkdirSync(profileDir, { recursive: true });

  fs.writeFileSync(
    path.join(etsDir, `${STARTUP_TASK_NAME}.ets`),
    `// Generated by @lynx/lynx-library-plugin. Do not edit.
import { common, StartupTask } from '@kit.AbilityKit';
import { setupGlobal } from '${REGISTRY_PACKAGE_NAME}';

@Sendable
export default class ${STARTUP_TASK_NAME} extends StartupTask {
  constructor() {
    super();
  }

  async init(_context: common.AbilityStageContext): Promise<void> {
    setupGlobal();
  }
}
`
  );

  let startup =
    existingStartup === undefined
      ? {
          startupTasks: [],
          configEntry,
        }
      : { ...existingStartup, startupTasks: [...existingStartup.startupTasks] };
  const duplicateTask = startup.startupTasks.find(
    (task) => task?.name === STARTUP_TASK_NAME
  );
  if (duplicateTask !== undefined) {
    throw new Error(
      `AppStartup task name ${STARTUP_TASK_NAME} is reserved by Lynx Autolink`
    );
  }
  startup.startupTasks.push({
    name: STARTUP_TASK_NAME,
    srcEntry: taskEntry,
    runOnThread: 'mainThread',
    waitOnMainThread: true,
  });

  if (existingStartup === undefined) {
    fs.writeFileSync(
      path.join(etsDir, 'LynxAutolinkStartupConfig.ets'),
      `// Generated by @lynx/lynx-library-plugin. Do not edit.
import { StartupConfig, StartupConfigEntry } from '@kit.AbilityKit';

export default class LynxAutolinkStartupConfig extends StartupConfigEntry {
  onConfig(): StartupConfig {
    return { timeoutMs: 10000 };
  }
}
`
    );
  }

  writeJson(path.join(profileDir, `${STARTUP_PROFILE_NAME}.json`), startup);
}

function addGeneratedResourceDirectory(modulePath, buildProfile, sourceRoot) {
  const resourcesValue = toModuleRelativePath(
    modulePath,
    path.join(sourceRoot, 'resources')
  );
  const targets =
    buildProfile?.targets?.length > 0
      ? buildProfile.targets
      : [{ name: 'default' }];

  return {
    ...buildProfile,
    targets: targets.map((target) => ({
      ...target,
      resource: {
        ...(target.resource ?? {}),
        directories: appendUnique(
          target.resource?.directories ?? ['./src/main/resources'],
          resourcesValue
        ),
      },
    })),
  };
}

function setAppStartupProfile(moduleJson) {
  if (!isObject(moduleJson?.module)) {
    throw new Error('module.json5 must define a module object');
  }
  return {
    ...moduleJson,
    module: {
      ...moduleJson.module,
      appStartup: `$profile:${STARTUP_PROFILE_NAME}`,
    },
  };
}

function appendUnique(values = [], value) {
  return values.includes(value) ? [...values] : [...values, value];
}

function toModuleRelativePath(modulePath, target) {
  const relative = toPosixPath(path.relative(modulePath, target));
  return relative === '..' || relative.startsWith('../')
    ? relative
    : `./${relative}`;
}

function resolveComparablePath(...parts) {
  const resolved = path.resolve(...parts);
  return fs.existsSync(resolved) ? fs.realpathSync(resolved) : resolved;
}

function toFileDependency(fromDir, targetDir) {
  return `file:${toModuleRelativePath(fromDir, targetDir)}`;
}

function rebaseFileDependency(fromDir, targetDir, dependency) {
  if (!dependency.startsWith('file:')) {
    return dependency;
  }
  const dependencyPath = dependency.slice('file:'.length);
  if (dependencyPath.length === 0) {
    throw new Error('Harmony file dependency must include a path');
  }
  return toFileDependency(targetDir, path.resolve(fromDir, dependencyPath));
}

function toStartupEntry(modulePath, target) {
  const entry = toModuleRelativePath(
    path.join(modulePath, 'src', 'main'),
    target
  );
  if (entry.length > 127) {
    throw new Error(
      `Generated AppStartup entry exceeds 127 characters: ${entry}`
    );
  }
  return entry;
}

function assertPathInside(root, target, label) {
  const relative = path.relative(path.resolve(root), path.resolve(target));
  if (
    relative === '..' ||
    relative.startsWith(`..${path.sep}`) ||
    path.isAbsolute(relative)
  ) {
    throw new Error(`${label} escapes ${root}: ${target}`);
  }
}

function assertRealPathInside(root, target, label) {
  assertPathInside(fs.realpathSync(root), fs.realpathSync(target), label);
}

function readJson(filePath) {
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf8'));
  } catch (error) {
    throw new Error(`${filePath}: ${error.message}`);
  }
}

function createJson5FileReader(parseJsonFile) {
  if (typeof parseJsonFile !== 'function') {
    throw new Error(
      'Harmony Lynx Autolink requires @ohos/hvigor.parseJsonFile'
    );
  }
  return (filePath) => {
    try {
      return parseJsonFile(filePath);
    } catch (error) {
      throw new Error(`${filePath}: ${error.message}`);
    }
  };
}

function requireJson5FileReader(readJson5File) {
  if (typeof readJson5File !== 'function') {
    throw new Error('Harmony Lynx Autolink requires a JSON5 file reader');
  }
}

function requireNonEmptyString(value, label) {
  if (typeof value !== 'string' || value.trim().length === 0) {
    throw new Error(`${label} must be a non-empty string`);
  }
  return value;
}

function isObject(value) {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function writeJson(filePath, value) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, `${JSON.stringify(value, null, 2)}\n`);
}

function toPosixPath(filePath) {
  return filePath.split(path.sep).join('/');
}

function compareText(left, right) {
  return left < right ? -1 : left > right ? 1 : 0;
}

module.exports = {
  REGISTRY_PACKAGE_NAME,
  STARTUP_PROFILE_NAME,
  STARTUP_TASK_NAME,
  configureHarmonyAutolinkHap,
  discoverHarmonyLibraries,
  generateRegistryHar,
  generateRegistrySource,
  prepareHarmonyAutolink,
  setupHarmonyAutolink,
};
