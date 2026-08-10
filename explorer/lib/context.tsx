// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { createContext, useContext, useState } from '@lynx-js/react';

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

  const safeArea: SafeAreaContext = {
    top: lynx.__globalProps.safeAreaTop || 0,
    bottom: lynx.__globalProps.safeAreaBottom || 0,
  };

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
