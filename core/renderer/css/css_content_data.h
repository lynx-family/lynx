// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_CSS_CSS_CONTENT_DATA_H_
#define CORE_RENDERER_CSS_CSS_CONTENT_DATA_H_

#include <string>

#include "base/include/value/base_string.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class AttributeHolder;

class ContentData {
 public:
  static ContentData* createTextContent(const base::String&);
  static ContentData* createImageContent(const std::string&);
  static ContentData* createAttrContent(const AttributeHolder*,
                                        const base::String&);

  virtual ~ContentData() {
    if (next_) delete next_;
  }
  virtual bool isText() const { return false; }
  virtual bool isImage() const { return false; }
  virtual bool isAttr() const { return false; }

  ContentData* next() { return next_; }
  void set_next(ContentData* next) { next_ = next; }

 private:
  ContentData* next_ = nullptr;
};

class TextContentData : public ContentData {
  friend class ContentData;

 public:
  TextContentData(const base::String& text) : text_(text) {}

  const base::String& text() const { return text_; }
  void set_text(const base::String& text) { text_ = text; }

  bool isText() const override { return true; }

 private:
  base::String text_;
};

class ImageContentData : public ContentData {
 public:
  ImageContentData(const std::string& url) : url_(url) {}

  const std::string& url() const { return url_; }
  void set_url(const std::string& url) { url_ = url; }

  bool isImage() const override { return true; }

 private:
  std::string url_;
};

class AttrContentData : public ContentData {
 public:
  AttrContentData(const AttributeHolder* owner, const base::String& text);

  const lepus::Value& attr_content();
  bool isAttr() const override { return true; }

 private:
  const AttributeHolder* attr_owner_;
  const base::String attr_key_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_CONTENT_DATA_H_
