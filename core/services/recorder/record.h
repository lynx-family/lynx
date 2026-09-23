// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_RECORD_H_
#define CORE_SERVICES_RECORDER_RECORD_H_

#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "base/include/log/log_context.h"
#include "base/include/log/logging.h"
#include "core/services/recorder/record_payload.h"

namespace lynx::tasm::recorder {
// Version of the observation log format, independent of the SDK version.
// Once the format is released, bump this version when adding a RecordType or
// changing the log output of ObserveRecord. Update the parsing script while
// retaining support for older versions. Refactoring that preserves the output
// and comment-only changes do not require a version bump.
inline constexpr char kObservationLogVersion[] = "0.0.1";

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

// These helpers only inspect cheap metadata. Payload serialization stays at
// the call site when needed; observation never changes the recorded arguments.
template <RecordType type, typename... Args>
void ObserveRecord(const char* name, const base::LogContext& context,
                   const Args&... args) {
  [[maybe_unused]] auto values = std::forward_as_tuple(args...);
  if constexpr (type == RecordType::LoadTemplate ||
                type == RecordType::LoadTemplateBundle) {
    LOGO(context << ' ' << name << " logVersion:" << kObservationLogVersion
                 << " url:" << std::get<0>(values)
                 << " bytes:" << std::get<1>(values).size()
                 << " hasData:" << (std::get<2>(values) != nullptr)
                 << ObservePayload(
                        context, kObservationLogVersion, "template",
                        std::string_view(reinterpret_cast<const char*>(
                                             std::get<1>(values).data()),
                                         std::get<1>(values).size()))
                 << (std::get<2>(values)
                         ? ObservePayload(context, kObservationLogVersion,
                                          "data", *std::get<2>(values))
                         : std::string{}));
  } else if constexpr (type == RecordType::Scripts ||
                       type == RecordType::PreloadScript ||
                       type == RecordType::ExternalScriptAsLoadComponent) {
    LOGO(context << ' ' << name << " url:" << std::get<0>(values)
                 << " bytes:" << std::string_view(std::get<1>(values)).size()
                 << ObservePayload(context, kObservationLogVersion, "script",
                                   std::get<1>(values)));
  } else if constexpr (type == RecordType::ThreadStrategy) {
    LOGO(context << ' ' << name << " logVersion:" << kObservationLogVersion
                 << " strategy:" << std::get<0>(values)
                 << " enableRuntime:" << std::get<2>(values));
  } else if constexpr (type == RecordType::ViewPort) {
    LOGO(context << ' ' << name << " width:" << std::get<3>(values)
                 << " height:" << std::get<2>(values) << " widthMode:"
                 << std::get<1>(values) << " heightMode:" << std::get<0>(values)
                 << " pixelRatio:" << std::get<6>(values));
  } else if constexpr (type == RecordType::Component) {
    LOGO(context << ' ' << name << " tag:" << std::get<0>(values)
                 << " type:" << static_cast<int>(std::get<1>(values)));
  } else if constexpr (type == RecordType::SetGlobalProps ||
                       type == RecordType::UpdateConfig) {
    LOGO(context << ' ' << name
                 << " entries:" << std::get<0>(values).GetLength()
                 << ObservePayload(context, kObservationLogVersion, "data",
                                   std::get<0>(values)));
  } else if constexpr (type == RecordType::UpdateMetaData) {
    LOGO(context << ' ' << name
                 << " hasData:" << (std::get<0>(values) != nullptr)
                 << (std::get<0>(values)
                         ? ObservePayload(context, kObservationLogVersion,
                                          "data", *std::get<0>(values))
                         : std::string{})
                 << ObservePayload(context, kObservationLogVersion,
                                   "globalProps", std::get<1>(values)));
  } else if constexpr (type == RecordType::ReloadTemplate ||
                       type == RecordType::UpdateDataByPreParsedData) {
    LOGO(context << ' ' << name
                 << " hasData:" << (std::get<0>(values) != nullptr)
                 << (std::get<0>(values)
                         ? ObservePayload(context, kObservationLogVersion,
                                          "data", *std::get<0>(values))
                         : std::string{}));
  } else if constexpr (type == RecordType::UpdateFontScale) {
    LOGO(context << ' ' << name << " scale:" << std::get<0>(values)
                 << " source:" << std::get<1>(values));
  } else if constexpr (type == RecordType::RequireTemplate) {
    LOGO(context << ' ' << name << " url:" << std::get<0>(values)
                 << " sync:" << std::get<1>(values));
  } else if constexpr (type == RecordType::LoadComponentWithCallback) {
    LOGO(context << ' ' << name << " url:" << std::get<0>(values)
                 << " bytes:" << std::get<1>(values).size() << " sync:"
                 << std::get<2>(values) << " callback:" << std::get<3>(values)
                 << ObservePayload(
                        context, kObservationLogVersion, "component",
                        std::string_view(reinterpret_cast<const char*>(
                                             std::get<1>(values).data()),
                                         std::get<1>(values).size())));
  } else if constexpr (type == RecordType::CustomEvent ||
                       type == RecordType::BubbleEvent) {
    LOGO(context << ' ' << name << " name:" << std::get<0>(values) << " tag:"
                 << std::get<1>(values) << " rootTag:" << std::get<2>(values)
                 << " paramsCount:" << std::get<3>(values).GetLength()
                 << ObservePayload(context, kObservationLogVersion, "params",
                                   std::get<3>(values)));
  } else if constexpr (type == RecordType::TouchEvent) {
    if constexpr (sizeof...(Args) == 4) {
      [[maybe_unused]] const auto& event = std::get<2>(values);
      LOGO(context << ' ' << name << " name:" << std::get<0>(values)
                   << " rootTag:" << std::get<1>(values) << " tag:" << event.tag
                   << " x:" << event.x << " y:" << event.y
                   << " multiFinger:" << event.is_multi_finger
                   << " timestamp:" << event.timestamp);
    } else {
      LOGO(context << ' ' << name << " name:" << std::get<0>(values) << " tag:"
                   << std::get<1>(values) << " rootTag:" << std::get<2>(values)
                   << " x:" << std::get<3>(values)
                   << " y:" << std::get<4>(values));
    }
  } else if constexpr (type == RecordType::NativeModuleFunctionCall) {
    // TODO(songshourui): Send native module arguments through DevTool when
    // DevTool is enabled instead of writing payloads to files.
    LOGO(context << ' ' << name << " module:" << std::get<0>(values)
                 << " method:" << std::get<1>(values)
                 << " argc:" << std::get<2>(values)
                 << " callbacks:" << std::get<5>(values));
  } else if constexpr (type == RecordType::NativeModuleCallback) {
    LOGO(context << ' ' << name << " module:" << std::get<0>(values)
                 << " method:" << std::get<1>(values)
                 << " callback:" << std::get<sizeof...(Args) - 2>(values));
  } else if constexpr (type == RecordType::GlobalEvent) {
    LOGO(context << ' ' << name << " module:" << std::get<0>(values)
                 << " method:" << std::get<1>(values));
  } else if constexpr (type == RecordType::SwitchEngineFromUIThread) {
    LOGO(context << ' ' << name << " attach:" << std::get<0>(values));
  } else {
    LOGO(context << ' ' << name);
  }
}

}  // namespace lynx::tasm::recorder

#if defined(ENABLE_TESTBENCH_RECORDER) && ENABLE_TESTBENCH_RECORDER
// Recorder implementation headers may be absent when the optional recorder
// target is not packaged. Keep those dependencies behind this single entry.
#include "core/services/recorder/lynxview_init_recorder.h"
#include "core/services/recorder/template_assembler_recorder.h"
#define LYNX_RECORD_ENABLED() true
#define LYNX_RECORD_CALL(type, ...)                          \
  ::lynx::tasm::recorder::TemplateAssemblerRecorder::Record< \
      ::lynx::tasm::recorder::RecordType::type>(__VA_ARGS__)
#else
#define LYNX_RECORD_ENABLED() LOGO_IS_ON()
#define LYNX_RECORD_CALL(type, ...)
#endif

// Evaluate the context and arguments once, only when at least one consumer is
// enabled. The existing record ID remains in args until Recorder uses context.
#define RECORD(type, context, ...)                                             \
  do {                                                                         \
    if (LYNX_RECORD_ENABLED()) {                                               \
      [&](const ::lynx::base::LogContext& record_context, auto&&... args) {    \
        if (LOGO_IS_ON()) {                                                    \
          ::lynx::tasm::recorder::ObserveRecord<                               \
              ::lynx::tasm::recorder::RecordType::type>(#type, record_context, \
                                                        args...);              \
        }                                                                      \
        LYNX_RECORD_CALL(type, std::forward<decltype(args)>(args)...);         \
      }(context, ##__VA_ARGS__);                                               \
    }                                                                          \
  } while (0)

#define RECORD_OPTIONAL(flag, type, context, ...) \
  do {                                            \
    if (LYNX_RECORD_ENABLED() && (flag)) {        \
      RECORD(type, context, ##__VA_ARGS__);       \
    }                                             \
  } while (0)

// Keep the prelude and recording call in the same scope. A return in the
// prelude exits the enclosing function, not a helper or lambda.
#define RECORD_WITH_EARLY_RETURN(before, type, context, ...) \
  do {                                                       \
    if (LYNX_RECORD_ENABLED()) {                             \
      before;                                                \
      RECORD(type, context, ##__VA_ARGS__);                  \
    }                                                        \
  } while (0)

#endif  // CORE_SERVICES_RECORDER_RECORD_H_
