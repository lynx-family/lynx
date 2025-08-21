// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxContext } from '../tasm/LynxContext';

export enum LynxResourceType {
  LYNX_RESOURCE_TYPE_GENERIC = 0,
  LYNX_RESOURCE_TYPE_IMAGE = 1,
  LYNX_RESOURCE_TYPE_FONT = 2,
  LYNX_RESOURCE_TYPE_LOTTIE = 3,
  LYNX_RESOURCE_TYPE_VIDEO = 4,
  LYNX_RESOURCE_TYPE_SVG = 5,
  LYNX_RESOURCE_TYPE_TEMPLATE = 6,
  LYNX_RESOURCE_TYPE_DYNAMIC_COMPONENT = 7,
  LYNX_RESOURCE_TYPE_LYNX_CORE_JS = 8,
  LYNX_RESOURCE_TYPE_EXTERNAL_JS_SOURCE = 9,
  // There are some differences between JSDynamicComponent and
  // TemplateDynamicComponent in the old logic, so here is a new type to be
  // compatible with the old logic.
  LYNX_RESOURCE_TYPE_TEMPLATE_DYNAMIC_COMPONENT = 10, // DynamicComponent from template
  LYNX_RESOURCE_TYPE_ASSETS = 11,
  LYNX_RESOURCE_TYPE_I18N_TEXT = 12,
  LYNX_RESOURCE_TYPE_GRAPHICS = 13,
  LYNX_RESOURCE_TYPE_THEME = 14,
}

export enum LynxResourceAsyncMode {
  EXACTLY_ASYNC = 1,
  EXACTLY_SYNC = 2,
  MOST_SYNC = 3
}

export class LynxResourceRequest {
  url: string;
  type: LynxResourceType;
  params?: Record<string, Object>;
  mode?: LynxResourceAsyncMode;
  context: LynxContext | undefined;

  constructor(url: string, type: LynxResourceType, context?: LynxContext) {
    this.url = url;
    this.type = type;
    this.context = context;
  }
}