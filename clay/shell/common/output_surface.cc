// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/output_surface.h"

#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/gl_context_switch.h"

namespace clay {

OutputSurface::~OutputSurface() = default;

void OutputSurface::CreateMainGrContext() {}

clay::GrContextPtr OutputSurface::GetMainGrContext() { return nullptr; }

std::unique_ptr<GLContextResult> OutputSurface::MakeCurrent() {
  return std::make_unique<GLContextDefaultResult>(true);
}

#ifdef ENABLE_SKITY
void OutputSurface::PrecompileDefaultSkityShaders() {
  auto config = GetSkityPrecompileConfig();
  if (!config) {
    TRACE_EVENT("clay", "PrecompileDefaultSkityShaders config is null");
    return;
  }

  clay::GrContextPtr main_context = GetMainGrContext();
  if (!main_context) {
    TRACE_EVENT("clay", "PrecompileDefaultSkityShaders main_context is null");
    return;
  }

  auto context_switch = MakeCurrent();
  if (!context_switch || !context_switch->GetResult()) {
    TRACE_EVENT("clay", "PrecompileDefaultSkityShaders make current failed");
    return;
  }

  TRACE_EVENT("clay", "PrecompileDefaultSkityShaders");
  auto precompile_context = main_context->CreatePrecompileContext(
      config->color_type, config->enable_msaa);
  if (precompile_context) {
    precompile_context->PrecompileDefaultShaders();
  }
}

std::optional<OutputSurface::SkityPrecompileConfig>
OutputSurface::GetSkityPrecompileConfig() const {
  return std::nullopt;
}
#endif  // ENABLE_SKITY

}  // namespace clay
