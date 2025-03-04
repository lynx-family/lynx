/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the current directory.
 */
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSLIB_H_
#define CORE_RUNTIME_JSI_JSLIB_H_
#include <memory>
#include <string>
#include <utility>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class FileBuffer : public Buffer {
 public:
  FileBuffer(const std::string& path);
  ~FileBuffer();

  size_t size() const override { return size_; }

  const uint8_t* data() const override { return data_; }

 private:
  size_t size_;
  uint8_t* data_;
};

// A trivial implementation of PreparedJavaScript that simply stores the source
// buffer and URL.
class SourceJavaScriptPreparation final : public piper::PreparedJavaScript,
                                          public piper::Buffer {
  std::shared_ptr<const piper::Buffer> buf_;
  std::string sourceURL_;

 public:
  SourceJavaScriptPreparation(std::shared_ptr<const piper::Buffer> buf,
                              std::string sourceURL)
      : buf_(std::move(buf)), sourceURL_(std::move(sourceURL)) {}

  const std::string& sourceURL() const { return sourceURL_; }

  std::shared_ptr<const piper::Buffer> buffer() const { return buf_; }

  size_t size() const override { return buf_->size(); }
  const uint8_t* data() const override { return buf_->data(); }
};

class QuickjsJavaScriptPreparation final : public piper::PreparedJavaScript {
  std::shared_ptr<const piper::Buffer> buf_;
  std::shared_ptr<const piper::Buffer> bin_;
  std::string source_url_;

 public:
  QuickjsJavaScriptPreparation(std::shared_ptr<const piper::Buffer> buf,
                               std::shared_ptr<const piper::Buffer> bin,
                               std::string source_url)
      : buf_(std::move(buf)),
        bin_(std::move(bin)),
        source_url_(std::move(source_url)) {}

  const std::string& SourceUrl() const { return source_url_; }
  std::shared_ptr<const piper::Buffer> Source() const { return buf_; }
  std::shared_ptr<const piper::Buffer> Bytecode() const { return bin_; }
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSLIB_H_
