// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_ALGORITHM_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_ALGORITHM_H_

#include <array>
#include <optional>

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/direction_selector.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/logic_direction_utils.h"

namespace lynx {
namespace starlight {
class LayoutComputedStyle;

class LayoutAlgorithm : public DirectionSelector {
 public:
  LayoutAlgorithm(LayoutObject* container);
  virtual ~LayoutAlgorithm();

  void SetupRoot(const Constraints& root_constraints);

  LayoutUnit PercentBase(Dimension dimension) const {
    return container_constraints_[dimension].ToPercentBase();
  }

  Constraints GenerateDefaultConstraint(const LayoutObject& child) const;
  bool IsInflowSubTreeInSync() const;

  const LayoutComputedStyle* GetCSSStyle() const { return container_style_; }

  void Initialize(const Constraints& constraints,
                  const SLNodeSet* fixed_node_set = nullptr);
  void InitializeWithCompose(const Constraints& constraints,
                             const ComposeConstraints& compose_constraints,
                             const SLNodeSet* fixed_node_set = nullptr);
  FloatSize SizeDetermination();
  virtual void Alignment();

  void Update(const Constraints& constraints);
  void UpdateWithCompose(const Constraints& constraints,
                         const ComposeConstraints& compose_constraints);

  virtual void SetContainerBaseline() = 0;

  virtual bool RequiresComposeConstraints() const { return false; }

  float CalculateFloatSizeFromLength(const NLength& length,
                                     const LayoutUnit& percent_base);

  const NLength& GapStyle(Dimension dimension) const;

 protected:
  virtual void Reset(){};
  void UpdateAvailableSizeAndMode(const Constraints& constraints);
  void UpdateComposeConstraints(const ComposeConstraints& constraints);
  FloatSize PostLayoutProcessingAndResultBorderBoxSize();
  virtual void AfterResultBorderBoxSize();

  virtual BoxPositions GetAbsoluteOrFixedItemInitialPosition(
      LayoutObject* absolute_or_fixed_item) {
    return BoxPositions{Position::kStart, Position::kStart};
  };

  // Absolute | Fixed
  virtual void MeasureAbsoluteAndFixed();

  virtual void AlignInFlowItems() = 0;

  // Initialize layout environment,init some relevant parameters
  virtual void InitializeAlgorithmEnv() = 0;

  virtual void InitializeChildren(const SLNodeSet* fixed_node_set = nullptr);

  // TODO(zzz):unified handle process
  virtual void SizeDeterminationByAlgorithm() = 0;

  LayoutObject* container_;
  const LayoutComputedStyle* container_style_;

  Constraints container_constraints_;
  // Content-box bounds retained for Compose measurement and placement.
  std::optional<ComposeConstraints> compose_constraints_;

  LayoutItems sticky_items;
  LayoutItems absolute_or_fixed_items_;
  LayoutItems inflow_items_;

 private:
  LayoutAlgorithm();

  // relative
  void HandleRelativePosition();

  virtual void AlignAbsoluteAndFixedItems();

  void ItemsUpdateAlignment();

  void InitializeFixedNode(const SLNodeSet* fixed_node_set = nullptr);
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_ALGORITHM_H_
