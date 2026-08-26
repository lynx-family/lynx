// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_UTILS_H_
#define CORE_RUNTIME_JS_UTILS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/base/json/json_util.h"
#include "core/runtime/js/jsi/jsi.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace runtime {
namespace js {
class JSIObjectWrapperManager;
std::optional<Value> valueFromLepus(
    Runtime& runtime, const lepus::Value& data,
    JSIObjectWrapperManager* jsi_object_wrapper_manager = nullptr);

std::optional<Array> arrayFromLepus(Runtime& runtime,
                                    const lepus::CArray& array);

// Evaluate a list of JS sources with (url, buffer) pairs via prepared JS.
// Returns true if `/lynx_core.js` is included in `sources`.
bool EvaluatePreloadSources(
    Runtime& runtime,
    std::vector<std::pair<std::string, std::shared_ptr<Buffer>>>& sources);

const char* JSRuntimeTypeToString(JSRuntimeType type);

using JSValueCircularArray = base::InlineVector<Object, 32>;

// Tracks the ancestor JS objects along the current traversal path together with
// the property key / array index used to reach each of them. When a circular
// reference is detected, this lets us report the full path that closes the
// cycle (e.g. `.user.friends[2].parent`) and the ancestor it points back to
// (e.g. `.user`), instead of only a static conversion-site tag. The same
// message is used for both the reported JSI exception and the LOGE log so the
// two never drift apart.
class CircularDataChecker {
 public:
  explicit CircularDataChecker(Runtime& runtime) : runtime_(runtime) {}

  // Builds the path segment for an object property key. Keys containing path
  // separators are quoted so the rendered path stays unambiguous.
  static std::string KeySegment(const std::string& key);
  // Builds the path segment for an array index, e.g. `[2]`.
  static std::string IndexSegment(size_t index);

  // Returns true and reports a JSI exception (once) if `object` is already one
  // of the ancestors on the current traversal path and circular-data check is
  // enabled (or unset). `segment` is the path segment used to reach `object`
  // from its parent (empty means the traversal root). `context` names the
  // conversion site, e.g. "ParseJSValue".
  bool CheckAndReport(const Object& object, const std::string& segment,
                      const char* context);

  // RAII helper that pushes one traversal step (the entered object plus the
  // segment used to reach it) and pops it on scope exit.
  class ScopedPath {
   public:
    ScopedPath(CircularDataChecker& checker, Object object,
               const std::string& segment)
        : checker_(checker) {
      checker_.frames_.emplace_back(std::move(object), segment);
    }
    ~ScopedPath() { checker_.frames_.pop_back(); }

    ScopedPath(const ScopedPath&) = delete;
    ScopedPath& operator=(const ScopedPath&) = delete;

   private:
    CircularDataChecker& checker_;
  };

 private:
  struct Frame {
    Frame(Object object, std::string segment)
        : object(std::move(object)), segment(std::move(segment)) {}
    Object object;
    std::string segment;
  };

  // Concatenation of every frame's segment, i.e. the path to the object that
  // is currently on top of the traversal stack.
  std::string CurrentPath() const;
  // Concatenation of frame segments in [0, index], i.e. the path to the
  // ancestor stored at `index`.
  std::string PathUpTo(size_t index) const;

  Runtime& runtime_;
  base::InlineVector<Frame, 32> frames_;
};

std::optional<lepus_value> ParseJSValue(
    Runtime& runtime, const Value& value,
    JSIObjectWrapperManager* jsi_object_wrapper_manager,
    const std::string& jsi_object_group_id, const std::string& targetSDKVersion,
    CircularDataChecker& checker, const std::string& segment = std::string());

bool IsCircularJSObject(Runtime& runtime, const Object& object,
                        const JSValueCircularArray& pre_object_vector);

// Convert string[] to std::vector<std::string>.
// The input value must be an array and each element in input must be string.
// Otherwise, the conversion will be aborted and return false.
bool ConvertPiperValueToStringVector(Runtime& rt, const Value& input,
                                     std::vector<std::string>& result);

class ScopedJSObjectPushPopHelper {
 public:
  ScopedJSObjectPushPopHelper(JSValueCircularArray& vector, Object object)
      : pre_object_vector_(vector) {
    pre_object_vector_.push_back(std::move(object));
  };
  ~ScopedJSObjectPushPopHelper() { pre_object_vector_.pop_back(); }

 private:
  JSValueCircularArray& pre_object_vector_;
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_UTILS_H_
