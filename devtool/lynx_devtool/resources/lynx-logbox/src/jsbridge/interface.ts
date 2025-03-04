// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface ILogBoxBridge {
  addEventListeners(): void;
  getErrorData(): void;
  dismissError(): void;
  toastMessage(msg: string): void;
  changeView: (viewNumber: number) => void;
  deleteCurrentView: (viewNumber: number) => void;
  queryResource: (url: string) => Promise<string>;
}
