// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transforms/quaternion.h"

#include <cmath>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace transforms {
namespace testing {

namespace {

const double kEpsilon = 1e-5;

#define EXPECT_QUATERNION(expected, actual)          \
  do {                                               \
    EXPECT_NEAR(expected.x(), actual.x(), kEpsilon); \
    EXPECT_NEAR(expected.y(), actual.y(), kEpsilon); \
    EXPECT_NEAR(expected.z(), actual.z(), kEpsilon); \
    EXPECT_NEAR(expected.w(), actual.w(), kEpsilon); \
  } while (false)

#define EXPECT_EULER(expected, actual)           \
  do {                                           \
    EXPECT_NEAR(expected.x, actual.x, kEpsilon); \
    EXPECT_NEAR(expected.y, actual.y, kEpsilon); \
    EXPECT_NEAR(expected.z, actual.z, kEpsilon); \
  } while (false)

void CompareQuaternions(const Quaternion& a, const Quaternion& b) {
  EXPECT_FLOAT_EQ(a.x(), b.x());
  EXPECT_FLOAT_EQ(a.y(), b.y());
  EXPECT_FLOAT_EQ(a.z(), b.z());
  EXPECT_FLOAT_EQ(a.w(), b.w());
}

}  // namespace

TEST(QuatTest, DefaultConstruction) {
  CompareQuaternions(Quaternion(0, 0, 0, 1), Quaternion());
}

TEST(QuatTest, Addition) {
  double values[] = {0, 1, 100};
  for (size_t i = 0; i < std::size(values); ++i) {
    float t = values[i];
    Quaternion a(t, 2 * t, 3 * t, 4 * t);
    Quaternion b(5 * t, 4 * t, 3 * t, 2 * t);
    Quaternion sum = a + b;
    CompareQuaternions(6.0 * Quaternion(t, t, t, t), sum);
  }
}

TEST(QuatTest, Scaling) {
  double values[] = {0, 10, 100};
  for (size_t i = 0; i < std::size(values); ++i) {
    double s = values[i];
    Quaternion q(1, 2, 3, 4);
    Quaternion expected(s, 2 * s, 3 * s, 4 * s);
    CompareQuaternions(expected, s * q);
    if (s > 0) CompareQuaternions(expected, s * q);
  }
}

TEST(QuatTest, Slerp) {
  Quaternion start(-0.1428387, -0.1428387, -0.1428387, 0.9689124);
  Quaternion stop(0.1428387, 0.1428387, 0.1428387, 0.9689124);
  Quaternion expected0_5(0, 0, 0, 1);
  Quaternion interpolated0_5 = start.Slerp(stop, 0.5);
  EXPECT_QUATERNION(expected0_5, interpolated0_5);

  Quaternion expected0_8(0.0862781, 0.0862781, 0.0862781, 0.9887711);
  Quaternion interpolated0_8 = start.Slerp(stop, 0.8);
  EXPECT_QUATERNION(expected0_8, interpolated0_8);
}

TEST(QuatTest, SlerpOppositeAngles) {
  //  Vector3dF axis(1, 1, 1);
  //  double start_radians = -base::kPiDouble / 2;
  //  double stop_radians = base::kPiDouble / 2;
  Quaternion start(-0.4082483, -0.4082483, -0.4082483, 0.7071068);
  Quaternion stop(0.4082483, 0.4082483, 0.4082483, 0.7071068);

  // When quaternions are pointed in the fully opposite direction, this is
  // ambiguous, so we rotate as per https://www.w3.org/TR/css-transforms-1/
  Quaternion expected(0, 0, 0, 1);

  Quaternion interpolated = start.Slerp(stop, 0.5f);
  EXPECT_QUATERNION(expected, interpolated);
}

TEST(QuatTest, SlerpRotateXRotateY) {
  //  Quaternion start(Vector3dF(1, 0, 0), base::kPiDouble / 2);
  //  Quaternion stop(Vector3dF(0, 1, 0), base::kPiDouble / 2);
  Quaternion start(0.7071068, 0, 0, 0.7071068);
  Quaternion stop(0, 0.7071068, 0, 0.7071068);
  Quaternion interpolated = start.Slerp(stop, 0.5f);

  double expected_angle = std::acos(1.0 / 3.0);
  double xy = std::sin(0.5 * expected_angle) / std::sqrt(2);
  Quaternion expected(xy, xy, 0, std::cos(0.5 * expected_angle));
  EXPECT_QUATERNION(expected, interpolated);
}

TEST(QuatTest, Slerp360) {
  Quaternion start(0, 0, 0, -1);  // 360 degree rotation.
  //  Quaternion stop(Vector3dF(0, 0, 1), base::kPiDouble / 2);
  Quaternion stop(0, 0, 0.7071068, 0.7071068);
  Quaternion interpolated = start.Slerp(stop, 0.5f);
  double expected_half_angle = M_PI / 8;
  Quaternion expected(0, 0, std::sin(expected_half_angle),
                      std::cos(expected_half_angle));
  EXPECT_QUATERNION(expected, interpolated);
}

TEST(QuatTest, SlerpObtuseAngle) {
  //  Quaternion start(Vector3dF(1, 1, 0), base::kPiDouble / 2);
  Quaternion start(0.5, 0.5, 0, 0.7071068);
  //  Quaternion stop(Vector3dF(0, 1, -1), 3 * base::kPiDouble / 2);
  Quaternion stop(0, 0.5, -0.5, -0.7071068);
  Quaternion interpolated = start.Slerp(stop, 0.5f);
  double expected_half_angle = -std::atan(0.5);
  double xz = std::sin(expected_half_angle) / std::sqrt(2);
  Quaternion expected(xz, 0, xz, -std::cos(expected_half_angle));
  EXPECT_QUATERNION(expected, interpolated);
}

TEST(QuatTest, ConvertToEuler) {  // Z-Y-X Euler angles
  //  Quaternion start(Vector3dF(1, 1, 0), base::kPiDouble / 2);
  Quaternion q0(0.5, 0.5, 0, 0.7071068);
  Euler expected0(1.5707963, 0.7853981, 0.7853982);
  EXPECT_EULER(expected0, q0.ConvertToEuler());

  //  Quaternion stop(Vector3dF(0, 1, -1), 3 * base::kPiDouble / 2);
  Quaternion q1(0, 0.5, -0.5, -0.7071068);
  Euler expected1(-0.7853982, -0.7853982, 1.5707963);
  EXPECT_EULER(expected1, q1.ConvertToEuler());

  // Quaternion start(Vector3dF(1, 0, 0), base::kPiDouble / 3);
  Quaternion q2(0.5, 0, 0, 0.8660254);
  Euler expected2(1.0471975, 0.0, 0.0);
  EXPECT_EULER(expected2, q2.ConvertToEuler());
}

}  // namespace testing
}  // namespace transforms
}  // namespace lynx
