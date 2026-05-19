// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/ui_delegate_harmony.h"

#include <arkui/native_node.h>

#include <cctype>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/ui_wrapper/layout/harmony/layout_context_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/painting_context_harmony.h"
#include "core/shell/harmony/embedder_platform_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {
namespace harmony {

struct NumSpec {
  const char* key;
  ArkUI_NodeAttributeType attr;
  int index;
  bool is_int;
};

struct StrSpec {
  const char* key;
  ArkUI_NodeAttributeType attr;
  int index;
};

struct ArrSpec {
  const char* key;
  ArkUI_NodeAttributeType attr;
  int count;
};

static const NumSpec kNumSpecs[] = {
    {"width", NODE_WIDTH, 0, false},
    {"height", NODE_HEIGHT, 0, false},
    {"x", NODE_POSITION, 0, false},
    {"y", NODE_POSITION, 1, false},
    {"visibility", NODE_VISIBILITY, 0, true},
    {"opacity", NODE_OPACITY, 0, false},
    {"clip", NODE_CLIP, 0, true},
    {"renderGroup", NODE_RENDER_GROUP, 0, true},
    {"zIndex", NODE_Z_INDEX, 0, true},
    {"transformCenterX", NODE_TRANSFORM_CENTER, 0, false},
    {"transformCenterY", NODE_TRANSFORM_CENTER, 1, false},
    {"grayScale", NODE_GRAY_SCALE, 0, false},
    {"blur", NODE_BLUR, 0, false},
    {"brightness", NODE_BRIGHTNESS, 0, false},
    {"contrast", NODE_CONTRAST, 0, false},
    {"saturation", NODE_SATURATION, 0, false},
    {"hitTestBehavior", NODE_HIT_TEST_BEHAVIOR, 0, true},
    {"visibleAreaChangeRatio", NODE_VISIBLE_AREA_CHANGE_RATIO, 0, false},
    {"clipShapeType", NODE_CLIP_SHAPE, 0, true},
};

static const StrSpec kStrSpecs[] = {
    {"id", NODE_ID, 0},
    {"clipShapePath", NODE_CLIP_SHAPE, 0},
};

static const ArrSpec kArrSpecs[] = {
    {"position", NODE_POSITION, 2},
    {"size", NODE_SIZE, 2},
    {"translate", NODE_TRANSLATE, 3},
    {"transformMatrix", NODE_TRANSFORM, 16},
    {"borderRadius", NODE_BORDER_RADIUS, 4},
};

static inline bool IsHexRRGGBBAA(const std::string& s) {
  auto t = lynx::base::RemoveSpaces(s);
  if (t.size() != 9 || t[0] != '#') return false;
  for (size_t i = 1; i < 9; ++i) {
    if (!std::isxdigit(static_cast<unsigned char>(t[i]))) return false;
  }
  return true;
}

static inline std::string HexFromARGB(uint32_t color) {
  lynx::tasm::CSSValue v(color, lynx::tasm::CSSValuePattern::NUMBER);
  return lynx::tasm::CSSDecoder::CSSValueNumberToString(
      lynx::tasm::kPropertyIDColor, v);
}

static void WriteUISection(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                           tasm::harmony::UIBase* ui) {
  writer.Key("tag");
  {
    const auto& tag = ui->Tag();
    writer.String(tag.c_str(), static_cast<rapidjson::SizeType>(tag.size()));
  }
  writer.Key("translateZ");
  writer.Double(ui->TranslateZ());
  writer.Key("overlayContent");
  writer.String(ui->IsOverlayContent() ? "true" : "false");
  writer.Key("overflowX");
  writer.String(ui->Overflow().overflow_x ? "true" : "false");
  writer.Key("overflowY");
  writer.String(ui->Overflow().overflow_y ? "true" : "false");
  {
    uint32_t top = ui->GetBorderTopColor();
    uint32_t right = ui->GetBorderRightColor();
    uint32_t bottom = ui->GetBorderBottomColor();
    uint32_t left = ui->GetBorderLeftColor();
    writer.Key("border-color");
    writer.StartArray();
    {
      auto s = HexFromARGB(top);
      writer.String(s.c_str(), static_cast<rapidjson::SizeType>(s.size()));
    }
    {
      auto s = HexFromARGB(right);
      writer.String(s.c_str(), static_cast<rapidjson::SizeType>(s.size()));
    }
    {
      auto s = HexFromARGB(bottom);
      writer.String(s.c_str(), static_cast<rapidjson::SizeType>(s.size()));
    }
    {
      auto s = HexFromARGB(left);
      writer.String(s.c_str(), static_cast<rapidjson::SizeType>(s.size()));
    }
    writer.EndArray();
  }
}

static void WriteNodeSection(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             tasm::harmony::UIBase* ui) {
  auto node = ui->DrawNode();
  auto& nm = tasm::harmony::NodeManager::Instance();
  auto write_str = [&](const char* key, const std::string& s) {
    writer.Key(key);
    writer.String(s.c_str(), static_cast<rapidjson::SizeType>(s.size()));
  };
  auto write_float = [&](const char* key, float v) {
    auto sv = std::to_string(v);
    writer.Key(key);
    writer.String(sv.c_str(), static_cast<rapidjson::SizeType>(sv.size()));
  };
  auto write_int = [&](const char* key, int v) {
    auto sv = std::to_string(v);
    writer.Key(key);
    writer.String(sv.c_str(), static_cast<rapidjson::SizeType>(sv.size()));
  };
  auto write_num_attr = [&](const char* key, ArkUI_NodeAttributeType attr,
                            int index, bool is_int) {
    if (is_int) {
      write_int(key, nm.GetAttribute<int>(node, attr, index));
    } else {
      write_float(key, nm.GetAttribute<float>(node, attr, index));
    }
  };
  for (const auto& s : kNumSpecs) {
    write_num_attr(s.key, s.attr, s.index, s.is_int);
  }
  for (const auto& s : kStrSpecs) {
    auto val = nm.GetAttribute<std::string>(node, s.attr, s.index);
    if (!val.empty()) {
      write_str(s.key, val);
    }
  }
  {
    auto bg = nm.GetAttribute<uint32_t>(node, NODE_BACKGROUND_COLOR, 0);
    auto bgs = HexFromARGB(bg);
    writer.Key("background-color");
    writer.String(bgs.c_str(), static_cast<rapidjson::SizeType>(bgs.size()));
  }
  auto write_float_array = [&](const char* key, ArkUI_NodeAttributeType attr,
                               int count) {
    auto item = nm.GetAttribute(node, attr);
    if (item && item->size >= count) {
      writer.Key(key);
      writer.StartArray();
      for (int i = 0; i < count; ++i) {
        auto sv = std::to_string(item->value[i].f32);
        writer.String(sv.c_str(), static_cast<rapidjson::SizeType>(sv.size()));
      }
      writer.EndArray();
    }
  };
  for (const auto& s : kArrSpecs) {
    write_float_array(s.key, s.attr, s.count);
  }
}

static void GetAllPropertyAndValues(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    tasm::harmony::UIBase* ui, const char* type,
    shell::EmbedderPlatformHarmony* platform) {
  writer.StartObject();
  if (std::strcmp(type, "ui") == 0) {
    WriteUISection(writer, ui);
  } else {
    WriteNodeSection(writer, ui);
  }
  writer.EndObject();
}

std::unique_ptr<PaintingCtxPlatformImpl>
UIDelegateHarmony::CreatePaintingContext() {
  return std::make_unique<PaintingContextHarmony>(ui_owner_);
}

std::unique_ptr<LayoutCtxPlatformImpl>
UIDelegateHarmony::CreateLayoutContext() {
  return std::make_unique<LayoutContextHarmony>(node_owner_);
}

std::unique_ptr<PropBundleCreator>
UIDelegateHarmony::CreatePropBundleCreator() {
  return std::make_unique<PropBundleCreatorHarmony>();
}

std::unique_ptr<runtime::NativeModuleFactory>
UIDelegateHarmony::GetCustomModuleFactory() {
  return std::move(module_factory_);
}

void UIDelegateHarmony::OnLynxCreate(
    const std::shared_ptr<shell::ListEngineProxy>& list_engine_proxy,
    const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
    const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
    const std::shared_ptr<shell::LynxLayoutProxy>& layout_proxy,
    const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
    const std::shared_ptr<shell::EventTrackerProxy>& event_tracker_proxy,
    const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
    const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
    const fml::RefPtr<fml::TaskRunner>& layout_task_runner, int32_t instance_id,
    bool is_embedded_mode) {
  auto lynx_context = lynx_context_.lock();
  if (!lynx_context) {
    return;
  }
  lynx_context->OnLynxCreate(
      list_engine_proxy, engine_proxy, runtime_proxy, perf_controller_proxy,
      event_tracker_proxy, resource_loader, ui_task_runner, layout_task_runner);
  node_owner_->SetTriggerLayoutCallback(
      [layout_proxy]() { layout_proxy->TriggerLayout(); });
}

void UIDelegateHarmony::OnUpdateScreenMetrics(float width, float height,
                                              float device_pixel_ratio) {
  screen_width_ = width;
  screen_height_ = height;
  device_pixel_ratio_ = device_pixel_ratio;
  auto lynx_context = lynx_context_.lock();
  if (lynx_context && lynx_context->GetExtensionDelegate()) {
    lynx_context->GetExtensionDelegate()->OnDevicePixelRatioChanged(
        device_pixel_ratio);
  }
}

void UIDelegateHarmony::OnPageConfigDecoded(
    const std::shared_ptr<PageConfig>& config) {
  auto lynx_context = lynx_context_.lock();
  if (lynx_context) {
    lynx_context->GetFluencyTraceHelper().SetPageConfigProbability(
        config->GetEnableScrollFluencyMonitor());
    lynx_context->SetEnableTextOverflow(config->GetEnableTextOverflow());
    lynx_context->SetTapSlop(config->GetTapSlop());
    lynx_context->SetHasTouchPseudo(config->GetEnableFiberArch());
    lynx_context->SetLongPressDuration(config->GetLongPressDuration());
    lynx_context->SetEnableHarmonyNewOverlay(
        config->GetEnableHarmonyNewOverlay() ||
        LynxEnv::GetInstance().EnableHarmonyNewOverlay());
    if (config->GetEnableNewGesture() && ui_owner_) {
      ui_owner_->InitGestureArenaManager(lynx_context.get());
    }
    if (config->GetSyncXElementRegistry() && ui_owner_) {
      ui_owner_->SetEnableSyncXElementRegistry();
    }
    lynx_context->SetEnableEventThrough(config->GetEnableEventThrough());
    lynx_context->SetEnableHarmonyVisibleAreaChangeForExposure(
        config->GetEnableHarmonyVisibleAreaChangeForExposure());
    lynx_context->SetEnableMultiTouch(config->GetEnableMultiTouch());
    lynx_context->SetEnableExposureWhenReload(
        config->GetEnableExposureWhenReload());
    lynx_context->SetEnableTransformedTouchPosition(
        config->GetEnableTransformedTouchPosition());
  }
}

void UIDelegateHarmony::TakeSnapshot(
    size_t max_width, size_t max_height, int quality, float screen_scale_factor,
    const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
    TakeSnapshotCompletedCallback callback) {
  if (!platform_) return;
  platform_->TakeSnapshot(max_width, max_height, quality, screenshot_runner,
                          std::move(callback));
}

int UIDelegateHarmony::GetNodeForLocation(int x, int y) {
  auto lynx_context = lynx_context_.lock();
  if (lynx_context) {
    auto root = lynx_context->GetUIOwner()->Root();
    float pos[2] = {
        static_cast<float>(x) / root->GetContext()->ScaledDensity(),
        static_cast<float>(y) / root->GetContext()->ScaledDensity()};
    tasm::harmony::EventTarget* ui = root->HitTest(pos);
    return ui ? ui->Sign() : 0;
  }
  return 0;
}

std::vector<float> UIDelegateHarmony::GetTransformValue(
    int id, const std::vector<float>& pad_border_margin_layout) {
  std::vector<float> res(32, 0);
  std::vector<float> point(8, 0);
  auto lynx_context = lynx_context_.lock();
  if (!lynx_context) {
    return {};
  }
  auto ui = lynx_context->GetUIOwner()->FindUIBySign(id);
  if (!ui) {
    return {};
  }
  for (int i = 0; i < 4; ++i) {
    if (i == 0) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::PAD_LEFT] +
              pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::PAD_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::PAD_TOP] +
              pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::PAD_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    } else if (i == 1) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    } else if (i == 2) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM], point);
    } else {
      ui->GetTransformValue(
          -pad_border_margin_layout[BoxModelOffset::MARGIN_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          pad_border_margin_layout[BoxModelOffset::MARGIN_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          -pad_border_margin_layout[BoxModelOffset::MARGIN_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          pad_border_margin_layout[BoxModelOffset::MARGIN_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    }
    res[i * 8] = point[0];
    res[i * 8 + 1] = point[1];
    res[i * 8 + 2] = point[2];
    res[i * 8 + 3] = point[3];
    res[i * 8 + 4] = point[4];
    res[i * 8 + 5] = point[5];
    res[i * 8 + 6] = point[6];
    res[i * 8 + 7] = point[7];
  }
  return res;
}

std::string UIDelegateHarmony::GetLynxUITree() {
  std::string res;
  auto lynx_context = lynx_context_.lock();
  if (lynx_context) {
    auto root = lynx_context->GetUIOwner()->Root();
    if (root) {
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      std::function<void(tasm::harmony::UIBase*)> write_node =
          [&](tasm::harmony::UIBase* ui) {
            writer.StartObject();
            writer.Key("name");
            const auto& tag = ui->Tag();
            writer.String(tag.c_str(),
                          static_cast<rapidjson::SizeType>(tag.size()));
            writer.Key("id");
            writer.Int(ui->Sign());
            writer.Key("frame");
            writer.StartArray();
            const double frame_vals[4] = {ui->left_, ui->top_, ui->width_,
                                          ui->height_};
            for (double v : frame_vals) {
              writer.Double(v);
            }
            writer.EndArray();
            writer.Key("children");
            writer.StartArray();
            for (auto* child : ui->Children()) {
              write_node(child);
            }
            writer.EndArray();
            writer.EndObject();
          };
      write_node(root);
      res = buffer.GetString();
    }
  }
  return res;
}

std::string UIDelegateHarmony::GetUINodeInfo(int id) {
  std::string res;
  auto lynx_context = lynx_context_.lock();
  if (!lynx_context) {
    return res;
  }
  auto ui = lynx_context->GetUIOwner()->FindUIBySign(id);
  if (!ui) {
    return res;
  }
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.StartObject();
  writer.Key("id");
  writer.Int(ui->Sign());
  writer.Key("editableProps");
  writer.StartObject();
  writer.Key("border");
  writer.StartArray();
  writer.Double(ui->GetBorderTopWidth());
  writer.Double(ui->GetBorderRightWidth());
  writer.Double(ui->GetBorderBottomWidth());
  writer.Double(ui->GetBorderLeftWidth());
  writer.EndArray();
  writer.Key("margin");
  writer.StartArray();
  writer.Double(ui->margin_top_);
  writer.Double(ui->margin_right_);
  writer.Double(ui->margin_bottom_);
  writer.Double(ui->margin_left_);
  writer.EndArray();
  writer.Key("frame");
  writer.StartArray();
  writer.Double(ui->left_);
  writer.Double(ui->top_);
  writer.Double(ui->width_);
  writer.Double(ui->height_);
  writer.EndArray();
  writer.Key("visible");
  writer.Bool(ui->IsVisible());
  writer.EndObject();

  writer.Key("ui");
  writer.StartObject();
  writer.Key("name");
  {
    const auto& tag = ui->Tag();
    writer.String(tag.c_str(), static_cast<rapidjson::SizeType>(tag.size()));
  }
  writer.Key("readonlyProps");
  GetAllPropertyAndValues(writer, ui, "ui", platform_);
  writer.EndObject();

  writer.Key("view");
  writer.StartObject();
  writer.Key("name");
  writer.String("ArkUI_Node");
  writer.Key("readonlyProps");
  GetAllPropertyAndValues(writer, ui, "view", platform_);
  writer.EndObject();
  writer.EndObject();
  res = buffer.GetString();
  return res;
}

static std::vector<float> ParseFloatArray(const std::string& s) {
  auto parts = lynx::base::SplitString<std::string, std::string>(
      s, std::string(" ,\t\n"));
  std::vector<float> vals;
  vals.reserve(parts.size());
  for (const auto& p : parts) {
    if (p.empty()) {
      continue;
    }
    float v = 0.f;
    if (!lynx::base::StringToFloat(p, v)) {
      return {};
    }
    vals.push_back(v);
  }
  return vals;
}

static uint32_t ParseColorString(const std::string& s) {
  tasm::CSSParserConfigs configs;
  tasm::CSSStringParser parser(s.c_str(), static_cast<uint32_t>(s.size()),
                               configs);
  auto v = parser.ParseCSSColor();
  if (!v.IsEmpty() && v.IsNumber()) {
    auto lv = v.GetValue();
    return lv.UInt32();
  }
  return 0;
}

int UIDelegateHarmony::SetUIStyle(int id, const std::string& name,
                                  const std::string& content) {
  auto lynx_context = lynx_context_.lock();
  if (!lynx_context) {
    return -1;
  }
  auto ui = lynx_context->GetUIOwner()->FindUIBySign(id);
  if (!ui) {
    return -1;
  }
  if (name == "frame") {
    auto vals = ParseFloatArray(content);
    if (vals.size() != 4) {
      return -1;
    }
    float paddings[4] = {ui->padding_left_, ui->padding_top_,
                         ui->padding_right_, ui->padding_bottom_};
    float margins[4] = {ui->margin_left_, ui->margin_top_, ui->margin_right_,
                        ui->margin_bottom_};
    float sticky[4] = {0, 0, 0, 0};
    ui->UpdateLayout(vals[0], vals[1], vals[2], vals[3], paddings, margins,
                     sticky, vals[3], 0);
    return 0;
  }
  if (name == "margin") {
    auto vals = ParseFloatArray(content);
    if (vals.size() != 4) {
      return -1;
    }
    float paddings[4] = {ui->padding_left_, ui->padding_top_,
                         ui->padding_right_, ui->padding_bottom_};
    float margins[4] = {vals[3], vals[0], vals[1], vals[2]};
    float sticky[4] = {0, 0, 0, 0};
    float new_left = ui->left_ - ui->margin_left_ + vals[3];
    float new_top = ui->top_ - ui->margin_top_ + vals[0];
    ui->UpdateLayout(new_left, new_top, ui->width_, ui->height_, paddings,
                     margins, sticky, ui->height_, 0);
    return 0;
  }
  if (name == "border") {
    auto vals = ParseFloatArray(content);
    if (vals.size() != 4) {
      return -1;
    }
    auto arr = lepus::CArray::Create();
    arr->push_back(lepus::Value(vals[3]));  // left
    arr->push_back(lepus::Value(vals[1]));  // right
    arr->push_back(lepus::Value(vals[0]));  // top
    arr->push_back(lepus::Value(vals[2]));  // bottom
    std::unordered_map<std::string, lepus::Value> props;
    props.emplace("border-left-width", lepus::Value(vals[3]));
    props.emplace("border-right-width", lepus::Value(vals[1]));
    props.emplace("border-top-width", lepus::Value(vals[0]));
    props.emplace("border-bottom-width", lepus::Value(vals[2]));
    tasm::PropBundleHarmony bundle(
        props, base::flex_optional<std::vector<lepus::Value>>());
    ui->UpdateProps(&bundle);
    ui->OnNodeReady();
    return 0;
  }
  if (name == "background-color") {
    if (!IsHexRRGGBBAA(content)) {
      return -1;
    }
    uint32_t c = ParseColorString(lynx::base::RemoveSpaces(content));
    std::unordered_map<std::string, lepus::Value> props;
    props.emplace("background-color", lepus::Value(static_cast<double>(c)));
    tasm::PropBundleHarmony bundle(
        props, base::flex_optional<std::vector<lepus::Value>>());
    ui->UpdateProps(&bundle);
    ui->OnNodeReady();
    return 0;
  }
  if (name == "border-color") {
    if (!IsHexRRGGBBAA(content)) {
      return -1;
    }
    uint32_t c = ParseColorString(lynx::base::RemoveSpaces(content));
    std::unordered_map<std::string, lepus::Value> props;
    props.emplace("border-color", lepus::Value(static_cast<double>(c)));
    tasm::PropBundleHarmony bundle(
        props, base::flex_optional<std::vector<lepus::Value>>());
    ui->UpdateProps(&bundle);
    ui->OnNodeReady();
    return 0;
  }
  if (name == "visible") {
    bool vis = !(content == "false" || content == "0");
    int v = vis ? static_cast<int>(starlight::VisibilityType::kVisible)
                : static_cast<int>(starlight::VisibilityType::kHidden);
    std::unordered_map<std::string, lepus::Value> props;
    props.emplace("visibility", lepus::Value(static_cast<double>(v)));
    tasm::PropBundleHarmony bundle(
        props, base::flex_optional<std::vector<lepus::Value>>());
    ui->UpdateProps(&bundle);
    return 0;
  }
  return -1;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
