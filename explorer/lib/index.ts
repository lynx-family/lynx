// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export {
  isSparkling,
  isSparklingAvailable,
  supportsExplicitRouteOwnership,
  supportsSparklingContainer,
  openSchema,
  openWithSparkling,
  navigateTo,
  navigateBack,
} from './navigation';
export {
  SPARKLING_GO_EXTENSION_ID,
  SPARKLING_GO_ROOT_SCHEME,
  getAvailableExtensions,
  launchExtension,
} from './extensions';
export type {
  ExplorerExtension,
  ExplorerExtensionCapabilities,
} from './extensions';

export type { ContainerRequest } from './navigation';

export {
  getPreferredContainer,
  setPreferredContainer,
} from './containerPreference';
export type { PreferredContainer } from './containerPreference';

export { AppContextProvider, useTheme, useSafeArea } from './context';

export {
  getRecentSessions,
  addRecentSession,
  clearRecentSessions,
  getRecentUrls,
  addRecentUrl,
  clearRecentUrls,
} from './recentHistory';
export type { LaunchSession, LaunchSessionSource } from './recentHistory';
export {
  parseLaunchCommand,
  setCommandBoolean,
  setCommandTheme,
  retargetLaunchCommand,
  runtimeRequiredByURL,
} from './launchCommand';
export type {
  LaunchCommand,
  ExplorerRuntime,
  CommandTheme,
} from './launchCommand';

export type {
  ThemePreference,
  ResolvedTheme,
  ThemeContext,
  SafeAreaContext,
} from './context';
