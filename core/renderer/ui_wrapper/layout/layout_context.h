// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_CONTEXT_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_CONTEXT_H_

#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "base/include/closure.h"
#include "core/public/layout_ctx_platform_impl.h"
#include "core/public/layout_node_manager.h"
#include "core/public/pipeline_option.h"
#include "core/public/platform_extra_bundle.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/css/dynamic_css_styles_manager.h"
#include "core/renderer/dom/layout_bundle.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/page_config.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/types/layout_constraints.h"
#include "core/renderer/ui_wrapper/layout/layout_context_data.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"
#include "core/services/timing_handler/timing.h"

namespace lynx {
namespace starlight {
class LayoutObject;
class LayoutEventData;
}  // namespace starlight
namespace tasm {

using RequestLayoutCallback = base::MoveOnlyClosure<void>;

using lynx::tasm::layout::CalculatedViewport;
using lynx::tasm::layout::LayoutInfo;
using lynx::tasm::layout::LayoutInfoArray;
using lynx::tasm::layout::MeasureMode;
using lynx::tasm::layout::Viewport;

class HierarchyObserver;

class LayoutContext : public std::enable_shared_from_this<LayoutContext>,
                      public starlight::LayoutEventHandler,
                      public LayoutNodeManager {
 public:
  class Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}
    virtual void OnLayoutUpdate(int tag, float x, float y, float width,
                                float height,
                                const std::array<float, 4>& paddings,
                                const std::array<float, 4>& margins,
                                const std::array<float, 4>& borders,
                                const std::array<float, 4>* sticky_positions,
                                float max_height) = 0;
    void OnLayoutAfter(const PipelineOptions& options) {
      OnLayoutAfter(options, nullptr, false);
    };
    virtual void OnLayoutAfter(
        const PipelineOptions& options,
        std::unique_ptr<PlatformExtraBundleHolder> holder, bool has_layout) = 0;
    virtual void OnNodeLayoutAfter(int32_t id) = 0;
    virtual void PostPlatformExtraBundle(
        int32_t id, std::unique_ptr<tasm::PlatformExtraBundle> bundle) = 0;
    virtual void OnCalculatedViewportChanged(const CalculatedViewport& viewport,
                                             int tag) = 0;
    virtual void SetTiming(tasm::Timing timing) = 0;
    virtual void OnFirstMeaningfulLayout() = 0;
    virtual void SetEnableAirStrictMode(bool enable_air_strict_mode) = 0;
  };

  LayoutContext(std::unique_ptr<Delegate> delegate,
                std::unique_ptr<LayoutCtxPlatformImpl> platform_impl,
                const LynxEnvConfig& lynx_env_config, int32_t instance_id);
  virtual ~LayoutContext();

  // used for platform
  void SetMeasureFunc(int32_t id,
                      std::unique_ptr<MeasureFunc> measure_func) override;
  void MarkDirtyAndRequestLayout(int32_t id) override;
  void MarkDirtyAndForceLayout(int32_t id) override;

  bool IsDirty(int32_t id) override;
  FlexDirection GetFlexDirection(int32_t id) override;
  float GetWidth(int32_t id) override;
  float GetHeight(int32_t id) override;
  float GetPaddingLeft(int32_t id) override;
  float GetPaddingTop(int32_t id) override;
  float GetPaddingRight(int32_t id) override;
  float GetPaddingBottom(int32_t id) override;
  float GetMarginLeft(int32_t id) override;
  float GetMarginTop(int32_t id) override;
  float GetMarginRight(int32_t id) override;
  float GetMarginBottom(int32_t id) override;
  float GetMinWidth(int32_t id) override;
  float GetMaxWidth(int32_t id) override;
  float GetMinHeight(int32_t id) override;
  float GetMaxHeight(int32_t id) override;
  LayoutResult UpdateMeasureByPlatform(int32_t id, float width,
                                       int32_t width_mode, float height,
                                       int32_t height_mode,
                                       bool final_measure) override;
  void AlignmentByPlatform(int32_t id, float offset_top,
                           float offset_left) override;

  void UpdateLayoutNodeByBundle(int32_t id,
                                std::unique_ptr<LayoutBundle> bundle);

  LayoutNode* CreateLayoutNode(int32_t id, const base::String& tag);
  void UpdateLayoutNodeProps(int32_t id,
                             const std::shared_ptr<PropBundle>& props);
  void UpdateLayoutNodeFontSize(int32_t id, double cur_node_font_size,
                                double root_node_font_size, double font_scale);
  void UpdateLayoutNodeStyle(int32_t id, CSSPropertyID css_id,
                             const tasm::CSSValue& value);
  void ResetLayoutNodeStyle(int32_t id, CSSPropertyID css_id);
  void UpdateLayoutNodeAttribute(int32_t id, starlight::LayoutAttribute key,
                                 const lepus::Value& value);
  void ResetLayoutNodeAttribute(int32_t id, starlight::LayoutAttribute key);
  void InsertLayoutNode(int32_t parent_id, int32_t child_id, int index);
  void RemoveLayoutNodeAtIndex(int32_t parent_id, int index);
  void MoveLayoutNode(int32_t parent_id, int32_t child_id, int from_index,
                      int to_index);
  void InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id,
                              int32_t ref_id);
  void RemoveLayoutNode(int32_t parent_id, int32_t child_id);
  void DestroyLayoutNode(int32_t id);
  void AttachLayoutNodeType(int32_t id, const base::String& tag,
                            bool allow_inline,
                            const std::shared_ptr<PropBundle>& props);
  void MarkDirty(int32_t id);
  void DispatchLayoutUpdates(const PipelineOptions& options);
  void SetPageConfigForLayoutThread(const std::shared_ptr<PageConfig>& config);

  void SetEnableLayout();

  // Check fixed node, if position type has been changed, update the
  // fixed node set.
  void CheckFixed(LayoutNode* node);
  // Should update fixed node set if needed
  void UpdateFixedNodeSet(LayoutNode* node, bool is_insert);
  inline const SLNodeSet* GetFixedNodeSet() {
    return static_cast<const SLNodeSet*>(&fixed_node_set_);
  }

  void SetFontFaces(const FontFacesMap& fontfaces);

  // Thread safe
  void Layout(const PipelineOptions& options = PipelineOptions());
  void UpdateViewport(float width, int width_mode, float height,
                      int height_mode, bool need_layout = true);

  // Thread unsafe
  void SetRoot(int32_t id);

  inline LayoutNode* root() { return root_; }

  inline void SetHierarchyObserver(
      const std::shared_ptr<HierarchyObserver>& hierarchy_observer) {
    hierarchy_observer_ = hierarchy_observer;
  }

  inline int root_id() {
    if (root_ == nullptr) {
      return -1;
    }
    return root_->id();
  }

  inline const Viewport& GetViewPort() const { return viewport_; }

  void UpdateLynxEnvForLayoutThread(LynxEnvConfig env);

  void UpdateLayoutInfo(LayoutNode* node);

  void SetRequestLayoutCallback(RequestLayoutCallback callback) {
    request_layout_callback_ = std::move(callback);
  }

  std::weak_ptr<LayoutCtxPlatformImpl> GetWeakPlatformImpl() const {
    return std::weak_ptr<LayoutCtxPlatformImpl>(platform_impl_);
  }

  std::unordered_map<int32_t, LayoutInfoArray> GetSubTreeLayoutInfo(
      int32_t root_id, Viewport viewport = Viewport{});
#if ENABLE_TESTBENCH_RECORDER
  void SetRecordId(int64_t record_id) { record_id_ = record_id; }
#endif

 private:
  class CircularLayoutDependencyDetector {
   public:
    static constexpr int64_t kTimeWindow = 60000;  // 1min
    static constexpr int64_t kContinuousViewportUpdateMaxGap = 50;
    void DetectCircularLayoutDependency();

   private:
    bool in_error_state_ = false;
    int64_t continuous_viewport_update_start_time_ = -1;
    int64_t last_viewport_update_time_ = -1;
  };
  CircularLayoutDependencyDetector circular_layout_detector_;

  // Should be call on the thread that layout engine work on
  void RequestLayout(const PipelineOptions& options = PipelineOptions());
  void DispatchLayoutBeforeRecursively(LayoutNode* node);
  void LayoutRecursively(LayoutNode* node, const PipelineOptions& options);
  void DestroyPlatformNodesIfNeeded();
  bool SetViewportSizeToRootNode();
  int GetIndexForChild(LayoutNode* parent, LayoutNode* child);
  inline void UpdateLayoutNodePropsInner(
      LayoutNode* node, const std::shared_ptr<PropBundle>& props);
  inline void UpdateLayoutNodeFontSizeInner(LayoutNode* node,
                                            double cur_node_font_size,
                                            double root_node_font_size,
                                            double font_scale);
  inline void UpdateLayoutNodeStyleInner(LayoutNode* node, CSSPropertyID css_id,
                                         const tasm::CSSValue& value);
  inline void ResetLayoutNodeStyleInner(LayoutNode* node, CSSPropertyID css_id);
  inline void UpdateLayoutNodeAttributeInner(LayoutNode* node,
                                             starlight::LayoutAttribute key,
                                             const lepus::Value& value);
  inline void SetRootInner(LayoutNode* node);

  inline bool NoNeedPlatformLayoutNode(
      const base::String& tag, const std::shared_ptr<PropBundle>& props);

  inline void AttachLayoutNodeTypeInner(
      LayoutNode* node, const base::String& tag, bool allow_inline,
      const std::shared_ptr<PropBundle>& props);

  inline LayoutNode* InitLayoutNodeWithBundle(int32_t id, LayoutBundle* bundle);

  LayoutNode* FindNodeById(int32_t id);

  virtual void OnLayoutEvent(const starlight::LayoutObject* node,
                             starlight::LayoutEventType type,
                             const starlight::LayoutEventData& data) override;

  starlight::LayoutConfigs GetLayoutConfigs() {
    if (page_config_) {
      return page_config_->GetLayoutConfigs();
    }
    return starlight::LayoutConfigs();
  }

  // SetLayoutEarlyExitTiming needs to be called during an early return to
  // simulate layout timing when the layout is not actually executed.
  void SetLayoutEarlyExitTiming(const PipelineOptions& options);
  void GetLayoutInfoRecursively(
      std::unordered_map<int32_t, LayoutInfoArray>& result, LayoutNode* node);

  starlight::Constraints ConvertViewportToOneSideConstraint(Viewport viewport);
  bool IfNeedsUpdateLayoutInfo(LayoutNode* node);

  std::shared_ptr<LayoutCtxPlatformImpl> platform_impl_;
  std::unique_ptr<Delegate> delegate_;
  LayoutNode* root_;
  bool layout_wanted_;
  bool has_viewport_ready_;
  bool enable_layout_;
  bool has_layout_required_;
  Viewport viewport_;
  std::shared_ptr<HierarchyObserver> hierarchy_observer_;
  // Help to record those platform node that have been removed during diff so
  // that we can trigger destroy operation on platform
  std::unordered_set<int> destroyed_platform_nodes_;
  std::unordered_map<int32_t, LayoutNode> layout_nodes_;
  SLNodeSet fixed_node_set_;
  std::unordered_map<base::String, LayoutNodeType> node_type_recorder_;
  // used for copy constructor when LayoutNode init css_style
  std::unique_ptr<starlight::ComputedCSSStyle> init_css_style_;
  std::shared_ptr<PageConfig> page_config_;
  LynxEnvConfig lynx_env_config_;
  const int32_t instance_id_ = 0;
#if ENABLE_TESTBENCH_RECORDER
  int64_t record_id_;
#endif
  bool has_first_page_layout_ = false;

  CalculatedViewport calculated_viewport_;

  RequestLayoutCallback request_layout_callback_;

  LayoutContext(const LayoutContext&) = delete;
  LayoutContext& operator=(const LayoutContext&) = delete;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_CONTEXT_H_
