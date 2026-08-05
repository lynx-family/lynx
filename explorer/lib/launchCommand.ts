// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export type ExplorerRuntime = 'lynx' | 'sparkling';
export type CommandTheme = 'dark' | 'light' | null;

export interface LaunchCommand {
  readonly input: string;
  readonly runtime: ExplorerRuntime;
  readonly runtimeLocked: boolean;
  readonly fullscreen: boolean;
  readonly hiddenNav: boolean;
  readonly theme: CommandTheme;
}

function splitRoute(input: string) {
  const hashIndex = input.indexOf('#');
  const hash = hashIndex >= 0 ? input.slice(hashIndex) : '';
  const route = hashIndex >= 0 ? input.slice(0, hashIndex) : input;
  // Legacy wrappers put the resource query after another `?`. Container
  // options belong to that innermost resource.
  const queryIndex = route.lastIndexOf('?');
  return {
    base: queryIndex >= 0 ? route.slice(0, queryIndex) : route,
    query: queryIndex >= 0 ? route.slice(queryIndex + 1) : '',
    hash,
  };
}

export function getLaunchQueryItem(
  input: string,
  key: string
): string | undefined {
  const { query } = splitRoute(input);
  for (const item of query.split('&')) {
    if (!item) continue;
    const [rawKey, ...rawValue] = item.split('=');
    try {
      if (decodeURIComponent(rawKey) === key) {
        return decodeURIComponent(rawValue.join('=') || '');
      }
    } catch {
      // Malformed input stays editable; native validation owns the error.
    }
  }
  return undefined;
}

export function setLaunchQueryItem(
  input: string,
  key: string,
  value?: string
): string {
  const { base, query, hash } = splitRoute(input);
  const encodedKey = encodeURIComponent(key);
  const items = query
    .split('&')
    .filter(Boolean)
    .filter((item) => {
      const itemKey = item.split('=', 1)[0];
      try {
        return decodeURIComponent(itemKey) !== key;
      } catch {
        return itemKey !== encodedKey;
      }
    });
  if (value !== undefined) {
    items.push(`${encodedKey}=${encodeURIComponent(value)}`);
  }
  return `${base}${items.length ? `?${items.join('&')}` : ''}${hash}`;
}

export function isTruthyLaunchValue(value: string | undefined): boolean {
  return (
    value !== undefined && ['1', 'true', 'yes'].includes(value.toLowerCase())
  );
}

export function runtimeRequiredByURL(input: string): ExplorerRuntime | null {
  return /^hybrid:\/\/lynxview_page(?:[/?#]|$)/i.test(input.trim())
    ? 'sparkling'
    : null;
}

export function parseLaunchCommand(
  input: string,
  preferredRuntime: ExplorerRuntime
): LaunchCommand {
  const requiredRuntime = runtimeRequiredByURL(input);
  const runtime = requiredRuntime ?? preferredRuntime;
  const explicitFullscreen = isTruthyLaunchValue(
    getLaunchQueryItem(input, 'fullscreen')
  );
  const hideNav = isTruthyLaunchValue(
    getLaunchQueryItem(input, 'hide_nav_bar')
  );
  const hideStatus = isTruthyLaunchValue(
    getLaunchQueryItem(input, 'hide_status_bar')
  );
  const transparentStatus = isTruthyLaunchValue(
    getLaunchQueryItem(input, 'trans_status_bar')
  );
  const rawTheme =
    getLaunchQueryItem(input, 'force_theme_style') ??
    getLaunchQueryItem(input, 'back_button_style');

  return {
    input,
    runtime,
    runtimeLocked: requiredRuntime !== null,
    fullscreen:
      explicitFullscreen ||
      (runtime === 'sparkling' && hideNav && hideStatus && transparentStatus),
    hiddenNav:
      explicitFullscreen ||
      hideNav ||
      isTruthyLaunchValue(getLaunchQueryItem(input, 'hidden_nav')),
    theme: rawTheme === 'dark' || rawTheme === 'light' ? rawTheme : null,
  };
}

export function setCommandBoolean(
  command: LaunchCommand,
  option: 'fullscreen' | 'hiddenNav',
  enabled: boolean
): string {
  let route = command.input;
  if (option === 'fullscreen') {
    if (command.runtime === 'sparkling') {
      route = setLaunchQueryItem(route, 'fullscreen', undefined);
      route = setLaunchQueryItem(route, 'hidden_nav', undefined);
      route = setLaunchQueryItem(
        route,
        'hide_nav_bar',
        enabled ? 'true' : undefined
      );
      route = setLaunchQueryItem(
        route,
        'hide_status_bar',
        enabled ? 'true' : undefined
      );
      return setLaunchQueryItem(
        route,
        'trans_status_bar',
        enabled ? 'true' : undefined
      );
    }
    route = setLaunchQueryItem(route, 'hide_nav_bar', undefined);
    route = setLaunchQueryItem(route, 'trans_status_bar', undefined);
    return setLaunchQueryItem(
      route,
      'fullscreen',
      enabled ? 'true' : undefined
    );
  }

  const key = command.runtime === 'sparkling' ? 'hide_nav_bar' : 'hidden_nav';
  const alias = command.runtime === 'sparkling' ? 'hidden_nav' : 'hide_nav_bar';
  route = setLaunchQueryItem(route, alias, undefined);
  return setLaunchQueryItem(route, key, enabled ? 'true' : undefined);
}

export function setCommandTheme(
  command: LaunchCommand,
  theme: CommandTheme
): string {
  const sparkling = command.runtime === 'sparkling';
  let route = setLaunchQueryItem(
    command.input,
    sparkling ? 'back_button_style' : 'force_theme_style',
    undefined
  );
  route = setLaunchQueryItem(
    route,
    sparkling ? 'force_theme_style' : 'back_button_style',
    theme ?? undefined
  );
  route = setLaunchQueryItem(
    route,
    sparkling ? 'nav_bar_color' : 'bar_color',
    theme === null ? undefined : theme === 'dark' ? '181D25' : 'F0F2F5'
  );
  route = setLaunchQueryItem(
    route,
    'title_color',
    theme === null ? undefined : theme === 'dark' ? 'FFFFFF' : '000000'
  );
  return route;
}

export function retargetLaunchCommand(
  command: LaunchCommand,
  runtime: ExplorerRuntime
): string {
  if (command.runtimeLocked || command.runtime === runtime)
    return command.input;

  const keys = [
    'fullscreen',
    'hide_nav_bar',
    'hidden_nav',
    'trans_status_bar',
    'hide_status_bar',
    'force_theme_style',
    'back_button_style',
    'nav_bar_color',
    'bar_color',
    'title_color',
  ];
  let input = command.input;
  keys.forEach((key) => {
    input = setLaunchQueryItem(input, key, undefined);
  });

  const target: LaunchCommand = { ...command, input, runtime };
  if (command.fullscreen) {
    input = setCommandBoolean({ ...target, input }, 'fullscreen', true);
  } else if (command.hiddenNav) {
    input = setCommandBoolean({ ...target, input }, 'hiddenNav', true);
  }
  return setCommandTheme({ ...target, input }, command.theme);
}
