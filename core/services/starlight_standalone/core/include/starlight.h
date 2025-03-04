// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_H_
#define CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "core/services/starlight_standalone/core/include/starlight_config.h"

namespace starlight {

using SLNodeRef = void*;

class MeasureDelegate {
 public:
  virtual ~MeasureDelegate() = default;
  virtual SLSize Measure(SLConstraints& constraint) = 0;
  virtual float Baseline(SLConstraints& constraint) = 0;
  virtual void Alignment() = 0;
};

// life cycle
SLNodeRef CreateWithConfig(const LayoutConfig& config);
void UpdateConfig(const SLNodeRef node, const LayoutConfig& config);

/**
 * This method uses the external constraints and the CSS dimensions of the root
 *node to determine the final layout properties. Call this method before
 *CalculateLayout to finalize the external constraints for the root node if
 *there are external constraints that need to be applied.
 **/
void UpdateViewport(const SLNodeRef node, float width,
                    StarlightMeasureMode width_mode, float height,
                    StarlightMeasureMode height_mode);
void Free(SLNodeRef node);
void InsertChild(SLNodeRef node, SLNodeRef child, int32_t index = -1);
void RemoveChild(const SLNodeRef parent, const SLNodeRef child);
void RemoveChild(const SLNodeRef parent, int32_t index);
void RemoveAllChild(const SLNodeRef node);
void MoveChild(SLNodeRef node, SLNodeRef child, uint32_t from_index,
               uint32_t to_index);
SLNodeRef GetParent(SLNodeRef node);
uint32_t GetChildCount(SLNodeRef node);
SLNodeRef GetChild(SLNodeRef node, uint32_t index);

// layout
void CalculateLayout(SLNodeRef node);
void MarkDirty(SLNodeRef node);
bool IsDirty(SLNodeRef node);

// layout result
SLSize GetLayoutSize(SLNodeRef node);
SLPoint GetLayoutOffset(SLNodeRef node);
float GetLayoutMargin(SLNodeRef node, SLEdge edge);
float GetLayoutPadding(SLNodeRef node, SLEdge edge);
float GetLayoutBorder(SLNodeRef node, SLEdge edge);

// RTL
bool IsRTL(const SLNodeRef node);

// display
void SetDisplay(const SLNodeRef node, SLDisplayType type);
void SetAspectRatio(const SLNodeRef node, float value);

// flex
void SetFlexGrow(const SLNodeRef node, float value);
void SetFlexShrink(const SLNodeRef node, float value);
void SetFlexBasis(const SLNodeRef node, const SLLength value);

// JustifyContent,AlignSelf
void SetJustifyContent(const SLNodeRef node, SLJustifyContentType type);
void SetAlignContent(const SLNodeRef node, SLAlignContentType type);
void SetAlignSelf(const SLNodeRef node, SLFlexAlignType type);
void SetAlignItems(const SLNodeRef node, SLFlexAlignType type);
void SetDirection(const SLNodeRef node, SLDirectionType type);
void SetFlexDirection(const SLNodeRef node, SLFlexDirection type);
void SetPosition(const SLNodeRef node, SLPositionType type);
void SetFlexWrap(const SLNodeRef node, SLFlexWrapType type);

// length
void SetWidth(const SLNodeRef node, const SLLength value);
void SetHeight(const SLNodeRef node, const SLLength value);
void SetMinWidth(const SLNodeRef node, const SLLength value);
void SetMinHeight(const SLNodeRef node, const SLLength value);
void SetMaxWidth(const SLNodeRef node, const SLLength value);
void SetMaxHeight(const SLNodeRef node, const SLLength value);
SLLength GetWidth(const SLNodeRef node);
SLLength GetHeight(const SLNodeRef node);

void SetLeft(const SLNodeRef node, const SLLength value);
void SetRight(const SLNodeRef node, const SLLength value);
void SetBottom(const SLNodeRef node, const SLLength value);
void SetTop(const SLNodeRef node, const SLLength value);
void SetInlineStart(const SLNodeRef node, const SLLength value);
void SetInlineEnd(const SLNodeRef node, const SLLength value);

void SetMarginLeft(const SLNodeRef node, const SLLength value);
void SetMarginRight(const SLNodeRef node, const SLLength value);
void SetMarginTop(const SLNodeRef node, const SLLength value);
void SetMarginBottom(const SLNodeRef node, const SLLength value);
void SetMarginInlineStart(const SLNodeRef node, const SLLength value);
void SetMarginInlineEnd(const SLNodeRef node, const SLLength value);
void SetMargin(const SLNodeRef node, const SLLength value);

void SetPaddingLeft(const SLNodeRef node, const SLLength value);
void SetPaddingRight(const SLNodeRef node, const SLLength value);
void SetPaddingTop(const SLNodeRef node, const SLLength value);
void SetPaddingBottom(const SLNodeRef node, const SLLength value);
void SetPaddingInlineStart(const SLNodeRef node, const SLLength value);
void SetPaddingInlineEnd(const SLNodeRef node, const SLLength value);
void SetPadding(const SLNodeRef node, const SLLength value);

void SetBorderLeft(const SLNodeRef node, const SLLength value);
void SetBorderRight(const SLNodeRef node, const SLLength value);
void SetBorderTop(const SLNodeRef node, const SLLength value);
void SetBorderBottom(const SLNodeRef node, const SLLength value);
void SetBorder(const SLNodeRef node, const SLLength value);

/**
@brief Set the specified style property for the given node.
The value can be a CSS string that will be parsed and applied to the node.
@return Returns true if the style property was successfully set; false
otherwise.
*/
bool SetStyle(SLNodeRef node, const std::string& name,
              const std::string& value);
/**
@brief Set multiple style properties for the given node.
This is a convenience function that can be used for performance optimization.
@return Returns true if the style properties were successfully set; false
otherwise.
*/
bool SetMultiStyles(SLNodeRef node, const std::vector<std::string>& names,
                    const std::vector<std::string>& values);
bool ResetStyle(SLNodeRef node, const std::string& name);

// measurefuc
bool HasMeasureDelegate(SLNodeRef node);
/**
 * @brief Set the measurement delegate of a node.
 *
 * @param delegate The measurement delegate object, which must implement the
 * MeasureDelegate interface.
 *
 * This function sets the measurement delegate of a node to the specified
 * measurement delegate object. The measurement delegate object must implement
 * the MeasureDelegate interface, which includes the following functions:
 *   - Measure: for measuring the node.
 *   - Baseline: for calculating the baseline of the node.
 *   - Alignment: for aligning the node.
 * Users need to manage the lifecycle of the measurement delegate object
 * themselves to ensure that it is released before the node is destroyed.
 */
void SetMeasureDelegate(SLNodeRef node, MeasureDelegate* delegate);

}  // namespace starlight

#endif  // CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_H_
