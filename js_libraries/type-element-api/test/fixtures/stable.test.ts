// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { ElementRef } from '@lynx-js/type-element-api/stable';

const view: ElementRef = __CreateView(1);
__SwapElement(view, view);
void view;

// @ts-expect-error -- Experimental APIs must not leak into the stable entry point.
void __CreateElementTemplate;
// @ts-expect-error -- Compose APIs must not leak into the stable entry point.
void __CreateCompose;
// @ts-expect-error -- Compose types must not be exported from the stable entry point.
type UnexpectedComposeElementRef = import('@lynx-js/type-element-api/stable').ComposeElementRef;
// @ts-expect-error -- Internal APIs must not leak into the stable entry point.
void __OnLifecycleEvent;
// @ts-expect-error -- Internal exported types must not leak into the stable entry point.
type UnexpectedPipelineOptions = import('@lynx-js/type-element-api/stable').PipelineOptions;
