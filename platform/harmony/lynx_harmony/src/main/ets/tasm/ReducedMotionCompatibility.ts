// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import accessibility from '@ohos.accessibility';

export type AnimationReduceStateChangeCallback = (enabled: boolean) => void;

interface AccessibilityCompatibilityApi {
  isAnimationReduceEnabledSync?: () => boolean;
  onAnimationReduceStateChange?: (
    callback: AnimationReduceStateChangeCallback
  ) => void;
  offAnimationReduceStateChange?: (
    callback: AnimationReduceStateChangeCallback
  ) => void;
}

function getAccessibilityApi(): AccessibilityCompatibilityApi {
  return accessibility as AccessibilityCompatibilityApi;
}

export function isAnimationReduceAvailable(): boolean {
  const api = getAccessibilityApi();
  return (
    typeof api.isAnimationReduceEnabledSync === 'function' &&
    typeof api.onAnimationReduceStateChange === 'function'
  );
}

export function isAnimationReduceEnabled(): boolean {
  return getAccessibilityApi().isAnimationReduceEnabledSync?.() ?? false;
}

export function onAnimationReduceStateChange(
  callback: AnimationReduceStateChangeCallback
): void {
  getAccessibilityApi().onAnimationReduceStateChange?.(callback);
}

export function offAnimationReduceStateChange(
  callback: AnimationReduceStateChangeCallback
): void {
  getAccessibilityApi().offAnimationReduceStateChange?.(callback);
}
