// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/path_harmony_node.h"

#include <cmath>
#include <initializer_list>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <utility>

#include "gfx/public/path_parser.h"

namespace lynx::tasm {

struct HarmonyNodePathDescriptor::Builder {
  float scale;
  bool valid{true};
  bool in_path{false};
  bool draws{false};
  std::vector<Command> commands;

  void Add(char verb, std::initializer_list<float> args) {
    if (verb == 'M') {
      in_path = true;
    } else if (!in_path) {
      valid = false;
      return;
    }
    Command command{verb, static_cast<unsigned>(args.size()), {}};
    unsigned index = 0;
    for (float arg : args) {
      // Arc rotation and flags are not coordinates or lengths.
      float value = verb == 'A' && index >= 2 && index <= 4 ? arg : arg * scale;
      if (!std::isfinite(value)) valid = false;
      command.args[index++] = value;
    }
    if (verb != 'M' && verb != 'Z') draws = true;
    commands.push_back(command);
  }

  LBSPathConsumer Consumer() {
    LBSPathConsumer consumer{};
    consumer.ctx = this;
    consumer.MoveToPoint = [](void* ctx, float x, float y) {
      static_cast<Builder*>(ctx)->Add('M', {x, y});
    };
    consumer.LineToPoint = [](void* ctx, float x, float y) {
      static_cast<Builder*>(ctx)->Add('L', {x, y});
    };
    consumer.QuadToPoint = [](void* ctx, float cx, float cy, float x, float y) {
      static_cast<Builder*>(ctx)->Add('Q', {cx, cy, x, y});
    };
    consumer.CubicToPoint = [](void* ctx, float ax, float ay, float bx,
                               float by, float x, float y) {
      static_cast<Builder*>(ctx)->Add('C', {ax, ay, bx, by, x, y});
    };
    consumer.EllipticToPoint = [](void* ctx, float, float, float rx, float ry,
                                  float angle, bool large, bool sweep, float x,
                                  float y) {
      static_cast<Builder*>(ctx)->Add(
          'A', {std::abs(rx), std::abs(ry), angle, static_cast<float>(large),
                static_cast<float>(sweep), x, y});
    };
    consumer.ClosePath = [](void* ctx) {
      static_cast<Builder*>(ctx)->Add('Z', {});
    };
    return consumer;
  }

  void Read(gfx::PathView view) {
    const auto& header = view.header;
    if (header.kind == gfx::PathKind::kRect ||
        header.kind == gfx::PathKind::kRoundedRect) {
      const float x = header.x, y = header.y;
      const float right = x + header.width, bottom = y + header.height;
      if (!std::isfinite(header.width) || !std::isfinite(header.height) ||
          header.width < 0 || header.height < 0) {
        valid = false;
        return;
      }
      if (header.kind == gfx::PathKind::kRect) {
        Add('M', {x, y});
        Add('L', {right, y});
        Add('L', {right, bottom});
        Add('L', {x, bottom});
      } else {
        const float* r = header.radii;
        for (unsigned i = 0; i < 8; ++i) {
          if (!std::isfinite(r[i]) || r[i] < 0) valid = false;
        }
        Add('M', {x + r[0], y});
        Add('L', {right - r[2], y});
        Add('A', {r[2], r[3], 0, 0, 1, right, y + r[3]});
        Add('L', {right, bottom - r[5]});
        Add('A', {r[4], r[5], 0, 0, 1, right - r[4], bottom});
        Add('L', {x + r[6], bottom});
        Add('A', {r[6], r[7], 0, 0, 1, x, bottom - r[7]});
        Add('L', {x, y + r[1]});
        Add('A', {r[0], r[1], 0, 0, 1, x + r[0], y});
      }
      Add('Z', {});
      draws = header.width > 0 && header.height > 0;
      return;
    }
    if (header.kind != gfx::PathKind::kCommands ||
        (header.command_count && !view.commands) ||
        (header.point_count && !view.points)) {
      valid = false;
      return;
    }
    uint32_t point_index = 0;
    for (uint32_t i = 0; i < header.command_count && valid; ++i) {
      const auto verb = view.commands[i];
      unsigned count = 0;
      char command = 0;
      switch (verb) {
        case gfx::PathVerb::kBegin:
          count = 1;
          command = 'M';
          break;
        case gfx::PathVerb::kLine:
          count = 1;
          command = 'L';
          break;
        case gfx::PathVerb::kQuadratic:
          count = 2;
          command = 'Q';
          break;
        case gfx::PathVerb::kCubic:
          count = 3;
          command = 'C';
          break;
        case gfx::PathVerb::kClose:
          Add('Z', {});
          continue;
        case gfx::PathVerb::kEnd:
          in_path = false;
          continue;
        default:
          valid = false;
          continue;
      }
      if (count > header.point_count - point_index) {
        valid = false;
        break;
      }
      const auto* p = view.points + point_index;
      if (count == 1) Add(command, {p[0].x, p[0].y});
      if (count == 2) Add(command, {p[0].x, p[0].y, p[1].x, p[1].y});
      if (count == 3)
        Add(command, {p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y});
      point_index += count;
    }
    valid = valid && point_index == header.point_count;
  }
};

HarmonyNodePathDescriptor HarmonyNodePathDescriptor::Build(
    gfx::PathSourceView source, gfx::PathFillRule fill_rule, float density) {
  if (fill_rule != gfx::PathFillRule::kNonzero) {
    return HarmonyNodePathDescriptor(HarmonyNodePathStatus::kUnsupported);
  }
  if (!std::isfinite(density) || density <= 0) {
    return HarmonyNodePathDescriptor(HarmonyNodePathStatus::kInvalid);
  }
  Builder builder{density};
  if (const auto* svg = std::get_if<gfx::SvgPathView>(&source)) {
    builder.scale *= svg->source_to_layout_scale;
    if (!std::isfinite(builder.scale) || builder.scale <= 0 ||
        svg->data.find('\0') != std::string::npos) {
      return HarmonyNodePathDescriptor(HarmonyNodePathStatus::kInvalid);
    }
    auto consumer = builder.Consumer();
    LBSParsePathWithConsumer(svg->data.c_str(), &consumer);
    builder.valid = builder.valid && !consumer.error;
  } else {
    builder.Read(std::get<gfx::PathView>(source));
  }
  if (!builder.valid) {
    return HarmonyNodePathDescriptor(HarmonyNodePathStatus::kInvalid);
  }
  HarmonyNodePathDescriptor descriptor(builder.draws
                                           ? HarmonyNodePathStatus::kReady
                                           : HarmonyNodePathStatus::kEmpty);
  descriptor.commands_ = std::move(builder.commands);
  return descriptor;
}

HarmonyNodePathPayload HarmonyNodePathDescriptor::MakePayload(
    float x, float y, float width, float height) const {
  if (status_ == HarmonyNodePathStatus::kInvalid ||
      status_ == HarmonyNodePathStatus::kUnsupported) {
    return {status_, {}};
  }
  if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) ||
      !std::isfinite(height) || width <= 0 || height <= 0) {
    return {HarmonyNodePathStatus::kInvalid, {}};
  }
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(std::numeric_limits<float>::max_digits10);
  for (const auto& command : commands_) {
    output << command.verb;
    for (unsigned i = 0; i < command.count; ++i) {
      float value = command.args[i];
      if (command.verb != 'A') {
        value += i % 2 == 0 ? x : y;
      } else if (i >= 5) {
        value += i == 5 ? x : y;
      }
      if (!std::isfinite(value)) return {HarmonyNodePathStatus::kInvalid, {}};
      if (i != 0) output << ' ';
      output << value;
    }
  }
  return {status_, output.str(), width, height};
}

}  // namespace lynx::tasm
