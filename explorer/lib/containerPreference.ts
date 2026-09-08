// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { supportsSparklingContainer } from './navigation';

export type PreferredContainer = 'legacy' | 'sparkling';

export const PREFERRED_CONTAINER_STORAGE_KEY = 'preferredContainer';

let cachedPreference: PreferredContainer | undefined;

function initialPreference(): PreferredContainer {
  const props = (lynx.__globalProps as unknown) as Record<string, unknown>;
  const value =
    props.explorerPreferredContainer ?? props.explorerQRContainerPreference;
  return value === 'sparkling' && supportsSparklingContainer()
    ? 'sparkling'
    : 'legacy';
}

export function getPreferredContainer(): PreferredContainer {
  cachedPreference ??= initialPreference();
  if (cachedPreference === 'sparkling' && !supportsSparklingContainer()) {
    return 'legacy';
  }
  return cachedPreference;
}

export function setPreferredContainer(value: PreferredContainer): void {
  'background only';
  cachedPreference =
    value === 'sparkling' && supportsSparklingContainer() ? value : 'legacy';
  NativeModules.ExplorerModule.saveToLocalStorage(
    PREFERRED_CONTAINER_STORAGE_KEY,
    cachedPreference
  );
}
