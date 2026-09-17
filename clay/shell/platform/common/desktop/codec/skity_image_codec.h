// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_SKITY_IMAGE_CODEC_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_SKITY_IMAGE_CODEC_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/gfx/geometry/size.h"
#include "clay/gfx/image/codec.h"
#include "skity/io/data.hpp"

namespace clay {

class SkityImageCodec final : public Codec {
 public:
  static fml::RefPtr<Codec> Create(std::shared_ptr<skity::Data> encoded_data,
                                   const Size& decode_size);

  ~SkityImageCodec() override;

  int FrameCount() const override;
  void NextFrame(const CodecCallback& callback) override;
  int FrameDuration(int index) const override;

 private:
  class Impl;

  explicit SkityImageCodec(std::unique_ptr<Impl> impl);

  std::unique_ptr<Impl> impl_;

  FML_FRIEND_MAKE_REF_COUNTED(SkityImageCodec);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(SkityImageCodec);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_SKITY_IMAGE_CODEC_H_
