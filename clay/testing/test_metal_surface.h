// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_TEST_METAL_SURFACE_H_
#define TESTING_TEST_METAL_SURFACE_H_

#include "base/include/fml/macros.h"
#include "clay/testing/test_metal_context.h"
#include "third_party/skia/include/core/SkSize.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

namespace clay {

//------------------------------------------------------------------------------
/// @brief      Creates a MTLTexture backed SkSurface and context that can be
///             used to render to in unit-tests.
///
class TestMetalSurface {
 public:
  static bool PlatformSupportsMetal();

  static std::unique_ptr<TestMetalSurface> Create(
      const TestMetalContext& test_metal_context,
      SkISize surface_size = SkISize::MakeEmpty());

  static std::unique_ptr<TestMetalSurface> Create(
      const TestMetalContext& test_metal_context, int64_t texture_id,
      SkISize surface_size = SkISize::MakeEmpty());

  virtual ~TestMetalSurface();

  virtual bool IsValid() const;

  virtual sk_sp<GrDirectContext> GetGrContext() const;

  virtual sk_sp<SkSurface> GetSurface() const;

  virtual sk_sp<SkImage> GetRasterSurfaceSnapshot();

  virtual TestMetalContext::TextureInfo GetTextureInfo();

 protected:
  TestMetalSurface();

 private:
  std::unique_ptr<TestMetalSurface> impl_;

  explicit TestMetalSurface(std::unique_ptr<TestMetalSurface> impl);

  BASE_DISALLOW_COPY_AND_ASSIGN(TestMetalSurface);
};

}  // namespace clay

#endif  // TESTING_TEST_METAL_SURFACE_H_
