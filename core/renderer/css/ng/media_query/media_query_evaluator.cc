// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/media_query/media_query_evaluator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#include "base/include/float_comparison.h"

namespace lynx {
namespace css {

namespace {

// Returns a value converted into CSS pixels using the supplied font roots.
// Unknown/text-only units fall back to the raw numeric value so the caller
// can still compare against an actual number when the AST chose not to tag
// the unit (this only happens for forward-compat placeholders).
double ToPixels(const MediaFeatureValue& v, const MediaValues& values) {
  // Plain numbers (no unit) compare as-is -- this also covers the
  // number/integer case the old enum-based switch handled explicitly.
  if (v.IsNumber()) return v.Numeric();
  if (!v.IsLength()) return v.Numeric();

  switch (v.Unit()) {
    case MediaFeatureUnit::kPixels:
      return v.Numeric();
    case MediaFeatureUnit::kRem:
      return v.Numeric() * values.RootFontSize();
    case MediaFeatureUnit::kEm:
      return v.Numeric() * values.FontSize();
    case MediaFeatureUnit::kViewportWidth:
      return v.Numeric() * 0.01 * values.ViewportWidth();
    case MediaFeatureUnit::kViewportHeight:
      return v.Numeric() * 0.01 * values.ViewportHeight();
    case MediaFeatureUnit::kViewportMin:
      return v.Numeric() * 0.01 *
             std::min(values.ViewportWidth(), values.ViewportHeight());
    case MediaFeatureUnit::kViewportMax:
      return v.Numeric() * 0.01 *
             std::max(values.ViewportWidth(), values.ViewportHeight());
    case MediaFeatureUnit::kPercent:
      // Percent against the viewport width is the common interpretation in
      // media queries; callers that want height-relative comparison should
      // not use percent here.
      return v.Numeric() * 0.01 * values.ViewportWidth();
    default:
      break;
  }
  return v.Numeric();
}

double ToDppx(const MediaFeatureValue& v, const MediaValues&) {
  if (v.IsNumber()) return v.Numeric();
  if (!v.IsResolution()) return v.Numeric();
  switch (v.Unit()) {
    case MediaFeatureUnit::kDppx:
      return v.Numeric();
    case MediaFeatureUnit::kDpi:
      return v.Numeric() / 96.0;
    case MediaFeatureUnit::kDpcm:
      return v.Numeric() / 37.795275591;
    default:
      break;
  }
  return v.Numeric();
}

double ToNumber(const MediaFeatureValue& v, const MediaValues&) {
  return v.Numeric();
}

// Compares `actual` against `bound` using `op`. When `op` is kNone we
// treat it as "plain-form equality" for numeric values; boolean features
// are handled by their own branch and never reach this helper.
bool CompareNumeric(double actual, MediaFeatureOperator op, double bound) {
  // Treat NaN-bound as always-false to be safe.
  if (std::isnan(actual) || std::isnan(bound)) return false;
  switch (op) {
    case MediaFeatureOperator::kNone:
    case MediaFeatureOperator::kEq:
      return actual == bound;
    case MediaFeatureOperator::kLt:
      return actual < bound;
    case MediaFeatureOperator::kLe:
      return actual <= bound;
    case MediaFeatureOperator::kGt:
      return actual > bound;
    case MediaFeatureOperator::kGe:
      return actual >= bound;
  }
  return false;
}

// For the Level 4 range syntax `A op1 name op2 B`, the parser stores the
// left side as (FlipRelOp(op1), A) on `feature`. We evaluate both halves
// with the actual feature value as `actual` and the boundary values from
// the AST.
bool CompareRange(double actual, const MediaFeature& feature,
                  double (*to_actual)(const MediaFeatureValue&,
                                      const MediaValues&),
                  const MediaValues& values) {
  if (!feature.LeftValue().IsValid()) return false;
  const bool left_ok = CompareNumeric(actual, feature.LeftOperator(),
                                      to_actual(feature.LeftValue(), values));
  if (!left_ok) return false;
  if (!feature.HasRightBound()) return true;
  return CompareNumeric(actual, feature.RightOperator(),
                        to_actual(feature.RightValue(), values));
}

bool CompareRatio(double actual_num, double actual_den, MediaFeatureOperator op,
                  double bound_num, double bound_den) {
  if (actual_den <= 0.0 || bound_den <= 0.0) return false;
  const double lhs = actual_num * bound_den;
  const double rhs = bound_num * actual_den;
  if (std::isnan(lhs) || std::isnan(rhs)) return false;
  switch (op) {
    case MediaFeatureOperator::kNone:
    case MediaFeatureOperator::kEq:
      return base::DoublesEqual(lhs, rhs);
    case MediaFeatureOperator::kLt:
      return lhs < rhs && !base::DoublesEqual(lhs, rhs);
    case MediaFeatureOperator::kLe:
      return lhs < rhs || base::DoublesEqual(lhs, rhs);
    case MediaFeatureOperator::kGt:
      return lhs > rhs && !base::DoublesEqual(lhs, rhs);
    case MediaFeatureOperator::kGe:
      return lhs > rhs || base::DoublesEqual(lhs, rhs);
  }
  return false;
}

void ExtractRatio(const MediaFeatureValue& v, double& num, double& den) {
  if (v.IsRatio()) {
    num = v.Numeric();
    den = v.Denominator();
  } else {
    num = v.Numeric();
    den = 1.0;
  }
}

bool CompareRatioRange(double actual_num, double actual_den,
                       const MediaFeature& feature) {
  if (!feature.LeftValue().IsValid()) return false;
  double bound_num, bound_den;
  ExtractRatio(feature.LeftValue(), bound_num, bound_den);
  if (!CompareRatio(actual_num, actual_den, feature.LeftOperator(), bound_num,
                    bound_den))
    return false;
  if (!feature.HasRightBound()) return true;
  ExtractRatio(feature.RightValue(), bound_num, bound_den);
  return CompareRatio(actual_num, actual_den, feature.RightOperator(),
                      bound_num, bound_den);
}

}  // namespace

bool MediaQueryEvaluator::Eval(const MediaQuerySet& query_set) const {
  // Empty list matches (spec: equivalent to `all`). Invalid input is
  // represented by the parser as a single `not all` entry, not an empty set.
  if (query_set.IsEmpty()) return true;
  for (const auto& query : query_set.Queries()) {
    if (query && Eval(*query)) return true;
  }
  return false;
}

bool MediaQueryEvaluator::Eval(const MediaQuery& query) const {
  // Media type check. `all` (and an empty string for bare conditions)
  // always passes; unsupported types fail before the condition is even
  // considered so that `not print` on a `screen`-only surface matches.
  bool type_ok = true;
  const std::string& type = query.MediaType();
  if (!type.empty() && type != MediaQuery::kTypeAll) {
    // The renderer is always a "screen"-like surface today. Anything else
    // (print/speech/tty/custom) does not match.
    type_ok = (type == MediaQuery::kTypeScreen);
  }

  bool condition_ok = true;
  if (query.Condition()) {
    condition_ok = EvalNode(*query.Condition());
  }

  const bool both = type_ok && condition_ok;
  switch (query.Restrictor()) {
    case MediaQueryRestrictor::kNot:
      return !both;
    case MediaQueryRestrictor::kOnly:
    case MediaQueryRestrictor::kNone:
      return both;
  }
  return both;
}

bool MediaQueryEvaluator::EvalNode(const MediaQueryExpNode& node) const {
  switch (node.GetType()) {
    case MediaQueryExpNode::Type::kFeature: {
      const auto& feature =
          static_cast<const MediaQueryFeatureExpNode&>(node).Feature();
      return EvalFeature(feature);
    }
    case MediaQueryExpNode::Type::kNested: {
      const auto& inner =
          static_cast<const MediaQueryNestedExpNode&>(node).Inner();
      return EvalNode(inner.get());
    }
    case MediaQueryExpNode::Type::kNot: {
      const auto& operand =
          static_cast<const MediaQueryNotExpNode&>(node).Operand();
      return !EvalNode(operand.get());
    }
    case MediaQueryExpNode::Type::kAnd: {
      const auto& compound =
          static_cast<const MediaQueryCompoundExpNode&>(node);
      return EvalNode(compound.Left().get()) &&
             EvalNode(compound.Right().get());
    }
    case MediaQueryExpNode::Type::kOr: {
      const auto& compound =
          static_cast<const MediaQueryCompoundExpNode&>(node);
      return EvalNode(compound.Left().get()) ||
             EvalNode(compound.Right().get());
    }
  }
  return false;
}

bool MediaQueryEvaluator::EvalFeature(const MediaFeature& feature) const {
  switch (feature.Id()) {
    case MediaFeatureId::kWidth:
    case MediaFeatureId::kDeviceWidth:
      return EvalNumericFeature(values_.ViewportWidth(), feature,
                                MediaFeatureOperator::kNone, &ToPixels);
    case MediaFeatureId::kMinWidth:
    case MediaFeatureId::kMinDeviceWidth:
      return EvalNumericFeature(values_.ViewportWidth(), feature,
                                MediaFeatureOperator::kGe, &ToPixels);
    case MediaFeatureId::kMaxWidth:
    case MediaFeatureId::kMaxDeviceWidth:
      return EvalNumericFeature(values_.ViewportWidth(), feature,
                                MediaFeatureOperator::kLe, &ToPixels);
    case MediaFeatureId::kHeight:
    case MediaFeatureId::kDeviceHeight:
      return EvalNumericFeature(values_.ViewportHeight(), feature,
                                MediaFeatureOperator::kNone, &ToPixels);
    case MediaFeatureId::kMinHeight:
    case MediaFeatureId::kMinDeviceHeight:
      return EvalNumericFeature(values_.ViewportHeight(), feature,
                                MediaFeatureOperator::kGe, &ToPixels);
    case MediaFeatureId::kMaxHeight:
    case MediaFeatureId::kMaxDeviceHeight:
      return EvalNumericFeature(values_.ViewportHeight(), feature,
                                MediaFeatureOperator::kLe, &ToPixels);
    case MediaFeatureId::kOrientation:
      return EvalOrientationFeature(feature);
    case MediaFeatureId::kResolution:
      return EvalNumericFeature(values_.DevicePixelRatio(), feature,
                                MediaFeatureOperator::kNone, &ToDppx);
    case MediaFeatureId::kMinResolution:
      return EvalNumericFeature(values_.DevicePixelRatio(), feature,
                                MediaFeatureOperator::kGe, &ToDppx);
    case MediaFeatureId::kMaxResolution:
      return EvalNumericFeature(values_.DevicePixelRatio(), feature,
                                MediaFeatureOperator::kLe, &ToDppx);
    case MediaFeatureId::kAspectRatio:
    case MediaFeatureId::kDeviceAspectRatio:
      return EvalAspectRatioFeature(feature, MediaFeatureOperator::kNone);
    case MediaFeatureId::kMinAspectRatio:
    case MediaFeatureId::kMinDeviceAspectRatio:
      return EvalAspectRatioFeature(feature, MediaFeatureOperator::kGe);
    case MediaFeatureId::kMaxAspectRatio:
    case MediaFeatureId::kMaxDeviceAspectRatio:
      return EvalAspectRatioFeature(feature, MediaFeatureOperator::kLe);
    case MediaFeatureId::kHover:
      return EvalHoverFeature(feature);
    case MediaFeatureId::kPointer:
      return EvalPointerFeature(feature);
    case MediaFeatureId::kPrefersColorScheme:
      return EvalColorSchemeFeature(feature);
    case MediaFeatureId::kColor:
      return EvalNumericFeature(
          static_cast<double>(values_.ColorBitsPerComponent()), feature,
          MediaFeatureOperator::kNone, &ToNumber);
    case MediaFeatureId::kMinColor:
      return EvalNumericFeature(
          static_cast<double>(values_.ColorBitsPerComponent()), feature,
          MediaFeatureOperator::kGe, &ToNumber);
    case MediaFeatureId::kMaxColor:
      return EvalNumericFeature(
          static_cast<double>(values_.ColorBitsPerComponent()), feature,
          MediaFeatureOperator::kLe, &ToNumber);
    case MediaFeatureId::kDevicePixelRatio:
      return EvalNumericFeature(values_.DevicePixelRatio(), feature,
                                MediaFeatureOperator::kNone, &ToNumber);
    case MediaFeatureId::kMinDevicePixelRatio:
      return EvalNumericFeature(values_.DevicePixelRatio(), feature,
                                MediaFeatureOperator::kGe, &ToNumber);
    case MediaFeatureId::kMaxDevicePixelRatio:
      return EvalNumericFeature(values_.DevicePixelRatio(), feature,
                                MediaFeatureOperator::kLe, &ToNumber);
    case MediaFeatureId::kUnknown:
      return EvalCustomFeature(feature);
  }
  return false;
}

// ---- feature families ------------------------------------------------------

bool MediaQueryEvaluator::EvalNumericFeature(double actual,
                                             const MediaFeature& feature,
                                             MediaFeatureOperator implicit_op,
                                             ValueConverter converter) const {
  if (feature.IsBoolean()) return actual > 0.0;
  if (implicit_op == MediaFeatureOperator::kNone) {
    return CompareRange(actual, feature, converter, values_);
  }
  if (feature.LeftOperator() != MediaFeatureOperator::kNone) return false;
  MediaFeature resolved(feature.Id(), feature.Name(), implicit_op,
                        feature.LeftValue());
  return CompareRange(actual, resolved, converter, values_);
}

bool MediaQueryEvaluator::EvalOrientationFeature(
    const MediaFeature& feature) const {
  MediaOrientation want = values_.Orientation();
  if (feature.IsBoolean()) return want != MediaOrientation::kUnknown;

  const auto& v = feature.LeftValue();
  if (!v.IsIdent()) return false;
  MediaOrientation given = MediaOrientation::kUnknown;
  if (v.Text() == "portrait")
    given = MediaOrientation::kPortrait;
  else if (v.Text() == "landscape")
    given = MediaOrientation::kLandscape;

  if (want == MediaOrientation::kUnknown ||
      given == MediaOrientation::kUnknown) {
    return false;
  }
  return want == given;
}

bool MediaQueryEvaluator::EvalAspectRatioFeature(
    const MediaFeature& feature, MediaFeatureOperator implicit_op) const {
  const double w = values_.ViewportWidth();
  const double h = values_.ViewportHeight();
  if (h <= 0.0) return false;

  if (feature.IsBoolean()) return w > 0.0;

  if (implicit_op == MediaFeatureOperator::kNone) {
    return CompareRatioRange(w, h, feature);
  }
  if (feature.LeftOperator() != MediaFeatureOperator::kNone) return false;
  MediaFeature resolved(feature.Id(), feature.Name(), implicit_op,
                        feature.LeftValue());
  return CompareRatioRange(w, h, resolved);
}

bool MediaQueryEvaluator::EvalHoverFeature(const MediaFeature& feature) const {
  const MediaTristate state = values_.Hover();
  if (feature.IsBoolean()) return state == MediaTristate::kPresent;
  const auto& v = feature.LeftValue();
  if (!v.IsIdent()) return false;
  if (v.Text() == "none") return state == MediaTristate::kNone;
  if (v.Text() == "hover") return state == MediaTristate::kPresent;
  return false;
}

bool MediaQueryEvaluator::EvalPointerFeature(
    const MediaFeature& feature) const {
  const MediaTristate state = values_.Pointer();
  if (feature.IsBoolean()) return state == MediaTristate::kPresent;
  const auto& v = feature.LeftValue();
  if (!v.IsIdent()) return false;
  if (v.Text() == "none") return state == MediaTristate::kNone;
  if (v.Text() == "coarse" || v.Text() == "fine") {
    // We don't distinguish pointer precision today; any concrete pointer
    // value matches when pointer is present.
    return state == MediaTristate::kPresent;
  }
  return false;
}

bool MediaQueryEvaluator::EvalColorSchemeFeature(
    const MediaFeature& feature) const {
  if (feature.IsBoolean()) {
    return values_.PreferredColorScheme() !=
           MediaPreferredColorScheme::kNoPreference;
  }
  const auto& v = feature.LeftValue();
  if (!v.IsIdent()) return false;
  const auto want = values_.PreferredColorScheme();
  if (v.Text() == "light") return want == MediaPreferredColorScheme::kLight;
  if (v.Text() == "dark") return want == MediaPreferredColorScheme::kDark;
  return false;
}

bool MediaQueryEvaluator::EvalCustomFeature(const MediaFeature& feature) const {
  // Unknown feature: fall back to string-based matching against the name.
  // Currently no custom features are supported; fail closed.
  return false;
}

}  // namespace css
}  // namespace lynx
