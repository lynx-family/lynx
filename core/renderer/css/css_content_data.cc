// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_content_data.h"

#include "core/renderer/dom/attribute_holder.h"

namespace lynx {
namespace tasm {

ContentData* ContentData::createTextContent(const base::String& text) {
  return new TextContentData(text);
}

ContentData* ContentData::createImageContent(const std::string& url) {
  return new ImageContentData(url);
}

ContentData* ContentData::createAttrContent(const AttributeHolder* node,
                                            const base::String& attr) {
  return new AttrContentData(node, attr);
}

AttrContentData::AttrContentData(const AttributeHolder* owner,
                                 const base::String& attr)
    : attr_owner_(owner), attr_key_(attr) {}

const lepus::Value& AttrContentData::attr_content() {
  static lepus::Value kTempLepusValue = lepus::Value();
  if (attr_owner_ == nullptr) return kTempLepusValue;

  auto& styles = attr_owner_->attributes();
  auto iter = styles.find(attr_key_);
  if (iter != styles.end()) return iter->second;

  return kTempLepusValue;
}

// TODO: another type

}  // namespace tasm
}  // namespace lynx
