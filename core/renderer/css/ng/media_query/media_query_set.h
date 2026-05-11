// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// "MediaQuerySet" is the top-level container for a comma-separated media
// query list such as "screen and (min-width: 600px), (orientation: portrait)".
// This is the structured form held by ConditionRule::media_queries_.

#ifndef CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_SET_H_
#define CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_SET_H_

#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/media_query/media_query.h"

namespace lynx {
namespace css {

class MediaQuerySet : public fml::RefCountedThreadSafeStorage {
 public:
  MediaQuerySet() = default;
  explicit MediaQuerySet(std::vector<fml::RefPtr<const MediaQuery>> queries)
      : queries_(std::move(queries)) {}

  void ReleaseSelf() const override { delete this; }

  void Append(fml::RefPtr<const MediaQuery> query) {
    queries_.push_back(std::move(query));
  }

  const std::vector<fml::RefPtr<const MediaQuery>>& Queries() const {
    return queries_;
  }

  bool IsEmpty() const { return queries_.empty(); }

  // Produces the comma-separated canonical form. Empty sets stringify to an
  // empty string, matching the invariant used by ConditionRule today.
  std::string Serialize() const;

  // Serialization helpers for the template binary. The lepus_value shape is
  // simply `[ query0, query1, ... ]`, mirroring how LynxCSSSelectorList is
  // encoded. `FromLepus` returns null on non-array input so the caller can
  // treat it as an empty set ("not all") which never matches.
  lepus_value ToLepus() const;
  static fml::RefPtr<const MediaQuerySet> FromLepus(const lepus_value& value);

 private:
  ~MediaQuerySet() override = default;

  std::vector<fml::RefPtr<const MediaQuery>> queries_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_SET_H_
