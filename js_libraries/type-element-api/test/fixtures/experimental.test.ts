// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { ComposeElementKind, ComposeElementRef, ElementRef } from '@lynx-js/type-element-api/experimental';

const view: ElementRef = __CreateView(1);
const template: ElementRef = __CreateElementTemplate('test', null, null, null, 'test');
const compose: ComposeElementRef = __CreateCompose(1, 1 as ComposeElementKind);
__SetComposeModifier(compose, null);
void view;
void template;
void compose;

// @ts-expect-error -- Internal APIs must not leak into the experimental entry point.
void __OnLifecycleEvent;
// @ts-expect-error -- Internal scheduling APIs must not leak into the experimental entry point.
void __AsyncResolveSubtree;
// @ts-expect-error -- Internal exported types must not leak into the experimental entry point.
type UnexpectedPipelineOptions = import('@lynx-js/type-element-api/experimental').PipelineOptions;
