// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDisplayListApplier+Internal.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <QuartzCore/QuartzCore.h>

@implementation LynxRenderer {
  __weak UIView<LynxRendererHost>* _host;
  LynxDisplayListApplier* _applier;
  LynxRendererContext* _renderer_context;

  lynx::tasm::DisplayList* list_;

  int32_t sign_;
}

- (instancetype)initWithRenderHost:(UIView<LynxRendererHost>*)host
                           andSign:(int32_t)sign
                        andContext:(LynxRendererContext*)context {
  self = [super init];
  if (self) {
    _host = host;
    sign_ = sign;
    _renderer_context = context;
  }
  return self;
}

- (int32_t)sign {
  return sign_;
}

- (UIView<LynxRendererHost>*)rendererHost {
  return _host;
}

- (LynxRendererContext*)context {
  return _renderer_context;
}

- (void)ensureLynxDisplayListApplier {
  if (_applier != nil) {
    return;
  }

  _applier = [[LynxDisplayListApplier alloc] initWithView:_host andContext:_renderer_context];
}

- (void)updateDisplayList:(lynx::tasm::DisplayList*)list {
  list_ = list;

  [self ensureLynxDisplayListApplier];

  [_applier applyDisplayList:list_];
  [_applier syncHostDecorationLayers];
}

- (lynx::tasm::DisplayList*)getDisplayList {
  return list_;
}

- (void)detachHostDecorationLayers {
  if (_applier == nil) {
    return;
  }
  [_applier detachHostDecorationLayers];
}

- (void)reattachHostDecorationLayers {
  if (_applier == nil) {
    return;
  }
  [_applier reattachHostDecorationLayers];
  [_applier syncHostDecorationLayers];
}

- (void)syncHostDecorationLayers {
  if (_applier == nil) {
    return;
  }
  [_applier syncHostDecorationLayers];
}

#pragma mark - SubtreeProperties

- (void)applySubtreeProperties:(const lynx::tasm::SubtreeProperty*)properties count:(size_t)count {
  if (!_host || count == 0 || properties == nullptr) {
    return;
  }

  bool needs_sync = false;
  for (size_t i = 0; i < count; i++) {
    const auto& prop = properties[i];

    if (prop.type == lynx::tasm::DisplayListSubtreePropertyOpType::kTransform) {
      [self applyTransform:prop.data.transform];
      needs_sync = true;
    } else if (prop.type == lynx::tasm::DisplayListSubtreePropertyOpType::kOpacity) {
      [self applyOpacity:prop.data.opacity];
      needs_sync = true;
    }
  }

  if (needs_sync) {
    [self syncHostDecorationLayers];
  }
}

/**
 * Apply 4x4 transform matrix to the host view's layer.
 *
 * The input matrix from C++ Matrix44 is stored in column-major order:
 * [ m00 m10 m20 m30 ]  col 0: m[0], m[1], m[2], m[3]
 * [ m01 m11 m21 m31 ]  col 1: m[4], m[5], m[6], m[7]
 * [ m02 m12 m22 m32 ]  col 2: m[8], m[9], m[10], m[11]
 * [ m03 m13 m23 m33 ]  col 3: m[12], m[13], m[14], m[15]
 *
 * C++ uses column-vector convention (v' = M * v), where translation is in column 3.
 * CATransform3D uses row-vector convention (v' = v * M), where translation is in row 4.
 *
 * To convert, we need to transpose the matrix:
 * - C++ column 0 -> CATransform3D row 0
 * - C++ column 1 -> CATransform3D row 1
 * - C++ column 2 -> CATransform3D row 2
 * - C++ column 3 -> CATransform3D row 3 (translation)
 */
- (void)applyTransform:(const float*)m {
  if (!_host || m == nullptr) {
    return;
  }

  CATransform3D transform;

  // C++ column 0 -> CATransform3D row 0
  transform.m11 = m[0];  // m00
  transform.m12 = m[1];  // m10
  transform.m13 = m[2];  // m20
  transform.m14 = m[3];  // m30 (perspective X)

  // C++ column 1 -> CATransform3D row 1
  transform.m21 = m[4];  // m01
  transform.m22 = m[5];  // m11
  transform.m23 = m[6];  // m21
  transform.m24 = m[7];  // m31 (perspective Y)

  // C++ column 2 -> CATransform3D row 2
  transform.m31 = m[8];   // m02
  transform.m32 = m[9];   // m12
  transform.m33 = m[10];  // m22
  transform.m34 = m[11];  // m32 (perspective Z)

  // C++ column 3 -> CATransform3D row 3 (translation)
  transform.m41 = m[12];  // m03 (translate X)
  transform.m42 = m[13];  // m13 (translate Y)
  transform.m43 = m[14];  // m23 (translate Z)
  transform.m44 = m[15];  // m33

  _host.layer.transform = transform;
}

- (void)applyOpacity:(float)opacity {
  if (!_host) {
    return;
  }

  _host.alpha = MAX(0.0f, MIN(1.0f, opacity));
}

// Override point for subclasses to handle attribute updates.
// Called when the native framework sends new props to this renderer.
- (void)updateAttributes:(NSDictionary*)props {
  // Subclasses should override this method to process props.
}

// Override point for subclasses to handle platform-specific extra data.
// Called when the shadow node sends extra data to this renderer.
- (void)updatePlatformExtraBundle:(id)data {
  // Subclasses should override this method to process extra data.
}

@end
