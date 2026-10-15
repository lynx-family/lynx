// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_LEGACY_REGEXP_COMPILER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_LEGACY_REGEXP_COMPILER_H_

struct LEPUSContext;

namespace lynx {
namespace tasm {
// Emit the established regexp bytecode format when compiling template source.
// Current runtimes recompile it from the serialized pattern and flags.
void InstallLegacyRegExpCompiler(LEPUSContext* ctx);
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_LEGACY_REGEXP_COMPILER_H_
