// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/lynx_clay/platform_renderer_clay.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/skity/picture_skity.h"
#include "clay/ui/compositing/pending_container_layer.h"
#include "clay/ui/compositing/pending_effect_layer.h"
#include "clay/ui/compositing/pending_offset_layer.h"
#include "clay/ui/compositing/pending_picture_layer.h"
#include "clay/ui/compositing/pending_transform_layer.h"
#include "gfx/public/display_list_reader.h"
#include "skity/graphic/paint.hpp"
#include "skity/recorder/picture_recorder.hpp"
#include "skity/recorder/recording_canvas.hpp"

namespace lynx::tasm {

class ClayDisplayListRecording {
 public:
  struct Picture {
    size_t next_item;
    fml::RefPtr<::clay::PictureSkity> content;
  };

  std::vector<Picture> pictures;
};

namespace {

using Op = DisplayListOpType;

template <typename Box>
::clay::FloatRoundedRect ToRoundedRect(const Box& box) {
  const ::clay::FloatRect bounds(box.x, box.y, box.w, box.h);
  if (!box.has_radii) {
    return ::clay::FloatRoundedRect(bounds);
  }
  const auto& radii = box.radii;
  return {bounds,
          {radii[0], radii[1]},
          {radii[2], radii[3]},
          {radii[6], radii[7]},
          {radii[4], radii[5]}};
}

::clay::PendingEffectLayer* AppendClip(::clay::PendingContainerLayer* layer,
                                       const DisplayListItem& item) {
  const auto& clip = item.payload.clip_rect;
  auto* clipped = new ::clay::PendingEffectLayer();
  if (clip.has_radii) {
    clipped->SetClipRRect(ToRoundedRect(clip));
  } else {
    clipped->SetClipRect({clip.x, clip.y, clip.w, clip.h});
  }
  layer->AppendChild(clipped);
  return clipped;
}

}  // namespace

RecordedClayDisplayList RecordClayDisplayList(const DisplayList& list) {
  ClayDisplayListRecording recording;
  std::optional<skity::PictureRecorder> recorder;
  std::vector<skity::RRect> boxes;
  auto finish_picture = [&](size_t next_item) {
    if (recorder) {
      recording.pictures.push_back(
          {next_item, fml::MakeRefCounted<::clay::PictureSkity>(
                          ::clay::DynamicOps{}, recorder->FinishRecording())});
      recorder.reset();
    }
  };

  DisplayListReader reader(list);
  for (size_t index = 0; reader.HasNext(); ++index) {
    const auto& item = reader.Next();
    switch (item.type) {
      case Op::kBegin:
      case Op::kEnd:
      case Op::kClipRect:
      case Op::kDrawView:
        finish_picture(index);
        break;
      case Op::kRecordBox:
        boxes.push_back(ToRoundedRect(item.payload.record_box));
        break;
      case Op::kFill: {
        const auto [color, clip_index] = item.payload.fill;
        if (!(color >> 24) || clip_index < 0 ||
            static_cast<size_t>(clip_index) >= boxes.size() ||
            boxes[clip_index].IsEmpty()) {
          break;
        }
        if (!recorder) {
          recorder.emplace();
          skity::DisplayListBuildOptions options;
          options.build_rtree = true;
          // Recording bounds must not clip out-of-viewport content.
          recorder->BeginRecording(
              skity::Rect::MakeLTRB(-1e9f, -1e9f, 1e9f, 1e9f), options);
        }
        skity::Paint paint;
        paint.SetColor(color);
        paint.SetAntiAlias(true);
        paint.SetStyle(skity::Paint::kFill_Style);
        recorder->GetRecordingCanvas()->DrawRRect(boxes[clip_index], paint);
        break;
      }
      default:
        // Unsupported paint operations leave the surrounding structure intact.
        break;
    }
  }
  finish_picture(list.GetContentItemsSize());
  return recording.pictures.empty()
             ? nullptr
             : std::make_shared<ClayDisplayListRecording>(std::move(recording));
}

PlatformRendererClay::PlatformRendererClay(
    int id, PlatformRendererType type, const base::String& tag,
    const PlatformRendererInitConfig& config,
    std::function<void()> request_paint)
    : PlatformRendererImpl(id, type, tag),
      request_paint_(std::move(request_paint)) {
  opacity_.data.opacity = 1.f;
  SetFragmentParentId(config.fragment_parent_id);
  SetDirectChildOfCompatibleComponent(
      config.is_direct_child_of_compatible_component);
}

void PlatformRendererClay::OnUpdateDisplayList(DisplayList list) {
  // An empty content buffer carries only new properties. A hidden fragment
  // sends Begin/End without paint, which must replace the previous recording.
  if (list.GetContentItemsSize() > 0) {
    display_list_ = std::move(list);
    current_recording_ = std::move(pending_recording_);
  }
  pending_recording_.reset();
  if (request_paint_) {
    request_paint_();
  }
}

void PlatformRendererClay::OnRemoveFromParent(bool) {
  if (request_paint_) {
    request_paint_();
  }
}

std::unique_ptr<::clay::PendingContainerLayer>
PlatformRendererClay::BuildPendingLayers(
    const fml::RefPtr<::clay::GPUUnrefQueue>& unref_queue) const {
  if (display_list_.GetContentItemsSize() == 0) {
    return nullptr;
  }
  auto root = std::make_unique<::clay::PendingContainerLayer>();
  struct Scope {
    ::clay::PendingContainerLayer* layer;
    ::clay::FloatPoint offset;
  } current{root.get(), {}};
  base::InlineVector<Scope, 4> scopes;
  const auto& children = Children();
  auto child = children.begin();
  size_t picture_index = 0;
  auto append_picture = [&](size_t next_item) {
    if (!current_recording_ ||
        picture_index == current_recording_->pictures.size()) {
      return;
    }
    const auto& picture = current_recording_->pictures[picture_index];
    if (picture.next_item == next_item) {
      auto* layer = new ::clay::PendingPictureLayer();
      layer->set_picture(std::make_unique<::clay::Picture>(
          ::clay::GPUObject(picture.content, unref_queue)));
      current.layer->AppendChild(layer);
      ++picture_index;
    }
  };

  DisplayListReader reader(display_list_);
  for (size_t index = 0; reader.HasNext(); ++index) {
    append_picture(index);
    const auto& item = reader.Next();
    switch (item.type) {
      case Op::kBegin: {
        const auto& node = item.payload.begin;
        // The parent's DrawView supplies the outermost position.
        scopes.push_back(current);
        if (scopes.size() > 1 && (node.x != 0.f || node.y != 0.f)) {
          const ::clay::FloatPoint offset(node.x, node.y);
          auto* translated = new ::clay::PendingOffsetLayer(offset);
          current.layer->AppendChild(translated);
          current = {translated, current.offset + offset};
        }
        break;
      }
      case Op::kEnd:
        if (scopes.empty()) {
          return nullptr;
        }
        current = scopes.back();
        scopes.pop_back();
        break;
      case Op::kClipRect:
        current.layer = AppendClip(current.layer, item);
        break;
      case Op::kDrawView: {
        const auto& slot = item.payload.draw_view;
        auto match = child;
        if (match == children.end() || (*match)->GetId() != slot.view_id) {
          // Missing slots can shift the registry's insertion positions.
          match = std::find_if(children.begin(), children.end(),
                               [&](const auto& candidate) {
                                 return candidate->GetId() == slot.view_id;
                               });
        }
        if (match == children.end()) {
          break;
        }
        if (match == child) {
          ++child;
        }
        if (auto layers = static_cast<const PlatformRendererClay*>(match->get())
                              ->BuildPendingLayers(unref_queue)) {
          const ::clay::FloatPoint offset(slot.offset_x, slot.offset_y);
          auto* translated =
              new ::clay::PendingOffsetLayer(offset - current.offset);
          current.layer->AppendChild(translated);
          translated->AppendChild(layers.release());
        }
        break;
      }
      default:
        break;
    }
  }
  if (!scopes.empty()) {
    return nullptr;
  }
  append_picture(display_list_.GetContentItemsSize());

  auto wrap_root =
      [&root](std::unique_ptr<::clay::PendingContainerLayer> layer) {
        layer->AppendChild(root.release());
        root = std::move(layer);
      };
  reader.Reset();
  const auto& first = reader.Next();
  if (display_list_.RootNeedClipBounds() && first.type == Op::kBegin) {
    const auto& node = first.payload.begin;
    auto layer = std::make_unique<::clay::PendingEffectLayer>();
    layer->SetClipRect({0, 0, node.w, node.h});
    wrap_root(std::move(layer));
  }
  if (opacity_.data.opacity != 1.f) {
    auto layer = std::make_unique<::clay::PendingEffectLayer>();
    layer->SetOpacity(
        static_cast<int>(std::clamp(opacity_.data.opacity, 0.f, 1.f) * 255.f));
    wrap_root(std::move(layer));
  }
  if (const auto* transform = GetTransform()) {
    const auto* m = transform->data.transform;
    const skity::Matrix matrix(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7],
                               m[8], m[9], m[10], m[11], m[12], m[13], m[14],
                               m[15]);
    if (!matrix.IsIdentity()) {
      wrap_root(std::make_unique<::clay::PendingTransformLayer>(matrix));
    }
  }
  return root;
}

fml::RefPtr<PlatformRenderer> PlatformRendererClayFactory::CreateRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>&,
    const PlatformRendererInitConfig& config) {
  return fml::MakeRefCounted<PlatformRendererClay>(id, type, base::String(),
                                                   config, request_paint_);
}

fml::RefPtr<PlatformRenderer>
PlatformRendererClayFactory::CreateExtendedRenderer(
    int id, const base::String& tag, const fml::RefPtr<PropBundle>&,
    const PlatformRendererInitConfig& config) {
  return fml::MakeRefCounted<PlatformRendererClay>(
      id, PlatformRendererType::kUnknown, tag, config, request_paint_);
}

}  // namespace lynx::tasm
