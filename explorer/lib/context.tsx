// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { createContext, useContext, useEffect, useState } from '@lynx-js/react';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export type ThemePreference = 'Auto' | 'Light' | 'Dark';
export type ResolvedTheme = 'light' | 'dark';

export interface ThemeContext {
  /** User's preference: Auto, Light, or Dark */
  preference: ThemePreference;
  /** The effective theme after resolving Auto against the system theme */
  resolved: ResolvedTheme;
  /** Set the user's theme preference (persists via NativeModules) */
  setPreference: (theme: ThemePreference) => void;
  /** Map a base CSS class name to its themed variant, e.g. "page" → "page__dark" */
  withTheme: (className: string) => string;
}

export interface SafeAreaContext {
  top: number;
  bottom: number;
  left: number;
  right: number;
}

interface AppContextValue {
  theme: ThemeContext;
  safeArea: SafeAreaContext;
}

// ---------------------------------------------------------------------------
// Context
// ---------------------------------------------------------------------------

const AppContext = createContext<AppContextValue>(null!);

// ---------------------------------------------------------------------------
// Provider
// ---------------------------------------------------------------------------

export function AppContextProvider(props: { children: any }) {
  const [preference, setPreferenceState] = useState<ThemePreference>(() => {
    const stored = lynx.__globalProps.preferredTheme as string | undefined;
    if (stored && stored !== 'Auto') return stored as ThemePreference;
    // On child pages, preferredTheme in storage may be stale if the user just
    // toggled the theme. Use frontendTheme (set synchronously from URL params
    // by the native side) as a reliable hint of the parent page's resolved theme.
    const frontend = lynx.__globalProps.frontendTheme as string | undefined;
    if (frontend === 'dark') return 'Dark';
    return (stored as ThemePreference) || 'Auto';
  });

  const resolveTheme = (pref: ThemePreference): ResolvedTheme => {
    if (pref !== 'Auto') {
      return pref.toLowerCase() as ResolvedTheme;
    }
    return (
      (lynx.__globalProps.theme?.toLowerCase() as ResolvedTheme) || 'light'
    );
  };

  const resolved = resolveTheme(preference);

  const setPreference = (theme: ThemePreference) => {
    if (theme === preference) return;
    setPreferenceState(theme);
    NativeModules.ExplorerModule.saveThemePreferences('preferredTheme', theme);
  };

  // Output both `className` and `className__light` for light mode to support
  // SCSS files that use either convention (homepage uses __light, showcase uses
  // plain class names for the light variant).
  const withTheme = (className: string) =>
    resolved === 'dark'
      ? `${className}__dark`
      : `${className} ${className}__light`;

  const getSafeArea = (): SafeAreaContext => {
    const screenWidth = Number(lynx.__globalProps.screenWidth || 0);
    const screenHeight = Number(lynx.__globalProps.screenHeight || 0);
    const isPortrait =
      screenWidth > 0 && screenHeight > 0 ? screenHeight >= screenWidth : true;
    const isIOS = SystemInfo.platform === 'iOS';
    const isNotchScreen =
      lynx.__globalProps.isNotchScreen ||
      (lynx.__globalProps.safeAreaTop || 0) > 20 ||
      (lynx.__globalProps.safeAreaBottom || 0) > 0 ||
      (lynx.__globalProps.safeAreaLeft || 0) > 0 ||
      (lynx.__globalProps.safeAreaRight || 0) > 0;
    const portraitTopFallback = isNotchScreen ? 54 : 20;
    const portraitBottomFallback = isNotchScreen ? 34 : 0;
    const landscapeSideFallback = isIOS ? 54 : 0;
    return {
      top: Math.max(
        lynx.__globalProps.safeAreaTop || 0,
        isPortrait ? portraitTopFallback : 0
      ),
      bottom: Math.max(
        lynx.__globalProps.safeAreaBottom || 0,
        isPortrait ? portraitBottomFallback : 0
      ),
      left: Math.max(
        lynx.__globalProps.safeAreaLeft || 0,
        isPortrait ? 0 : landscapeSideFallback
      ),
      right: Math.max(
        lynx.__globalProps.safeAreaRight || 0,
        isPortrait ? 0 : landscapeSideFallback
      ),
    };
  };
  const [safeArea, setSafeArea] = useState<SafeAreaContext>(getSafeArea);

  useEffect(() => {
    const syncSafeArea = (data?: Record<string, unknown>) => {
      Object.assign(lynx.__globalProps, data || {});
      setSafeArea(getSafeArea());
    };
    const listener = (event: { data?: Record<string, unknown> }) => {
      syncSafeArea(event.data);
    };
    const coreContext = lynx.getCoreContext();
    coreContext.addEventListener('__NotifyGlobalPropsUpdated', listener);
    syncSafeArea();
    return () => {
      coreContext.removeEventListener('__NotifyGlobalPropsUpdated', listener);
    };
  }, []);

  return (
    <AppContext.Provider
      value={{
        theme: { preference, resolved, setPreference, withTheme },
        safeArea,
      }}
    >
      {props.children}
    </AppContext.Provider>
  );
}

// ---------------------------------------------------------------------------
// Hooks
// ---------------------------------------------------------------------------

export function useTheme(): ThemeContext {
  return useContext(AppContext).theme;
}

export function useSafeArea(): SafeAreaContext {
  return useContext(AppContext).safeArea;
}
