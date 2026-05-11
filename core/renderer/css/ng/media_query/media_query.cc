// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/media_query/media_query.h"

#include <sstream>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace css {

namespace {

const char* RestrictorToString(MediaQueryRestrictor restrictor) {
  switch (restrictor) {
    case MediaQueryRestrictor::kOnly:
      return "only ";
    case MediaQueryRestrictor::kNot:
      return "not ";
    case MediaQueryRestrictor::kNone:
      return "";
  }
  return "";
}

// Out-of-range restrictor values (e.g. produced by a newer encoder running
// against an older decoder) collapse to kNone so the rest of the pipeline
// treats the query as an unrestricted one instead of asserting.
MediaQueryRestrictor RestrictorFromU32(uint32_t raw) {
  switch (static_cast<MediaQueryRestrictor>(raw)) {
    case MediaQueryRestrictor::kNone:
    case MediaQueryRestrictor::kOnly:
    case MediaQueryRestrictor::kNot:
      return static_cast<MediaQueryRestrictor>(raw);
  }
  return MediaQueryRestrictor::kNone;
}

}  // namespace

std::string MediaQuery::Serialize() const {
  std::ostringstream os;
  os << RestrictorToString(restrictor_);

  const bool has_condition = static_cast<bool>(condition_);
  const bool has_type = !media_type_.empty();

  if (has_type) {
    os << media_type_;
    if (has_condition) {
      os << " and " << condition_->Serialize();
    }
  } else if (has_condition) {
    os << condition_->Serialize();
  }
  return os.str();
}

lepus_value MediaQuery::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(restrictor_));
  arr->emplace_back(media_type_);
  const bool has_condition = static_cast<bool>(condition_);
  arr->emplace_back(has_condition);
  if (has_condition) {
    arr->emplace_back(condition_->ToLepus());
  }
  return lepus_value(std::move(arr));
}

// static
fml::RefPtr<const MediaQuery> MediaQuery::FromLepus(const lepus_value& value) {
  if (!value.IsArray()) return nullptr;
  const auto& arr = value.Array();
  if (arr->size() < 3) return nullptr;
  MediaQueryRestrictor restrictor = RestrictorFromU32(arr->get(0).UInt32());
  std::string media_type = arr->get(1).StdString();
  const bool has_condition = arr->get(2).Bool();
  fml::RefPtr<const MediaQueryExpNode> condition;
  if (has_condition) {
    condition = MediaQueryExpNode::FromLepus(arr->get(3));
    if (!condition) {
      return fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNot,
                                             std::string(kTypeAll), nullptr);
    }
  }
  return fml::MakeRefCounted<MediaQuery>(restrictor, std::move(media_type),
                                         std::move(condition));
}

}  // namespace css
}  // namespace lynx
