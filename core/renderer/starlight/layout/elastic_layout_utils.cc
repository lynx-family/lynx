// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/elastic_layout_utils.h"

#include "base/include/float_comparison.h"
#include "core/renderer/starlight/layout/box_info.h"

namespace lynx {
namespace starlight {

namespace {

struct ElasticInternalVariables {
  float initial_total_elastic_factor_ = 0.f;
  float total_elastic_factor_ = 0.f;
  float total_scaled_elastic_factor_ = 0.f;
  float remaining_space_ = 0.f;
  float initial_free_space_ = 0.f;
};

float CalculateRemainingSpace(
    const ElasticLayoutUtils::ElasticInfos& elastic_info, float available_space,
    InlineFloatArray& computed_item_sizes, const InlineBoolArray& freeze) {
  float initial_free_space = available_space;
  for (size_t idx = elastic_info.start_idx_; idx < elastic_info.end_idx_;
       ++idx) {
    const auto& margin = elastic_info.targets_[idx]->GetBoxInfo()->margin_;
    float main_margins = margin[elastic_info.direction_selector_.MainFront()] +
                         margin[elastic_info.direction_selector_.MainBack()];

    // For frozen items, use their outer target main size;
    if (freeze[idx - elastic_info.start_idx_]) {
      initial_free_space -= computed_item_sizes[idx] + main_margins;
    }
    // for other items, use their outer flex base size.
    else {
      // In the flex specification, flex_base_size does not clamp the content
      // box size.
      initial_free_space -= elastic_info.elastic_bases_[idx] + main_margins;
    }
  }
  // remaining space should 'subtract' main axis gaps between the items
  initial_free_space -= elastic_info.main_axis_gap_ *
                        (elastic_info.end_idx_ - elastic_info.start_idx_ - 1);
  return initial_free_space;
}

void FreezeItem(size_t idx, ElasticLayoutUtils::ElasticInfos& elastic_infos,
                ElasticInternalVariables& internal_variables,
                ElasticLayoutUtils::ElasticFactorGetter elastic_factor_getter,
                InlineBoolArray& freeze) {
  LayoutObject* item = elastic_infos.targets_[idx];
  freeze[idx - elastic_infos.start_idx_] = true;
  float elastic_factor = elastic_factor_getter(*item);
  if (base::FloatsLarger(elastic_factor, 0.f)) {
    internal_variables.total_elastic_factor_ -= elastic_factor;
    if (!elastic_infos.is_elastic_grow_) {
      internal_variables.total_scaled_elastic_factor_ -=
          elastic_factor * elastic_infos.elastic_bases_[idx];
    }
  }
}

bool ResolveOneLine(
    ElasticLayoutUtils::ElasticInfos& elastic_infos,
    ElasticInternalVariables& internal_variables,
    ElasticLayoutUtils::ElasticFactorGetter elastic_factor_getter,
    InlineFloatArray& computed_item_sizes, InlineBoolArray& freeze,
    float free_space) {
  // 9.7-4 Loop
  float total_violations = 0;
  base::InlineVector<size_t, kChildrenInlineVectorSize> min_violations;
  base::InlineVector<size_t, kChildrenInlineVectorSize> max_violations;

  float used_space = 0;

  internal_variables.remaining_space_ = free_space;

  float adjust_remaining_free_space = free_space;
  if (base::FloatsLarger(elastic_infos.total_elastic_factor_override_, 0.f)) {
    adjust_remaining_free_space =
        internal_variables.initial_free_space_ *
        internal_variables.initial_total_elastic_factor_ /
        elastic_infos.total_elastic_factor_override_;
  } else {
    adjust_remaining_free_space = internal_variables.initial_free_space_ *
                                  internal_variables.total_elastic_factor_;
  }
  if (std::abs(adjust_remaining_free_space) < std::abs(free_space))
    free_space = adjust_remaining_free_space;

  for (size_t idx = elastic_infos.start_idx_; idx < elastic_infos.end_idx_;
       ++idx) {
    if (!freeze[idx - elastic_infos.start_idx_]) {
      const auto& item = *elastic_infos.targets_[idx];
      // In the flex specification, flex_base_size does not clamp the content
      // box size.
      float calc_main_size = elastic_infos.elastic_bases_[idx];

      if (elastic_infos.is_elastic_grow_ &&
          base::FloatsLarger(free_space, 0.f) &&
          base::FloatsLarger(internal_variables.total_elastic_factor_, 0.f)) {
        calc_main_size += (elastic_factor_getter(item) /
                           internal_variables.total_elastic_factor_) *
                          free_space;
      } else if (base::FloatsLarger(0.f, free_space) &&
                 base::FloatsLarger(
                     internal_variables.total_scaled_elastic_factor_, 0.f)) {
        calc_main_size +=
            (elastic_factor_getter(item) * elastic_infos.elastic_bases_[idx] /
             internal_variables.total_scaled_elastic_factor_) *
            free_space;
      }

      // d. Fix min/max violations
      float adjust_main_size = elastic_infos.direction_selector_.IsHorizontal()
                                   ? item.ClampExactWidth(calc_main_size)
                                   : item.ClampExactHeight(calc_main_size);

      computed_item_sizes[idx] = adjust_main_size;
      // In the flex specification, flex_base_size does not clamp the content
      // box size.
      used_space += adjust_main_size - elastic_infos.elastic_bases_[idx];

      if (adjust_main_size > calc_main_size) {
        min_violations.push_back(idx);
      } else if (adjust_main_size < calc_main_size) {
        max_violations.push_back(idx);
      }

      total_violations += adjust_main_size - calc_main_size;
    }
  }

  if (base::IsZero(total_violations)) {
    internal_variables.remaining_space_ -= used_space;
  }
  // Positive: Freeze all the items with min violations.
  else if (total_violations > 0) {
    for (auto idx : min_violations) {
      FreezeItem(idx, elastic_infos, internal_variables, elastic_factor_getter,
                 freeze);
    }
  }
  // Negative: Freeze all the items with max violations.
  else {
    for (auto idx : max_violations) {
      FreezeItem(idx, elastic_infos, internal_variables, elastic_factor_getter,
                 freeze);
    }
  }

  return !base::IsZero(total_violations);
}

ElasticInternalVariables GenerateInitialInternalVariables(
    const ElasticLayoutUtils::ElasticInfos infos,
    const ElasticLayoutUtils::ElasticFactorGetter& factor_getter,
    float available_spaces) {
  ElasticInternalVariables result;
  for (size_t idx = infos.start_idx_; idx < infos.end_idx_; ++idx) {
    float elastic_factor = factor_getter(*infos.targets_[idx]);
    if (base::FloatsLarger(elastic_factor, 0.f)) {
      result.total_elastic_factor_ += elastic_factor;
      result.total_scaled_elastic_factor_ +=
          elastic_factor * infos.elastic_bases_[idx];
    }
  }
  result.initial_total_elastic_factor_ = result.total_elastic_factor_;
  return result;
}

}  // namespace

float ElasticLayoutUtils::ComputeElasticItemSizes(
    ElasticInfos& elastic_infos, float available_spaces,
    ElasticFactorGetter elastic_factor_getter,
    InlineFloatArray& computed_item_sizes) {
  // 9.7-1 Determine the used flex factor.
  // Sum the outer hypothetical main sizes of all items on the line.
  size_t start = elastic_infos.start_idx_;
  size_t end = elastic_infos.end_idx_;

  InlineBoolArray freeze(end - start, false);
  ElasticInternalVariables variables = GenerateInitialInternalVariables(
      elastic_infos, elastic_factor_getter, available_spaces);

  bool is_grow = elastic_infos.is_elastic_grow_;

  // 9.7-2 Size inflexible items: Freeze, setting its target main size to
  // its hypothetical main size when ...
  for (size_t idx = start; idx < end; ++idx) {
    // any item that has a flex factor of zero
    const auto& item = *elastic_infos.targets_[idx];
    bool is_inflexible = false;
    if (!base::FloatsLarger(elastic_factor_getter(item), 0.f)) {
      is_inflexible = true;
    }
    // if using the flex grow factor: any item that has a flex base size
    // greater than its hypothetical main size
    else if (is_grow && (elastic_infos.elastic_bases_[idx] >
                         elastic_infos.hypothetical_sizes_[idx])) {
      is_inflexible = true;
    }
    // if using the flex shrink factor: any item that has a flex base size
    // smaller than its hypothetical main size
    else if (!is_grow && (elastic_infos.elastic_bases_[idx] <
                          elastic_infos.hypothetical_sizes_[idx])) {
      is_inflexible = true;
    }
    if (is_inflexible) {
      FreezeItem(idx, elastic_infos, variables, elastic_factor_getter, freeze);
      computed_item_sizes[idx] = elastic_infos.hypothetical_sizes_[idx];
    }
  }  // TODO: min-max clamp for inflexible items

  //    // 9.7-3 Calculate initial free space. Sum the outer sizes of all items
  //    on the
  //    // line, and subtract this from the flex container’s inner main size.
  //    For
  //    // frozen items, use their outer target main size; for other items, use
  //    their
  //    // outer flex base size.
  //    // 9.7-4 Loop
  float remaining_space = CalculateRemainingSpace(
      elastic_infos, available_spaces, computed_item_sizes, freeze);
  variables.initial_free_space_ = remaining_space;
  variables.remaining_space_ = remaining_space;
  while (true) {
    if (!ResolveOneLine(elastic_infos, variables, elastic_factor_getter,
                        computed_item_sizes, freeze, remaining_space)) {
      // until ResolveOneLine returns true;
      break;
    }
    remaining_space = CalculateRemainingSpace(elastic_infos, available_spaces,
                                              computed_item_sizes, freeze);
  }
  return variables.remaining_space_;
}

float ElasticLayoutUtils::ComputeHypotheticalSizes(
    const LayoutItems& targets, const InlineFloatArray& elastic_bases,
    const DirectionSelector& direction_selector,
    InlineFloatArray& hypothetical_sizes) {
  float total_hypothetical_size = 0.f;
  if (targets.size() != elastic_bases.size() ||
      targets.size() != hypothetical_sizes.size()) {
    LOGF("Array sizes mismatch when computing hypothetical sizes");
    return total_hypothetical_size;
  }

  for (size_t idx = 0; idx < elastic_bases.size(); ++idx) {
    hypothetical_sizes[idx] =
        direction_selector.IsHorizontal()
            ? targets[idx]->ClampExactWidth(elastic_bases[idx])
            : targets[idx]->ClampExactHeight(elastic_bases[idx]);
    const auto& margins = targets[idx]->GetBoxInfo()->margin_;
    total_hypothetical_size += hypothetical_sizes[idx] +
                               margins[direction_selector.MainFront()] +
                               margins[direction_selector.MainBack()];
  }
  return total_hypothetical_size;
}

}  // namespace starlight
}  // namespace lynx
