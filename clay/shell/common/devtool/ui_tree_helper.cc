// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/devtool/ui_tree_helper.h"

#include <array>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <string>

#include "clay/gfx/style/color.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/rendering/render_object.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx::tasm::ui_tree {
namespace {

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void WriteString(JsonWriter& writer, const std::string& value) {
  writer.String(value.c_str(), static_cast<rapidjson::SizeType>(value.size()));
}

void WriteFrame(JsonWriter& writer, const clay::BaseView* view) {
  writer.StartArray();
  writer.Double(view->Left());
  writer.Double(view->Top());
  writer.Double(view->Width());
  writer.Double(view->Height());
  writer.EndArray();
}

void WriteUITreeNode(JsonWriter& writer, clay::BaseView* view);

void WriteUITreeChildren(JsonWriter& writer, clay::BaseView* view) {
  for (auto* child : view->GetChildren()) {
    if (child->IsAnonymousView()) {
      WriteUITreeChildren(writer, child);
    } else {
      WriteUITreeNode(writer, child);
    }
  }
}

void WriteUITreeNode(JsonWriter& writer, clay::BaseView* view) {
  writer.StartObject();
  writer.Key("name");
  WriteString(writer, view->GetName());
  writer.Key("id");
  writer.Int(view->id());
  writer.Key("frame");
  WriteFrame(writer, view);
  writer.Key("children");
  writer.StartArray();
  WriteUITreeChildren(writer, view);
  writer.EndArray();
  writer.EndObject();
}

bool IsSeparator(char value) {
  return value == ',' || std::isspace(static_cast<unsigned char>(value));
}

void SkipSeparators(const char*& cursor, const char* end) {
  while (cursor != end && IsSeparator(*cursor)) {
    ++cursor;
  }
}

bool ParseFloatArray(const std::string& content, std::array<float, 4>& values) {
  const char* cursor = content.c_str();
  const char* end = cursor + content.size();
  for (float& value : values) {
    SkipSeparators(cursor, end);
    if (cursor == end) {
      return false;
    }

    errno = 0;
    char* parsed_end = nullptr;
    value = std::strtof(cursor, &parsed_end);
    if (parsed_end == cursor || parsed_end > end || errno == ERANGE ||
        !std::isfinite(value)) {
      return false;
    }
    cursor = parsed_end;
    if (cursor != end && !IsSeparator(*cursor)) {
      return false;
    }
  }

  SkipSeparators(cursor, end);
  return cursor == end;
}

bool ParseRRGGBBAA(const std::string& content, clay::Color& color) {
  size_t character_count = 0;
  for (char value : content) {
    if (!std::isspace(static_cast<unsigned char>(value)) &&
        ++character_count > 9) {
      return false;
    }
  }
  return character_count == 9 && clay::Color::Parse(content, &color);
}

}  // namespace

std::string GetLynxUITree(clay::ViewContext* view_context) {
  auto* root = view_context ? view_context->GetPageView() : nullptr;
  if (!root || root->id() < 0) {
    return {};
  }
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);
  WriteUITreeNode(writer, root);
  return buffer.GetString();
}

std::string GetUINodeInfo(clay::ViewContext* view_context, int id) {
  auto* view = view_context ? view_context->GetViewById(id) : nullptr;
  if (!view) {
    return {};
  }

  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);
  writer.StartObject();
  writer.Key("id");
  writer.Int(view->id());
  writer.Key("editableProps");
  writer.StartObject();
  writer.Key("border");
  writer.StartArray();
  writer.Double(view->BorderTop());
  writer.Double(view->BorderRight());
  writer.Double(view->BorderBottom());
  writer.Double(view->BorderLeft());
  writer.EndArray();
  writer.Key("margin");
  writer.StartArray();
  writer.Double(view->MarginTop());
  writer.Double(view->MarginRight());
  writer.Double(view->MarginBottom());
  writer.Double(view->MarginLeft());
  writer.EndArray();
  writer.Key("frame");
  WriteFrame(writer, view);
  writer.Key("visible");
  writer.Bool(view->Visible());
  writer.EndObject();

  writer.Key("ui");
  writer.StartObject();
  writer.Key("name");
  WriteString(writer, view->GetName());
  writer.Key("readonlyProps");
  writer.StartObject();
  writer.Key("tagName");
  WriteString(writer, view->GetName());
  writer.Key("idSelector");
  WriteString(writer, view->GetIdSelector());
  writer.Key("refIdSelector");
  WriteString(writer, view->GetRefIdSelector());
  writer.Key("attachedToTree");
  writer.Bool(view->attach_to_tree());
  writer.Key("opacity");
  auto* render_object = view->render_object();
  writer.Double(render_object->HasOpacity() ? render_object->Opacity() : 1.0f);
  writer.Key("padding");
  writer.StartArray();
  writer.Double(view->PaddingTop());
  writer.Double(view->PaddingRight());
  writer.Double(view->PaddingBottom());
  writer.Double(view->PaddingLeft());
  writer.EndArray();
  writer.EndObject();
  writer.EndObject();

  writer.Key("view");
  writer.StartObject();
  writer.Key("name");
  writer.String("ClayView");
  writer.Key("readonlyProps");
  writer.StartObject();
  writer.Key("frame");
  WriteFrame(writer, view);
  writer.Key("childCount");
  writer.Uint(static_cast<unsigned int>(view->child_count()));
  writer.EndObject();
  writer.EndObject();
  writer.EndObject();
  return buffer.GetString();
}

int SetUIStyle(clay::ViewContext* view_context, int id, const std::string& name,
               const std::string& content) {
  auto* view = view_context ? view_context->GetViewById(id) : nullptr;
  if (!view) {
    return -1;
  }

  if (name == "frame") {
    std::array<float, 4> values;
    if (!ParseFloatArray(content, values)) {
      return -1;
    }
    view_context->SetBounds(id, values[0], values[1], values[2], values[3]);
  } else if (name == "margin") {
    std::array<float, 4> values;
    if (!ParseFloatArray(content, values)) {
      return -1;
    }
    const float left = view->Left() - view->MarginLeft() + values[3];
    const float top = view->Top() - view->MarginTop() + values[0];
    const float width = view->Width();
    const float height = view->Height();
    view_context->SetMargins(id, values[3], values[0], values[1], values[2]);
    view_context->SetBounds(id, left, top, width, height);
  } else if (name == "border") {
    std::array<float, 4> values;
    if (!ParseFloatArray(content, values)) {
      return -1;
    }
    auto& border = view->render_object()->MutableBorder();
    auto* page = view_context->GetPageView();
    border.width_left_ = page->RoundPixels(values[3]);
    border.width_top_ = page->RoundPixels(values[0]);
    border.width_right_ = page->RoundPixels(values[1]);
    border.width_bottom_ = page->RoundPixels(values[2]);
    view->OnBorderChanged(border);
    view->OnLayoutChange();
  } else if (name == "visible") {
    if (content == "true") {
      view->SetVisible(true);
    } else if (content == "false") {
      view->SetVisible(false);
    } else {
      return -1;
    }
  } else if (name == "background-color" || name == "border-color") {
    clay::Color color;
    if (!ParseRRGGBBAA(content, color)) {
      return -1;
    }
    if (name == "background-color") {
      view->SetBackgroundColor(color);
    } else {
      auto& border = view->render_object()->MutableBorder();
      border.color_left_ = color.argb;
      border.color_top_ = color.argb;
      border.color_right_ = color.argb;
      border.color_bottom_ = color.argb;
      view->OnBorderChanged(border);
    }
  } else {
    return -1;
  }

  view_context->DidUpdateAttributes(id);
  return 0;
}

}  // namespace lynx::tasm::ui_tree
