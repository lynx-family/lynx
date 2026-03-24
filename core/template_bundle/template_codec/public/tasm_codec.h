// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//! @file tasm_codec.h
//! @brief TASM codec C++ API - Internal interface for C++ callers.
//!
//! This header exposes a C++ API for internal use within the codebase.
//! External users should include tasm_codec_capi.h instead.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/template_bundle/template_codec/public/tasm_codec_types.h"

namespace lynx {
namespace tasm {
namespace codec {

using EncodeResult = lynx::tasm::EncodeResult;
using DecodeResult = lynx::tasm::DecodeResult;

//! Encode template bundle from options JSON.
//!
//! @param options_json JSON string containing encode options.
//! @return EncodeResult with encoded data.
EncodeResult Encode(const std::string& options_json);

//! Decode template bundle from binary data.
//!
//! @param data Pointer to binary data.
//! @param len Length of binary data in bytes.
//! @return DecodeResult with decoded result.
DecodeResult Decode(const uint8_t* data, size_t len);

}  // namespace codec
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_H_
