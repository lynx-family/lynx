// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_MTS_RENDERER_H_
#define CORE_RENDERER_MTS_RENDERER_H_

#include <memory>

#include "core/public/pipeline_option.h"
#include "core/renderer/data/template_data.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class TemplateEntry;
struct UpdatePageOption;

// Defines the rendering behaviors owned by an MTS rendering backend. The
// concrete renderer is finalized after page config is decoded, while
// TemplateAssembler remains responsible for the common load/update/reload
// lifecycle.
class MTSRenderer {
 public:
  explicit MTSRenderer(TemplateAssembler& tasm) : tasm_(tasm) {}
  virtual ~MTSRenderer() = default;

  virtual TemplateData ProcessData(
      const std::shared_ptr<TemplateData>& template_data,
      bool is_first_screen) = 0;

  virtual void Load(const std::shared_ptr<TemplateEntry>& entry,
                    const TemplateData& data,
                    std::shared_ptr<PipelineOptions>& pipeline_options) = 0;

  // Either data or global_props may be null when only one part of metadata is
  // updated. Both are non-null for a backend-supported combined update.
  virtual void UpdateMetaData(
      const TemplateData* data, const lepus::Value* global_props,
      bool global_props_need_render, const UpdatePageOption& update_page_option,
      std::shared_ptr<PipelineOptions>& pipeline_options) = 0;

  // Returns whether data and global props can be committed through one MTS
  // render invocation in the current runtime.
  virtual bool CanUpdateMetaDataAtomically() = 0;

  virtual void Reset(const std::shared_ptr<TemplateEntry>& entry) = 0;

 protected:
  TemplateAssembler& tasm_;
};

std::unique_ptr<MTSRenderer> CreateMTSRenderer(TemplateAssembler& tasm,
                                               bool enable_fiber_arch);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_MTS_RENDERER_H_
