// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Node tree that models a CSS media condition expression (feature / nested /
// not / and / or). Each node type is ref-counted and immutable so that a
// parsed MediaQuery can be shared between the decoder, rule set, and
// evaluator.

#ifndef CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_EXP_H_
#define CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_EXP_H_

#include <cstdint>
#include <string>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/media_query/media_feature.h"

namespace lynx {
namespace css {

class MediaQueryExpNode : public fml::RefCountedThreadSafeStorage {
 public:
  // Values are assigned explicit stable numbers because they are serialized
  // into the template binary (via ToLepus). Existing values MUST NOT be
  // renumbered; new node kinds MUST be appended at the end.
  enum class Type : uint8_t {
    kFeature = 0,
    kNested = 1,
    kNot = 2,
    kAnd = 3,
    kOr = 4,
  };

  explicit MediaQueryExpNode(Type type) : type_(type) {}

  void ReleaseSelf() const override { delete this; }

  Type GetType() const { return type_; }

  // Produce a canonical textual form. Useful for logging and for the
  // existing ConditionRule::Condition() surface which consumers already
  // reach via a string. Implemented per subclass.
  virtual std::string Serialize() const = 0;

  // Serialize the subtree rooted at this node to a lepus_value. The shape
  // is `[ type(u8), payload ]`; payload differs per node kind (see .cc).
  // `FromLepus` is a polymorphic factory that dispatches on the `type` slot
  // and rebuilds the matching concrete subclass; returns null on invalid
  // input so callers can treat it as an empty set ("not all").
  virtual lepus_value ToLepus() const = 0;
  static fml::RefPtr<const MediaQueryExpNode> FromLepus(
      const lepus_value& value);

 protected:
  ~MediaQueryExpNode() override = default;

 private:
  Type type_;
};

// Leaf: a single `(feature <op> value)` assertion.
class MediaQueryFeatureExpNode final : public MediaQueryExpNode {
 public:
  explicit MediaQueryFeatureExpNode(MediaFeature feature)
      : MediaQueryExpNode(Type::kFeature), feature_(std::move(feature)) {}

  const MediaFeature& Feature() const { return feature_; }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  MediaFeature feature_;
};

// Parenthesized grouping node: `( inner )`.
class MediaQueryNestedExpNode final : public MediaQueryExpNode {
 public:
  explicit MediaQueryNestedExpNode(fml::RefPtr<const MediaQueryExpNode> inner)
      : MediaQueryExpNode(Type::kNested), inner_(std::move(inner)) {}

  const fml::RefPtr<const MediaQueryExpNode>& Inner() const { return inner_; }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  fml::RefPtr<const MediaQueryExpNode> inner_;
};

// `not ( operand )`.
class MediaQueryNotExpNode final : public MediaQueryExpNode {
 public:
  explicit MediaQueryNotExpNode(fml::RefPtr<const MediaQueryExpNode> operand)
      : MediaQueryExpNode(Type::kNot), operand_(std::move(operand)) {}

  const fml::RefPtr<const MediaQueryExpNode>& Operand() const {
    return operand_;
  }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  fml::RefPtr<const MediaQueryExpNode> operand_;
};

// Binary combinator base for `and` / `or`. Kept as a concrete shared class
// because the only thing that differs between them is the connective token
// and the `Type` tag.
class MediaQueryCompoundExpNode : public MediaQueryExpNode {
 public:
  MediaQueryCompoundExpNode(Type type,
                            fml::RefPtr<const MediaQueryExpNode> left,
                            fml::RefPtr<const MediaQueryExpNode> right)
      : MediaQueryExpNode(type),
        left_(std::move(left)),
        right_(std::move(right)) {}

  const fml::RefPtr<const MediaQueryExpNode>& Left() const { return left_; }
  const fml::RefPtr<const MediaQueryExpNode>& Right() const { return right_; }

 protected:
  ~MediaQueryCompoundExpNode() override = default;

 private:
  fml::RefPtr<const MediaQueryExpNode> left_;
  fml::RefPtr<const MediaQueryExpNode> right_;
};

class MediaQueryAndExpNode final : public MediaQueryCompoundExpNode {
 public:
  MediaQueryAndExpNode(fml::RefPtr<const MediaQueryExpNode> left,
                       fml::RefPtr<const MediaQueryExpNode> right)
      : MediaQueryCompoundExpNode(Type::kAnd, std::move(left),
                                  std::move(right)) {}

  std::string Serialize() const override;
  lepus_value ToLepus() const override;
};

class MediaQueryOrExpNode final : public MediaQueryCompoundExpNode {
 public:
  MediaQueryOrExpNode(fml::RefPtr<const MediaQueryExpNode> left,
                      fml::RefPtr<const MediaQueryExpNode> right)
      : MediaQueryCompoundExpNode(Type::kOr, std::move(left),
                                  std::move(right)) {}

  std::string Serialize() const override;
  lepus_value ToLepus() const override;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_MEDIA_QUERY_MEDIA_QUERY_EXP_H_
