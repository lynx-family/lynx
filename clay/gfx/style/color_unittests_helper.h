// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_COLOR_UNITTESTS_HELPER_H_
#define CLAY_GFX_STYLE_COLOR_UNITTESTS_HELPER_H_

#define EXPECT_EQ_RGBO(__color__, __r__, __g__, __b__, __o__) \
  EXPECT_EQ(__color__.Red(), __r__);                          \
  EXPECT_EQ(__color__.Green(), __g__);                        \
  EXPECT_EQ(__color__.Blue(), __b__);                         \
  EXPECT_DOUBLE_EQ(__color__.Opacity(), __o__)

#define EXPECT_EQ_RGB(__color__, __r__, __g__, __b__) \
  EXPECT_EQ(__color__.Red(), __r__);                  \
  EXPECT_EQ(__color__.Green(), __g__);                \
  EXPECT_EQ(__color__.Blue(), __b__)

#define EXPECT_EQ_RGBA(__color__, __r__, __g__, __b__, __a__) \
  EXPECT_EQ(__color__.Red(), __r__);                          \
  EXPECT_EQ(__color__.Green(), __g__);                        \
  EXPECT_EQ(__color__.Blue(), __b__);                         \
  EXPECT_DOUBLE_EQ(__color__.Alpha(), __a__)

#endif  // CLAY_GFX_STYLE_COLOR_UNITTESTS_HELPER_H_
