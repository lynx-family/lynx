// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LAYOUT_MEDIATOR_H_
#define CORE_SHELL_LAYOUT_MEDIATOR_H_

#include <memory>
#include <vector>

#include "core/public/pipeline_option.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "core/renderer/ui_wrapper/painting/catalyzer.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/shell/lynx_actor_specialization.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/native_facade.h"
#include "core/shell/tasm_operation_queue.h"

namespace lynx {
namespace shell {

class LayoutMediator : public tasm::LayoutContext::Delegate,
                       public std::enable_shared_from_this<LayoutMediator> {
 public:
  LayoutMediator(const std::shared_ptr<TASMOperationQueue> &operation_queue);
  void SetRuntimeActor(
      const std::shared_ptr<LynxActor<runtime::LynxRuntime>> &actor) {
    runtime_actor_ = actor;
  }

  void OnLayoutUpdate(int tag, float x, float y, float width, float height,
                      const std::array<float, 4> &paddings,
                      const std::array<float, 4> &margins,
                      const std::array<float, 4> &borders,
                      const std::array<float, 4> *sticky_positions,
                      float max_height) override;
  void OnNodeLayoutAfter(int32_t id) override;
  void OnLayoutAfter(const tasm::PipelineOptions &option,
                     std::unique_ptr<tasm::PlatformExtraBundleHolder> holder,
                     bool has_layout) override;
  void PostPlatformExtraBundle(
      int32_t id, std::unique_ptr<tasm::PlatformExtraBundle> bundle) override;
  void OnCalculatedViewportChanged(
      const tasm::layout::CalculatedViewport &viewport, int tag) override;
  void SetTiming(tasm::Timing timing) override;
  void OnFirstMeaningfulLayout() override;
  void Init(const std::shared_ptr<LynxActor<LynxEngine>> &actor,
            const std::shared_ptr<LynxActor<NativeFacade>> &facade_actor,
            const std::shared_ptr<LynxActor<tasm::timing::TimingHandler>>
                &timing_actor,
            tasm::NodeManager *node_manager,
            tasm::AirNodeManager *air_node_manager,
            tasm::Catalyzer *catalyzer) {
    engine_actor_ = actor;
    facade_actor_ = facade_actor;
    timing_actor_ = timing_actor;
    node_manager_ = node_manager;
    air_node_manager_ = air_node_manager;
    catalyzer_ = catalyzer;
  }
  void SetEnableAirStrictMode(bool enable_air_strict_mode) override {
    enable_air_strict_mode_ = enable_air_strict_mode;
  }

  static void HandleLayoutVoluntarily(TASMOperationQueue *queue,
                                      tasm::Catalyzer *catalyzer);

 private:
  static void HandlePendingLayoutTask(TASMOperationQueue *queue,
                                      tasm::Catalyzer *catalyzer,
                                      tasm::PipelineOptions option);
  static void HandleListOrComponentUpdated(
      tasm::NodeManager *node_manager, const tasm::PipelineOptions &options);

  std::shared_ptr<LynxActor<LynxEngine>> engine_actor_;
  std::shared_ptr<LynxActor<NativeFacade>> facade_actor_;
  std::shared_ptr<LynxActor<runtime::LynxRuntime>> runtime_actor_;
  std::shared_ptr<LynxActor<tasm::timing::TimingHandler>> timing_actor_;
  // tasm thread and layout thread is same one
  // when strategy is {ALL_ON_UI, MOST_ON_TASM}
  std::shared_ptr<TASMOperationQueue> operation_queue_;
  // dont own, external ptr from class ElementManager
  // thread safe, because they only run on tasm thread
  tasm::NodeManager *node_manager_;
  tasm::AirNodeManager *air_node_manager_;
  tasm::Catalyzer *catalyzer_;

  // TODO(heshan):now trigger onFirstScreen when first layout,
  // but it may be triggered when update data...
  bool has_first_layout_{false};
  bool enable_air_strict_mode_{false};
};

}  // namespace shell
}  // namespace lynx
#endif  // CORE_SHELL_LAYOUT_MEDIATOR_H_
