// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_SINK_ACCESSOR_H_
#define CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_SINK_ACCESSOR_H_

#include <list>
#include <tuple>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/gfx/shared_image/shared_image_sink.h"

namespace clay {

class SharedImageBacking;
class SharedImageRepresentation;
class RepresentationStorageManager;

class SharedImageSinkAccessor {
 public:
  using RepresentationFactory =
      std::function<fml::RefPtr<SharedImageRepresentation>(
          fml::RefPtr<SharedImageBacking>)>;

  SharedImageSinkAccessor(fml::RefPtr<SharedImageSink> sink,
                          const RepresentationFactory& repr_factory);

  ~SharedImageSinkAccessor();

  fml::RefPtr<SharedImageRepresentation> UpdateFront();

  fml::RefPtr<SharedImageRepresentation> UpdateFrontSequence();

  fml::RefPtr<SharedImageRepresentation> UpdateFrontToLatest();

  void ReleaseFront();

  std::tuple<fml::RefPtr<SharedImageRepresentation>, uint32_t> AcquireBack(
      const skity::Vec2& size);

  std::tuple<fml::RefPtr<SharedImageRepresentation>, uint32_t,
             SharedImageSink::AcquireBackStatus>
  TryAcquireBack(const skity::Vec2& size);

  std::tuple<fml::RefPtr<SharedImageRepresentation>, uint32_t>
  AcquireBackForced(const skity::Vec2& size);

  bool SwapBack();

  BASE_DISALLOW_COPY_AND_ASSIGN(SharedImageSinkAccessor);

 private:
  fml::RefPtr<SharedImageRepresentation> GetRepresentation(
      SharedImageBacking* backing);

  fml::RefPtr<SharedImageSink> sink_;
  RepresentationFactory repr_factory_;
  fml::RefPtr<SharedImageRepresentation> front_repr_;
  fml::RefPtr<SharedImageRepresentation> back_repr_;
  std::list<
      std::tuple<SharedImageBacking*, fml::RefPtr<SharedImageRepresentation>>>
      image_repr_cache_;
  fml::RefPtr<RepresentationStorageManager> repr_texture_manager_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_SINK_ACCESSOR_H_
