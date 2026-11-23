// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// SIZE_OK: The bounded v1 compiler keeps semantic and backend error priority
// at one atomic trust boundary.

#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <optional>
#include <utility>

#include "base/include/linked_hash_map.h"
#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fragment/custom_draw_handler.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/starlight/style/borders_data.h"

namespace lynx::tasm::detail {
namespace {
constexpr size_t kMaxTableEntries = 65536;
constexpr size_t kMaxResources = 4096;
constexpr size_t kMaxDepth = 64;
constexpr size_t kMaxPayloadBytes = 16 * 1024 * 1024;
constexpr size_t kMaxValueNodes = 262144;
using Code = DrawRecordingError::Code;
using Transform = std::array<double, 6>;
constexpr Transform kIdentity{1, 0, 0, 1, 0, 0};
struct RectBounds {
  double x{0}, y{0}, width{0}, height{0};
};
struct PathInfo {
  uint32_t tag{0};
  RectBounds bounds;
  double radii[4]{};
  int64_t outer_ref{0}, inner_ref{-1};
  size_t depth{1};
  bool supported{false};
  bool IsBox() const { return tag == 1 || tag == 2; }
};
enum class PaintCapability {
  kSupported,
  kUnsupported,
  kStroke,
  kRadial,
  kSweep
};
struct PaintInfo {
  uint32_t source{0};
  uint32_t argb{0};
  double alpha{1};
  double start[2]{}, end[2]{};
  base::Vector<uint32_t> colors;
  base::Vector<double> stops;
  bool stroke{false};
  PaintCapability capability{PaintCapability::kSupported};
};

struct BoxRecording {
  DrawRecordingContext context;
  double border_width_dp{0};
  uint32_t percent_corners{0};
  uint32_t scope_endpoints{0};
  PathInfo path;
  PaintInfo paint;
};

bool FitsFloat(double value) {
  return std::isfinite(value) &&
         std::abs(value) <= std::numeric_limits<float>::max();
}
bool NearlyEqual(double a, double b, double extent) {
  return std::abs(a - b) <=
         8 * std::numeric_limits<float>::epsilon() *
             std::max({1.0, extent, std::abs(a), std::abs(b)});
}
bool UniformInset(const PathInfo& outer, const PathInfo& inner) {
  const auto& o = outer.bounds;
  const auto& in = inner.bounds;
  const double inset = in.x - o.x, extent = std::max(o.width, o.height);
  if (!std::isfinite(inset) || inset <= 0 || in.width <= 0 || in.height <= 0 ||
      !NearlyEqual(in.y - o.y, inset, extent) ||
      !NearlyEqual(o.width - in.width, 2 * inset, extent) ||
      !NearlyEqual(o.height - in.height, 2 * inset, extent))
    return false;
  double expected[4];
  std::copy(std::begin(outer.radii), std::end(outer.radii), expected);
  for (auto& r : expected) r = std::max(r - inset, 0.0);
  const double side = std::min(in.width, in.height);
  for (int i = 0; i < 2; ++i) {
    const int j = 3 - i;
    const double sum = expected[i] + expected[j];
    if (sum > side) {
      expected[i] *= side / sum;
      expected[j] *= side / sum;
    }
  }
  for (int i = 0; i < 4; ++i)
    if (!NearlyEqual(expected[i], inner.radii[i], extent)) return false;
  return true;
}
uint32_t FoldAlpha(uint32_t argb, double alpha) {
  const auto byte = static_cast<uint32_t>(std::round((argb >> 24) * alpha));
  return (argb & 0x00ffffffu) | (byte << 24);
}
bool LowerLinearPaint(const RoundedRectangle& clip, RoundedRectangle* tile,
                      float* angle, base::Vector<uint32_t>* colors,
                      base::Vector<float>* stops, const float endpoints[4]) {
  const double left = std::min({clip.GetX(), endpoints[0], endpoints[2]});
  const double top = std::min({clip.GetY(), endpoints[1], endpoints[3]});
  const double right = std::max(
      {static_cast<double>(clip.GetX()) + clip.GetWidth(),
       static_cast<double>(endpoints[0]), static_cast<double>(endpoints[2])});
  const double bottom = std::max(
      {static_cast<double>(clip.GetY()) + clip.GetHeight(),
       static_cast<double>(endpoints[1]), static_cast<double>(endpoints[3])});
  if (!FitsFloat(right - left) || !FitsFloat(bottom - top)) return false;
  tile->SetX(static_cast<float>(left));
  tile->SetY(static_cast<float>(top));
  tile->SetWidth(static_cast<float>(right - left));
  tile->SetHeight(static_cast<float>(bottom - top));
  const double dx = static_cast<double>(endpoints[2]) - endpoints[0];
  const double dy = static_cast<double>(endpoints[3]) - endpoints[1];
  if (!FitsFloat(dx) || !FitsFloat(dy)) return false;
  const double degrees = 180.0 / std::acos(-1.0);
  double degrees_angle = std::atan2(dx, -dy) * degrees;
  if (degrees_angle < 0) degrees_angle += 360;
  *angle = static_cast<float>(degrees_angle);
  const double direction_x = std::sin(*angle / degrees);
  const double direction_y = -std::cos(*angle / degrees);
  const double width = tile->GetWidth(), height = tile->GetHeight();
  const double span =
      std::abs(width * direction_x) + std::abs(height * direction_y);
  if (!FitsFloat(span) || span <= 0) return false;
  const double center_x = tile->GetX() + width / 2;
  const double center_y = tile->GetY() + height / 2;
  const double start = ((endpoints[0] - center_x) * direction_x +
                        (endpoints[1] - center_y) * direction_y + span / 2) /
                       span;
  const double scale = (dx * direction_x + dy * direction_y) / span;
  if (!std::isfinite(start) || !std::isfinite(scale) || scale <= 0)
    return false;
  // V1 empty primitives still validate the actual projection before elision.
  if (clip.GetWidth() == 0 || clip.GetHeight() == 0) return true;
  // The caller already charged the two expanded endpoint entries.
  for (auto& stop : (*stops))
    stop = static_cast<float>(std::clamp(start + stop * scale, 0.0, 1.0));
  const uint32_t first = (*colors).front(), last = (*colors).back();
  (*colors).insert((*colors).begin(), first);
  (*colors).push_back(last);
  (*stops).insert((*stops).begin(), 0);
  (*stops).push_back(1);
  return true;
}

class V1Reader {
  // A cursor owns a runtime handle only during decoding. Its parent is an
  // active schema cursor (or a stable payload frame), never part of the IR.
  class Value {
   public:
    Value(lepus::Value source, V1Reader* reader, const Value* parent = nullptr,
          uint32_t slot = 9)
        : source_(std::move(source)),
          reader_(reader),
          parent_(parent),
          depth_(parent ? parent->depth_ + 1 : 1) {
      valid_ = reader_->Observe(*this, slot);
      if (!valid_) reader_->input_failed_ = true;
    }
    bool IsArrayOrJSArray() const {
      return valid_ && source_.IsArrayOrJSArray();
    }
    bool IsString() const { return valid_ && source_.IsString(); }
    bool IsNumber() const { return valid_ && source_.IsNumber(); }
    bool IsBool() const { return valid_ && source_.IsBool(); }
    bool IsNil() const { return valid_ && source_.IsNil(); }
    auto StringView() const { return source_.StringView(); }
    double Number() const { return source_.Number(); }
    bool Bool() const { return source_.Bool(); }
    int64_t GetLength() const { return valid_ ? length_ : -1; }
    Value GetProperty(uint32_t slot) const {
      return Value(valid_ && !reader_->failed_ ? source_.GetProperty(slot)
                                               : lepus::Value(),
                   reader_, this, slot);
    }

   private:
    friend class V1Reader;
    lepus::Value source_;
    V1Reader* reader_;
    const Value* parent_;
    size_t depth_;
    int64_t length_{-1};
    bool valid_{false};
  };

 public:
  V1Reader(double pixels, ComposeDrawPhase phase,
           BoxRecording* box_recording = nullptr)
      : pixels_(pixels), phase_(phase), box_recording_(box_recording) {}
  std::optional<DrawRecordingError> Read(const lepus::Value& wire,
                                         DisplayList* out, bool* shadow) {
    if (!ChargeNodes(1, kMaxValueNodes, &snapshot_nodes_)) return error_;
    const Value input(wire, this);
    if (!Row(input, 9)) return error_;
    const auto discriminator = input.GetProperty(0u);
    if (!discriminator.IsString() ||
        discriminator.StringView() != "lynx.draw.recording") {
      Fail(Code::kVersion);
      return error_;
    }
    uint32_t major, minor;
    if (!UInt(input.GetProperty(1u), &major) || major != 1) {
      Fail(Code::kVersion);
      return error_;
    }
    if (!UInt(input.GetProperty(2u), &minor)) return error_;
    const auto commands = input.GetProperty(3u);
    const auto paths = input.GetProperty(4u);
    const auto paints = input.GetProperty(5u);
    const auto resources = input.GetProperty(6u);
    if (!Table(commands) || !Table(paths) || !Table(paints) ||
        !Table(resources))
      return error_;
    resource_count_ = static_cast<size_t>(resources.GetLength());
    if (box_recording_ &&
        (commands.GetLength() != 1 || paths.GetLength() != 1 ||
         paints.GetLength() != 1 || resource_count_ != 0))
      return DrawRecordingError{Code::kMalformed};
    paths_.reserve(paths.GetLength());
    paints_.reserve(paints.GetLength());
    clip_phase_error_ = phase_ == ComposeDrawPhase::kClip &&
                        (commands.GetLength() != 1 || paints.GetLength() != 0);
    for (uint32_t i = 0; i < static_cast<uint32_t>(paths.GetLength()); ++i)
      if (!Path(paths.GetProperty(i))) return error_;
    for (uint32_t i = 0; i < resource_count_; ++i) {
      const auto row = resources.GetProperty(i);
      std::string name;
      uint32_t version;
      if (!Row(row, 3) || !Text(row.GetProperty(0u), &name, true) ||
          !UInt(row.GetProperty(1u), &version) ||
          !ValidateValue(row.GetProperty(2u)))
        return error_;
      Unknown(std::move(name));
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(paints.GetLength()); ++i) {
      PaintInfo paint;
      if (!Paint(paints.GetProperty(i), &paint)) return error_;
      if (paint.capability != PaintCapability::kSupported) {
        if (paint_capability_ == PaintCapability::kSupported)
          paint_capability_ = paint.capability;
        const bool stroke = paint.stroke;
        const auto capability = paint.capability;
        paint = {};
        paint.stroke = stroke;
        paint.capability = capability;
      }
      paints_.push_back(std::move(paint));
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(commands.GetLength()); ++i) {
      error_.command_index = i;
      if (!Command(commands.GetProperty(i), i)) return error_;
    }
    if (!state_.empty()) {
      Fail();
      return error_;
    }
    error_.command_index = DrawRecordingError::kNoCommand;
    if (!Metadata(input.GetProperty(8u))) return error_;
    if (!unknown_.empty()) {
      return DrawRecordingError{Code::kUnsupported,
                                DrawRecordingError::kNoCommand,
                                std::move(unknown_)};
    }
    if (!Features(input.GetProperty(7u)) || failed_) return error_;
    if (path_capability_error_) return DrawRecordingError{Code::kUnsupported};
    if (paint_capability_ != PaintCapability::kSupported) {
      const char* ns =
          paint_capability_ == PaintCapability::kStroke   ? "paint.stroke"
          : paint_capability_ == PaintCapability::kRadial ? "paint.radial"
          : paint_capability_ == PaintCapability::kSweep  ? "paint.sweep"
                                                          : "";
      return DrawRecordingError{Code::kUnsupported,
                                DrawRecordingError::kNoCommand, ns};
    }
    if (clip_phase_error_) return DrawRecordingError{Code::kUnsupported};
    if (lower_error_) return lower_error_;
    if (box_recording_) {
      box_recording_->path = paths_[0];
      box_recording_->paint = std::move(paints_[0]);
    } else {
      *out = scratch_.Build();
      *shadow = has_shadow_;
    }
    return std::nullopt;
  }

  bool CompileBox(const BoxRecording& box_recording, double width,
                  double height, double density, DisplayList* output) {
    PathInfo path = box_recording.path;
    path.bounds = {0, 0, width, height};
    const float side = static_cast<float>(std::min(width, height));
    for (size_t i = 0; i < 4; ++i) {
      const size_t logical = box_recording.context.rtl ? (i ^ 1u) : i;
      const double authored = box_recording.path.radii[logical];
      const bool percent =
          (box_recording.percent_corners & (1u << logical)) != 0;
      path.radii[i] = percent ? side * (static_cast<float>(authored) / 100.f)
                              : authored * density;
      if (!FitsFloat(path.radii[i])) return false;
    }
    NormalizeBoxRadii(path, side);
    RoundedRectangle clip, inner;
    if (!Geometry(path, &clip)) return false;
    const double dp = box_recording.border_width_dp;
    const double border_width_px =
        phase_ == ComposeDrawPhase::kForeground
            ? (dp == 0 ? 1.f : std::ceil(dp * density))
            : 0;
    const float inset =
        static_cast<float>(std::min(border_width_px, std::ceil(side / 2.0)));
    const float inner_width = static_cast<float>(width) - 2.f * inset;
    const float inner_height = static_cast<float>(height) - 2.f * inset;
    const bool has_inner = inset > 0 && inner_width > 0 && inner_height > 0;
    if (has_inner) {
      path.bounds = {inset, inset, inner_width, inner_height};
      for (double& radius : path.radii)
        radius = std::max(0.f, static_cast<float>(radius) - inset);
      NormalizeBoxRadii(path, std::min(inner_width, inner_height));
      if (!Geometry(path, &inner)) return false;
    }
    if (!DrawBox(clip, inner, has_inner, box_recording.paint,
                 box_recording.scope_endpoints, width, height))
      return false;
    *output = scratch_.Build();
    return true;
  }

 private:
  static void NormalizeBoxRadii(PathInfo& path, float side) {
    for (size_t i = 0; i < 2; ++i) {
      const double sum = path.radii[i] + path.radii[3 - i];
      if (sum > side) {
        path.radii[i] = static_cast<float>(path.radii[i] * (side / sum));
        path.radii[3 - i] =
            static_cast<float>(path.radii[3 - i] * (side / sum));
      }
    }
  }

  bool Fail(Code code = Code::kMalformed) {
    if (!input_failed_) error_.code = code;
    failed_ = true;
    return false;
  }
  bool Row(const Value& a, size_t size) {
    return (a.IsArrayOrJSArray() &&
            static_cast<size_t>(a.GetLength()) == size) ||
           Fail();
  }
  bool Table(const Value& a) { return a.IsArrayOrJSArray() || Fail(); }
  bool Number(const Value& v, double* out) {
    if (!v.IsNumber()) return Fail();
    *out = v.Number();
    return true;
  }
  bool Nonnegative(const Value& v, double* out, bool positive = false) {
    return Number(v, out) && ((*out >= 0 && (!positive || *out > 0)) || Fail());
  }
  bool UInt(const Value& v, uint32_t* out) {
    double n;
    if (!Number(v, &n) || n < 0 || n > UINT32_MAX || std::trunc(n) != n)
      return Fail();
    *out = static_cast<uint32_t>(n);
    return true;
  }
  bool Tag(const Value& row, uint32_t* tag) {
    if (!row.IsArrayOrJSArray() || row.GetLength() < 1) return Fail();
    return UInt(row.GetProperty(0u), tag);
  }
  bool Boolean(const Value& v, bool* out) {
    if (!v.IsBool()) return Fail();
    *out = v.Bool();
    return true;
  }
  bool Text(const Value& v, std::string* out, bool nonempty = false) {
    if (!v.IsString()) return Fail();
    auto text = v.StringView();
    if (nonempty && text.empty()) return Fail();
    out->assign(text.data(), text.size());
    return true;
  }
  template <typename Enum>
  bool EnumValue(const Value& v, uint32_t max, Enum* out) {
    uint32_t n;
    if (!UInt(v, &n)) return false;
    if (n > max) return Fail(Code::kUnsupported);
    *out = static_cast<Enum>(n);
    return true;
  }
  bool Ref(const Value& v, size_t bound, int64_t* out, bool optional = false) {
    double n;
    if (!Number(v, &n) || std::trunc(n) != n) return Fail();
    if (optional && n == -1) {
      *out = -1;
      return true;
    }
    if (n < 0 || n >= static_cast<double>(bound)) return Fail();
    *out = static_cast<int64_t>(n);
    return true;
  }
  bool Bounds(const Value& a, uint32_t start, RectBounds* b) {
    return Number(a.GetProperty(start), &b->x) &&
           Number(a.GetProperty(start + 1), &b->y) &&
           Nonnegative(a.GetProperty(start + 2), &b->width) &&
           Nonnegative(a.GetProperty(start + 3), &b->height) &&
           ((std::isfinite(b->x + b->width) &&
             std::isfinite(b->y + b->height)) ||
            Fail(Code::kOverflow));
  }
  template <typename Array>
  bool Numbers(const Value& a, uint32_t start, Array* values) {
    for (uint32_t i = 0; i < std::size(*values); ++i)
      if (!Number(a.GetProperty(start + i), &(*values)[i])) return false;
    return true;
  }
  bool Matrix(const Value& a, Transform* matrix) {
    return Row(a, 6) && Numbers(a, 0, matrix);
  }

  bool ChargeNodes(size_t added, size_t limit, size_t* used) {
    if (added > limit - *used) return Fail(Code::kCapacity);
    *used += added;
    return true;
  }
  size_t SnapshotTableCapacity(uint32_t slot) const {
    switch (slot) {
      case 3:
      case 4:
      case 5:
        return kMaxTableEntries;
      case 6:
        return kMaxResources;
      default:
        return std::numeric_limits<size_t>::max();
    }
  }
  bool Observe(Value& value, uint32_t slot) {
    if (failed_) return false;
    const auto& source = value.source_;
    if (source.IsArrayOrJSArray()) {
      // Charge the entire child range before reading any child or reserving
      // typed storage. Expanded DAG occurrences consume the same budget.
      if (value.depth_ > kMaxDepth) return Fail(Code::kCapacity);
      uint64_t identity;
      std::memcpy(&identity, &source.value(), sizeof(identity));
      for (auto* parent = value.parent_; parent; parent = parent->parent_) {
        uint64_t ancestor;
        std::memcpy(&ancestor, &parent->source_.value(), sizeof(ancestor));
        if (identity == ancestor &&
            source.IsJSValue() == parent->source_.IsJSValue() &&
            (!source.IsJSValue() || source.env() == parent->source_.env()))
          return Fail();
      }
      value.length_ = source.GetLength();
      if (value.length_ < 0) return Fail();
      const auto n = static_cast<size_t>(value.length_);
      if (value.depth_ == 2 && n > SnapshotTableCapacity(slot))
        return Fail(Code::kCapacity);
      return ChargeNodes(n, kMaxValueNodes, &snapshot_nodes_);
    }
    size_t bytes = 8;
    if (source.IsString())
      bytes = source.StringView().size();
    else if (source.IsNumber()) {
      if (!std::isfinite(source.Number())) return Fail();
    } else if (!source.IsNil() && !source.IsBool())
      return Fail();
    return ChargeNodes(bytes, kMaxPayloadBytes, &payload_bytes_);
  }

  bool ValidateValue(Value input) {
    struct Frame {
      Value source;
      uint32_t next{0};
    };
    // Observe caps array depth, so parent cursors stay in this inline buffer.
    base::InlineVector<Frame, kMaxDepth> stack;
    const auto visit = [&](Value value) {
      if (value.IsArrayOrJSArray()) {
        stack.push_back(Frame{std::move(value), 0});
      } else if (!value.IsString() && !value.IsBool() && !value.IsNumber() &&
                 !value.IsNil()) {
        return Fail();
      }
      return true;
    };
    if (!visit(std::move(input))) return false;
    while (!stack.empty()) {
      auto& frame = stack.back();
      if (frame.next == static_cast<uint32_t>(frame.source.GetLength())) {
        stack.pop_back();
        continue;
      }
      if (!visit(frame.source.GetProperty(frame.next++))) return false;
    }
    return true;
  }
  void Unknown(std::string name) {
    if (unknown_.empty()) unknown_ = std::move(name);
  }
  bool Extension(const Value& row, size_t ref_bound, bool tagged = false,
                 size_t* path_depth = nullptr) {
    uint32_t tag, version;
    std::string name;
    if (!Row(row, 5) || (!tagged && (!Tag(row, &tag) || tag != 255)) ||
        !Text(row.GetProperty(1u), &name, true) ||
        !UInt(row.GetProperty(2u), &version))
      return false;
    const auto refs = row.GetProperty(4u);
    if (!Table(refs)) return false;
    for (uint32_t i = 0; i < static_cast<uint32_t>(refs.GetLength()); ++i) {
      int64_t ref;
      if (!Ref(refs.GetProperty(i), ref_bound, &ref)) return false;
      if (path_depth)
        *path_depth = std::max(*path_depth, paths_[ref].depth + 1);
    }
    if (!ValidateValue(row.GetProperty(3u))) return false;
    Unknown(std::move(name));
    return true;
  }
  bool BoxCorners(const Value& row, uint32_t offset, PathInfo* path) {
    uint32_t mask;
    if (!UInt(row.GetProperty(offset), &mask) || mask > 15) return Fail();
    path->tag = 2;
    for (uint32_t i = 0; i < 4; ++i) {
      double& radius = path->radii[i];
      const bool percent = (mask & (1u << i)) != 0;
      if (!Nonnegative(row.GetProperty(offset + 1 + i), &radius) ||
          !FitsFloat(radius) || (percent && radius > 100))
        return Fail();
      if (percent) box_recording_->percent_corners |= 1u << i;
    }
    return true;
  }
  bool LogicalBoxPath(const Value& row, PathInfo* path) {
    if (!ScopeExtension(row, "lynx.logical-box")) return false;
    const auto payload = row.GetProperty(3u);
    if (!Table(payload) ||
        (payload.GetLength() != 1 && payload.GetLength() != 6))
      return Fail();
    const auto width = payload.GetProperty(0u);
    if (phase_ == ComposeDrawPhase::kForeground) {
      if (!Nonnegative(width, &box_recording_->border_width_dp) ||
          !FitsFloat(box_recording_->border_width_dp))
        return Fail();
    } else if (!width.IsNil()) {
      return Fail();
    }
    path->tag = 1;
    return payload.GetLength() == 1 || BoxCorners(payload, 1, path);
  }
  bool Path(const Value& row) {
    PathInfo path;
    if (!Tag(row, &path.tag)) return false;
    if (box_recording_) {
      if (!LogicalBoxPath(row, &path)) return false;
      path.supported = true;
      paths_.push_back(path);
      return true;
    }
    switch (path.tag) {
      case 1:
        if (!Row(row, 5) || !Bounds(row, 1, &path.bounds)) return false;
        path.supported = true;
        break;
      case 2: {
        double radii[8]{};
        if (!Row(row, 13) || !Bounds(row, 1, &path.bounds)) return false;
        for (uint32_t i = 0; i < 8; ++i)
          if (!Nonnegative(row.GetProperty(i + 5), &radii[i])) return false;
        for (const auto& pair : {std::pair{0, 2}, {6, 4}, {1, 7}, {3, 5}})
          if (!std::isfinite(radii[pair.first] + radii[pair.second]))
            return Fail(Code::kOverflow);
        path.supported = true;
        for (size_t i = 0; i < 4; ++i) {
          path.radii[i] = radii[2 * i];
          path.supported &= radii[2 * i] == radii[2 * i + 1];
        }
        break;
      }
      case 3:
        if (!Row(row, 3) ||
            !Ref(row.GetProperty(1u), paths_.size(), &path.outer_ref) ||
            !Ref(row.GetProperty(2u), paths_.size(), &path.inner_ref, true))
          return false;
        path.depth += paths_[path.outer_ref].depth;
        path.supported = paths_[path.outer_ref].IsBox();
        if (path.inner_ref != -1) {
          path.depth = std::max(path.depth, paths_[path.inner_ref].depth + 1);
          path.supported =
              path.supported && paths_[path.inner_ref].IsBox() &&
              UniformInset(paths_[path.outer_ref], paths_[path.inner_ref]);
        }
        break;
      case 4: {
        uint32_t fill_rule;
        if (!Row(row, 3) || !EnumValue(row.GetProperty(1u), 1, &fill_rule))
          return false;
        const auto verbs = row.GetProperty(2u);
        if (!Table(verbs)) return false;
        bool open = false;
        for (uint32_t i = 0; i < static_cast<uint32_t>(verbs.GetLength());
             ++i) {
          const auto verb = verbs.GetProperty(i);
          uint32_t tag;
          if (!Tag(verb, &tag)) return false;
          uint32_t coordinates;
          switch (tag) {
            case 1:
            case 2:
              coordinates = 2;
              break;
            case 3:
              coordinates = 4;
              break;
            case 4:
              coordinates = 6;
              break;
            case 5:
              coordinates = 5;
              break;
            case 6:
              coordinates = 0;
              break;
            default:
              return Fail(Code::kUnsupported);
          }
          if (!Row(verb, coordinates + 1) || (!open && tag != 1)) return Fail();
          for (uint32_t j = 0; j < coordinates; ++j) {
            double value;
            if (!Number(verb.GetProperty(j + 1), &value)) return false;
            if (tag == 5 && j == 4 && value <= 0) return Fail();
          }
          open = tag != 6;
        }
        break;
      }
      case 255:
        if (!Extension(row, paths_.size(), true, &path.depth)) return false;
        break;
      default:
        return Fail(Code::kUnsupported);
    }
    if (path.depth > kMaxDepth) return Fail(Code::kCapacity);
    path_capability_error_ |= !path.supported;
    paths_.push_back(path);
    return true;
  }
  bool Stops(const Value& colors, const Value& stops, PaintInfo* paint) {
    if (!Table(colors) || !Table(stops)) return false;
    if (colors.GetLength() < 2 || colors.GetLength() != stops.GetLength())
      return Fail();
    if (paint) {
      paint->colors.reserve(colors.GetLength());
      paint->stops.reserve(stops.GetLength());
    }
    double previous = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(colors.GetLength()); ++i) {
      uint32_t color;
      double stop = 0;
      if (!UInt(colors.GetProperty(i), &color) ||
          !Number(stops.GetProperty(i), &stop) || stop < previous || stop > 1)
        return Fail();
      previous = stop;
      if (paint) {
        paint->colors.push_back(color);
        paint->stops.push_back(stop);
      }
    }
    return true;
  }
  bool ScopeExtension(const Value& row, const char* expected) {
    uint32_t version, tag;
    if (!Row(row, 5) || !Tag(row, &tag) || tag != 255) return Fail();
    const auto name = row.GetProperty(1u);
    if (!name.IsString() || name.StringView() != expected ||
        !UInt(row.GetProperty(2u), &version) || version != 1)
      return Fail(Code::kUnsupported);
    return Row(row.GetProperty(4u), 0);
  }
  bool Source(const Value& row, PaintInfo* paint) {
    if (!Tag(row, &paint->source)) return false;
    switch (paint->source) {
      case 1:
        return Row(row, 2) && UInt(row.GetProperty(1u), &paint->argb);
      case 2: {
        uint32_t tile = 0;
        Transform transform;
        if (!Row(row, 9) || !Numbers(row, 1, &paint->start) ||
            !Numbers(row, 3, &paint->end))
          return false;
        if (paint->start[0] == paint->end[0] &&
            paint->start[1] == paint->end[1])
          return Fail();
        if (!Stops(row.GetProperty(5u), row.GetProperty(6u), paint) ||
            !EnumValue(row.GetProperty(7u), 3, &tile) ||
            !Matrix(row.GetProperty(8u), &transform))
          return false;
        if (tile != 0 || transform != kIdentity)
          paint->capability = PaintCapability::kUnsupported;
        return true;
      }
      case 3: {
        double center[2];
        double radius;
        uint32_t tile = 0;
        Transform transform;
        if (!Row(row, 8) || !Numbers(row, 1, &center) ||
            !Nonnegative(row.GetProperty(3u), &radius, true) ||
            !Stops(row.GetProperty(4u), row.GetProperty(5u), nullptr) ||
            !EnumValue(row.GetProperty(6u), 3, &tile) ||
            !Matrix(row.GetProperty(7u), &transform))
          return false;
        paint->capability = PaintCapability::kRadial;
        return true;
      }
      case 4: {
        double center[2];
        Transform transform;
        if (!Row(row, 6) || !Numbers(row, 1, &center) ||
            !Stops(row.GetProperty(3u), row.GetProperty(4u), nullptr) ||
            !Matrix(row.GetProperty(5u), &transform))
          return false;
        paint->capability = PaintCapability::kSweep;
        return true;
      }
      case 255:
        if (box_recording_) {
          if (!ScopeExtension(row, "lynx.scope-linear")) return false;
          const auto payload = row.GetProperty(3u);
          if (!Row(payload, 6)) return false;
          for (uint32_t i = 0; i < 4; ++i) {
            const auto coord = payload.GetProperty(i);
            if (coord.IsNil()) {
              box_recording_->scope_endpoints |= 1u << i;
            } else if (!Number(coord,
                               i < 2 ? &paint->start[i] : &paint->end[i - 2])) {
              return false;
            }
          }
          paint->source = 2;
          return Stops(payload.GetProperty(4u), payload.GetProperty(5u), paint);
        }
        paint->capability = PaintCapability::kUnsupported;
        return Extension(row, resource_count_, true);
      default:
        return Fail(Code::kUnsupported);
    }
  }
  bool Filter(const Value& row) {
    if (row.IsNil()) return true;
    uint32_t tag;
    if (!Tag(row, &tag)) return false;
    switch (tag) {
      case 1: {
        uint32_t color, blend;
        return Row(row, 3) && UInt(row.GetProperty(1u), &color) &&
               EnumValue(row.GetProperty(2u), 2, &blend);
      }
      case 2: {
        const auto matrix = row.GetProperty(1u);
        double values[20];
        return Row(row, 2) && Row(matrix, 20) && Numbers(matrix, 0, &values);
      }
      case 255:
        return Extension(row, resource_count_, true);
      default:
        return Fail(Code::kUnsupported);
    }
  }
  bool Style(const Value& row, PaintInfo* paint) {
    uint32_t tag;
    if (!Tag(row, &tag)) return false;
    if (tag == 1) return Row(row, 1);
    if (tag != 2) return Fail(Code::kUnsupported);
    paint->stroke = true;
    double width, miter, phase;
    uint32_t cap, join;
    if (!Row(row, 8) || !Nonnegative(row.GetProperty(1u), &width) ||
        !EnumValue(row.GetProperty(2u), 2, &cap) ||
        !EnumValue(row.GetProperty(3u), 2, &join) ||
        !Nonnegative(row.GetProperty(4u), &miter, true) ||
        !Number(row.GetProperty(6u), &phase))
      return false;
    const auto dashes = row.GetProperty(5u);
    if (!Table(dashes) || dashes.GetLength() % 2 != 0) return Fail();
    for (uint32_t i = 0; i < static_cast<uint32_t>(dashes.GetLength()); ++i) {
      double value;
      if (!Nonnegative(dashes.GetProperty(i), &value, true)) return false;
    }
    const auto effect = row.GetProperty(7u);
    return effect.IsNil() || Extension(effect, resource_count_);
  }
  bool Paint(const Value& row, PaintInfo* paint) {
    if (!Row(row, 4) || !Source(row.GetProperty(0u), paint)) return false;
    const auto options = row.GetProperty(1u);
    uint32_t blend = 0;
    bool antialias = false;
    if (!Row(options, 4) || !Number(options.GetProperty(0u), &paint->alpha) ||
        !EnumValue(options.GetProperty(1u), 2, &blend) ||
        !Boolean(options.GetProperty(2u), &antialias))
      return false;
    const auto filter = options.GetProperty(3u);
    if (!Filter(filter) || !Style(row.GetProperty(2u), paint)) return false;
    paint->alpha = std::clamp(paint->alpha, 0.0, 1.0);
    const auto effects = row.GetProperty(3u);
    if (!Table(effects)) return false;
    for (uint32_t i = 0; i < static_cast<uint32_t>(effects.GetLength()); ++i)
      if (!Extension(effects.GetProperty(i), resource_count_)) return false;
    if (paint->stroke)
      paint->capability = PaintCapability::kStroke;
    else if (paint->capability == PaintCapability::kSupported &&
             (blend != 0 || !antialias || !filter.IsNil() ||
              effects.GetLength()))
      paint->capability = PaintCapability::kUnsupported;
    return true;
  }
  struct State {
    uint32_t tag;
    Transform matrix;
  };
  bool Push(uint32_t tag) {
    if (state_.size() >= kMaxDepth) return Fail(Code::kCapacity);
    state_.push_back(State{tag, matrix_});
    return true;
  }
  bool Pop(uint32_t tag) {
    if (state_.empty() || state_.back().tag != tag) return Fail();
    matrix_ = state_.back().matrix;
    state_.pop_back();
    return true;
  }
  bool Concat(const Transform& n) {
    const auto& m = matrix_;
    Transform next{
        m[0] * n[0] + m[2] * n[1],        m[1] * n[0] + m[3] * n[1],
        m[0] * n[2] + m[2] * n[3],        m[1] * n[2] + m[3] * n[3],
        m[0] * n[4] + m[2] * n[5] + m[4], m[1] * n[4] + m[3] * n[5] + m[5]};
    for (double v : next)
      if (!std::isfinite(v)) return Fail(Code::kOverflow);
    matrix_ = next;
    return true;
  }

  bool DrawRefs(const Value& row, int64_t* path, int64_t* paint) {
    if (!Ref(row.GetProperty(1u), paths_.size(), path) ||
        !Ref(row.GetProperty(2u), paints_.size(), paint))
      return false;
    if (paths_[*path].tag == 3 && paints_[*paint].stroke)
      return Fail(Code::kUnsupported);
    return true;
  }
  bool CanLower() const {
    return !path_capability_error_ &&
           paint_capability_ == PaintCapability::kSupported &&
           !clip_phase_error_ && !lower_error_;
  }
  bool Command(const Value& row, size_t index) {
    uint32_t tag;
    if (!Tag(row, &tag)) return false;
    if (phase_ == ComposeDrawPhase::kClip && tag != 5) clip_phase_error_ = true;
    if (box_recording_ && tag != 1) return Fail(Code::kUnsupported);
    switch (tag) {
      case 1: {
        int64_t path = 0, paint = 0;
        if (!Row(row, 3) || !DrawRefs(row, &path, &paint)) return false;
        if (!box_recording_ && CanLower()) Draw(path, paint);
        return true;
      }
      case 2:
        return Row(row, 1) && Push(2);
      case 3:
        return Row(row, 1) && Pop(2);
      case 4: {
        Transform transform;
        if (!Row(row, 7) || !Numbers(row, 1, &transform) || !Concat(transform))
          return false;
        if (CanLower() && transform != kIdentity) LowerFail(Code::kUnsupported);
        return true;
      }
      case 5: {
        int64_t path = 0;
        uint32_t op = 0;
        bool antialias = false;
        if (!Row(row, 4) || !Ref(row.GetProperty(1u), paths_.size(), &path) ||
            !EnumValue(row.GetProperty(2u), 1, &op) ||
            !Boolean(row.GetProperty(3u), &antialias))
          return false;
        if (CanLower()) {
          if (phase_ != ComposeDrawPhase::kClip || op != 0 || !antialias ||
              !paths_[path].IsBox()) {
            LowerFail(Code::kUnsupported, "drawing.clip");
          } else {
            RoundedRectangle clip;
            if (Geometry(paths_[path], &clip)) scratch_.ClipRect(clip);
          }
        }
        return true;
      }
      case 6: {
        const auto bounds = row.GetProperty(1u);
        RectBounds box;
        int64_t paint;
        if (!Row(row, 3) || !Row(bounds, 4) || !Bounds(bounds, 0, &box) ||
            !Ref(row.GetProperty(2u), paints_.size(), &paint) || !Push(6))
          return false;
        if (CanLower()) LowerFail(Code::kUnsupported);
        return true;
      }
      case 7:
        if (!Row(row, 1) || !Pop(6)) return false;
        if (CanLower()) LowerFail(Code::kUnsupported);
        return true;
      case 8:
        if (!Row(row, 1) || !content_) return Fail();
        if (CanLower()) LowerFail(Code::kUnsupported);
        return true;
      case 9: {
        int64_t path = 0, paint = 0;
        double x = 0, y = 0, blur = 0, spread = 0;
        uint32_t kind = 0, caster = 0;
        if (!Row(row, 9) || !DrawRefs(row, &path, &paint) ||
            !Number(row.GetProperty(3u), &x) ||
            !Number(row.GetProperty(4u), &y) ||
            !Nonnegative(row.GetProperty(5u), &blur) ||
            !Number(row.GetProperty(6u), &spread) ||
            !EnumValue(row.GetProperty(7u), 1, &kind) ||
            !EnumValue(row.GetProperty(8u), 1, &caster))
          return false;
        if (CanLower()) {
          if (!paths_[path].IsBox() || kind != 0 || caster != 1 ||
              paints_[paint].source != 1)
            LowerFail(Code::kUnsupported);
          else
            Shadow(paths_[path], paints_[paint], x, y, blur, spread);
        }
        return true;
      }
      case 10:
        if (!Row(row, 1) || index != 0) return Fail();
        content_ = true;
        if (CanLower()) LowerFail(Code::kUnsupported);
        return true;
      case 255:
        return Extension(row, resource_count_, true);
      default:
        return Fail(Code::kUnsupported);
    }
  }
  bool Metadata(const Value& rows) {
    if (!Table(rows)) return false;
    base::LinkedHashMap<std::string, bool> keys;
    for (uint32_t i = 0; i < static_cast<uint32_t>(rows.GetLength()); ++i) {
      const auto row = rows.GetProperty(i);
      std::string key;
      if (!Row(row, 2) || !Text(row.GetProperty(0u), &key)) return false;
      if (!keys.insert_default_if_absent(std::move(key)).second) return Fail();
      if (!ValidateValue(row.GetProperty(1u))) return false;
    }
    return true;
  }
  bool Features(const Value& rows) {
    if (!Table(rows)) return false;
    for (uint32_t i = 0; i < static_cast<uint32_t>(rows.GetLength()); ++i) {
      const auto name = rows.GetProperty(i);
      if (!name.IsString()) return Fail();
    }
    return true;
  }

  bool LowerFail(Code code = Code::kMalformed, const char* ns = "") {
    if (!lower_error_)
      lower_error_ = DrawRecordingError{code, error_.command_index, ns};
    return false;
  }
  bool ChargeData(size_t bytes) {
    if (bytes > static_cast<size_t>(INT32_MAX) - bytes_)
      return LowerFail(Code::kCapacity);
    bytes_ += bytes;
    return true;
  }
  bool Float(double physical, float* output) {
    if (!FitsFloat(physical / pixels_)) return LowerFail(Code::kOverflow);
    *output = static_cast<float>(physical / pixels_);
    return true;
  }
  bool Geometry(const PathInfo& path, RoundedRectangle* output) {
    const auto* b = &path.bounds;
    float x, y, w, h;
    if (!Float(b->x, &x) || !Float(b->y, &y) || !Float(b->width, &w) ||
        !Float(b->height, &h))
      return false;
    if (!FitsFloat(b->x / pixels_ + b->width / pixels_) ||
        !FitsFloat(b->y / pixels_ + b->height / pixels_) ||
        !std::isfinite(x + w) || !std::isfinite(y + h))
      return LowerFail(Code::kOverflow);
    output->SetX(x);
    output->SetY(y);
    output->SetWidth(w);
    output->SetHeight(h);
    float r[4]{};
    const auto& radii = path.radii;
    for (size_t i = 0; i < 4; ++i) {
      if (!Float(radii[i], &r[i])) return false;
    }
    output->SetRadiusXTopLeft(r[0]);
    output->SetRadiusYTopLeft(r[0]);
    output->SetRadiusXTopRight(r[1]);
    output->SetRadiusYTopRight(r[1]);
    output->SetRadiusXBottomRight(r[2]);
    output->SetRadiusYBottomRight(r[2]);
    output->SetRadiusXBottomLeft(r[3]);
    output->SetRadiusYBottomLeft(r[3]);
    return true;
  }

  bool Draw(int64_t path_ref, int64_t paint_ref) {
    const auto* path = &paths_[path_ref];
    RoundedRectangle clip, inner;
    const bool has_inner = path->tag == 3 && path->inner_ref != -1;
    if (path->tag == 3) {
      if (has_inner && !Geometry(paths_[path->inner_ref], &inner)) return false;
      path = &paths_[path->outer_ref];
    }
    if (!Geometry(*path, &clip)) return false;
    return DrawBox(clip, inner, has_inner, paints_[paint_ref]);
  }
  bool DrawBox(const RoundedRectangle& clip, const RoundedRectangle& inner,
               bool has_inner, const PaintInfo& paint,
               uint32_t scope_endpoints = 0, double width = 0,
               double height = 0) {
    RoundedRectangle tile;
    const bool empty = clip.GetWidth() == 0 || clip.GetHeight() == 0;
    base::Vector<uint32_t> colors;
    base::Vector<float> stops;
    float angle = 180;
    if (paint.source == 1) {
      if (empty) return true;
      const uint32_t color = FoldAlpha(paint.argb, paint.alpha);
      int32_t clip_index;
      scratch_.RecordBoxModel(clip, clip_index);
      if (!has_inner) {
        scratch_.Fill(color, clip_index);
        return true;
      }
      int32_t inner_index;
      scratch_.RecordBoxModel(inner, inner_index);
      starlight::BordersData border;
      border.color_top = border.color_right = border.color_bottom =
          border.color_left = color;
      border.style_top = border.style_right = border.style_bottom =
          border.style_left = starlight::BorderStyleType::kSolid;
      scratch_.Border(clip_index, inner_index, border);
      return true;
    }
    const size_t n = paint.colors.size();
    float endpoints[4];
    for (size_t i = 0; i < 4; ++i) {
      double coord = i < 2 ? paint.start[i] : paint.end[i - 2];
      if (scope_endpoints & (1u << i)) coord = i % 2 == 0 ? width : height;
      if (!Float(coord, &endpoints[i])) return false;
    }
    if (endpoints[0] == endpoints[2] && endpoints[1] == endpoints[3])
      return LowerFail(Code::kOverflow);
    if (!empty) {
      if (!ChargeData((n + 2) * 8)) return false;
      colors.reserve(n + 2);
      stops.reserve(n + 2);
      for (uint32_t color : paint.colors)
        colors.push_back(FoldAlpha(color, paint.alpha));
      for (double stop : paint.stops) stops.push_back(static_cast<float>(stop));
    }
    if (!LowerLinearPaint(clip, &tile, &angle, &colors, &stops, endpoints))
      return LowerFail(Code::kOverflow);
    if (empty) return true;
    int32_t clip_index, tile_index, inner_index = -1;
    scratch_.RecordBoxModel(clip, clip_index);
    scratch_.RecordBoxModel(tile, tile_index);
    if (has_inner) scratch_.RecordBoxModel(inner, inner_index);
    scratch_.LinearGradient(angle, colors, stops, tile_index, clip_index, 1, 1,
                            inner_index);
    return true;
  }
  bool Shadow(const PathInfo& path, const PaintInfo& paint, double offset_x,
              double offset_y, double blur_physical, double spread_physical) {
    RoundedRectangle clip, shadow;
    float x, y, blur, spread;
    if (!Geometry(path, &clip) || !Float(offset_x, &x) ||
        !Float(offset_y, &y) || !Float(blur_physical, &blur) ||
        !Float(spread_physical, &spread))
      return false;
    if (!Fragment::PrepareCustomOutsetShadow(clip, x, y, spread, &shadow))
      return LowerFail(Code::kOverflow);
    if (clip.GetWidth() == 0 || clip.GetHeight() == 0) return true;
    int32_t clip_index, shadow_index;
    scratch_.RecordBoxModel(clip, clip_index);
    scratch_.RecordBoxModel(shadow, shadow_index);
    scratch_.BoxShadow(shadow_index, clip_index,
                       FoldAlpha(paint.argb, paint.alpha), blur,
                       DisplayListBuilder::BoxShadowClipMode::kOutset);
    has_shadow_ = true;
    return true;
  }
  double pixels_;
  ComposeDrawPhase phase_;
  BoxRecording* box_recording_;
  size_t bytes_{0};
  DisplayListBuilder scratch_;
  base::Vector<PathInfo> paths_;
  base::Vector<PaintInfo> paints_;
  base::Vector<State> state_;
  Transform matrix_{kIdentity};
  DrawRecordingError error_;
  std::optional<DrawRecordingError> lower_error_;
  PaintCapability paint_capability_{PaintCapability::kSupported};
  std::string unknown_;
  size_t resource_count_{0};
  size_t snapshot_nodes_{0}, payload_bytes_{0};
  bool failed_{false}, input_failed_{false}, content_{false};
  bool path_capability_error_{false}, clip_phase_error_{false},
      has_shadow_{false};
};
}  // namespace

namespace {
class BoxRecordingDrawHandler final : public CustomDrawHandler {
 public:
  BoxRecordingDrawHandler(BoxRecording box_recording, ComposeDrawPhase phase)
      : CustomDrawHandler(phase == ComposeDrawPhase::kForeground ? kForeground
                                                                 : kBehind),
        box_recording_(std::move(box_recording)),
        phase_(phase) {}

  void Draw(Fragment& fragment, DisplayListBuilder& builder,
            Phase /*phase*/) override {
    auto* element = fragment.element();
    const auto& env = element->element_manager()->GetLynxEnvConfig();
    const double pixels = env.PhysicalPixelsPerLayoutUnit();
    const double width = element->width(), height = element->height();
    if (!std::isfinite(pixels) || pixels <= 0 || width <= 0 || height <= 0 ||
        !FitsFloat(width * pixels) || !FitsFloat(height * pixels))
      return;
    if (phase_ == ComposeDrawPhase::kBehind && box_recording_.path.tag == 1 &&
        box_recording_.paint.source == 1) {
      RoundedRectangle box;
      box.SetWidth(width);
      box.SetHeight(height);
      int32_t index;
      builder.RecordBoxModel(box, index);
      builder.Fill(
          FoldAlpha(box_recording_.paint.argb, box_recording_.paint.alpha),
          index);
      return;
    }
    const double density = box_recording_.context.density_override.value_or(
        env.DevicePixelRatio());
    if (!std::isfinite(density) || density <= 0) return;
    const std::array<double, 4> key{width, height, pixels, density};
    if (!cached_ || cached_key_ != key) {
      DisplayList list;
      if (!V1Reader(pixels, phase_)
               .CompileBox(box_recording_, static_cast<float>(width * pixels),
                           static_cast<float>(height * pixels), density, &list))
        return;
      cached_.emplace(std::move(list));
      cached_key_ = key;
    }
    if (!builder.AppendDrawingContent(*cached_)) {
      LOGE("box recording append rejected: capacity exceeded");
    }
  }

 private:
  BoxRecording box_recording_;
  ComposeDrawPhase phase_;
  std::optional<DisplayList> cached_;
  std::array<double, 4> cached_key_{};
};
}  // namespace

std::optional<DrawRecordingError> BindBoxRecording(
    const lepus::Value& input, ComposeDrawPhase phase,
    const DrawRecordingContext& context,
    fml::RefPtr<CustomDrawHandler>* handler) {
  BoxRecording box_recording;
  box_recording.context = context;
  if (auto error =
          V1Reader(1, phase, &box_recording).Read(input, nullptr, nullptr))
    return error;
  *handler = fml::MakeRefCounted<BoxRecordingDrawHandler>(
      std::move(box_recording), phase);
  return std::nullopt;
}

std::optional<DrawRecordingError> DecodeDrawDisplayList(
    const lepus::Value& input, double pixels, ComposeDrawPhase phase,
    DisplayList* output, bool* has_outset_shadow) {
  if (!std::isfinite(pixels) || pixels <= 0) return DrawRecordingError{};
  return V1Reader(pixels, phase).Read(input, output, has_outset_shadow);
}
}  // namespace lynx::tasm::detail
