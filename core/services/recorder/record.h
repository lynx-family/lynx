// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_RECORD_H_
#define CORE_SERVICES_RECORDER_RECORD_H_

namespace lynx::tasm::recorder {
// Compile-time actions; values are not part of the recorded file format.
enum class RecordType {
  // Reserved for observation-only events; never creates a recording action.
  OnlyObserve,
  Remove,
  ViewPort,
  ThreadStrategy,
  Component,
  Scripts,
  PreloadScript,
  GlobalEvent,
  SharedData,
  NativeModuleCallback,
  NativeModuleFunctionCall,
  LoadTemplateBundle,
  LoadTemplate,
  ReloadTemplate,
  SetGlobalProps,
  UpdateMetaData,
  UpdateConfig,
  UpdateFontScale,
  UpdateDataByPreParsedData,
  TouchEvent,
  CustomEvent,
  BubbleEvent,
  RequireTemplate,
  LoadComponentWithCallback,
  ExternalScriptAsLoadComponent,
  SwitchEngineFromUIThread,
};

}  // namespace lynx::tasm::recorder

#if defined(ENABLE_TESTBENCH_RECORDER) && ENABLE_TESTBENCH_RECORDER

// Recorder implementation headers may be absent when the optional recorder
// target is not packaged. Keep those dependencies behind this single entry.
#include "core/services/recorder/lynxview_init_recorder.h"
#include "core/services/recorder/template_assembler_recorder.h"
#define RECORD(type, ...)                                    \
  ::lynx::tasm::recorder::TemplateAssemblerRecorder::Record< \
      ::lynx::tasm::recorder::RecordType::type>(__VA_ARGS__)

#define RECORD_OPTIONAL(flag, type, ...) \
  do {                                   \
    if (flag) {                          \
      RECORD(type, __VA_ARGS__);         \
    }                                    \
  } while (0)
// Keep the prelude and recording call in the same scope. A return in the
// prelude exits the enclosing function, not a helper or lambda.
#define RECORD_WITH_EARLY_RETURN(before, type, ...) \
  do {                                              \
    before;                                         \
    RECORD(type, __VA_ARGS__);                      \
  } while (0)

#else
// Neither the condition nor arguments are evaluated in a recorder-free build.
#define RECORD(type, ...)
#define RECORD_OPTIONAL(flag, type, ...)
#define RECORD_WITH_EARLY_RETURN(before, type, ...)
#endif

#endif  // CORE_SERVICES_RECORDER_RECORD_H_
