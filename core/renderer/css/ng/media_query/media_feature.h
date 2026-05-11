// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Data objects describing a single "(feature <op> value)" fragment of a
// CSS media query. Only data carriers; evaluation logic lives in
// MediaQueryEvaluator.

#ifndef CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_FEATURE_H_
#define CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_FEATURE_H_

#include <cstdint>
#include <string>
#include <utility>

namespace lynx {

namespace lepus {
class Value;
}  // namespace lepus

using lepus_value = lepus::Value;

namespace css {

// Comparison operator used between the feature name and the feature value.
// Covers the CSS Media Queries Level 4 range syntax.
//
// Values are assigned explicit stable numbers because they are serialized
// into the template binary (via MediaFeature::ToLepus). Existing values
// MUST NOT be renumbered; new operators MUST be appended at the end.
enum class MediaFeatureOperator : uint8_t {
  kNone = 0,  // plain form: (feature) or (feature: value)
  kEq = 1,    // (feature = value)
  kLt = 2,    // (feature < value)
  kLe = 3,    // (feature <= value)
  kGt = 4,    // (feature > value)
  kGe = 5,    // (feature >= value)
};

// Top-level discriminator for a media feature value. The value type is kept
// separate from the concrete unit (carried in MediaFeatureUnit).
//
// Values are serialized into the template binary; existing values MUST NOT
// be renumbered, new values MUST be appended.
enum class MediaFeatureType : uint8_t {
  kInvalid = 0,    // unparsed / unrecognized
  kNumber = 1,     // unit-less: 1, 2.5
  kDimension = 2,  // has a unit (length or resolution, see MediaFeatureUnit)
  kRatio = 3,      // 16/9
  kIdent = 4,      // landscape, dark, hover, ...
  kBoolean = 5,    // `(hover)` with no explicit value
};

// Secondary discriminator carrying the concrete unit for dimension values.
// Length and resolution suffixes share the same enum namespace.
//
// Values are serialized; existing values MUST NOT be renumbered, new values
// MUST be appended at the end.
enum class MediaFeatureUnit : uint8_t {
  kUnknown = 0,

  // Length units
  kPixels = 1,          // 100px
  kEm = 2,              // 1.25em
  kRem = 3,             // 1.5rem
  kPercent = 4,         // 50%
  kViewportWidth = 5,   // 50vw
  kViewportHeight = 6,  // 50vh
  kViewportMin = 7,     // 50vmin
  kViewportMax = 8,     // 50vmax

  // Resolution units
  kDppx = 9,   // 2dppx / 2x
  kDpi = 10,   // 96dpi
  kDpcm = 11,  // 38dpcm
};

// Well-known media feature identifiers. Each standard feature name (including
// its min-/max- prefixed variants) is given a stable enum value so that the
// evaluator can use a fast switch instead of string comparison.
//
// Values are serialized into the template binary; existing values MUST NOT be
// renumbered, new entries MUST be appended before the end.
enum class MediaFeatureId : uint8_t {
  kUnknown = 0,

  kWidth = 1,
  kMinWidth = 2,
  kMaxWidth = 3,

  kHeight = 4,
  kMinHeight = 5,
  kMaxHeight = 6,

  kDeviceWidth = 7,
  kMinDeviceWidth = 8,
  kMaxDeviceWidth = 9,

  kDeviceHeight = 10,
  kMinDeviceHeight = 11,
  kMaxDeviceHeight = 12,

  kOrientation = 13,

  kResolution = 14,
  kMinResolution = 15,
  kMaxResolution = 16,

  kAspectRatio = 17,
  kMinAspectRatio = 18,
  kMaxAspectRatio = 19,

  kDeviceAspectRatio = 20,
  kMinDeviceAspectRatio = 21,
  kMaxDeviceAspectRatio = 22,

  kHover = 23,
  kPointer = 24,
  kPrefersColorScheme = 25,

  kColor = 26,
  kMinColor = 27,
  kMaxColor = 28,

  kDevicePixelRatio = 29,
  kMinDevicePixelRatio = 30,
  kMaxDevicePixelRatio = 31,
};

// Map a lowercased feature name to its enum id. Returns kUnknown for
// unrecognized names. For unknown features the evaluator falls back to
// matching by the Name() string.
MediaFeatureId ResolveMediaFeatureId(const std::string& name);

// Returns true if `unit` names a length suffix (px, em, %, vw, ...).
inline bool IsLengthUnit(MediaFeatureUnit unit) {
  return unit >= MediaFeatureUnit::kPixels &&
         unit <= MediaFeatureUnit::kViewportMax;
}

// Returns true if `unit` names a resolution suffix (dppx, dpi, dpcm).
inline bool IsResolutionUnit(MediaFeatureUnit unit) {
  return unit >= MediaFeatureUnit::kDppx && unit <= MediaFeatureUnit::kDpcm;
}

// Value carrier for a media feature. A top-level type tag disambiguates the
// payload, and a secondary `MediaFeatureUnit` identifies the concrete unit
// for `kDimension` values.
class MediaFeatureValue {
 public:
  MediaFeatureValue() = default;

  static MediaFeatureValue Number(double value) {
    MediaFeatureValue v;
    v.type_ = MediaFeatureType::kNumber;
    v.numeric_ = value;
    return v;
  }

  static MediaFeatureValue Dimension(double value, MediaFeatureUnit unit) {
    MediaFeatureValue v;
    v.type_ = MediaFeatureType::kDimension;
    v.unit_ = unit;
    v.numeric_ = value;
    return v;
  }

  static MediaFeatureValue Boolean() {
    MediaFeatureValue v;
    v.type_ = MediaFeatureType::kBoolean;
    return v;
  }

  static MediaFeatureValue Ratio(double numerator, double denominator) {
    MediaFeatureValue v;
    v.type_ = MediaFeatureType::kRatio;
    v.numeric_ = numerator;
    v.denominator_ = denominator;
    return v;
  }

  static MediaFeatureValue Ident(std::string name) {
    MediaFeatureValue v;
    v.type_ = MediaFeatureType::kIdent;
    v.text_ = std::move(name);
    return v;
  }

  // Placeholder for values that parsed syntactically but whose unit is not
  // understood by this evaluator. Keep the raw text around for diagnostics.
  static MediaFeatureValue Invalid(std::string text) {
    MediaFeatureValue v;
    v.type_ = MediaFeatureType::kInvalid;
    v.text_ = std::move(text);
    return v;
  }

  MediaFeatureType Type() const { return type_; }
  MediaFeatureUnit Unit() const { return unit_; }
  double Numeric() const { return numeric_; }
  double Denominator() const { return denominator_; }
  const std::string& Text() const { return text_; }

  bool IsValid() const { return type_ != MediaFeatureType::kInvalid; }
  bool IsDimension() const { return type_ == MediaFeatureType::kDimension; }
  // Length / resolution are both serialized as `kDimension`; the concrete
  // family is recovered from the unit suffix.
  bool IsLength() const { return IsDimension() && IsLengthUnit(unit_); }
  bool IsResolution() const { return IsDimension() && IsResolutionUnit(unit_); }
  bool IsRatio() const { return type_ == MediaFeatureType::kRatio; }
  bool IsIdent() const { return type_ == MediaFeatureType::kIdent; }
  bool IsNumber() const { return type_ == MediaFeatureType::kNumber; }

  // Serialization helpers for the template binary. Shape:
  //   [ type(u8) << 16 | unit(u8),
  //     numeric(double), denominator(double), text(string) ]
  // Fields are stored in a fixed-size array; extending the value in the
  // future means appending a new slot so that older decoders still read the
  // first five correctly.
  lepus_value ToLepus() const;
  static MediaFeatureValue FromLepus(const lepus_value& value);

 private:
  MediaFeatureType type_ = MediaFeatureType::kInvalid;
  MediaFeatureUnit unit_ = MediaFeatureUnit::kUnknown;
  double numeric_ = 0.0;
  double denominator_ = 1.0;
  std::string text_;
};

// A single media feature assertion. Media Queries Level 4 also allows the
// "range" form `(100px < width < 500px)`; we model this with an optional
// `right_` bound instead of splitting the node into two comparisons.
class MediaFeature {
 public:
  MediaFeature() = default;

  MediaFeature(MediaFeatureId id, std::string name, MediaFeatureOperator op,
               MediaFeatureValue value)
      : id_(id),
        name_(std::move(name)),
        left_op_(op),
        left_value_(std::move(value)) {}

  MediaFeatureId Id() const { return id_; }
  const std::string& Name() const { return name_; }

  MediaFeatureOperator LeftOperator() const { return left_op_; }
  const MediaFeatureValue& LeftValue() const { return left_value_; }

  MediaFeatureOperator RightOperator() const { return right_op_; }
  const MediaFeatureValue& RightValue() const { return right_value_; }

  bool HasRightBound() const {
    return right_op_ != MediaFeatureOperator::kNone;
  }

  // For "A op1 name op2 B" style range queries (Level 4).
  void SetRightBound(MediaFeatureOperator op, MediaFeatureValue value) {
    right_op_ = op;
    right_value_ = std::move(value);
  }

  // A feature is "boolean" when it is written as `(hover)` with no value.
  bool IsBoolean() const {
    return left_op_ == MediaFeatureOperator::kNone &&
           left_value_.Type() == MediaFeatureType::kBoolean;
  }

  // Serialization helpers for the template binary. The lepus_value shape is:
  //   [ feature_id(u8), name(string),
  //     left_op(u8), left_value(array),
  //     has_right(bool),
  //     right_op(u8)|false, right_value(array)|absent ]
  // When `has_right == false` the right-hand slots are simply not present;
  // decoders must tolerate both forms.
  lepus_value ToLepus() const;
  static MediaFeature FromLepus(const lepus_value& value);

 private:
  MediaFeatureId id_ = MediaFeatureId::kUnknown;
  std::string name_;
  MediaFeatureOperator left_op_ = MediaFeatureOperator::kNone;
  MediaFeatureValue left_value_;
  MediaFeatureOperator right_op_ = MediaFeatureOperator::kNone;
  MediaFeatureValue right_value_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_FEATURE_H_
