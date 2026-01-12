// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/layout_context_clay.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/platform_extra_bundle_clay.h"
#include "clay/lynx_adaptor/prop_bundle_impl.h"
#include "clay/ui/common/measure_constraint.h"

namespace {

std::vector<std::string> SeparateString(const std::string& src, char ch,
                                        char end_ch) {
  std::vector<std::string> temp_vec;

  const size_t len = src.length();
  size_t start = 0;
  size_t end = 0;

  while (start < len) {
    end = src.find(ch, start);
    // for url(string), string can have ch
    // inside, so we should skip this case
    while (end != std::string::npos && end > 1 && src[end - 1] != ')') {
      end = src.find(ch, end + 1);
    }
    if (end == std::string::npos) {
      break;
    }
    // skip ','
    if (end - start != 0) {
      temp_vec.emplace_back(src.substr(start, end - start));
    }

    start = end + 1;
  }

  if (start < len) {
    size_t is_end_ch = src.find_last_not_of(end_ch);
    if (is_end_ch == std::string::npos || is_end_ch < start) {
      temp_vec.emplace_back(src.substr(start));
    } else {
      temp_vec.emplace_back(src.substr(start, is_end_ch - start + 1));
    }
  }

  return temp_vec;
}

std::vector<std::string> ParseFontFaceSrcAttr(const std::string& src) {
  // for example:
  // "local("text1"),url("test2")" -->
  // 1. test1
  // 2. test2
  const std::string identify = "http";
  const std::string url_prefix = "url(";
  const std::string local_prefix = "local(";

  // align with lynx behavior
  const std::string asset_prefix = "asset://lynx-fonts/";

  std::vector<std::string> src_vec;
  src_vec = SeparateString(src, ',', ';');
  for (auto& src_str : src_vec) {
    std::string_view str = src_str;
    if (str.empty()) continue;
    str = lynx::base::TrimString(str, " ", lynx::base::TrimPositions::TRIM_ALL);

    size_t is_find_url = str.find(url_prefix);
    if (is_find_url != std::string::npos) {
      str = str.substr(url_prefix.length());
      size_t pos = str.find(')');
      if (pos != std::string::npos) {
        str = str.substr(0, pos);
        str = lynx::base::TrimString(str, "\"",
                                     lynx::base::TrimPositions::TRIM_ALL);
        str = lynx::base::TrimString(str, "\'",
                                     lynx::base::TrimPositions::TRIM_ALL);
      }
      src_str = str;
    } else {
      size_t is_find_local = str.find(local_prefix);
      if (is_find_local != std::string::npos) {
        str = str.substr(local_prefix.length());
        size_t pos = str.find(')');
        if (pos != std::string::npos) {
          str = str.substr(0, pos);
          str = lynx::base::TrimString(str, "\"",
                                       lynx::base::TrimPositions::TRIM_ALL);
          str = lynx::base::TrimString(str, "\'",
                                       lynx::base::TrimPositions::TRIM_ALL);
        }
        src_str = asset_prefix + std::string(str);
      }
    }
  }
  return src_vec;
}

}  // namespace

namespace lynx {
namespace tasm {

class MeasureFuncImpl : public MeasureFunc {
 public:
  MeasureFuncImpl(LayoutContextClay* context, int sign)
      : context_(context), sign_(sign) {}

  LayoutResult Measure(float width, int32_t width_mode, float height,
                       int32_t height_mode, bool final_measure) override {
    LayoutResult result =
        context_->MeasureImpl(sign_, width, width_mode, height, height_mode);
    return result;
  }
  void Alignment() override { context_->AlignmentImpl(sign_); }

 private:
  LayoutContextClay* context_ = nullptr;
  int sign_ = -1;
};

LayoutContextClay::LayoutContextClay(clay::ViewContext* view_context)
    : view_context_(view_context) {
  view_context_->SetLayoutDelegate(this);
}

LayoutContextClay::~LayoutContextClay() {
  view_context_->SetLayoutDelegate(nullptr);
}

void LayoutContextClay::SetLayoutNodeManager(
    LayoutNodeManager* layout_node_manager) {
  layout_node_manager_ = layout_node_manager;
}

int LayoutContextClay::CreateLayoutNode(int id, const std::string& tag,
                                        PropBundle* painting_data,
                                        bool allow_inline) {
  auto node = view_context_->CreateShadowNode(id, tag.c_str(), allow_inline);
  int rule = 0;
  if (node) {
    bool measurable = node->GetMeasurable() || node->GetCustomMeasurable();
    rule = 0x2 | (measurable ? 0x1 : 0x0);
  }

  if (rule & 0x2) {    // is_shadow_node
    if (rule & 0x1) {  // is_measurable
      layout_node_manager_->SetMeasureFunc(
          id, std::make_unique<MeasureFuncImpl>(this, id));
    }
    // update props
    SetAttribute(id, painting_data);
    if (rule & 0x1) {
      return LayoutNodeType::CUSTOM;
    }
    return LayoutNodeType::VIRTUAL | LayoutNodeType::CUSTOM;
  } else if (allow_inline) {
    // update props
    SetAttribute(id, painting_data);
    return LayoutNodeType::INLINE;
  }
  // Set type in LayoutContext::CreateLayoutNodeSync
  return LayoutNodeType::COMMON;
}

void LayoutContextClay::UpdateLayoutNode(int id, PropBundle* painting_data) {
  SetAttribute(id, painting_data);
}

void LayoutContextClay::OnLayoutBefore(int id) {}

void LayoutContextClay::InsertLayoutNode(int parent, int child, int index) {
  view_context_->AddShadowNode(child, parent, index);
}

void LayoutContextClay::RemoveLayoutNode(int parent, int child, int index) {
  view_context_->RemoveShadowNode(child);
}

void LayoutContextClay::OnLayout(int id, float left, float top, float width,
                                 float height,
                                 const std::array<float, 4>& paddings,
                                 const std::array<float, 4>& borders) {
  std::array<float, 4> rounded_paddings;
  std::array<float, 4> rounded_borders;
  rounded_paddings[0] = std::roundf(paddings[0]);
  rounded_paddings[1] = std::roundf(paddings[1]);
  rounded_paddings[2] = std::roundf(paddings[2]);
  rounded_paddings[3] = std::roundf(paddings[3]);
  rounded_borders[0] = std::roundf(borders[0]);
  rounded_borders[1] = std::roundf(borders[1]);
  rounded_borders[2] = std::roundf(borders[2]);
  rounded_borders[3] = std::roundf(borders[3]);
  view_context_->OnLayout(id, std::roundf(width), clay::MeasureMode::kDefinite,
                          std::roundf(height), clay::MeasureMode::kDefinite,
                          rounded_paddings, rounded_borders);
}

void LayoutContextClay::UpdateRootSize(float width, float height) {
  if (width == 0 || height == 0) {
    return;
  }
  view_context_->UpdateRootSize(width, height);
}

void LayoutContextClay::DestroyLayoutNodes(const std::unordered_set<int>& ids) {
  for (auto& child : ids) {
    view_context_->RemoveShadowNode(child);
  }
  for (auto& child : ids) {
    view_context_->DestroyShadowNode(child);
  }
}

void LayoutContextClay::Destroy() { view_context_->DestroyAllShadowNode(); }

std::unique_ptr<PlatformExtraBundle> LayoutContextClay::GetPlatformExtraBundle(
    int32_t id) {
  auto const& bundle = view_context_->GetTextBundle(id);
  if (bundle == nullptr) {
    return nullptr;
  }
  return std::make_unique<PlatformExtraBundleClay>(id, nullptr,
                                                   std::move(bundle));
}

void LayoutContextClay::SetFontFaces(const CSSFontFaceRuleMap& fontfaces) {
  for (auto iter = fontfaces.begin(); iter != fontfaces.end(); ++iter) {
    std::string font_family = iter->first;
    auto token_list = iter->second;
    if (token_list.size() == 0) {
      continue;
    }
    auto token = token_list[0];
    auto src_map = token->second;
    if (src_map.find("src") == src_map.end()) {
      continue;
    }

    std::string src_value = src_map["src"];
    auto src_vec = ParseFontFaceSrcAttr(src_value);
    size_t size = src_vec.size();
    const char* src[size];
    int index = 0;
    for (auto& str : src_vec) {
      src[index++] = str.c_str();
    }
    view_context_->SetFontFace(font_family.c_str(), src, (int)size);
  }
}

LayoutResult LayoutContextClay::MeasureImpl(int sign, int width, int width_mode,
                                            int height, int height_mode) {
  float out_width = 0.f;
  float out_height = 0.f;
  float out_baseline = 0.f;
  view_context_->TextMeasure(
      sign, width, static_cast<clay::MeasureMode>(width_mode), height,
      static_cast<clay::MeasureMode>(height_mode), out_width, out_height);
  out_baseline = view_context_->GetBaseline(sign);
  return LayoutResult{out_width, out_height, out_baseline};
}

void LayoutContextClay::AlignmentImpl(int sign) {
  view_context_->Alignment(sign);
}

void LayoutContextClay::ScheduleLayout() {
  if (has_pending_layout_) {
    view_context_->ScheduleLayout();
    has_pending_layout_ = false;
  }
}

void LayoutContextClay::OnTriggerLayout() {
  layout_proxy_->TriggerLayout();
  has_pending_layout_ = true;
}

void LayoutContextClay::SetAttribute(int sign, PropBundle* attributes) {
  auto pda = static_cast<PropBundleImpl*>(attributes);

  for (auto& event : pda->event_handlers()) {
    view_context_->AddShadowNodeEventProp(sign, event.c_str());
  }

  if (!pda || pda->map().empty()) {
    return;
  }
  auto& map = pda->mutable_map();
  auto iter = map.begin();
  for (; iter != map.end(); iter++) {
    view_context_->SetShadowNodeAttribute(sign, iter->first.c_str(),
                                          iter->second);
  }
}

void LayoutContextClay::OnMarkDirty(int32_t id) {
  layout_node_manager_->MarkDirtyAndRequestLayout(id);
}

void LayoutContextClay::OnAlignNativeNode(int32_t id, float offset_top,
                                          float offset_left) {
  layout_node_manager_->AlignmentByPlatform(id, offset_top, offset_left);
}

ClayMeasureOutput LayoutContextClay::OnMeasureNativeNode(
    int32_t id, float width, int width_mode, float height, int height_mode) {
  auto size = layout_node_manager_->UpdateMeasureByPlatform(
      id, width, width_mode, height, height_mode, true);
  return {size.width_, size.height_, size.baseline_};
}

ClayLayoutStyles LayoutContextClay::OnGetLayoutStyles(int32_t id) {
  ClayLayoutStyles result;
  result.padding_left =
      static_cast<int>(layout_node_manager_->GetPaddingLeft(id));
  result.padding_top =
      static_cast<int>(layout_node_manager_->GetPaddingTop(id));
  result.padding_right =
      static_cast<int>(layout_node_manager_->GetPaddingRight(id));
  result.padding_bottom =
      static_cast<int>(layout_node_manager_->GetPaddingBottom(id));
  result.margin_left =
      static_cast<int>(layout_node_manager_->GetMarginLeft(id));
  result.margin_top = static_cast<int>(layout_node_manager_->GetMarginTop(id));
  result.margin_right =
      static_cast<int>(layout_node_manager_->GetMarginRight(id));
  result.margin_bottom =
      static_cast<int>(layout_node_manager_->GetMarginBottom(id));

  result.width = layout_node_manager_->GetWidth(id);
  result.height = layout_node_manager_->GetHeight(id);

  return result;
}

}  // namespace tasm
}  // namespace lynx
