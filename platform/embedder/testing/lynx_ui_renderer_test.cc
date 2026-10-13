// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/testing/lynx_ui_renderer_test.h"

#include <utility>

#include "core/renderer/ui_wrapper/common/prop_bundle_creator_default.h"
#include "core/renderer/ui_wrapper/painting/empty/painting_context_implementation.h"

namespace lynx {
namespace embedder {
namespace {

class TestPaintingRef : public tasm::PaintingCtxPlatformRef {
 public:
  explicit TestPaintingRef(std::shared_ptr<TestPaintingState> state)
      : state_(std::move(state)) {}
  void InsertPaintingNode(int parent, int child, int index) override {
    state_->nodes.at(child).parent = parent;
  }

 private:
  std::shared_ptr<TestPaintingState> state_;
};

class TestPaintingContext : public tasm::PaintingContextPlatformImpl {
 public:
  explicit TestPaintingContext(std::shared_ptr<TestPaintingState> state)
      : state_(std::move(state)) {
    platform_ref_ = std::make_shared<TestPaintingRef>(state_);
  }

  void CreatePaintingNode(int id, const std::string& tag,
                          const fml::RefPtr<tasm::PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override {
    state_->nodes[id].tag = tag;
  }
  void UpdateLayout(int id, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height, uint32_t node_index,
                    bool display_none) override {
    auto& node = state_->nodes.at(id);
    node.width = width;
    node.height = height;
  }

 private:
  std::shared_ptr<TestPaintingState> state_;
};

struct TestLayoutState {
  std::weak_ptr<shell::LynxLayoutProxy> proxy;
  fml::RefPtr<fml::TaskRunner> ui_runner;
};

class TestLayoutContext : public tasm::LayoutCtxPlatformImpl {
 public:
  explicit TestLayoutContext(std::shared_ptr<TestLayoutState> state)
      : state_(std::move(state)) {}
  int CreateLayoutNode(int, const std::string&, tasm::PropBundle*,
                       bool) override {
    return tasm::LayoutNodeType::COMMON;
  }
  void UpdateLayoutNode(int, tasm::PropBundle*) override {}
  void InsertLayoutNode(int, int, int) override {}
  void RemoveLayoutNode(int, int, int) override {}
  void DestroyLayoutNodes(const std::unordered_set<int>&) override {}
  void ScheduleLayout() override {
    state_->ui_runner->PostTask([proxy = state_->proxy]() {
      if (auto layout = proxy.lock()) {
        layout->TriggerLayout();
      }
    });
  }
  void OnLayoutBefore(int) override {}
  void OnLayout(int, float, float, float, float, const std::array<float, 4>&,
                const std::array<float, 4>&) override {}
  void Destroy() override {}
  void SetFontFaces(const tasm::CSSFontFaceRuleMap&) override {}
  void SetLayoutNodeManager(tasm::LayoutNodeManager*) override {}

 private:
  std::shared_ptr<TestLayoutState> state_;
};

class TestUIDelegate : public tasm::UIDelegate {
 public:
  explicit TestUIDelegate(std::shared_ptr<TestPaintingState> state)
      : state_(std::move(state)) {}
  std::unique_ptr<tasm::PaintingCtxPlatformImpl> CreatePaintingContext()
      override {
    return std::make_unique<TestPaintingContext>(state_);
  }
  std::unique_ptr<tasm::LayoutCtxPlatformImpl> CreateLayoutContext() override {
    return std::make_unique<TestLayoutContext>(layout_state_);
  }
  std::unique_ptr<tasm::PropBundleCreator> CreatePropBundleCreator() override {
    return std::make_unique<tasm::PropBundleCreatorDefault>();
  }
  std::unique_ptr<runtime::NativeModuleFactory> GetCustomModuleFactory()
      override {
    return nullptr;
  }
  bool UsesLogicalPixels() const override { return true; }
  void OnLynxCreate(const std::shared_ptr<shell::ListEngineProxy>&,
                    const std::shared_ptr<shell::LynxEngineProxy>&,
                    const std::shared_ptr<shell::LynxRuntimeProxy>&,
                    const std::shared_ptr<shell::LynxLayoutProxy>& layout_proxy,
                    const std::shared_ptr<shell::PerfControllerProxy>&,
                    const std::shared_ptr<shell::EventTrackerProxy>&,
                    const std::shared_ptr<pub::LynxResourceLoader>&,
                    const fml::RefPtr<fml::TaskRunner>& ui_runner,
                    const fml::RefPtr<fml::TaskRunner>&, int32_t,
                    bool) override {
    layout_state_->proxy = layout_proxy;
    layout_state_->ui_runner = ui_runner;
  }

 private:
  std::shared_ptr<TestPaintingState> state_;
  std::shared_ptr<TestLayoutState> layout_state_ =
      std::make_shared<TestLayoutState>();
};

}  // namespace

TestLynxUIRenderer::TestLynxUIRenderer(lynx_view_builder_t* builder)
    : LynxUIRenderer(builder),
      state_(std::make_shared<TestPaintingState>()),
      ui_delegate_(std::make_unique<TestUIDelegate>(state_)) {}

TestLynxUIRenderer::~TestLynxUIRenderer() = default;

std::unique_ptr<LynxUIRenderer> LynxUIRenderer::CreateWithBuilder(
    lynx_view_builder_t* builder) {
  return std::make_unique<TestLynxUIRenderer>(builder);
}

}  // namespace embedder
}  // namespace lynx
