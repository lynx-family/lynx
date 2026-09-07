// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { ElementRef } from '@lynx-js/type-element-api';

const view: ElementRef = __CreateView(1);
void view;

// @ts-expect-error -- Experimental APIs must not leak into the package root.
void __CreateElementTemplate;
// @ts-expect-error -- Internal APIs must not leak into the package root.
void __OnLifecycleEvent;
