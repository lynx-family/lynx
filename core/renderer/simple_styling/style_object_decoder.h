// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIMPLE_STYLING_STYLE_OBJECT_DECODER_H_
#define CORE_RENDERER_SIMPLE_STYLING_STYLE_OBJECT_DECODER_H_

#include <memory>

#include "core/renderer/css/css_property.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx::style {

class StyleObjectDecoder {
 public:
  virtual ~StyleObjectDecoder() = default;
  virtual bool DecodeStyleObject(tasm::StyleMap& attr,
                                 const tasm::CSSRange& range) = 0;
};

// Define a function pointer type for creating a StyleObjectDecoder instance.
// This type represents a function that takes a pointer to an array of uint8_t
// data and the length of that data as parameters, and returns a unique pointer
// to a StyleObjectDecoder.
// @param data: Pointer to the binary data used for initializing the decoder.
// @param length: The length of the binary data.
// @return: A unique pointer to a StyleObjectDecoder instance.
typedef std::unique_ptr<StyleObjectDecoder> (*DecoderCreatorFunc)(
    uint8_t* data, size_t length);

}  // namespace lynx::style

#endif  // CORE_RENDERER_SIMPLE_STYLING_STYLE_OBJECT_DECODER_H_
