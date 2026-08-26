// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { ComposeElementKind, ComposeElementRef, ElementRef, PipelineOptions } from '@lynx-js/type-element-api/internal';

const view: ElementRef = __CreateView(1);
const template: ElementRef = __CreateElementTemplate('test', null, null, null, 'test');
const compose: ComposeElementRef = __CreateCompose(1, 1 as ComposeElementKind);
__OnLifecycleEvent([]);
__AsyncResolveSubtree(view);
const pipelineOptions: PipelineOptions = { pipelineID: 'test', needTimestamps: true };
void view;
void template;
void compose;
void pipelineOptions;
