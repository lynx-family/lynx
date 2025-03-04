// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

declare module "*.png?inline";

declare let NativeModules: {
  ExplorerModule: {
    openScan(): void;
    openSchema(url: string): void;
    getSettingInfo(): any;
    setThreadMode(index: number): void;
    openDevtoolSwitchPage(): void;
  };
};
