// Copyright (C) 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_EVENT_UTILS_PERFETTO_H_
#define BASE_TRACE_NATIVE_TRACE_EVENT_UTILS_PERFETTO_H_

#include <cstring>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/trace/native/internal_trace_category.h"
#include "base/trace/native/trace_export.h"
#include "base/trace/native/track_event_wrapper.h"

namespace lynx {
namespace trace {

enum class TraceEventType : int32_t {
  TYPE_UNSPECIFIED = 0,
  TYPE_SLICE_BEGIN = 1,
  TYPE_SLICE_END = 2,
  TYPE_INSTANT = 3,
  TYPE_COUNTER = 4,
};

using argsVecPair =
    std::pair<std::vector<const char*>, std::vector<std::string>>;
using FuncType = std::function<void(lynx::perfetto::EventContext)>;

TRACE_EXPORT uint64_t GetFlowId();
TRACE_EXPORT void TraceEventImplementation(
    const char* category_name, const char* name, TraceEventType phase,
    const lynx::perfetto::Track* track_id, const uint64_t& timestamp,
    const FuncType& callback);
TRACE_EXPORT void TraceEventImplementation(
    const char* category_name, const std::string& name, TraceEventType phase,
    const lynx::perfetto::Track* track_id, const uint64_t& timestamp,
    const FuncType& callback);
TRACE_EXPORT void TraceEventImplementation(
    const char* category_name,
    const lynx::perfetto::CounterTrack& counter_track, TraceEventType phase,
    const uint64_t& timestamp, const uint64_t& counter);
TRACE_EXPORT bool TraceEventCategoryEnabled(const char* category);
TRACE_EXPORT void TraceRuntimeProfile(const std::string& runtime_profile,
                                      const uint64_t track_id,
                                      const int32_t profile_id);

// remove_cvref is an implementation of std::remove_cvref from
// C++20.
//
// Specification:
// https://en.cppreference.com/w/cpp/types/remove_cvref
template <class T>
struct remove_cvref {
  using type = typename std::remove_cv<typename std::remove_cv<
      typename std::remove_reference<T>::type>::type>::type;
};
template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T, bool = std::is_enum<T>::value>
struct safe_underlying_type {
  using type = typename std::underlying_type<T>::type;
};

template <typename T>
struct safe_underlying_type<T, false> {
  using type = T;
};

// TraceFormatTraits implementations for primitive types.
template <typename T, class = void>
struct TraceFormatTraits;

// Specialisation for signed integer types (note: it excludes enums, which have
// their own explicit specialisation).
template <typename T>
struct TraceFormatTraits<
    T, typename std::enable_if<std::is_integral<T>::value &&
                               !std::is_same<T, bool>::value &&
                               std::is_signed<T>::value>::type> {
  inline static void WriteIntoTrace(lynx::perfetto::LynxDebugAnnotation* debug,
                                    T value) {
    debug->set_int_value(value);
  }
};

// Specialisation for unsigned integer types (note: it excludes enums, which
// have their own explicit specialisation).
template <typename T>
struct TraceFormatTraits<
    T, typename std::enable_if<std::is_integral<T>::value &&
                               !std::is_same<T, bool>::value &&
                               std::is_unsigned<T>::value>::type> {
  inline static void WriteIntoTrace(lynx::perfetto::LynxDebugAnnotation* debug,
                                    T value) {
    debug->set_uint_value(value);
  }
};

// Specialisation for bools.
template <>
struct TraceFormatTraits<bool> {
  inline static void WriteIntoTrace(lynx::perfetto::LynxDebugAnnotation* debug,
                                    bool value) {
    debug->set_bool_value(value);
  }
};

// Specialisations for C-style strings.
template <>
struct TraceFormatTraits<const char*> {
  inline static void WriteIntoTrace(lynx::perfetto::LynxDebugAnnotation* debug,
                                    const char* value) {
    debug->set_string_value(value, strlen(value));
  }
};

template <size_t N>
struct TraceFormatTraits<char[N]> {
  inline static void WriteIntoTrace(lynx::perfetto::LynxDebugAnnotation* debug,
                                    const char value[N]) {
    debug->set_string_value(value, N);
  }
};

// Specialisation for C++ strings.
template <>
struct TraceFormatTraits<std::string> {
  inline static void WriteIntoTrace(lynx::perfetto::LynxDebugAnnotation* debug,
                                    const std::string& value) {
    debug->set_string_value(value);
  }
};

template <typename ValueType>
inline void WriteTraceEventArgs(lynx::perfetto::EventContext ctx,
                                const char* arg_name, ValueType&& arg_value) {
  auto* debug = ctx.event()->add_debug_annotations();
  debug->set_name(arg_name);
  TraceFormatTraits<trace::remove_cvref_t<ValueType>>::WriteIntoTrace(
      std::move(debug), std::forward<ValueType>(arg_value));
}
inline void WriteTraceEventArgs(lynx::perfetto::EventContext ctx,
                                FuncType callback) {
  callback(std::move(ctx));
}
inline void WriteTraceEventArgs(lynx::perfetto::EventContext ctx) {}
template <typename ValueType, typename... Arguments>
inline void WriteTraceEventArgs(lynx::perfetto::EventContext ctx,
                                const char* arg_name, ValueType&& arg_value,
                                Arguments... args) {
  auto* debug = ctx.event()->add_debug_annotations();
  debug->set_name(arg_name);
  TraceFormatTraits<trace::remove_cvref_t<ValueType>>::WriteIntoTrace(
      std::move(debug), std::forward<ValueType>(arg_value));
  WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
}

template <typename EventNameType>
inline void TraceEventBegin(const char* category, const EventNameType& name) {
  TraceEventImplementation(category, name, TraceEventType::TYPE_SLICE_BEGIN,
                           nullptr, 0, nullptr);
}

template <typename EventNameType>
inline void TraceEventBegin(const char* category, const EventNameType& name,
                            FuncType callback) {
  TraceEventImplementation(category, name, TraceEventType::TYPE_SLICE_BEGIN,
                           nullptr, 0, [&](lynx::perfetto::EventContext ctx) {
                             WriteTraceEventArgs(std::move(ctx), callback);
                           });
}

template <typename EventNameType, typename... Arguments>
inline void TraceEventBegin(const char* category, const EventNameType& name,
                            const char* key, Arguments&&... args) {
  TraceEventImplementation(category, name, TraceEventType::TYPE_SLICE_BEGIN,
                           nullptr, 0, [&](lynx::perfetto::EventContext ctx) {
                             WriteTraceEventArgs(
                                 std::move(ctx), key,
                                 std::forward<Arguments>(args)...);
                           });
}
template <typename EventNameType, typename... Arguments>
inline void TraceEventBegin(const char* category, const EventNameType& name,
                            const lynx::perfetto::Track& track_id,
                            uint64_t timestamp, Arguments&&... args) {
  TraceEventImplementation(
      category, name, TraceEventType::TYPE_SLICE_BEGIN, &track_id, timestamp,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}
template <typename EventNameType, typename... Arguments>
inline void TraceEventBegin(const char* category, const EventNameType& name,
                            uint64_t timestamp, Arguments&&... args) {
  TraceEventImplementation(
      category, name, TraceEventType::TYPE_SLICE_BEGIN, nullptr, timestamp,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}
template <typename EventNameType, typename... Arguments>
inline void TraceEventBegin(const char* category, const EventNameType& name,
                            const lynx::perfetto::Track& track_id,
                            Arguments&&... args) {
  TraceEventImplementation(
      category, name, TraceEventType::TYPE_SLICE_BEGIN, &track_id, 0,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}

inline void TraceEventEnd(const char* category) {
  TraceEventImplementation(category, nullptr, TraceEventType::TYPE_SLICE_END,
                           nullptr, 0, nullptr);
}
inline void TraceEventEnd(const char* category, FuncType callback) {
  TraceEventImplementation(category, nullptr, TraceEventType::TYPE_SLICE_END,
                           nullptr, 0, [&](lynx::perfetto::EventContext ctx) {
                             WriteTraceEventArgs(std::move(ctx), callback);
                           });
}
template <typename... Arguments>
inline void TraceEventEnd(const char* category, const char* key,
                          Arguments&&... args) {
  TraceEventImplementation(category, nullptr, TraceEventType::TYPE_SLICE_END,
                           nullptr, 0, [&](lynx::perfetto::EventContext ctx) {
                             WriteTraceEventArgs(
                                 std::move(ctx), key,
                                 std::forward<Arguments>(args)...);
                           });
}
template <typename... Arguments>
inline void TraceEventEnd(const char* category,
                          const lynx::perfetto::Track& track_id,
                          uint64_t timestamp, Arguments&&... args) {
  TraceEventImplementation(
      category, nullptr, TraceEventType::TYPE_SLICE_END, &track_id, timestamp,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}
template <typename... Arguments>
inline void TraceEventEnd(const char* category, uint64_t timestamp,
                          Arguments&&... args) {
  TraceEventImplementation(
      category, nullptr, TraceEventType::TYPE_SLICE_END, nullptr, timestamp,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}
template <typename... Arguments>
inline void TraceEventEnd(const char* category,
                          const lynx::perfetto::Track& track_id,
                          Arguments&&... args) {
  TraceEventImplementation(
      category, nullptr, TraceEventType::TYPE_SLICE_END, &track_id, 0,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}

template <typename EventNameType>
inline void TraceEventInstant(const char* category, const EventNameType& name) {
  TraceEventImplementation(category, name, TraceEventType::TYPE_INSTANT,
                           nullptr, 0, nullptr);
}
template <typename EventNameType>
inline void TraceEventInstant(const char* category, const EventNameType& name,
                              FuncType callback) {
  TraceEventImplementation(category, name, TraceEventType::TYPE_INSTANT,
                           nullptr, 0, [&](lynx::perfetto::EventContext ctx) {
                             WriteTraceEventArgs(std::move(ctx), callback);
                           });
}
template <typename EventNameType, typename... Arguments>
inline void TraceEventInstant(const char* category, const EventNameType& name,
                              const char* key, Arguments&&... args) {
  TraceEventImplementation(category, name, TraceEventType::TYPE_INSTANT,
                           nullptr, 0, [&](lynx::perfetto::EventContext ctx) {
                             WriteTraceEventArgs(
                                 std::move(ctx), key,
                                 std::forward<Arguments>(args)...);
                           });
}
template <typename EventNameType, typename... Arguments>
inline void TraceEventInstant(const char* category, const EventNameType& name,
                              const lynx::perfetto::Track& track_id,
                              uint64_t timestamp, Arguments&&... args) {
  TraceEventImplementation(
      category, name, TraceEventType::TYPE_INSTANT, &track_id, timestamp,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}
template <typename EventNameType, typename... Arguments>
inline void TraceEventInstant(const char* category, const EventNameType& name,
                              uint64_t timestamp, Arguments&&... args) {
  TraceEventImplementation(
      category, name, TraceEventType::TYPE_INSTANT, nullptr, timestamp,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}
template <typename EventNameType, typename... Arguments>
inline void TraceEventInstant(const char* category, const EventNameType& name,
                              const lynx::perfetto::Track& track_id,
                              Arguments&&... args) {
  TraceEventImplementation(
      category, name, TraceEventType::TYPE_INSTANT, &track_id, 0,
      [&](lynx::perfetto::EventContext ctx) {
        WriteTraceEventArgs(std::move(ctx), std::forward<Arguments>(args)...);
      });
}

inline void TraceCounter(const char* category,
                         const lynx::perfetto::CounterTrack& track,
                         uint64_t timestamp, uint64_t counter) {
  TraceEventImplementation(category, track, TraceEventType::TYPE_COUNTER,
                           timestamp, counter);
}
inline void TraceCounter(const char* category,
                         const lynx::perfetto::CounterTrack& track,
                         uint64_t counter) {
  TraceEventImplementation(category, track, TraceEventType::TYPE_COUNTER, 0,
                           counter);
}

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_TRACE_EVENT_UTILS_PERFETTO_H_
