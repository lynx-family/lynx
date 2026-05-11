// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// "MediaQuery" models a single entry in a comma-separated media query list
// following the CSS Media Queries Level 4 grammar:
//
//   <media-query> = <media-condition>
//                 | [ not | only ]? <media-type> [ and <media-condition> ]?
//
// Pure data carrier; evaluation is done by MediaQueryEvaluator.

#ifndef CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_H_
#define CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_H_

#include <cstdint>
#include <string>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/media_query/media_query_exp.h"

namespace lynx {
namespace css {

// Modifier applied to the media type.
//
// Values are assigned explicit stable numbers because they are serialized
// into the template binary (via MediaQuery::ToLepus). Existing values MUST
// NOT be renumbered; new restrictors MUST be appended at the end.
enum class MediaQueryRestrictor : uint8_t {
  kNone = 0,
  kOnly = 1,
  kNot = 2,
};

class MediaQuery : public fml::RefCountedThreadSafeStorage {
 public:
  // Well-known media types. We only model the ones CSS still keeps
  // first-class; everything else can be carried in `type_name_` verbatim.
  static constexpr const char* kTypeAll = "all";
  static constexpr const char* kTypeScreen = "screen";
  static constexpr const char* kTypePrint = "print";

  MediaQuery(MediaQueryRestrictor restrictor, std::string media_type,
             fml::RefPtr<const MediaQueryExpNode> condition)
      : restrictor_(restrictor),
        media_type_(std::move(media_type)),
        condition_(std::move(condition)) {}

  void ReleaseSelf() const override { delete this; }

  MediaQueryRestrictor Restrictor() const { return restrictor_; }
  const std::string& MediaType() const { return media_type_; }
  const fml::RefPtr<const MediaQueryExpNode>& Condition() const {
    return condition_;
  }

  // Produces `<restrictor>? <media-type> [ and <condition> ]?`, keeping the
  // tokens lowercase and in canonical order.
  std::string Serialize() const;

  // Serialization helpers for the template binary. The lepus_value shape is:
  //   [ restrictor(u8), media_type(string), has_condition(bool),
  //     condition(array)|absent ]
  // Decoders MUST tolerate `has_condition == false` with the condition slot
  // simply missing.
  lepus_value ToLepus() const;
  static fml::RefPtr<const MediaQuery> FromLepus(const lepus_value& value);

 private:
  ~MediaQuery() override = default;

  MediaQueryRestrictor restrictor_ = MediaQueryRestrictor::kNone;
  std::string media_type_;
  fml::RefPtr<const MediaQueryExpNode> condition_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_H_
