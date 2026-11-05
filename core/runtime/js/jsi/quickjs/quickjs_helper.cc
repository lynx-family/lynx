// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/quickjs/quickjs_helper.h"

#include <algorithm>
#include <cstring>
#include <memory>

#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/quickjs/quickjs_exception.h"
// #include "quickjs_runtime.h"
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#if ENABLE_TRACE_PERFETTO
#include "base/include/fml/memory/js_memory_track_define.h"
#endif
#include "base/trace/native/trace_event.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "quickjs/include/quickjs.h"
extern "C" {
#include "quickjs/include/quickjs-libc.h"
}
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace runtime {
namespace js {
namespace detail {
namespace {

class QuickjsHeapSnapshot final : public CStringRefBuffer {
 public:
  explicit QuickjsHeapSnapshot(const char *snapshot)
      : CStringRefBuffer(snapshot) {}
  ~QuickjsHeapSnapshot() override {
    js_profile_free_heap_snapshot(reinterpret_cast<const char *>(data()));
  }

  QuickjsHeapSnapshot(const QuickjsHeapSnapshot &) = delete;
  QuickjsHeapSnapshot &operator=(const QuickjsHeapSnapshot &) = delete;
};

int ApplyJSModeStripFlag(QuickjsRuntime *rt, int eval_flags) {
  auto &env = tasm::LynxEnv::GetInstance();
  if (!tasm::performance::MemoryMonitor::Enable() && !env.IsDevToolEnabled() &&
      !rt->GetPageOptions().GetDebuggable() && env.EnableJSModeStrip()) {
    return eval_flags | LEPUS_EVAL_FLAG_STRIP;
  }
  return eval_flags & ~LEPUS_EVAL_FLAG_STRIP;
}

}  // namespace

QuickjsJSValueValue::QuickjsJSValueValue(LEPUSContext *ctx, LEPUSValue val)
    : val_(val), rt_(LEPUS_GetRuntime(ctx)) {
  //  static int calc = 0;
  //  time = calc;
  //  LOGE( "create ptr=" << JS_VALUE_GET_PTR(val_) << " create time="
  //  << calc); calc++;
  // DCHECK((uintptr_t)(&((QuickjsJSValueValue *)(0x0))->val_) == 8);
}

void QuickjsJSValueValue::invalidate() {
  //  LOGE( "invalidate ptr=" << LEPUS_VALUE_GET_PTR(val_) << "
  //  invalidate time=" << time);
  if (!LEPUS_IsGCModeRT(rt_)) LEPUS_FreeValueRT(rt_, val_);
  QJSValueValueAllocator::Delete(rt_, static_cast<void *>(this));
}

LEPUSValue QuickjsJSValueValue::Get() const { return val_; }

Runtime::PointerValue *QuickjsHelper::makeStringValue(LEPUSContext *ctx,
                                                      LEPUSValue str) {
  void *ret = QJSValueValueAllocator::New(LEPUS_GetRuntime(ctx));
  return new (ret) QuickjsJSValueValue(ctx, str);
}

Runtime::PointerValue *QuickjsHelper::makeObjectValue(LEPUSContext *ctx,
                                                      LEPUSValue obj) {
  void *ret = QJSValueValueAllocator::New(LEPUS_GetRuntime(ctx));
  return new (ret) QuickjsJSValueValue(ctx, obj);
}

Runtime::PointerValue *QuickjsHelper::makeJSValueValue(LEPUSContext *ctx,
                                                       LEPUSValue obj) {
  void *ret = QJSValueValueAllocator::New(LEPUS_GetRuntime(ctx));
  return new (ret) QuickjsJSValueValue(ctx, obj);
}

Object QuickjsHelper::createJSValue(LEPUSContext *ctx, LEPUSValue obj) {
  return Runtime::make<Object>(makeJSValueValue(ctx, obj));
}

PropNameID QuickjsHelper::createPropNameID(LEPUSContext *ctx,
                                           LEPUSValue propName) {
  return Runtime::make<PropNameID>(makeStringValue(ctx, propName));
}

String QuickjsHelper::createString(LEPUSContext *ctx, LEPUSValue str) {
  return Runtime::make<String>(makeStringValue(ctx, str));
}

Symbol QuickjsHelper::createSymbol(LEPUSContext *ctx, LEPUSValue sym) {
  return Runtime::make<Symbol>(makeJSValueValue(ctx, sym));
}

Object QuickjsHelper::createObject(LEPUSContext *ctx, LEPUSValue obj) {
  return Runtime::make<Object>(makeObjectValue(ctx, obj));
}

Value QuickjsHelper::createValue(LEPUSValue value, QuickjsRuntime *rt) {
  if (LEPUS_IsInteger(value)) {
    return Value(LEPUS_VALUE_GET_INT(value));
  } else if (LEPUS_IsNumber(value)) {
    return Value(LEPUS_VALUE_GET_FLOAT64(value));
  } else if (LEPUS_IsBool(value)) {
    bool temp = static_cast<bool>(LEPUS_ToBool(rt->getJSContext(), value));
    return Value(temp);
  } else if (LEPUS_IsNull(value)) {
    return Value(nullptr);
  } else if (LEPUS_IsUndefined(value)) {
    return Value();
  } else if (LEPUS_IsSymbol(value)) {
    return Value(createSymbol(rt->getJSContext(), value));
  } else if (LEPUS_IsString(value)) {
    return Value(createString(rt->getJSContext(), value));
  } else if (LEPUS_IsObject(value) || LEPUS_IsException(value)) {
    return Value(createObject(rt->getJSContext(), value));
  } else {
    int64_t tag = LEPUS_VALUE_GET_TAG(value);
    LOGE("createValue failed type is unknown:" << tag);
    std::string msg =
        "createValue failed type is unknown:" + std::to_string(tag);
    rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(msg));
    return Value();
  }
}

LEPUSValue QuickjsHelper::symbolRef(const Symbol &sym) {
  const QuickjsJSValueValue *quickjs_sym =
      static_cast<const QuickjsJSValueValue *>(Runtime::getPointerValue(sym));
  return quickjs_sym->Get();
}

LEPUSValue QuickjsHelper::valueRef(const PropNameID &sym) {
  return static_cast<const QuickjsJSValueValue *>(Runtime::getPointerValue(sym))
      ->Get();
}

LEPUSValue QuickjsHelper::stringRef(const String &sym) {
  return static_cast<const QuickjsJSValueValue *>(Runtime::getPointerValue(sym))
      ->Get();
}

LEPUSValue QuickjsHelper::objectRef(const Object &sym) {
  return static_cast<const QuickjsJSValueValue *>(Runtime::getPointerValue(sym))
      ->Get();
}

std::string QuickjsHelper::LEPUSStringToSTLString(LEPUSContext *ctx,
                                                  LEPUSValue s) {
  const char *c = LEPUS_ToCString(ctx, s);
  if (c == nullptr) {
    if (LEPUS_IsGCMode(ctx))
      LEPUS_GetException(ctx);  // just disconnect the reference-relation from
                                // ctx to exception_val
    else
      LEPUS_FreeValue(ctx, LEPUS_GetException(ctx));

    return "Error!";
  }
  std::string ret(c);
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, c);
  return ret;
}

std::optional<Value> QuickjsHelper::call(QuickjsRuntime *rt, const Function &f,
                                         const Object &jsThis,
                                         LEPUSValue *arguments, size_t nArgs) {
  LEPUSValue thisObj = QuickjsHelper::objectRef(jsThis);
  LEPUSValue target_object = LEPUS_IsUninitialized(thisObj)
                                 ? LEPUS_GetGlobalObject(rt->getJSContext())
                                 : thisObj;
  LEPUSValue func = objectRef(f);
  LEPUSValue result = LEPUS_Call(rt->getJSContext(), func, target_object,
                                 static_cast<int>(nArgs), arguments);

  if (!LEPUS_IsGCMode(rt->getJSContext()) && LEPUS_IsUninitialized(thisObj)) {
    LEPUS_FreeValue(rt->getJSContext(), target_object);
  }

  bool has_exception = !QuickjsException::ReportExceptionIfNeeded(*rt, result);
  lepus_std_loop(rt->getJSContext());
  while (LEPUS_MoveUnhandledRejectionToException((rt->getJSContext()))) {
    LEPUSValue exception_val = LEPUS_GetException(rt->getJSContext());
    rt->reportJSIException(QuickjsException(*rt, exception_val));
  }
  // Before, If function inside quickjs triggered an exception, it return an
  // object as `Exception` type. This type is invisible to jsi, thus cannot be
  // identified. Now, return `undefined` here, the same result as V8.
  if (has_exception) {
    return std::optional<Value>();
  }
  return createValue(result, rt);
}

std::optional<Value> QuickjsHelper::callAsConstructor(QuickjsRuntime *rt,
                                                      LEPUSValue obj,
                                                      LEPUSValue *args,
                                                      int nArgs) {
  LEPUSValue result =
      LEPUS_CallConstructor(rt->getJSContext(), obj, nArgs, args);
  auto result_holder = createValue(result, rt);

  bool has_exception = !QuickjsException::ReportExceptionIfNeeded(*rt, result);
  lepus_std_loop(rt->getJSContext());
  while (LEPUS_MoveUnhandledRejectionToException(rt->getJSContext())) {
    LEPUSValue exception_val = LEPUS_GetException(rt->getJSContext());
    rt->reportJSIException(QuickjsException(*rt, exception_val));
  }
  // Same raison as `QuickjsHelper::call`
  if (has_exception) {
    return std::optional<Value>();
  }
  return result_holder;
}

std::string QuickjsHelper::getErrorMessage(LEPUSContext *ctx,
                                           LEPUSValue &exception_value) {
  std::string error_msg;
  // Even if most of the caller make a check to exception_value before calling
  // getErrorMessage, here we double check if exception_value is an exception or
  // an error to make sure we will not get a message like: [object Object].
  if (LEPUS_IsException(exception_value) ||
      LEPUS_IsError(ctx, exception_value)) {
    auto str = LEPUS_ToCString(ctx, exception_value);
    if (str) {
      error_msg.append(str);
    }
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, str);
  }
  return error_msg;
}

base::expected<Value, JSINativeException> QuickjsHelper::evalBuf(
    QuickjsRuntime *rt, LEPUSContext *ctx, const char *buf, size_t buf_len,
    const char *filename, int eval_flags, int start_line_offset,
    bool enable_js_coverage) {
  eval_flags = ApplyJSModeStripFlag(rt, eval_flags);
  LEPUSValue val;
  if (enable_js_coverage) {
    val = LEPUS_Eval_WITH_COVERAGE(ctx, buf, buf_len, filename, eval_flags,
                                   start_line_offset,
                                   static_cast<int32_t>(rt->getRuntimeId()));
  } else {
    val =
        LEPUS_Eval2(ctx, buf, buf_len, filename, eval_flags, start_line_offset);
  }
  auto maybe_error = QuickjsException::TryCatch(*rt, val);
  if (maybe_error.has_value()) {
    LOGE("evalBuf failed:" << filename);
    return base::unexpected(JSINativeException(
        maybe_error->name(), maybe_error->message(), maybe_error->stack(), true,
        error::E_BTS_RUNTIME_ERROR_SCRIPT_ERROR));
  }
  Value evalRes = createValue(val, rt);
  // createValue did not add ref count to val;
  // so don't use LEPUS_FreeValue
  //   LEPUS_FreeValue(ctx, val);
  lepus_std_loop(ctx);
  return evalRes;
}

base::expected<Value, JSINativeException> QuickjsHelper::evalBin(
    QuickjsRuntime *rt, LEPUSContext *ctx, const char *buf, size_t buf_len,
    const char *filename, int eval_flags) {
  eval_flags = ApplyJSModeStripFlag(rt, eval_flags);
  LEPUSValue val = LEPUS_EvalBinary(ctx, reinterpret_cast<const uint8_t *>(buf),
                                    buf_len, eval_flags);
  auto maybe_error = QuickjsException::TryCatch(*rt, val);
  if (maybe_error.has_value()) {
    LOGE("evalBin failed:" << filename);
    return base::unexpected(JSINativeException(
        maybe_error->name(), maybe_error->message(), maybe_error->stack(), true,
        error::E_BTS_RUNTIME_ERROR_BYTECODE_SCRIPT_ERROR));
  }
  Value evalRes = createValue(val, rt);
  // createValue did not add ref count to val;
  // so don't use LEPUS_FreeValue
  // LEPUS_FreeValue(ctx, val);
  lepus_std_loop(ctx);
  return evalRes;
}

LEPUSValue QuickjsHelper::ThrowJsException(
    LEPUSContext *ctx, const JSINativeException &exception) {
  DCHECK(!exception.message().empty());
  auto err = LEPUS_NewError(ctx);
  HandleScope func_scope(ctx, &err, HANDLE_TYPE_LEPUS_VALUE);
  LEPUSValue error_value = LEPUS_NewStringLen(
      ctx, exception.message().empty() ? "" : exception.message().c_str(),
      exception.message().length());
  func_scope.PushHandle(&error_value, HANDLE_TYPE_LEPUS_VALUE);
  LEPUSValue stack_value = LEPUS_NewStringLen(
      ctx, exception.stack().empty() ? "" : exception.stack().c_str(),
      exception.stack().length());
  func_scope.PushHandle(&stack_value, HANDLE_TYPE_LEPUS_VALUE);
  if (LEPUS_IsError(ctx, err)) {
    LEPUS_DefinePropertyValueStr(ctx, err, "message", error_value,
                                 LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
    if (exception.IsJSError()) {
      LEPUS_DefinePropertyValueStr(
          ctx, err, "stack", stack_value,
          LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
      LEPUSValue name_value = LEPUS_NewStringLen(ctx, exception.name(),
                                                 std::strlen(exception.name()));
      func_scope.PushHandle(&name_value, HANDLE_TYPE_LEPUS_VALUE);
      LEPUS_DefinePropertyValueStr(
          ctx, err, "name", name_value,
          LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
    } else {
      LEPUS_DefinePropertyValueStr(
          ctx, err, "cause", stack_value,
          LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
    }
  }
  return LEPUS_Throw(ctx, err);
}

QuickjsHelper::MemoryUsage QuickjsHelper::GetMemoryUsage(
    LEPUSRuntime *runtime) {
  QuickjsHelper::MemoryUsage result;
  memset(&result, 0, sizeof(result));
  if (runtime) {
    LEPUSMemoryUsageBrief usage;
    LEPUS_ComputeMemoryUsageBrief(runtime, &usage);
    result.base_size = usage.base_malloc_size;
    result.heap_size = usage.malloc_size;
    result.page_rss_size =
        LEPUS_IsGCModeRT(runtime) ? usage.page_rss_size : usage.malloc_size;
  }
  if (result.heap_size == 0 && runtime != nullptr) {
    result.heap_size = LEPUS_GetHeapSize(runtime);
  }
  return result;
}

std::string QuickjsHelper::GetDebugDescription(LEPUSRuntime *runtime) {
  if (runtime) {
#if ENABLE_TRACE_PERFETTO
    auto usage = GetMemoryUsage(runtime);
    std::string vm_type =
        std::string("quickjs(") + (LEPUS_IsGCModeRT(runtime) ? "gc)" : "rc)");

    std::stringstream vm_info;
    vm_info << "{\n"
            << "  \"vm_type\": \"" << vm_type << "\",\n"
            << "  \"base_usage\": " << usage.base_size << ",\n"
            << "  \"page_rss_usage\": " << usage.page_rss_size << ",\n"
            << "  \"acc_usage\": " << usage.heap_size << "\n"
            << "}\n";
    return vm_info.str();
#else
    return std::string("quickjs(") +
           (LEPUS_IsGCModeRT(runtime) ? "gc)" : "rc)");
#endif
  } else {
    return "";
  }
}

std::unique_ptr<Buffer> QuickjsHelper::TakeHeapSnapshot(
    LEPUSContext *context, const char *&error, int32_t slot,
    SlotSnapshotFunction take_slot_snapshot) {
  if (!context) {
    error = "No QuickJS context";
    return nullptr;
  }
  if (slot < -1 || slot >= LEPUS_MEMORY_SIZE_SLOTS) {
    error = "Slot unavailable";
    return nullptr;
  }
  constexpr uint32_t flags = LEPUS_HEAP_SNAPSHOT_SUMMARIZE_DIAGNOSTICS |
                             LEPUS_HEAP_SNAPSHOT_INCLUDE_LOCATIONS |
                             LEPUS_HEAP_SNAPSHOT_CLEAR_CONSOLE_MESSAGES;
  const char *snapshot =
      slot < 0 ? js_profile_take_heap_snapshot_flags(context, flags)
               : take_slot_snapshot(context, slot, flags);
  if (snapshot == nullptr) {
    error = "Capture failed";
    return nullptr;
  }
  error = nullptr;
  return std::make_unique<QuickjsHeapSnapshot>(snapshot);
}

#if ENABLE_TRACE_PERFETTO
namespace {
void AddSnapshotAnnotations(lynx::perfetto::EventContext &ctx, int32_t slot,
                            size_t trigger_bytes, size_t retained_bytes,
                            const QuickjsHelper::ResidualSnapshotInfo *info) {
  if (slot < 0 || slot >= LEPUS_MEMORY_SIZE_SLOTS) return;
  auto *event = ctx.event();
  trace::WriteTraceEventArgs(ctx, "slot", slot);
  trace::WriteTraceEventArgs(ctx, "include_internal_allocations", true);
  trace::WriteTraceEventArgs(ctx, "trigger_retained_bytes", trigger_bytes);
  trace::WriteTraceEventArgs(ctx, "retained_bytes", retained_bytes);
  if (!info) return;
  trace::WriteTraceEventArgs(ctx, "post_exit_page_count",
                             info->post_exit_page_count);
  trace::WriteTraceEventArgs(ctx, "previous_retained_bytes",
                             info->previous_bytes);
  event->add_debug_annotations("capture_reason", info->reason);
  if (info->previous_bytes > 0) {
    const double change = (static_cast<double>(info->trigger_bytes) -
                           static_cast<double>(info->previous_bytes)) /
                          info->previous_bytes * 100;
    trace::WriteTraceEventArgs(ctx, "change_percent", change);
  }
}

void WriteHeapSnapshotToTrace(
    LEPUSContext *ctx, const std::string &identifier,
    const std::string &snapshot_id, const Buffer &snapshot,
    uint64_t capture_started_at_ns, int32_t slot = -1, size_t trigger_bytes = 0,
    size_t retained_bytes = 0,
    const QuickjsHelper::ResidualSnapshotInfo *info = nullptr) {
  constexpr size_t kMaxHeapSnapshotChunkSize = 128 * 1024;
  const size_t total_length = snapshot.size();
  const auto *bytes = reinterpret_cast<const char *>(snapshot.data());
  const size_t chunk_count = (total_length + kMaxHeapSnapshotChunkSize - 1) /
                             kMaxHeapSnapshotChunkSize;
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, "capture_snapshot", capture_started_at_ns, "id",
      identifier, "snapshot_id", snapshot_id, "total_length", total_length,
      "chunk_size", kMaxHeapSnapshotChunkSize, "chunk_count", chunk_count,
      "desc", QuickjsHelper::GetDebugDescription(LEPUS_GetRuntime(ctx)),
      [&](lynx::perfetto::EventContext ctx) {
        AddSnapshotAnnotations(ctx, slot, trigger_bytes, retained_bytes, info);
      });
  auto time_stamp = capture_started_at_ns;
  for (size_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
    const size_t offset = chunk_index * kMaxHeapSnapshotChunkSize;
    const size_t content_length =
        std::min(kMaxHeapSnapshotChunkSize, total_length - offset);
    std::string chunk(bytes + offset, content_length);
    TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, "snapshot_chunk", time_stamp++,
                        "id", identifier, "snapshot_id", snapshot_id,
                        "chunk_index", chunk_index, "offset", offset,
                        "content_length", content_length, "content", chunk,
                        [&](lynx::perfetto::EventContext ctx) {
                          AddSnapshotAnnotations(ctx, slot, trigger_bytes,
                                                 retained_bytes, info);
                        });
  }
}
}  // namespace

bool QuickjsHelper::TakeHeapSnapshotAndWriteToTrace(
    LEPUSContext *ctx, const std::string &identifier) {
  if (!ctx) {
    LOGI("Snapshot context missing: " << identifier);
    return false;
  }
  const auto capture_started_at_ns = trace::GetTraceTimeNs();
  const char *error = nullptr;
  auto snapshot = TakeHeapSnapshot(ctx, error);
  const std::string snapshot_id =
      identifier + "#" + std::to_string(capture_started_at_ns);
  if (snapshot != nullptr) {
    WriteHeapSnapshotToTrace(ctx, identifier, snapshot_id, *snapshot,
                             capture_started_at_ns);
    LOGI("VM snapshot: " << snapshot_id);
    return true;
  }
  LOGI("VM snapshot failed: " << snapshot_id);
  return false;
}

bool QuickjsHelper::TakeHeapSnapshotForSlotAndWriteToTrace(
    LEPUSContext *ctx, const std::string &identifier, int32_t slot,
    ResidualSnapshotInfo *info, SlotSnapshotFunction take_snapshot) {
  if (!ctx || slot < 0 || slot >= LEPUS_MEMORY_SIZE_SLOTS) {
    LOGI("Invalid snapshot context/slot: " << identifier << ", slot=" << slot);
    return false;
  }
  auto *runtime = LEPUS_GetRuntime(ctx);
  size_t slots[LEPUS_MEMORY_SIZE_SLOTS]{};
  if (!info) {
    LEPUS_DumpMemorySlots(runtime, slots);
  }
  const size_t trigger_bytes = info ? info->trigger_bytes : slots[slot];
  const auto capture_started_at_ns = trace::GetTraceTimeNs();
  const char *error = nullptr;
  auto snapshot = TakeHeapSnapshot(ctx, error, slot, take_snapshot);
  const std::string snapshot_id =
      identifier + "#" + std::to_string(capture_started_at_ns);
  if (!snapshot) {
    LOGI("Slot snapshot failed: " << snapshot_id);
    return false;
  }
  // The snapshot performs a full GC, so its slot bytes may have changed since
  // the trigger. Use the post-capture value as the next comparison baseline.
  LEPUS_DumpMemorySlots(runtime, slots);
  const size_t retained_bytes = slots[slot];
  if (info) {
    info->retained_bytes = retained_bytes;
  }
  WriteHeapSnapshotToTrace(ctx, identifier, snapshot_id, *snapshot,
                           capture_started_at_ns, slot, trigger_bytes,
                           retained_bytes, info);
  LOGI("Slot snapshot: " << snapshot_id);
  return true;
}
#endif

}  // namespace detail
}  // namespace js
}  // namespace runtime
}  // namespace lynx
