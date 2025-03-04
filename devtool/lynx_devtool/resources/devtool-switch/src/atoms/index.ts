import { atom } from 'jotai';
import { atomWithStorage } from 'jotai/utils';

import type { V8Enable } from '../rspeedy-env';

export const lynxDebug = atomWithStorage('debug', false, {
  getItem() {
    'background-only';
    return NativeModules.LynxDevToolSetModule.isLynxDebugEnabled?.() ?? false;
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchLynxDebug?.(value);
  },
  removeItem() {},
});

export const devtool = atomWithStorage('devtool', false, {
  getItem() {
    'background-only';
    return NativeModules.LynxDevToolSetModule.isDevToolEnabled?.() ?? false;
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchDevTool?.(value);
  },
  removeItem() {},
});

export const platform = atom(SystemInfo.platform);

export const longPress = atomWithStorage('longpress', false, {
  getItem() {
    'background-only';
    return (
      NativeModules.LynxDevToolSetModule.isLongPressMenuEnabled?.() ?? false
    );
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchLongPressMenu?.(value);
  },
  removeItem() {},
});

export const domInspect = atomWithStorage('dom-inspect', false, {
  getItem() {
    'background-only';
    return NativeModules.LynxDevToolSetModule.isDomTreeEnabled?.() ?? false;
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.enableDomTree?.(value);
  },
  removeItem() {},
});

export const logBox = atomWithStorage('logbox', false, {
  getItem() {
    'background-only';
    return NativeModules.LynxDevToolSetModule.isLogBoxEnabled?.() ?? false;
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchLogBox?.(value);
  },
  removeItem() {},
});

export const performanceTool = atomWithStorage('performance-tool', false, {
  getItem() {
    'background-only';
    return NativeModules.LynxDevToolSetModule.isDebugModeEnabled?.() ?? false;
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchDebugModeEnable?.(value);
  },
  removeItem() {},
});

export const pixelCopy = atomWithStorage('pixel-copy', false, {
  getItem() {
    'background-only';
    return NativeModules.LynxDevToolSetModule.isPixelCopyEnabled?.() ?? false;
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchPixelCopy?.(value);
  },
  removeItem() {},
});

export const highlightTouch = atomWithStorage('highlight', false, {
  getItem() {
    'background-only';
    return (
      NativeModules.LynxDevToolSetModule.isHighlightTouchEnabled?.() ?? false
    );
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchHighlightTouch?.(value);
  },
  removeItem() {},
});

export const v8 = atomWithStorage<V8Enable>('v8', 0, {
  getItem() {
    'background-only';
    return NativeModules.LynxDevToolSetModule.getV8Enabled?.() ?? 0;
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchV8?.(value);
  },
  removeItem() {},
});

export const quickjsDebug = atomWithStorage('quickjs-debug', false, {
  getItem() {
    'background-only';
    return (
      NativeModules.LynxDevToolSetModule.isQuickjsDebugEnabled?.() ?? false
    );
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchQuickjsDebug?.(value);
  },
  removeItem() {},
});

export const quickjsCache = atomWithStorage('quickjs-cache', false, {
  getItem() {
    'background-only';
    return (
      NativeModules.LynxDevToolSetModule.isQuickjsCacheEnabled?.() ?? false
    );
  },
  setItem(_, value) {
    'background-only';
    return NativeModules.LynxDevToolSetModule.switchQuickjsCache?.(value);
  },
  removeItem() {},
});
