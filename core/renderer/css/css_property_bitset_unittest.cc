// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_property_bitset.h"

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(CSSIDBitsetTest, SetAndHas) {
  CSSIDBitset bits;
  EXPECT_FALSE(bits.Has(kPropertyIDTop));
  bits.Set(kPropertyIDTop);
  EXPECT_TRUE(bits.Has(kPropertyIDTop));
}

TEST(CSSIDBitsetTest, HasAny) {
  CSSIDBitset bits;
  EXPECT_FALSE(bits.HasAny());
  bits.Set(kPropertyIDTop);
  EXPECT_TRUE(bits.HasAny());
}

TEST(CSSIDBitsetTest, Reset) {
  CSSIDBitset bits;
  EXPECT_FALSE(bits.HasAny());
  bits.Set(kPropertyIDTop);
  EXPECT_TRUE(bits.HasAny());
  EXPECT_TRUE(bits.Has(kPropertyIDTop));
  bits.Reset();
  EXPECT_FALSE(bits.HasAny());
  EXPECT_FALSE(bits.Has(kPropertyIDTop));
}

TEST(CSSIDBitsetTest, ResetById) {
  CSSIDBitset bits;
  bits.Set(kPropertyIDTop);
  bits.Set(kPropertyIDLeft);
  EXPECT_TRUE(bits.Has(kPropertyIDTop));
  EXPECT_TRUE(bits.Has(kPropertyIDLeft));

  bits.Reset(kPropertyIDTop);
  EXPECT_FALSE(bits.Has(kPropertyIDTop));
  EXPECT_TRUE(bits.Has(kPropertyIDLeft));
}

TEST(CSSIDBitsetTest, Iterator) {
  CSSIDBitset bits;
  bits.Set(kPropertyIDTop);
  bits.Set(kPropertyIDLeft);
  bits.Set(kPropertyIDFontSize);
  bits.Set(kPropertyIDColor);

  std::set<CSSPropertyID> expected_ids = {
      kPropertyIDTop, kPropertyIDLeft, kPropertyIDFontSize, kPropertyIDColor};

  for (CSSPropertyID id : bits) {
    EXPECT_EQ(expected_ids.erase(id), 1);
  }
  EXPECT_TRUE(expected_ids.empty());
}

TEST(CSSIDBitsetTest, BitwiseOperators) {
  CSSIDBitset bits1;
  bits1.Set(kPropertyIDTop);
  bits1.Set(kPropertyIDLeft);
  bits1.Set(kPropertyIDColor);

  CSSIDBitset bits2;
  bits2.Set(kPropertyIDLeft);
  bits2.Set(kPropertyIDFontSize);
  bits2.Set(kPropertyIDColor);

  // Test OR (|)
  CSSIDBitset or_result = bits1 | bits2;
  EXPECT_TRUE(or_result.Has(kPropertyIDTop));
  EXPECT_TRUE(or_result.Has(kPropertyIDLeft));
  EXPECT_TRUE(or_result.Has(kPropertyIDColor));
  EXPECT_TRUE(or_result.Has(kPropertyIDFontSize));
  EXPECT_FALSE(or_result.Has(kPropertyIDWidth));

  // Test AND (&)
  CSSIDBitset and_result = bits1 & bits2;
  EXPECT_FALSE(and_result.Has(kPropertyIDTop));
  EXPECT_TRUE(and_result.Has(kPropertyIDLeft));
  EXPECT_TRUE(and_result.Has(kPropertyIDColor));
  EXPECT_FALSE(and_result.Has(kPropertyIDFontSize));

  // Test XOR (^)
  CSSIDBitset xor_result = bits1 ^ bits2;
  EXPECT_TRUE(xor_result.Has(kPropertyIDTop));
  EXPECT_FALSE(xor_result.Has(kPropertyIDLeft));
  EXPECT_FALSE(xor_result.Has(kPropertyIDColor));
  EXPECT_TRUE(xor_result.Has(kPropertyIDFontSize));

  // Test assignment operators
  CSSIDBitset bits_copy = bits1;
  bits_copy |= bits2;
  EXPECT_EQ(bits_copy, or_result);

  bits_copy = bits1;
  bits_copy &= bits2;
  EXPECT_EQ(bits_copy, and_result);

  bits_copy = bits1;
  bits_copy ^= bits2;
  EXPECT_EQ(bits_copy, xor_result);
}

TEST(CSSIDBitsetTest, ConstexprConstructor) {
  constexpr CSSPropertyID initial_properties[] = {
      kPropertyIDTop, kPropertyIDLeft, kPropertyIDFontSize, kPropertyIDColor};

  constexpr CSSIDBitset bits(initial_properties);

  EXPECT_TRUE(bits.Has(kPropertyIDTop));
  EXPECT_TRUE(bits.Has(kPropertyIDLeft));
  EXPECT_TRUE(bits.Has(kPropertyIDFontSize));
  EXPECT_TRUE(bits.Has(kPropertyIDColor));
  EXPECT_FALSE(bits.Has(kPropertyIDWidth));
  EXPECT_FALSE(bits.Has(kPropertyIDHeight));

  // Verify with iterator
  std::set<CSSPropertyID> expected_ids(std::begin(initial_properties),
                                       std::end(initial_properties));
  std::set<CSSPropertyID> actual_ids;
  for (CSSPropertyID id : bits) {
    actual_ids.insert(id);
  }

  EXPECT_EQ(expected_ids, actual_ids);
}

TEST(CSSIDBitsetTest, FromKeys) {
  StyleMap map;
  map.insert_or_assign(kPropertyIDTop, CSSValue());
  map.insert_or_assign(kPropertyIDLeft, CSSValue());
  map.insert_or_assign(kPropertyIDFontSize, CSSValue());

  CSSIDBitset bits = CSSIDBitset::FromKeys(map);

  EXPECT_TRUE(bits.Has(kPropertyIDTop));
  EXPECT_TRUE(bits.Has(kPropertyIDLeft));
  EXPECT_TRUE(bits.Has(kPropertyIDFontSize));
  EXPECT_FALSE(bits.Has(kPropertyIDWidth));
  EXPECT_FALSE(bits.Has(kPropertyIDColor));
}

TEST(CSSIDBitsetTest, FromKeysEmpty) {
  StyleMap map;
  CSSIDBitset bits = CSSIDBitset::FromKeys(map);
  EXPECT_FALSE(bits.HasAny());
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
