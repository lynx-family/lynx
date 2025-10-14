// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skity/jpeg_codec_skity.h"

#include "third_party/libjpeg-turbo/turbojpeg.h"

namespace clay {

namespace {
struct TJHandlerWrapper {
  explicit TJHandlerWrapper(tjhandle h) : handle(h) {}

  ~TJHandlerWrapper() {
    if (this->handle) {
      tjDestroy(this->handle);
    }
  }

  tjhandle handle = nullptr;
};
}  // namespace

std::shared_ptr<skity::Data> JPEGCodecSkity::Encode(
    const skity::Pixmap* pixmap) {
  TJHandlerWrapper hw{tjInitCompress()};

  uint8_t* buf = nullptr;
  unsigned long size = 0;  // NOLINT

  int32_t ret =
      tjCompress2(hw.handle, static_cast<const uint8_t*>(pixmap->Addr()),
                  pixmap->Width(), pixmap->Width() * 4, pixmap->Height(),
                  TJPF_RGBA, &buf, &size, TJSAMP_444, 100, 0);

  if (ret != 0) {
    return nullptr;
  }

  return skity::Data::MakeWithCopy(buf, size);
}

}  // namespace clay
