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

interface RouteTarget {
  value: string;
  rebuild(value: string): string;
}

function splitRoute(input: string) {
  const hashIndex = input.indexOf('#');
  const hash = hashIndex >= 0 ? input.slice(hashIndex) : '';
  const route = hashIndex >= 0 ? input.slice(0, hashIndex) : input;
  const queryIndex = route.indexOf('?');
  return {
    base: queryIndex >= 0 ? route.slice(0, queryIndex) : route,
    query: queryIndex >= 0 ? route.slice(queryIndex + 1) : '',
    hash,
  };
}

function legacyWrapperTarget(input: string): RouteTarget | null {
  const { base, query, hash } = splitRoute(input);
  if (!/^lynx:\/\/open$/i.test(base)) return null;

  // Preserve Explorer's historical unescaped `url=https://...&...` form:
  // native routing treats the complete tail as the nested URL.
  let offset = 0;
  for (const item of query.split('&')) {
    if (item.startsWith('url=')) {
      const valueStart = offset + 4;
      const rawTail = query.slice(valueStart);
      const separator = rawTail.indexOf('&');
      if (separator >= 0 && rawTail.slice(0, separator).includes('://')) {
        return {
          value: rawTail,
          rebuild: (value) =>
            `${base}?${query.slice(0, valueStart)}${value}${hash}`,
        };
      }
    }
    offset += item.length + 1;
  }

  const items = query.split('&');
  const targets = items.flatMap((item, index) => {
    const separator = item.indexOf('=');
    if (separator < 0) return [];
    try {
      return decodeURIComponent(item.slice(0, separator)) === 'url'
        ? [{ index, separator }]
        : [];
    } catch {
      return [];
    }
  });
  if (targets.length !== 1) return null;

  const { index, separator } = targets[0];
  try {
    const value = decodeURIComponent(items[index].slice(separator + 1));
    if (!value) return null;
    return {
      value,
      rebuild: (nextValue) => {
        const rawKey = items[index].slice(0, separator);
        const nextItems = [...items];
        nextItems[index] = `${rawKey}=${encodeURIComponent(nextValue)}`;
        return `${base}?${nextItems.join('&')}${hash}`;
      },
    };
  } catch {
    return null;
  }
}

function localBundleTarget(input: string): RouteTarget | null {
  const separator = input.indexOf('?');
  if (
    separator < 0 ||
    !/^file:\/\/lynx$/i.test(input.slice(0, separator)) ||
    !/^local:\/\//i.test(input.slice(separator + 1))
  ) {
    return null;
  }
  return {
    value: input.slice(separator + 1),
    rebuild: (value) => `${input.slice(0, separator + 1)}${value}`,
  };
}

function innermostRoute(input: string, depth = 0): string {
  if (depth >= 2) return input;
  const wrapper = legacyWrapperTarget(input);
  if (wrapper) return innermostRoute(wrapper.value, depth + 1);
  return localBundleTarget(input)?.value ?? input;
}

function updateInnermostRoute(
  input: string,
  update: (route: string) => string,
  depth = 0
): string {
  if (depth < 2) {
    const wrapper = legacyWrapperTarget(input);
    if (wrapper) {
      return wrapper.rebuild(
        updateInnermostRoute(wrapper.value, update, depth + 1)
      );
    }
  }
  const local = localBundleTarget(input);
  return local ? local.rebuild(update(local.value)) : update(input);
}

export function getLaunchQueryItem(
  input: string,
  key: string
): string | undefined {
  const { query } = splitRoute(innermostRoute(input));
  let result: string | undefined;
  for (const item of query.split('&')) {
    if (!item) continue;
    const separator = item.indexOf('=');
    if (separator < 0) continue;
    try {
      if (decodeURIComponent(item.slice(0, separator)) === key) {
        result = decodeURIComponent(item.slice(separator + 1));
      }
    } catch {
      // Malformed input stays editable; native validation owns the error.
    }
  }
  return result;
}

export function setLaunchQueryItem(
  input: string,
  key: string,
  value?: string
): string {
  return updateInnermostRoute(input, (route) => {
    const { base, query, hash } = splitRoute(route);
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
  });
}

export function isTruthyLaunchValue(value: string | undefined): boolean {
  if (value === undefined) return false;
  const normalized = value.trimStart().toLowerCase();
  if (normalized.startsWith('y') || normalized.startsWith('t')) return true;
  const integer = Number.parseInt(normalized, 10);
  return !Number.isNaN(integer) && integer !== 0;
}

export function runtimeRequiredByURL(input: string): ExplorerRuntime | null {
  return /^hybrid:\/\/lynxview_page(?:[/?#]|$)/i.test(
    innermostRoute(input).trim()
  )
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
