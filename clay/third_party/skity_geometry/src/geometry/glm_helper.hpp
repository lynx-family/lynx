// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_GEOMETRY_GLM_HELPER_HPP
#define SRC_GEOMETRY_GLM_HELPER_HPP

#include <glm/glm.hpp>
#include <skity/geometry/matrix.hpp>
#include <skity/geometry/vector.hpp>

namespace skity {
template <typename O, typename I>
inline const O& To(const I& input) {
  static_assert(sizeof(I) == sizeof(O),
                "Input and output types must have the same size.");
  return reinterpret_cast<const O&>(input);
}

template <typename O, typename I>
inline O& To(I& input) {
  static_assert(sizeof(I) == sizeof(O),
                "Input and output types must have the same size.");
  return reinterpret_cast<O&>(input);
}

inline const glm::vec2& ToGLM(const Vec2& v) { return To<glm::vec2>(v); }

inline const glm::vec3& ToGLM(const Vec3& v) { return To<glm::vec3>(v); }

inline const glm::vec4& ToGLM(const Vec4& v) { return To<glm::vec4>(v); }

inline const glm::mat4& ToGLM(const Matrix& m) { return To<glm::mat4>(m); }

inline glm::mat4& ToGLM(Matrix& m) { return To<glm::mat4>(m); }

inline const Matrix& FromGLM(const glm::mat4& m) { return To<Matrix>(m); }

}  // namespace skity

#endif  // SRC_GEOMETRY_GLM_HELPER_HPP
