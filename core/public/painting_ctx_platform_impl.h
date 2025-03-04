// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_PAINTING_CTX_PLATFORM_IMPL_H_
#define CORE_PUBLIC_PAINTING_CTX_PLATFORM_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/lynx_actor.h"
#include "core/public/pipeline_option.h"
#include "core/public/platform_extra_bundle.h"
#include "core/public/prop_bundle.h"
#include "core/public/timing_collector_platform.h"
#include "core/public/timing_key.h"

namespace lynx {

namespace shell {
class DynamicUIOperationQueue;
}  // namespace shell

namespace tasm {
// painting context platform object ref.
class PaintingCtxPlatformRef {
 public:
  virtual ~PaintingCtxPlatformRef() = default;

  virtual void InsertPaintingNode(int parent, int child, int index) {}
  virtual void RemovePaintingNode(int parent, int child, int index,
                                  bool is_move) {}
  virtual void DestroyPaintingNode(int parent, int child, int index) {}

  virtual void OnCollectExtraUpdates(int32_t id) {}
  // update the data of the method of "scrolling"
  virtual void UpdateScrollInfo(int32_t container_id, bool smooth,
                                float estimated_offset, bool scrolling) {}

  virtual void SetGestureDetectorState(int64_t id, int32_t gesture_id,
                                       int32_t state) {}

  virtual void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                                       std::vector<int32_t> remove_ids) {}
  virtual void UpdateNodeReloadPatching(std::vector<int32_t> reload_ids) {}
  virtual void UpdateEventInfo(bool has_touch_pseudo) {}
  virtual void UpdateFlattenStatus(int id, bool flatten) {}

  virtual void ListReusePaintingNode(int id, const std::string& item_key){};
  virtual void ListCellWillAppear(int sign, const std::string& item_key){};
  virtual void ListCellDisappear(int sign, bool isExist,
                                 const std::string& item_key){};
  // insert the child's painting node of the list
  virtual void InsertListItemPaintingNode(int32_t list_id, int32_t child_id) {}
  // remove the child's painting node of the list
  virtual void RemoveListItemPaintingNode(int32_t list_id, int32_t child_id) {}
  // update the listContainer's contentOffset and contentSize
  virtual void UpdateContentOffsetForListContainer(int32_t container_id,
                                                   float content_size,
                                                   float delta_x, float delta_y,
                                                   bool is_init_scroll_offset) {
  }

  virtual void SetNeedMarkDrawEndTiming(
      std::weak_ptr<shell::TimingCollectorPlatform> weak_timing_collector,
      const tasm::PipelineID& pipeline_id) {}
};

class PaintingCtxPlatformImpl {
 public:
  virtual ~PaintingCtxPlatformImpl() {}
  virtual void SetUIOperationQueue(
      const std::shared_ptr<shell::DynamicUIOperationQueue>& queue){};
  virtual void SetInstanceId(const int32_t instance_id){};
  virtual void CreatePaintingNode(
      int id, const std::string& tag,
      const std::shared_ptr<PropBundle>& painting_data, bool flatten,
      bool create_node_async, uint32_t node_index = 0) = 0;
  virtual void InsertPaintingNode(int parent, int child, int index){};
  virtual void RemovePaintingNode(int parent, int child, int index,
                                  bool is_move){};
  virtual void DestroyPaintingNode(int parent, int child, int index){};
  virtual void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const std::shared_ptr<PropBundle>& painting_data) = 0;

  virtual std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                                  const pub::Value& info) = 0;

  virtual void UpdateLayout(int tag, float x, float y, float width,
                            float height, const float* paddings,
                            const float* margins, const float* borders,
                            const float* bounds, const float* sticky,
                            float max_height, uint32_t node_index = 0) = 0;
  virtual void UpdatePlatformExtraBundle(int32_t id,
                                         PlatformExtraBundle* bundle) {}

  virtual void SetKeyframes(std::unique_ptr<PropBundle> keyframes_data) = 0;
  virtual void Flush() = 0;
  virtual void FlushImmediately() { Flush(); };
  virtual void HandleValidate(int tag) = 0;
  virtual void FinishTasmOperation(const PipelineOptions& options) = 0;
  virtual void FinishLayoutOperation(const PipelineOptions& options) = 0;

  virtual std::vector<float> getBoundingClientOrigin(int id) = 0;
  virtual std::vector<float> getWindowSize(int id) = 0;
  virtual std::vector<float> GetRectToWindow(int id) = 0;

  virtual std::vector<float> GetRectToLynxView(int64_t id) = 0;
  virtual std::vector<float> ScrollBy(int64_t id, float width,
                                      float height) = 0;
  // TODO(liting.src): remove later.
  virtual void ConsumeGesture(int64_t id, int32_t gesture_id,
                              const pub::Value& params){};
  virtual void Invoke(
      int64_t id, const std::string& method, const pub::Value& params,
      const std::function<void(int32_t code, const pub::Value& data)>&
          callback) = 0;

  virtual int32_t GetTagInfo(const std::string& tag_name) = 0;
  virtual bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) = 0;

  virtual bool NeedAnimationProps() = 0;

  virtual void UpdateLayoutPatching() {}
  virtual void OnFirstMeaningfulLayout() {}

  // TODO(liting.src): remove this method after ui operation queue refactor.
  virtual void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                                       std::vector<int32_t> remove_ids) {}

  virtual void SetContextHasAttached() {}
  virtual void SetEnableVsyncAlignedFlush(bool enabled) {}

  virtual void InvokeUIMethod(int32_t view_id, const std::string& method,
                              std::unique_ptr<tasm::PropBundle> args,
                              int32_t callback_id) {}
  virtual void getAbsolutePosition(int id, float* position) {}

  // TODO(liting.src): remove this method after ui operation queue refactor.
  inline void SetTimingCollectorPlatform(
      const std::shared_ptr<shell::TimingCollectorPlatform>&
          timing_collector_platform) {
    timing_collector_platform_ = timing_collector_platform;
  };

  virtual void EnableUIOperationBatching(){};

  virtual bool DefaultOverflowAlwaysVisible() { return false; }

  // TODO(chenyouhui): remove this method after ui operation queue refactor.
  virtual bool EnableParallelElement() { return true; }

  // TODO(liting.src): remove this method after ui operation queue refactor.
  virtual bool HasEnableUIOperationBatching() { return false; }

  virtual bool EnableUIOperationQueue() { return false; }

  // platform object ref.
  const std::shared_ptr<PaintingCtxPlatformRef>& GetPlatformRef() {
    return platform_ref_;
  }

  // TODO(liting.src): remove this method after ui operation queue refactor.
  virtual base::closure ExecuteOperationSafely(base::closure op) { return op; }

 protected:
  std::shared_ptr<PaintingCtxPlatformRef> platform_ref_;
  std::shared_ptr<shell::TimingCollectorPlatform> timing_collector_platform_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_PAINTING_CTX_PLATFORM_IMPL_H_
