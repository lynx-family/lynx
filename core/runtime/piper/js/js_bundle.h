// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_JS_BUNDLE_H_
#define CORE_RUNTIME_PIPER_JS_JS_BUNDLE_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/runtime/jsi/jsi.h"

namespace lynx::piper {
// A `JsContent` represents content of a js file, either source code or
// bytecode.
class JsContent {
 public:
  enum class Type {
    SOURCE,
    BYTECODE,
    ERROR,
  };

  JsContent(std::shared_ptr<StringBuffer> buffer, Type type)
      : buffer_(std::move(buffer)), type_(type) {}
  JsContent(std::string buffer, Type type)
      : buffer_(std::make_shared<StringBuffer>(std::move(buffer))),
        type_(type) {}

  std::shared_ptr<const StringBuffer> GetBuffer() && {
    return std::move(buffer_);
  }

  const std::shared_ptr<const StringBuffer> &GetBuffer() const & {
    return buffer_;
  }

  bool IsSourceCode() const { return type_ == Type::SOURCE; }
  bool IsByteCode() const { return type_ == Type::BYTECODE; }
  bool IsError() const { return type_ == Type::ERROR; }

 private:
  std::shared_ptr<const StringBuffer> buffer_;
  Type type_;
};

class JsBundle {
 public:
  void AddJsContent(const std::string &path, JsContent content);

  std::optional<std::reference_wrapper<const JsContent>> GetJsContent(
      const std::string &path) const;

  const std::unordered_map<std::string, JsContent> &GetAllJsFiles() const;

 private:
  // A js bundle can contain both sources and bytecodes.
  std::unordered_map<std::string, JsContent> js_files_;
};
}  // namespace lynx::piper

#endif  // CORE_RUNTIME_PIPER_JS_JS_BUNDLE_H_
