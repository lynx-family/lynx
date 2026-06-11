// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Type declarations specific to this project.
 *
 * These types must be declared in their original scope to avoid conflicts
 * and prevent being overridden by '@lynx-js/react' type imports.
 */

import type { BaseEvent, StandardProps } from '@lynx-js/types';

declare global {
  declare module '*.png?inline';

  interface NativeModulesMap {
    ExplorerModule: {
      openScan(): void;
      openSchema(url: string): void;
      getSettingInfo(): Record<string, unknown>;
      setThreadMode(index: number): void;
      saveThemePreferences(key: string, value: string): void;
      navigateBack?(): void;
    };
    LynxNodeAPI?: {
      requireNodeAddon(addonName: string): void;
    };
  }

  declare let NativeModules: NativeModulesMap;

  interface globalThis {
    NativeModules?: NativeModulesMap;
  }

  declare var __lynx_node_addon_exports__:
    | Record<string, Record<string, (...args: unknown[]) => unknown>>
    | undefined;
}

declare module '@lynx-js/types' {
  interface GlobalProps {
    preferredTheme?: string;
    frontendTheme?: string;
    theme: string;
    isNotchScreen: boolean;
    safeAreaTop?: number;
    safeAreaBottom?: number;
  }

  interface IntrinsicElements extends Lynx.IntrinsicElements {
    input: InputProps;
  }
}

export interface InputProps extends StandardProps {
  /**
   * CSS class name for the input element
   */
  className?: string;

  /**
   * Event handler for input changes
   */
  bindinput?: (e: InputEvent) => void;

  /**
   * Event handler for blur events
   */
  bindblur?: (e: BlurEvent) => void;

  /**
   * Placeholder text when input is empty
   */
  placeholder?: string;

  /**
   * Text color of the input
   */
  'text-color'?: string;
}

export type InputEvent = BaseEvent<'input', { value: string }>;
export type BlurEvent = BaseEvent<'blur', { value: string }>;
