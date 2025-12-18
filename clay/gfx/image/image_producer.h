// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_PRODUCER_H_
#define CLAY_GFX_IMAGE_IMAGE_PRODUCER_H_

#include <memory>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/image/frame_info.h"
#include "clay/gfx/image/image.h"
#include "clay/gfx/image/image_produce_context.h"
#include "clay/gfx/rendering_backend.h"

#if defined(ENABLE_SVG)
#include "clay/gfx/image/svg_image_holder.h"
#endif

namespace clay {

class Codec;
class ImageDescriptor;
#if defined(ENABLE_SVG)
class SVGImageHolder;
#endif
class SkImageHolder;

// ImageProducer is responsible for decoding image raw data and caching all
// frames of the image.
class ImageProducer : public std::enable_shared_from_this<ImageProducer> {
 public:
  static std::shared_ptr<ImageProducer> CreateImageProducer(
      bool is_svg, GrDataPtr data, fml::RefPtr<ImageDescriptor> descriptor,
      const ImageProduceContext& produce_context,
      const std::shared_ptr<std::recursive_mutex>& mutex,
      bool use_texture_backend, bool enable_low_quality_image = false);
  ~ImageProducer();

  void SetData(bool is_svg, GrDataPtr data);
  GrDataPtr GetData() { return raw_data_; }
  size_t GetDataSize() { return raw_data_ ? DATA_GET_SIZE(raw_data_) : 0; }
  fml::RefPtr<ImageDescriptor> GetDescriptor() const { return descriptor_; }
  void SetExpectSizeCalculator(
      std::function<skity::Vec2(skity::Vec2)> calculator,
      bool force_use_original_size);

#if defined(ENABLE_SVG)
  void DecodeSVG();
  fml::RefPtr<SVGImageHolder> GetSVGFrame() const;
#endif  // ENABLE_SVG

  const GrImageInfo& GetInfo();
  size_t FrameCount() const { return frame_count_; }

  void OnFrameDecoded(const fml::RefPtr<SkImageHolder>& holder, bool result,
                      bool is_async = false);
  void OnFrameUploaded(const fml::RefPtr<SkImageHolder>& holder, bool result);

  void Reset(bool reuse = false);

  void DecodeNextFrame();
  void UpdateCurrentFrame();
  fml::RefPtr<SkImageHolder> GetCurrentFrame() const;
  bool NextFrameReady() const;
  bool CurrentFrameReady() const;
  bool SVGFrameReady() const;
  bool IsSingleFrameImage() const;
  bool EnableLowQualityImage() const { return enable_low_quality_image_; }
  bool WillNextFrameSizeChange() const;

  bool NeedDecode() const { return !decoded_ && !decoding_; }

  size_t GetAllocationSize() const;

 private:
  ImageProducer(bool is_svg, GrDataPtr data,
                fml::RefPtr<ImageDescriptor> descriptor,
                const ImageProduceContext& produce_context,
                const std::shared_ptr<std::recursive_mutex>& mutex,
                bool use_texture_backend,
                bool enable_low_quality_image = false);

  void AsyncDecodeNextFrame();
  void DecodePromiseFrame(const fml::RefPtr<clay::SkImageHolder>& frame);
  void DecodeBackendTextureFrame(const fml::RefPtr<clay::SkImageHolder>& frame);
  void DecodeBackendTextureFrameWithPriority(
      const fml::RefPtr<clay::SkImageHolder>& frame);
  void SyncDecodeNextFrame();

  bool PrepareCodec();
  void ReleaseAllFrames();
  void TryDestroyCodec();
  void EnsureDescriptorInitialized();
  bool ShouldCacheImage(const fml::RefPtr<SkImageHolder>& holder);
  void RegisterUploadTask(const fml::RefPtr<clay::SkImageHolder>& frame,
                          const FrameInfo& frame_info);
#if defined(ENABLE_SVG)
  void FetchSoftwareSVGImage();
  void FetchHardwareSVGImage();
#endif

  size_t frame_count_ = 0;
  int current_frame_index_ = -1;
  GrImageInfo orig_info_;    // Original image size.
  GrImageInfo render_info_;  // The actual image size after decoding.
  bool is_svg_;
  GrDataPtr raw_data_;
#if defined(ENABLE_SVG)
  fml::RefPtr<SVGImageHolder> svg_frame_;
#endif
  fml::RefPtr<SkImageHolder> current_frame_;
  fml::RefPtr<SkImageHolder> next_frame_;
  fml::RefPtr<Codec> codec_;
  fml::RefPtr<ImageDescriptor> descriptor_;
  bool enable_low_quality_image_ = false;
  bool decoding_ = false;
  // `decoded_` only affect single frame image.
  bool decoded_ = false;
  ImageProduceContext produce_context_;
  std::shared_ptr<std::recursive_mutex> mutex_;

  bool should_use_cache_ = true;
  bool use_texture_backend_ = true;
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_PRODUCER_H_
