// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface HarmonyLibraryInfo {
  npmName: string;
  packageDir: string;
  manifestPath: string;
  harmonyDir: string;
  sourceDir: string;
  ohPackageName: string;
  entry: string;
  moduleName: string;
}

export interface PrepareHarmonyAutolinkOptions {
  projectRoot?: string;
  consumerModule?: string;
  outputDir?: string;
  lynxDependency?: string;
}

export interface PrepareHarmonyAutolinkResult {
  libraries: HarmonyLibraryInfo[];
  consumerModuleDir: string;
  outputDir: string;
  generatedFiles: string[];
}

export interface HarmonyAutolinkPluginOptions
  extends PrepareHarmonyAutolinkOptions {
  taskName?: string;
}

export interface HvigorTaskLike {
  name: string;
  run(): void;
  postDependencies?: string[];
}

export interface HvigorNodeLike {
  registerTask(task: HvigorTaskLike): void;
}

export interface HvigorPluginLike {
  pluginId: string;
  apply(node: HvigorNodeLike): void;
}
