// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_TESTING_FIBER_MOCK_PAINTING_CONTEXT_H_
#define CORE_RENDERER_DOM_TESTING_FIBER_MOCK_PAINTING_CONTEXT_H_

#define private public
#define protect public
#include <memory>
#include <string>
#include <unordered_map>

#include "core/renderer/tasm/react/testing/mock_painting_context.h"

namespace lynx {
namespace tasm {
namespace testing {

class FiberMockPaintingContext : public PaintingContextPlatformImpl {
 public:
  void ResetFlushFlag();

  bool HasFlushed();

  void Flush() override;

  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info);

  std::unordered_map<int, std::string> captured_create_tags_map_;

  // TODO(liting.src): remove after painting context refactor.
  bool HasEnableUIOperationBatching() override;

 private:
  void CreatePaintingNode(int id, const std::string& tag,
                          const std::shared_ptr<PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override;
  void InsertPaintingNode(int parent, int child, int index) override;
  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;
  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const std::shared_ptr<PropBundle>& painting_data) override;
  void UpdateLayout(int tag, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height,
                    uint32_t node_index = 0) override;

  void SetKeyframes(std::unique_ptr<PropBundle> keyframes_data) override;

  int32_t GetTagInfo(const std::string& tag_name) override;

  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;

  bool NeedAnimationProps() override;

  void EnqueueOperation(shell::UIOperation op);

 private:
  bool flush_{false};
  std::unordered_map<int, std::unique_ptr<MockNode>> node_map_;
  std::unordered_map<std::string, lepus::Value> keyframes_;
  std::unordered_map<std::string, int32_t> mock_virtuality_map = {
      {"inline-text", LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL},
      {"view", LayoutNodeType::COMMON},
      {"inline-image", LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL},
      {"raw-text", LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL},
      {"list", LayoutNodeType::COMMON},
      {"text", LayoutNodeType::CUSTOM}};
  std::shared_ptr<shell::LynxUIOperationQueue> queue_ =
      std::make_shared<shell::LynxUIOperationQueue>();
};

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_TESTING_FIBER_MOCK_PAINTING_CONTEXT_H_
