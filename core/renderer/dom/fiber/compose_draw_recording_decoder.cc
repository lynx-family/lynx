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
#include <utility>

#include "base/include/linked_hash_map.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx::tasm::detail {
namespace v1 {
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
  explicit V1Reader(ComposeDrawPhase phase) : phase_(phase) {}
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
    // Command semantics are introduced separately. Validate the complete
    // bounded runtime contents before reporting the deferred capability.
    if (!ValidateValue(commands)) return error_;
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
    if (commands.GetLength()) return DrawRecordingError{Code::kUnsupported};
    *out = scratch_.Build();
    *shadow = false;
    return std::nullopt;
  }

 private:
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
  bool Path(const Value& row) {
    PathInfo path;
    if (!Tag(row, &path.tag)) return false;
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

  ComposeDrawPhase phase_;
  DisplayListBuilder scratch_;
  base::Vector<PathInfo> paths_;
  base::Vector<PaintInfo> paints_;
  DrawRecordingError error_;
  PaintCapability paint_capability_{PaintCapability::kSupported};
  std::string unknown_;
  size_t resource_count_{0};
  size_t snapshot_nodes_{0}, payload_bytes_{0};
  bool failed_{false}, input_failed_{false};
  bool path_capability_error_{false}, clip_phase_error_{false};
};
}  // namespace v1

std::optional<DrawRecordingError> DecodeV1DrawDisplayList(
    const lepus::Value& input, double pixels, ComposeDrawPhase phase,
    DisplayList* output, bool* has_outset_shadow) {
  if (!std::isfinite(pixels) || pixels <= 0) return DrawRecordingError{};
  return v1::V1Reader(phase).Read(input, output, has_outset_shadow);
}
}  // namespace lynx::tasm::detail

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fiber/compose_draw_recording_decoder.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx::tasm::detail {
namespace {
// Arrays shared with the Compose producers, in physical pixels:
// [1, x, y, width, height, ARGB]
// [2, x, y, width, height, startY, endY, colors[], stops[]]
// [3, x, y, width, height, topLeftRadius, topRightRadius, bottomRightRadius,
//  bottomLeftRadius, offsetX, offsetY, blur, spread, ARGB]
// Filled rectangles, clamped vertical gradients and outset shadows of a
// rounded rectangle are supported here.
constexpr double kCommandFill = 1;
constexpr double kCommandGradient = 2;
constexpr double kCommandOutsetShadow = 3;

struct OutsetShadow {
  float offset_x;
  float offset_y;
  float blur;
  float spread;
};

struct DrawRect {
  double kind{0};
  RoundedRectangle clip;
  RoundedRectangle tile;
  base::Vector<uint32_t> colors;
  base::Vector<float> stops;
  OutsetShadow shadow{};
};

bool ReadNumber(const lepus::Value& array, uint32_t index, double* result) {
  const auto value = array.GetProperty(index).ToLepusValue();
  if (!value.IsNumber() || !std::isfinite(value.Number())) return false;
  *result = value.Number();
  return true;
}

bool ReadColor(const lepus::Value& array, uint32_t index, uint32_t* result) {
  double value;
  if (!ReadNumber(array, index, &value) || value < 0 ||
      value > std::numeric_limits<uint32_t>::max() ||
      std::trunc(value) != value) {
    return false;
  }
  *result = static_cast<uint32_t>(value);
  return true;
}

bool FitsFloat(double value) {
  return std::isfinite(value) &&
         std::abs(value) <= std::numeric_limits<float>::max();
}

bool ParseRect(const lepus::Value& value, double pixels, size_t max_stops,
               DrawRect* rect) {
  if (!value.IsArrayOrJSArray()) return false;
  const int length = value.GetLength();
  if (!ReadNumber(value, 0, &rect->kind)) return false;
  const double kind = rect->kind;
  if (!((kind == kCommandFill && length == 6) ||
        (kind == kCommandGradient && length == 9) ||
        (kind == kCommandOutsetShadow && length == 14))) {
    return false;
  }
  double bounds[4];
  for (uint32_t i = 0; i < 4; ++i) {
    if (!ReadNumber(value, i + 1, &bounds[i])) return false;
    bounds[i] /= pixels;
    if (!FitsFloat(bounds[i])) return false;
  }
  if (bounds[2] < 0 || bounds[3] < 0 || !FitsFloat(bounds[0] + bounds[2]) ||
      !FitsFloat(bounds[1] + bounds[3]))
    return false;
  rect->clip.SetX(static_cast<float>(bounds[0]));
  rect->clip.SetY(static_cast<float>(bounds[1]));
  rect->clip.SetWidth(static_cast<float>(bounds[2]));
  rect->clip.SetHeight(static_cast<float>(bounds[3]));
  if (!std::isfinite(rect->clip.GetX() + rect->clip.GetWidth()) ||
      !std::isfinite(rect->clip.GetY() + rect->clip.GetHeight()))
    return false;
  if (kind == kCommandFill) {
    uint32_t color;
    if (!ReadColor(value, 5, &color)) return false;
    rect->colors.push_back(color);
    return true;
  }
  if (kind == kCommandOutsetShadow) {
    double radii[4];
    for (uint32_t i = 0; i < 4; ++i) {
      if (!ReadNumber(value, i + 5, &radii[i]) || radii[i] < 0) return false;
      radii[i] /= pixels;
      if (!FitsFloat(radii[i])) return false;
    }
    const auto r = [&radii](int i) { return static_cast<float>(radii[i]); };
    rect->clip.SetRadiusXTopLeft(r(0));
    rect->clip.SetRadiusYTopLeft(r(0));
    rect->clip.SetRadiusXTopRight(r(1));
    rect->clip.SetRadiusYTopRight(r(1));
    rect->clip.SetRadiusXBottomRight(r(2));
    rect->clip.SetRadiusYBottomRight(r(2));
    rect->clip.SetRadiusXBottomLeft(r(3));
    rect->clip.SetRadiusYBottomLeft(r(3));
    double shadow[4];
    for (uint32_t i = 0; i < 4; ++i) {
      if (!ReadNumber(value, i + 9, &shadow[i])) return false;
      shadow[i] /= pixels;
      if (!FitsFloat(shadow[i])) return false;
    }
    if (shadow[2] < 0) return false;
    rect->shadow = {
        static_cast<float>(shadow[0]), static_cast<float>(shadow[1]),
        static_cast<float>(shadow[2]), static_cast<float>(shadow[3])};
    uint32_t color;
    if (!ReadColor(value, 13, &color)) return false;
    rect->colors.push_back(color);
    return true;
  }
  double start, end;
  if (!ReadNumber(value, 5, &start) || !ReadNumber(value, 6, &end))
    return false;
  start /= pixels;
  end /= pixels;
  if (!FitsFloat(start) || !FitsFloat(end) || start >= end) return false;
  const auto colors = value.GetProperty(7u).ToLepusValue();
  const auto stops = value.GetProperty(8u).ToLepusValue();
  if (!colors.IsArrayOrJSArray() || !stops.IsArrayOrJSArray() ||
      colors.GetLength() < 2 || max_stops < 2 ||
      static_cast<size_t>(colors.GetLength()) > max_stops - 2 ||
      colors.GetLength() != stops.GetLength()) {
    return false;
  }

  // Expand the gradient tile to cover the draw rect, preserving the shader's
  // coordinates. Explicit endpoint colors implement Clamp without painting
  // translucent pixels twice or stretching the gradient to the clipped rect.
  const double top = std::min(bounds[1], start);
  const double height = std::max(bounds[1] + bounds[3], end) - top;
  if (!FitsFloat(height) || static_cast<float>(height) <= 0) return false;
  rect->tile = rect->clip;
  rect->tile.SetY(static_cast<float>(top));
  rect->tile.SetHeight(static_cast<float>(height));
  if (!std::isfinite(rect->tile.GetY() + rect->tile.GetHeight())) return false;
  double previous = 0;
  for (int i = 0; i < colors.GetLength(); ++i) {
    uint32_t color;
    double stop;
    if (!ReadColor(colors, i, &color) || !ReadNumber(stops, i, &stop) ||
        stop < previous || stop > 1)
      return false;
    if (i == 0) {
      rect->colors.push_back(color);
      rect->stops.push_back(0);
    }
    rect->colors.push_back(color);
    rect->stops.push_back(
        static_cast<float>((start + stop * (end - start) - top) / height));
    previous = stop;
  }
  rect->colors.push_back(rect->colors.back());
  rect->stops.push_back(1);
  return true;
}

}  // namespace

std::optional<DrawRecordingError> DecodeDrawDisplayList(
    const lepus::Value& input, double pixels, ComposeDrawPhase phase,
    DisplayList* output, bool* has_outset_shadow) {
  using Code = DrawRecordingError::Code;
  if (phase == ComposeDrawPhase::kClip)
    return DrawRecordingError{Code::kUnsupported};
  if (!std::isfinite(pixels) || pixels <= 0 || !input.IsArrayOrJSArray())
    return DrawRecordingError{};
  const int length = input.GetLength();
  if (length < 0) return DrawRecordingError{};
  if (static_cast<size_t>(length) > 65536)
    return DrawRecordingError{Code::kCapacity};

  size_t bytes = 0;
  constexpr size_t kMaxDataBytes = INT32_MAX;
  DisplayListBuilder scratch;
  bool has_shadow = false;
  for (int i = 0; i < length; ++i) {
    const size_t index = static_cast<size_t>(i);
    DrawRect rect;
    if (!ParseRect(input.GetProperty(static_cast<uint32_t>(i)).ToLepusValue(),
                   pixels, (kMaxDataBytes - bytes) / 8, &rect))
      return DrawRecordingError{Code::kMalformed, index};
    RoundedRectangle shadow;
    if (rect.kind == kCommandOutsetShadow &&
        !Fragment::PrepareCustomOutsetShadow(rect.clip, rect.shadow.offset_x,
                                             rect.shadow.offset_y,
                                             rect.shadow.spread, &shadow))
      return DrawRecordingError{Code::kOverflow, index};
    if (rect.clip.GetWidth() == 0 || rect.clip.GetHeight() == 0) continue;
    const size_t added_bytes =
        rect.kind == kCommandGradient ? rect.colors.size() * 8 : 0;
    bytes += added_bytes;
    int32_t clip_index;
    scratch.RecordBoxModel(rect.clip, clip_index);
    if (rect.kind == kCommandOutsetShadow) {
      int32_t shadow_index;
      scratch.RecordBoxModel(shadow, shadow_index);
      scratch.BoxShadow(shadow_index, clip_index, rect.colors[0],
                        rect.shadow.blur,
                        DisplayListBuilder::BoxShadowClipMode::kOutset);
      has_shadow = true;
    } else if (rect.kind == kCommandFill) {
      scratch.Fill(rect.colors[0], clip_index);
    } else {
      int32_t tile_index;
      scratch.RecordBoxModel(rect.tile, tile_index);
      const auto no_repeat =
          static_cast<int32_t>(starlight::BackgroundRepeatType::kNoRepeat);
      scratch.LinearGradient(180, rect.colors, rect.stops, tile_index,
                             clip_index, no_repeat, no_repeat);
    }
  }
  *output = scratch.Build();
  *has_outset_shadow = has_shadow;
  return std::nullopt;
}
}  // namespace lynx::tasm::detail
