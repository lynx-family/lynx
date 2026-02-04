// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.


export class TraceEventDef {
  static readonly CONTEXT_RELOAD = 'LynxContext.reload';
  static readonly CONTEXT_MEASURE_NATIVE = 'LynxContext.measureNativeNode';
  static readonly CONTEXT_ALIGN_NATIVE = 'LynxContext.alignNativeNode';
  static readonly CONTEXT_GET_ALL_TIMING_INFO = 'LynxContext.getAllTimingInfo';
  static readonly CONTEXT_RE_INIT_TEMPLATE = 'LynxContext.reInitTemplateRender';
  static readonly LYNX_VIEW_CREATE = 'LynxView.Create';
  static readonly LYNX_VIEW_LOAD = 'LynxView.Load';
  static readonly LYNX_VIEW_REUSE = 'LynxView.Reuse';
  static readonly LYNX_VIEW_RELOAD = 'LynxView.Reload';
  static readonly TEMPLATE_RENDER_INIT = 'TemplateRender.init';
  static readonly TEMPLATE_RENDER_CREATE_TASM = 'TemplateRender.createEmbedder';
  static readonly UIOWNER_INIT = 'LynxUIOwner.initialize';
  static readonly UIOWNER_CREATE_VIEW = 'UIOwner.createView.'
  static readonly UIBASE_CREATE_NATIVE_UI = 'UIBase.createNativeUI';
  static readonly DRAW_END_TIMING_ON_FRAME_CALLBACK = 'DrawEndTimingFrameCallback.onFrame';
  static readonly EVENT_REPORTER_ON_EVENT = 'LynxEventReporter.onEvent';
  static readonly EVENT_REPORTER_UPDATE_GENERIC_INFO = 'LynxEventReporter.updateGenericInfo';
  static readonly EVENT_REPORTER_HANDLE_EVENT = 'LynxEventReporter.handleEvent';
  static readonly LYNX_ENV_TRIM_MEMORY = 'LynxEnv.trimMemory';
}
