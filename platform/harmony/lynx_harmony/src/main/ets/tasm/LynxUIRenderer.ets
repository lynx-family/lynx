// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface LynxUIRenderer {
  getUIDelegate(): number[];

  aboutToDisappear(): void;

  // TODO(chenyouhui): Move those interfaces to LynxTemplateRenderer later.
  updateRefreshRate(updateRefreshRate: number): void;

  updateViewport(width: number, height: number): void;

  requestLayout(): void;

  keyboardStatusChanged(height: number): void;

  setAvoidKeyboardListener(avoidKeyboardListener: (translateY: number, isKeyboardTransition: boolean) => void): void;

  onEnterForeground(): void;

  onEnterBackground(): void;
}
