// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Evaluates a parsed MediaQuerySet / MediaQueryExpNode AST against a given
// MediaValues snapshot.
//
// Evaluation is intentionally pure: the evaluator holds a pointer to the
// MediaValues that describes the current surface, but never mutates it.
// This makes the evaluator cheap to construct on every matching pass.

#ifndef CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_EVALUATOR_H_
#define CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_EVALUATOR_H_

#include <string>

#include "core/renderer/css/ng/media_query/media_feature.h"
#include "core/renderer/css/ng/media_query/media_query.h"
#include "core/renderer/css/ng/media_query/media_query_exp.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "core/renderer/css/ng/media_query/media_values.h"

namespace lynx {
namespace css {

class MediaQueryEvaluator {
 public:
  MediaQueryEvaluator() = default;
  explicit MediaQueryEvaluator(const MediaValues& values) : values_(values) {}

  const MediaValues& Values() const { return values_; }
  void SetValues(const MediaValues& values) { values_ = values; }

  // True if any query in the set matches. An empty set matches (spec:
  // empty <media-query-list> evaluates to true). Invalid input is encoded
  // by the parser as a single `not all` entry, which yields false here.
  bool Eval(const MediaQuerySet& query_set) const;
  bool Eval(const MediaQuerySet* query_set) const {
    return query_set && Eval(*query_set);
  }

  // Evaluates a single media query, honoring the `only` / `not` restrictor.
  bool Eval(const MediaQuery& query) const;

  // Evaluates a bare media-condition node (no media-type).
  bool EvalNode(const MediaQueryExpNode& node) const;
  bool EvalNode(const MediaQueryExpNode* node) const {
    return node && EvalNode(*node);
  }

  // Evaluates one feature assertion.
  bool EvalFeature(const MediaFeature& feature) const;

 private:
  using ValueConverter = double (*)(const MediaFeatureValue&,
                                    const MediaValues&);

  bool EvalNumericFeature(double actual, const MediaFeature& feature,
                          MediaFeatureOperator implicit_op,
                          ValueConverter converter) const;
  bool EvalOrientationFeature(const MediaFeature& feature) const;
  bool EvalAspectRatioFeature(const MediaFeature& feature,
                              MediaFeatureOperator implicit_op) const;
  bool EvalHoverFeature(const MediaFeature& feature) const;
  bool EvalPointerFeature(const MediaFeature& feature) const;
  bool EvalColorSchemeFeature(const MediaFeature& feature) const;
  bool EvalCustomFeature(const MediaFeature& feature) const;

  MediaValues values_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_EVALUATOR_H_
