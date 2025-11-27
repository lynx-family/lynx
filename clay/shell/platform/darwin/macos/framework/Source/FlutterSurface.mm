// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterSurface.h"

#import <Metal/Metal.h>

@interface FlutterSurface () {
  CGSize _size;
  IOSurfaceRef _ioSurface;
  id<MTLTexture> _texture;
}
@end

@implementation FlutterSurface

- (IOSurfaceRef)ioSurface {
  return _ioSurface;
}

- (CGSize)size {
  return _size;
}

- (int64_t)textureId {
  return reinterpret_cast<int64_t>(_texture);
}

- (instancetype)initWithSize:(CGSize)size device:(id<MTLDevice>)device {
  if (self = [super init]) {
    self->_size = size;
    self->_ioSurface = [FlutterSurface createIOSurfaceWithSize:size];
    self->_texture = [FlutterSurface createTextureForIOSurface:_ioSurface size:size device:device];
  }
  return self;
}

static void ReleaseSurface(void* surface) {
  if (surface != nullptr) {
    CFBridgingRelease(surface);
  }
}

- (clay::GPUMTLTextureInfo)asGPUMTLTextureInfo {
  clay::GPUMTLTextureInfo res;
  memset(&res, 0, sizeof(clay::GPUMTLTextureInfo));
  res.texture = (__bridge void*)_texture;
  res.texture_id = self.textureId;
  res.destruction_callback = ReleaseSurface;
  res.destruction_context = (void*)CFBridgingRetain(self);
  return res;
}

+ (FlutterSurface*)fromGPUMTLTextureInfo:(const clay::GPUMTLTextureInfo*)textureInfo {
  return (__bridge FlutterSurface*)textureInfo->destruction_context;
}

- (void)dealloc {
  CFRelease(_ioSurface);
}

+ (IOSurfaceRef)createIOSurfaceWithSize:(CGSize)size {
  unsigned pixelFormat = 'BGRA';
  unsigned bytesPerElement = 4;

  size_t bytesPerRow = IOSurfaceAlignProperty(kIOSurfaceBytesPerRow, size.width * bytesPerElement);
  size_t totalBytes = IOSurfaceAlignProperty(kIOSurfaceAllocSize, size.height * bytesPerRow);
  NSDictionary* options = @{
    (id)kIOSurfaceWidth : @(size.width),
    (id)kIOSurfaceHeight : @(size.height),
    (id)kIOSurfacePixelFormat : @(pixelFormat),
    (id)kIOSurfaceBytesPerElement : @(bytesPerElement),
    (id)kIOSurfaceBytesPerRow : @(bytesPerRow),
    (id)kIOSurfaceAllocSize : @(totalBytes),
  };

  IOSurfaceRef res = IOSurfaceCreate((CFDictionaryRef)options);
  IOSurfaceSetValue(res, CFSTR("IOSurfaceColorSpace"), kCGColorSpaceSRGB);
  return res;
}

+ (id<MTLTexture>)createTextureForIOSurface:(IOSurfaceRef)surface
                                       size:(CGSize)size
                                     device:(id<MTLDevice>)device {
  MTLTextureDescriptor* textureDescriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                         width:size.width
                                                        height:size.height
                                                     mipmapped:NO];
  textureDescriptor.usage =
      MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget | MTLTextureUsageShaderWrite;
  // plane = 0 for BGRA.
  return [device newTextureWithDescriptor:textureDescriptor iosurface:surface plane:0];
}

@end
