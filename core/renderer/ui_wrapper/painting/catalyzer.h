// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_CATALYZER_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_CATALYZER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/include/base_export.h"
#include "core/public/prop_bundle.h"

namespace lynx {
namespace tasm {

class NodeIndexPair;
class Element;
class PaintingContext;
class AirElement;

class Catalyzer {
 public:
  Catalyzer(std::unique_ptr<PaintingContext> painting_context,
            int32_t instance_id);

  virtual ~Catalyzer() = default;

  inline PaintingContext* painting_context() { return painting_context_.get(); }

  inline void set_root(Element* root) { root_ = root; }
  inline Element* get_root() { return root_; }
  inline void set_air_root(AirElement* root) { air_root_ = root; }
  inline AirElement* get_air_root() { return air_root_; }

  bool NeedUpdateLayout();
  void UpdateLayoutRecursively();
  void UpdateLayoutRecursivelyWithoutChange();

  BASE_EXPORT_FOR_DEVTOOL std::vector<float> getBoundingClientOrigin(
      Element* node);
  BASE_EXPORT_FOR_DEVTOOL std::vector<float> GetRectToWindow(Element* node);
  BASE_EXPORT_FOR_DEVTOOL std::vector<float> getWindowSize(Element* node);

  std::vector<float> GetRectToLynxView(Element* node);
  std::vector<float> ScrollBy(int64_t id, float width, float height);
  // 1 - active, 2 - fail, 3 - end
  void SetGestureDetectorState(int64_t id, int32_t gesture_id, int32_t state);
  void ConsumeGesture(int64_t id, int32_t gesture_id, const pub::Value& params);
  void Invoke(int64_t id, const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback);
  inline int32_t GetInstanceId() { return instance_id_; }
#if ENABLE_TRACE_PERFETTO
  int64_t last_dump_time_ = 0;
  static constexpr int64_t kDumpThresholdMilliseconds = 16;
  static constexpr size_t kMaxAttributeValueLength = 100;
  void DumpElementTree();
#endif

 private:
  std::unique_ptr<PaintingContext> painting_context_;
  Element* root_ = nullptr;
  AirElement* air_root_ = nullptr;
  Catalyzer(const Catalyzer&) = delete;
  Catalyzer& operator=(const Catalyzer&) = delete;
  const int32_t instance_id_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_CATALYZER_H_
