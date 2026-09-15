// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ConfigurationConstant } from '@kit.AbilityKit';
import { ColorScheme } from '@lynx/lynx';

export const THEME_PREFERENCE_KEY = 'preferredTheme';

export function normalizeThemePreference(preference?: string): string {
  switch (preference?.toLowerCase()) {
    case 'dark':
      return 'Dark';
    case 'light':
      return 'Light';
    default:
      return 'Auto';
  }
}

export function getThemePreference(): string {
  return normalizeThemePreference(AppStorage.get<string>(THEME_PREFERENCE_KEY));
}

export function setThemePreference(preference: string): void {
  AppStorage.setOrCreate(THEME_PREFERENCE_KEY, normalizeThemePreference(preference));
}

export function resolveColorScheme(
  preference: string,
  systemColorMode: ConfigurationConstant.ColorMode
): ColorScheme {
  const normalized = normalizeThemePreference(preference);
  if (normalized === 'Dark') {
    return ColorScheme.DARK;
  }
  if (normalized === 'Light') {
    return ColorScheme.LIGHT;
  }
  return systemColorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK
    ? ColorScheme.DARK
    : ColorScheme.LIGHT;
}
