// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { beforeEach, describe, expect, it, vi } from 'vitest';
import * as navigationModule from './navigation';
import { clearRecentUrls, getRecentUrls } from './recentHistory';

const sparklingMocks = vi.hoisted(() => ({
  close: vi.fn(),
  navigate: vi.fn(),
  open: vi.fn(),
}));

vi.mock('sparkling-navigation', () => ({
  close: sparklingMocks.close,
  navigate: sparklingMocks.navigate,
  open: sparklingMocks.open,
}));

type ContainerRequest = 'automatic' | 'legacy' | 'sparkling';

interface NativeRouteResult {
  success: boolean;
  code: string;
  message: string;
}

type NativeRouteCallback = (result: NativeRouteResult) => void;
type SparklingCallback = (result: { code: number; msg: string }) => void;

interface NavigationAPI {
  supportsExplicitRouteOwnership(): boolean;
  supportsSparklingContainer(): boolean;
  openSchema(url: string, container?: ContainerRequest): Promise<void>;
  openWithSparkling(url: string): Promise<void>;
  navigateTo(
    path: string,
    params?: Record<string, string | number>
  ): Promise<void>;
  navigateBack(): Promise<void>;
}

const navigation = (navigationModule as unknown) as NavigationAPI;

const nativeOpen = vi.fn();
const legacyNativeOpen = vi.fn();
const nativeBack = vi.fn();

function installLynxGlobals(
  sparkling: boolean,
  capabilities: {
    openRoute?: boolean;
    navigateBack?: boolean;
    explicitRouteOwnership?: unknown;
    sparklingContainer?: unknown;
    iOSNumericBooleanProps?: boolean;
  } = {
    openRoute: true,
    navigateBack: true,
    explicitRouteOwnership: true,
    sparklingContainer: true,
  }
): void {
  const storage = new Map<string, string>();
  const explorerModule: Record<string, unknown> = {
    openSchema: legacyNativeOpen,
  };
  if (capabilities.openRoute !== false) {
    explorerModule.openRoute = nativeOpen;
  }
  if (capabilities.navigateBack !== false) {
    explorerModule.navigateBack = nativeBack;
  }
  const globalProps: Record<string, unknown> = sparkling
    ? {
        containerID: 'sparkling-container',
        sparklingAvailable: true,
        sparklingNavigation: true,
      }
    : {};
  if (capabilities.explicitRouteOwnership !== undefined) {
    globalProps.explorerSupportsExplicitRouteOwnership =
      capabilities.explicitRouteOwnership;
  }
  if (capabilities.sparklingContainer !== undefined) {
    globalProps.explorerSupportsSparklingContainer =
      capabilities.sparklingContainer;
  }
  if (capabilities.iOSNumericBooleanProps) {
    globalProps.explorerSupportsExplicitRouteOwnership = 1;
    globalProps.explorerSupportsSparklingContainer = 1;
    globalProps.sparklingAvailable = 1;
    globalProps.sparklingNavigation = 1;
    delete globalProps.containerID;
  }
  vi.stubGlobal('lynx', {
    __globalProps: globalProps,
    getStorageSync: (key: string) => storage.get(key),
    setStorageSync: (key: string, value: string) => storage.set(key, value),
  });
  vi.stubGlobal('NativeModules', {
    ExplorerModule: explorerModule,
    ...(sparkling ? { spkPipe: { call: vi.fn() } } : {}),
  });
}

function nativeCallback(mock: ReturnType<typeof vi.fn>): NativeRouteCallback {
  return mock.mock.calls[0]?.[2] as NativeRouteCallback;
}

describe('Explorer navigation', () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    vi.clearAllMocks();
    installLynxGlobals(false);
    clearRecentUrls();
  });

  it('reads explicit route ownership from the trusted host capability prop', () => {
    installLynxGlobals(false, {
      openRoute: false,
      navigateBack: true,
      explicitRouteOwnership: true,
    });
    let moduleLookups = 0;
    vi.stubGlobal('NativeModules', {
      get ExplorerModule() {
        moduleLookups += 1;
        throw new Error(
          'Native module discovery must not be used for capabilities'
        );
      },
    });

    expect(navigation.supportsExplicitRouteOwnership).toBeTypeOf('function');
    expect(navigation.supportsExplicitRouteOwnership()).toBe(true);
    expect(moduleLookups).toBe(0);
  });

  it('hides explicit route ownership without probing Android or Harmony modules', () => {
    installLynxGlobals(false, { openRoute: false, navigateBack: true });
    const report = vi.spyOn(console, 'error').mockImplementation(() => {});
    let moduleLookups = 0;
    vi.stubGlobal('NativeModules', {
      get ExplorerModule() {
        moduleLookups += 1;
        throw new Error('FUNCTION_NOT_FOUND');
      },
    });

    expect(navigation.supportsExplicitRouteOwnership()).toBe(false);
    expect(moduleLookups).toBe(0);
    expect(report).not.toHaveBeenCalled();
  });

  it.each(['true', 2, 0, false])(
    'rejects a non-enabled host capability value: %s',
    (explicitRouteOwnership) => {
      installLynxGlobals(false, {
        openRoute: true,
        navigateBack: true,
        explicitRouteOwnership,
      });

      expect(navigation.supportsExplicitRouteOwnership()).toBe(false);
    }
  );

  it('accepts Lynx iOS numeric boolean capability props', () => {
    installLynxGlobals(false, {
      openRoute: true,
      navigateBack: true,
      explicitRouteOwnership: 1,
      sparklingContainer: 1,
    });

    expect(navigation.supportsExplicitRouteOwnership()).toBe(true);
    expect(navigation.supportsSparklingContainer()).toBe(true);
  });

  it('reads Sparkling build support from a separate trusted capability', () => {
    installLynxGlobals(false, {
      openRoute: true,
      navigateBack: true,
      explicitRouteOwnership: true,
      sparklingContainer: true,
    });

    expect(navigation.supportsExplicitRouteOwnership()).toBe(true);
    expect(navigation.supportsSparklingContainer()).toBe(true);
  });

  it('keeps Legacy ownership available when Sparkling build support is absent', () => {
    installLynxGlobals(false, {
      openRoute: true,
      navigateBack: true,
      explicitRouteOwnership: true,
      sparklingContainer: false,
    });

    expect(navigation.supportsExplicitRouteOwnership()).toBe(true);
    expect(navigation.supportsSparklingContainer()).toBe(false);
  });

  it('opens through the native coordinator with automatic ownership by default', async () => {
    const pending = navigation.openSchema(
      'file://lynx?local://main.lynx.bundle'
    );

    expect(nativeOpen).toHaveBeenCalledWith(
      'file://lynx?local://main.lynx.bundle',
      'automatic',
      expect.any(Function)
    );
    expect(getRecentUrls()).toEqual([]);

    nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });

    await expect(pending).resolves.toBeUndefined();
    expect(getRecentUrls()).toEqual(['file://lynx?local://main.lynx.bundle']);
  });

  it.each(['automatic', 'legacy'] as const)(
    'falls back to the Android/Harmony openSchema ABI for %s ownership',
    async (container) => {
      installLynxGlobals(false, { openRoute: false, navigateBack: true });
      const url = 'file://lynx?local://main.lynx.bundle';

      await expect(
        navigation.openSchema(url, container)
      ).resolves.toBeUndefined();

      expect(legacyNativeOpen).toHaveBeenCalledWith(url);
      expect(nativeOpen).not.toHaveBeenCalled();
      expect(getRecentUrls()).toEqual([url]);
    }
  );

  it('never probes the iOS-only openRoute method on Android or Harmony', async () => {
    installLynxGlobals(false, { openRoute: false, navigateBack: false });
    let openRouteLookups = 0;
    const explorerModule = {
      openSchema: legacyNativeOpen,
      get openRoute(): never {
        openRouteLookups += 1;
        throw new Error('FUNCTION_NOT_FOUND');
      },
    };
    vi.stubGlobal('NativeModules', { ExplorerModule: explorerModule });
    const url = 'file://lynx?local://main.lynx.bundle';

    await expect(navigation.openSchema(url)).resolves.toBeUndefined();

    expect(openRouteLookups).toBe(0);
    expect(legacyNativeOpen).toHaveBeenCalledWith(url);
  });

  it('rejects explicit Sparkling ownership when the host lacks openRoute', async () => {
    installLynxGlobals(false, {
      openRoute: false,
      navigateBack: true,
      explicitRouteOwnership: true,
      sparklingContainer: true,
    });
    const report = vi.spyOn(console, 'error').mockImplementation(() => {});

    const pending = navigation.openWithSparkling(
      'file://lynx?local://main.lynx.bundle'
    );

    await expect(pending).rejects.toThrow('openRoute');
    expect(legacyNativeOpen).not.toHaveBeenCalled();
    expect(nativeOpen).not.toHaveBeenCalled();
    expect(getRecentUrls()).toEqual([]);
    expect(report).toHaveBeenCalledOnce();
  });

  it('records no recent entry when the native coordinator rejects the route', async () => {
    const report = vi.spyOn(console, 'error').mockImplementation(() => {});
    const pending = navigation.openSchema('not a route', 'legacy');

    nativeCallback(nativeOpen)({
      success: false,
      code: 'invalid_url',
      message: 'Invalid URL',
    });

    await expect(pending).rejects.toThrow('Invalid URL');
    expect(getRecentUrls()).toEqual([]);
    expect(report).toHaveBeenCalledOnce();
  });

  it('provides an explicit Sparkling open operation', async () => {
    expect(navigation.openWithSparkling).toBeTypeOf('function');

    const pending = navigation.openWithSparkling(
      'file://lynx?local://main.lynx.bundle'
    );

    expect(nativeOpen).toHaveBeenCalledWith(
      'file://lynx?local://main.lynx.bundle',
      'sparkling',
      expect.any(Function)
    );
    nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });
    await expect(pending).resolves.toBeUndefined();
  });

  it('keeps canonical schemes on the Sparkling router inside Sparkling', async () => {
    installLynxGlobals(true);
    const scheme = 'hybrid://lynxview_page?bundle=main.lynx.bundle';
    let callback: SparklingCallback | undefined;
    sparklingMocks.open.mockImplementation((_request, next) => {
      callback = next;
    });

    const pending = navigation.openSchema(scheme);

    expect(sparklingMocks.open).toHaveBeenCalledWith(
      { scheme },
      expect.any(Function)
    );
    expect(nativeOpen).not.toHaveBeenCalled();
    callback?.({ code: 1, msg: 'ok' });
    await expect(pending).resolves.toBeUndefined();
    expect(getRecentUrls()).toEqual([scheme]);
  });

  it('recognizes Lynx iOS numeric booleans inside a Sparkling container', async () => {
    installLynxGlobals(true, {
      openRoute: true,
      navigateBack: true,
      explicitRouteOwnership: 1,
      sparklingContainer: 1,
      iOSNumericBooleanProps: true,
    });
    const scheme = 'hybrid://lynxview_page?bundle=main.lynx.bundle';
    let callback: SparklingCallback | undefined;
    sparklingMocks.open.mockImplementation((_request, next) => {
      callback = next;
    });

    const pending = navigation.openSchema(scheme);

    expect(sparklingMocks.open).toHaveBeenCalledWith(
      { scheme },
      expect.any(Function)
    );
    expect(nativeOpen).not.toHaveBeenCalled();
    callback?.({ code: 1, msg: 'ok' });
    await expect(pending).resolves.toBeUndefined();
  });

  it.each([
    'http://example.com/page.lynx.bundle',
    'https://example.com/page.lynx.bundle',
    'file://lynx?local://main.lynx.bundle',
  ])(
    'keeps automatic raw input on the native semantic coordinator: %s',
    async (url) => {
      installLynxGlobals(true);

      const pending = navigation.openSchema(url);

      expect(nativeOpen).toHaveBeenCalledWith(
        url,
        'automatic',
        expect.any(Function)
      );
      expect(sparklingMocks.open).not.toHaveBeenCalled();
      nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });
      await expect(pending).resolves.toBeUndefined();
    }
  );

  it('keeps a Legacy wrapper around raw input on the native coordinator', async () => {
    installLynxGlobals(true);
    const target = 'https://example.com/page.lynx.bundle';
    const wrapper = `lynx://open?url=${encodeURIComponent(target)}`;

    const pending = navigation.openSchema(wrapper);

    expect(nativeOpen).toHaveBeenCalledWith(
      wrapper,
      'automatic',
      expect.any(Function)
    );
    expect(sparklingMocks.open).not.toHaveBeenCalled();
    nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });
    await expect(pending).resolves.toBeUndefined();
  });

  it('lets the native grammar classify a Legacy wrapper around canonical input', async () => {
    installLynxGlobals(true);
    const canonical = 'hybrid://lynxview_page?bundle=main.lynx.bundle';
    const wrapper = `lynx://open?url=${encodeURIComponent(canonical)}`;

    const pending = navigation.openSchema(wrapper);

    expect(nativeOpen).toHaveBeenCalledWith(
      wrapper,
      'automatic',
      expect.any(Function)
    );
    expect(sparklingMocks.open).not.toHaveBeenCalled();
    nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });
    await expect(pending).resolves.toBeUndefined();
  });

  it.each([
    'hybrid://webview?url=https%3A%2F%2Fexample.com',
    'hybrid://lynxview_page_evil?bundle=main.lynx.bundle',
  ])(
    'sends unsupported hybrid hosts to the authoritative native parser: %s',
    async (url) => {
      installLynxGlobals(true);
      const report = vi.spyOn(console, 'error').mockImplementation(() => {});

      const pending = navigation.openSchema(url);

      expect(nativeOpen).toHaveBeenCalledWith(
        url,
        'automatic',
        expect.any(Function)
      );
      expect(sparklingMocks.open).not.toHaveBeenCalled();
      nativeCallback(nativeOpen)({
        success: false,
        code: 'unsupported_hybrid_host',
        message: 'Unsupported Sparkling host',
      });
      await expect(pending).rejects.toThrow('Unsupported Sparkling host');
      expect(report).toHaveBeenCalledOnce();
    }
  );

  it('uses the native semantic descriptor for explicit Sparkling raw input', async () => {
    installLynxGlobals(true);
    const url = 'file://lynx?local://main.lynx.bundle';

    const pending = navigation.openWithSparkling(url);

    expect(nativeOpen).toHaveBeenCalledWith(
      url,
      'sparkling',
      expect.any(Function)
    );
    expect(sparklingMocks.open).not.toHaveBeenCalled();
    nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });
    await expect(pending).resolves.toBeUndefined();
  });

  it('honors explicit Legacy ownership inside Sparkling', async () => {
    installLynxGlobals(true);
    const url = 'file://lynx?local://legacy.lynx.bundle';

    const pending = navigation.openSchema(url, 'legacy');

    expect(nativeOpen).toHaveBeenCalledWith(
      url,
      'legacy',
      expect.any(Function)
    );
    expect(sparklingMocks.open).not.toHaveBeenCalled();
    nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });
    await expect(pending).resolves.toBeUndefined();
  });

  it('rejects a Sparkling open failure without invoking Legacy', async () => {
    installLynxGlobals(true);
    const report = vi.spyOn(console, 'error').mockImplementation(() => {});
    let callback: SparklingCallback | undefined;
    sparklingMocks.open.mockImplementation((_request, next) => {
      callback = next;
    });

    const pending = navigation.openSchema(
      'hybrid://lynxview_page?bundle=main.lynx.bundle'
    );
    callback?.({ code: 0, msg: 'Sparkling rejected the route' });

    await expect(pending).rejects.toThrow('Sparkling rejected the route');
    expect(nativeOpen).not.toHaveBeenCalled();
    expect(getRecentUrls()).toEqual([]);
    expect(report).toHaveBeenCalledOnce();
  });

  it('rejects a Sparkling navigate failure without invoking Legacy', async () => {
    installLynxGlobals(true);
    vi.spyOn(console, 'error').mockImplementation(() => {});
    let callback: SparklingCallback | undefined;
    sparklingMocks.navigate.mockImplementation((_request, next) => {
      callback = next;
    });

    const pending = navigation.navigateTo('showcase/main.lynx.bundle');
    callback?.({ code: -1, msg: 'Navigation failed' });

    await expect(pending).rejects.toThrow('Navigation failed');
    expect(nativeOpen).not.toHaveBeenCalled();
  });

  it('routes Legacy navigation through the native coordinator', async () => {
    const pending = navigation.navigateTo('showcase/main.lynx.bundle', {
      title: 'Showcase',
    });

    expect(nativeOpen).toHaveBeenCalledWith(
      'file://lynx?local://showcase/main.lynx.bundle?title=Showcase',
      'legacy',
      expect.any(Function)
    );
    nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });
    await expect(pending).resolves.toBeUndefined();
  });

  it('round-trips special characters in Legacy parameter keys and values', async () => {
    const params = {
      'display name&mode': 'Sparkling & Legacy = 100% / café?',
      count: 42,
    };

    const pending = navigation.navigateTo('showcase/main.lynx.bundle', params);

    const route = nativeOpen.mock.calls[0]?.[0] as string;
    const query = route.slice(route.lastIndexOf('?') + 1);
    expect(Object.fromEntries(new URLSearchParams(query))).toEqual({
      'display name&mode': params['display name&mode'],
      count: String(params.count),
    });
    expect(nativeOpen).toHaveBeenCalledWith(
      expect.any(String),
      'legacy',
      expect.any(Function)
    );
    nativeCallback(nativeOpen)({ success: true, code: 'ok', message: '' });
    await expect(pending).resolves.toBeUndefined();
  });

  it('routes close through Sparkling only in a Sparkling container', async () => {
    installLynxGlobals(true);
    let callback: SparklingCallback | undefined;
    sparklingMocks.close.mockImplementation((_request, next) => {
      callback = next;
    });

    const pending = navigation.navigateBack();

    expect(sparklingMocks.close).toHaveBeenCalledWith({}, expect.any(Function));
    expect(nativeBack).not.toHaveBeenCalled();
    callback?.({ code: 1, msg: 'ok' });
    await expect(pending).resolves.toBeUndefined();
  });

  it('routes close through the native coordinator outside Sparkling', async () => {
    const pending = navigation.navigateBack();

    expect(nativeBack).toHaveBeenCalledWith(expect.any(Function));
    const callback = nativeBack.mock.calls[0]?.[0] as NativeRouteCallback;
    callback({ success: true, code: 'ok', message: '' });
    await expect(pending).resolves.toBeUndefined();
  });

  it('rejects native close when the host lacks navigateBack', async () => {
    installLynxGlobals(false, { openRoute: false, navigateBack: false });
    const report = vi.spyOn(console, 'error').mockImplementation(() => {});

    const pending = navigation.navigateBack();

    await expect(pending).rejects.toThrow('navigateBack');
    expect(report).toHaveBeenCalledOnce();
  });

  it('never probes navigateBack without the trusted host capability', async () => {
    installLynxGlobals(false, { openRoute: false, navigateBack: false });
    const report = vi.spyOn(console, 'error').mockImplementation(() => {});
    let moduleLookups = 0;
    vi.stubGlobal('NativeModules', {
      get ExplorerModule(): never {
        moduleLookups += 1;
        throw new Error('FUNCTION_NOT_FOUND');
      },
    });

    await expect(navigation.navigateBack()).rejects.toThrow('navigateBack');

    expect(moduleLookups).toBe(0);
    expect(report).toHaveBeenCalledOnce();
  });
});
