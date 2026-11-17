// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "clay/lynx_adaptor/painting_context_clay.h"
#include "clay/ui/component/page_view.h"
#include "core/public/page_options.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"
#include "gfx/public/path_impl_factory.h"
#include "platform/lynx_clay/platform_renderer_clay.h"
#include "platform/lynx_clay/ui_delegate_lynx_clay.h"

namespace lynx::tasm {
namespace {

std::array<float, 4> CopyEdges(const float* edges) {
  return edges ? std::array{edges[0], edges[1], edges[2], edges[3]}
               : std::array<float, 4>{};
}

struct ClayDisplayListUpdate {
  ClayDisplayListUpdate(int id, DisplayList&& display_list)
      : id(id),
        list(std::move(display_list)),
        recording(RecordClayDisplayList(list)) {}

  int id;
  DisplayList list;
  RecordedClayDisplayList recording;
};

class FragmentPlatformRef final : public NativePaintingCtxPlatformRef {
 public:
  explicit FragmentPlatformRef(clay::ViewContext* view_context)
      : NativePaintingCtxPlatformRef(
            std::make_unique<PlatformRendererClayFactory>(
                [views = view_context->GetWeakPtr()] {
                  auto context = views.lock();
                  if (auto* page = context ? context->GetPageView() : nullptr) {
                    page->RequestRecordedFrame();
                  }
                })),
        view_context_(view_context->GetWeakPtr()),
        timing_ref_(view_context) {}

  void Apply(ClayDisplayListUpdate&& update) {
    if (auto* renderer = FindRenderer(update.id)) {
      renderer->SetPendingRecording(std::move(update.recording));
      UpdateDisplayList(update.id, std::move(update.list));
    }
  }

  std::unique_ptr<clay::PendingContainerLayer> BuildLayers(
      const fml::RefPtr<clay::GPUUnrefQueue>& unref_queue) const {
    const auto* root = FindRenderer(kRootId);
    return root ? root->BuildPendingLayers(unref_queue) : nullptr;
  }

  void FinishLayout() {
    const auto* root = finished_first_layout_ ? nullptr : FindRenderer(kRootId);
    if (!root || root->GetDisplayList().GetContentItemsSize() == 0) {
      return;
    }
    auto context = view_context_.lock();
    if (auto* page = context ? context->GetPageView() : nullptr) {
      finished_first_layout_ = true;
      page->OnFirstMeaningfulLayout();
    }
  }

  void SetPerfController(
      const std::shared_ptr<PerfControllerClay>& controller) {
    timing_ref_.SetPerfController(controller);
  }

  void SetNeedMarkPaintEndTiming(const PipelineID& pipeline_id) override {
    timing_ref_.SetNeedMarkPaintEndTiming(pipeline_id);
  }

 private:
  PlatformRendererClay* FindRenderer(int id) const {
    const auto it = renderers_.find(id);
    return it == renderers_.end()
               ? nullptr
               : static_cast<PlatformRendererClay*>(it->second.get());
  }

  std::weak_ptr<clay::ViewContext> view_context_;
  PaintingContextClayRef timing_ref_;
  bool finished_first_layout_ = false;
};

class PathClay final : public gfx::PathImpl {
 public:
  using gfx::PathImpl::PathImpl;
};

class PathClayFactory final : public gfx::PathImplFactory {
 public:
  std::unique_ptr<gfx::PathImpl> Create(gfx::PathSource source) const override {
    return std::make_unique<PathClay>(std::move(source));
  }
};

class NativePaintingContextClay final : public PaintingContextClay,
                                        public NativePaintingContext {
 public:
  explicit NativePaintingContextClay(clay::ViewContext* view_context);
  ~NativePaintingContextClay() override;

  NativePaintingContext* CastToNativeCtx() override { return this; }
  const gfx::PathImplFactory& GetPathImplFactory() const override {
    return path_factory_;
  }
  void SetPerfController(
      const std::shared_ptr<PerfControllerClay>& controller) override {
    GetNativeRef()->SetPerfController(controller);
  }
  void FinishTasmOperation(const std::shared_ptr<PipelineOptions>&) override {}
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override;
  void CreatePlatformRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& config) override;
  void CreatePlatformExtendedRenderer(
      int id, const base::String& tag, const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& config) override;
  void UpdateLayout(int id, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height, uint32_t node_index,
                    bool display_none) override;
  fml::RefPtr<PaintImage> CreateImage(int, base::String, const ImagePaintInfo&,
                                      float, float, int32_t, bool) override {
    return nullptr;
  }
  LayoutResult MeasureText(Element*, float width, int width_mode, float height,
                           int height_mode) override {
    // Intrinsic text measurement is provided with text recording.
    return {width_mode == SLMeasureModeDefinite ? width : 0.f,
            height_mode == SLMeasureModeDefinite ? height : 0.f};
  }
  void AlignText(Element* element) override {}
  void UpdateTextBundle(int id, intptr_t bundle) override {}
  void DestroyTextBundle(int id) override {}
  void UpdatePlatformEventBundle(int id, PlatformEventBundle bundle) override;

 private:
  std::shared_ptr<FragmentPlatformRef> GetNativeRef() const {
    return std::static_pointer_cast<FragmentPlatformRef>(platform_ref_);
  }

  void EnqueueDisplayList(int id, DisplayList list) override;
  void EnqueueDisplayLists(DisplayListUpdateBatch batch) override;
  void EnqueueReconstructEventTargetTreeRecursively() override {
    Enqueue([ref = GetNativeRef()] {
      ref->ReconstructEventTargetTreeRecursively();
    });
  }

  const PathClayFactory path_factory_;
  const fml::RefPtr<fml::TaskRunner> ui_runner_;
};

NativePaintingContextClay::NativePaintingContextClay(
    clay::ViewContext* view_context)
    : PaintingContextClay(view_context),
      ui_runner_(view_context->GetUITaskRunner()) {
  auto ref = std::make_shared<FragmentPlatformRef>(view_context);
  platform_ref_ = ref;
  if (auto* page = view_context->GetPageView()) {
    page->SetRecordedLayerProvider(
        [weak_ref = std::weak_ptr<FragmentPlatformRef>(ref),
         unref_queue = page->GetUnrefQueue()] {
          auto current = weak_ref.lock();
          return current ? current->BuildLayers(unref_queue) : nullptr;
        });
  }
}

NativePaintingContextClay::~NativePaintingContextClay() {
  auto ref = GetNativeRef();
  if (ui_runner_ && !ui_runner_->RunsTasksOnCurrentThread()) {
    ui_runner_->PostTask([ref = std::move(ref)] { ref->Destroy(); });
  } else {
    ref->Destroy();
  }
}

void NativePaintingContextClay::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  PaintingContextClay::FinishLayoutOperation(options);
  // Fragment layout runs in Core. Notify Clay after the first root display
  // list and its children have arrived on the same UI queue.
  Enqueue([ref = GetNativeRef()] { ref->FinishLayout(); });
}

void NativePaintingContextClay::CreatePlatformRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& config) {
  Enqueue([ref = GetNativeRef(), id, type, init_data, config] {
    ref->CreatePlatformRenderer(id, type, init_data, config);
  });
}

void NativePaintingContextClay::CreatePlatformExtendedRenderer(
    int id, const base::String& tag, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& config) {
  Enqueue([ref = GetNativeRef(), id, tag, init_data, config] {
    ref->CreatePlatformExtendedRenderer(id, tag, init_data, config);
  });
}

void NativePaintingContextClay::UpdateLayout(
    int id, float x, float y, float width, float height, const float* paddings,
    const float* margins, const float* borders, const float* bounds,
    const float* sticky, float max_height, uint32_t node_index,
    bool display_none) {
  Enqueue([ref = GetNativeRef(), id, x, y, width, height,
           padding = CopyEdges(paddings), margin = CopyEdges(margins),
           border = CopyEdges(borders)] {
    ref->UpdateLayoutMetrics(id, x, y, width, height, padding.data(),
                             margin.data(), border.data());
  });
}

void NativePaintingContextClay::UpdatePlatformEventBundle(
    int id, PlatformEventBundle bundle) {
  Enqueue([ref = GetNativeRef(), id, bundle = std::move(bundle)]() mutable {
    ref->UpdatePlatformEventBundle(id, std::move(bundle));
  });
}

void NativePaintingContextClay::EnqueueDisplayList(int id, DisplayList list) {
  Enqueue([ref = GetNativeRef(),
           update = ClayDisplayListUpdate{id, std::move(list)}]() mutable {
    ref->Apply(std::move(update));
  });
}

void NativePaintingContextClay::EnqueueDisplayLists(
    DisplayListUpdateBatch batch) {
  std::vector<ClayDisplayListUpdate> updates;
  updates.reserve(batch.size());
  for (auto& update : batch) {
    updates.emplace_back(update.id, std::move(update.display_list));
  }
  Enqueue([ref = GetNativeRef(), updates = std::move(updates)]() mutable {
    for (auto& update : updates) {
      ref->Apply(std::move(update));
    }
  });
}

class UIDelegateLynxClay final : public UIDelegateClay {
 public:
  using UIDelegateClay::UIDelegateClay;

  std::unique_ptr<PaintingCtxPlatformImpl> CreatePaintingContextWithOptions(
      const PageOptions& options) override {
    if (options.IsFragmentLayerRender()) {
      return BindPaintingContext(
          std::make_unique<NativePaintingContextClay>(GetViewContext()));
    }
    if (auto* page = GetViewContext()->GetPageView()) {
      page->SetRecordedLayerProvider(nullptr);
    }
    return CreatePaintingContext();
  }
};

}  // namespace

std::unique_ptr<UIDelegateClay> CreateLynxClayUIDelegate(
    clay::ViewContext* view_context,
    std::unique_ptr<runtime::NativeModuleFactory> module_factory) {
  return std::make_unique<UIDelegateLynxClay>(view_context,
                                              std::move(module_factory));
}

}  // namespace lynx::tasm
