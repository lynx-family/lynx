// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export {
  isSparkling,
  isSparklingAvailable,
  openSchema,
  navigateTo,
  navigateBack,
} from './navigation';

export { AppContextProvider, useTheme, useSafeArea } from './context';

export { getRecentUrls, addRecentUrl, clearRecentUrls } from './recentHistory';

export type {
  ThemePreference,
  ResolvedTheme,
  ThemeContext,
  SafeAreaContext,
} from './context';
