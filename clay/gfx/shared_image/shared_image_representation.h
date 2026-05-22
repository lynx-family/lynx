// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_REPRESENTATION_H_

#include <memory>
#include <optional>

#include "base/include/fml/memory/ref_counted.h"
#include "clay/public/clay.h"
#include "skity/geometry/vector.hpp"
#ifndef ENABLE_SKITY
#include "third_party/skia/include/core/SkImage.h"
#endif

namespace clay {

class SharedImageBacking;
class FenceSync;

enum class ImageRepresentationType {
  kGL,
  kCGL,
  kMetal,
  kD3D,
  kSkia,
  kSkity,
  kEGL,
  kANGLE,
  kEAGL,
  kVulkanImage,
  kLinuxShm,
};

class GLImageRepresentation;
class SkiaImageRepresentation;

class RepresentationStorageManager
    : public fml::RefCountedThreadSafe<RepresentationStorageManager> {
 public:
  virtual ~RepresentationStorageManager();
};

class SharedImageRepresentation
    : public fml::RefCountedThreadSafe<SharedImageRepresentation> {
 public:
  explicit SharedImageRepresentation(fml::RefPtr<SharedImageBacking> backing);

  virtual ~SharedImageRepresentation();

  virtual ImageRepresentationType GetType() const = 0;

  const skity::Vec2 GetSize() const;

  SharedImageBacking* GetBacking() const { return backing_.get(); }

  virtual bool BeginRead(ClaySharedImageReadResult* out) = 0;
  virtual bool EndRead() = 0;
  virtual bool BeginWrite(ClaySharedImageWriteResult* out) = 0;
  virtual bool EndWrite() = 0;

  virtual void ConsumeFence(std::unique_ptr<FenceSync>) = 0;
  virtual std::unique_ptr<FenceSync> ProduceFence() = 0;

  virtual fml::RefPtr<RepresentationStorageManager> GetTextureManager() const {
    return nullptr;
  }
  virtual void SetTextureManager(fml::RefPtr<RepresentationStorageManager>) {}

 private:
  fml::RefPtr<SharedImageBacking> backing_;
};

#ifndef ENABLE_SKITY
class SkiaImageRepresentation : public SharedImageRepresentation {
 public:
  explicit SkiaImageRepresentation(fml::RefPtr<SharedImageBacking> backing);
  ~SkiaImageRepresentation() override;

  virtual sk_sp<SkImage> GetSkImage() = 0;
#else
class SkityImage;
class SkityImageRepresentation : public SharedImageRepresentation {
 public:
  explicit SkityImageRepresentation(fml::RefPtr<SharedImageBacking> backing);
  ~SkityImageRepresentation() override;

  virtual std::shared_ptr<SkityImage> GetSkityImage() = 0;
#endif  // ENABLE_SKITY

  ImageRepresentationType GetType() const override;

  bool BeginRead(ClaySharedImageReadResult* out) override;
  bool BeginWrite(ClaySharedImageWriteResult* out) override;
  bool EndWrite() override;

  bool EndRead() override = 0;
};

class GLImageRepresentation : public SharedImageRepresentation {
 public:
  struct TextureInfo {
    /// Target texture of the active texture unit (example GL_TEXTURE_2D or
    /// GL_TEXTURE_RECTANGLE).
    uint32_t target;
    /// The name of the texture.
    uint32_t name;
    /// The texture format (example GL_RGBA8).
    uint32_t format;
    /// An optional size for GL_TEXTURE_RECTANGLE
    std::optional<skity::Vec2> size;
  };
  struct FramebufferInfo {
    /// The target of the color attachment of the frame-buffer. For example,
    /// GL_TEXTURE_2D or GL_RENDERBUFFER. In case of ambiguity when dealing
    /// with Window bound frame-buffers, 0 may be used.
    uint32_t target;

    /// The name of the framebuffer.
    uint32_t name;
  };

  explicit GLImageRepresentation(fml::RefPtr<SharedImageBacking> backing);

  bool BeginRead(ClaySharedImageReadResult* out) override;
  bool EndRead() override;
  bool BeginWrite(ClaySharedImageWriteResult* out) override;
  bool EndWrite() override;

 private:
  // The returned value remains valid as long as the caller holds the
  // representation
  virtual std::optional<TextureInfo> GetTexImage() = 0;
  virtual bool ReleaseTexImage() = 0;
  // The returned value remains valid as long as the caller holds the
  // representation
  virtual std::optional<FramebufferInfo> BindFrameBuffer() = 0;
  virtual bool UnbindFrameBuffer() = 0;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_REPRESENTATION_H_
