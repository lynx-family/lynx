// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

declare module '*.png?inline';

declare module '@lynx-js/types' {
  interface GlobalProps {
    preferredTheme?: string;
    frontendTheme?: string;
    theme: string;
    safeAreaTop?: number;
    safeAreaBottom?: number;
  }
}
