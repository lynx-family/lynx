// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxBackgroundImageLayerInfo.h"
#import "LynxBackgroundManager.h"
#import "LynxBackgroundUtils.h"
#import "LynxUI+Internal.h"
#import "LynxWeakProxy.h"

@interface LynxBackgroundSubBackgroundLayer () {
  bool _needCalculateInterval;
  double _singleFrameDuration;
  double _elapsedDuration;
}
@property(atomic, assign) BOOL isPreRendering;
@property(nonatomic, strong) CADisplayLink* displayLink;
@property(nonatomic, strong) NSMutableArray<UIImage*>* frameQueue;
@property(nonatomic, assign) CGSize viewSize;
@property(nonatomic, assign) LynxBorderRadii cornerRadii;
@property(nonatomic, assign) UIEdgeInsets borderInsets;
@property(nonatomic, strong) UIColor* layerBackgroundColor;
@property(nonatomic, assign) BOOL drawToEdge;
@property(nonatomic, assign) NSUInteger currentFrameIndex;
@property(nonatomic, assign) UIEdgeInsets capInsets;
@property(nonatomic, assign) BOOL isDirty;
@property(nonatomic, assign) BOOL contentsUpdating;
@property(nonatomic, strong) UIImage* currentBackgroundImage;
@property(nonatomic, assign) BOOL isGradientOnly;
@property(nonatomic, strong) CAShapeLayer* colorLayer;
@property(nonatomic, assign) BOOL isPixelated;
@end

/**
 *  This background drawing LynxBackgroundDrawables to the frame.
 */

@implementation LynxBackgroundSubBackgroundLayer
static const int kFrameQueueCapacity = 5;
dispatch_block_t mDispatchTask = NULL;

+ (dispatch_queue_t)concurrentDispatchQueue {
  static dispatch_queue_t displayQueue = NULL;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    displayQueue =
        dispatch_queue_create("com.lynx.background.gifRenderQueue", DISPATCH_QUEUE_CONCURRENT);
    // we use the highpri queue to prioritize UI rendering over other async operations
    dispatch_set_target_queue(displayQueue,
                              dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
  });

  return displayQueue;
}

- (void)clearAnimatedImage {
  if (mDispatchTask) {
    self.isPreRendering = NO;
  }
  if (_frameQueue) {
    // remove all cached frames and ensure thread safty.
    dispatch_barrier_async([[self class] concurrentDispatchQueue], ^{
      [self->_frameQueue removeAllObjects];
    });
  }
}

- (void)autoAdjustInsetsForContents:(UIImage*)frame {
  _elapsedDuration = 0;
  _currentBackgroundImage = frame;
  [self setNeedsDisplay];
}

- (void)markDirtyWithSize:(CGSize)viewSize
                    radii:(LynxBorderRadii)cornerRadii
             borderInsets:(UIEdgeInsets)borderInsets
          backgroundColor:(UIColor*)backgroundColor
               drawToEdge:(BOOL)drawToEdge
                capInsets:(UIEdgeInsets)insets
           isGradientOnly:(BOOL)isGradientOnly
              isPixelated:(BOOL)isPixelated {
  _viewSize = viewSize;
  _cornerRadii = cornerRadii;
  _borderInsets = borderInsets;
  _layerBackgroundColor = backgroundColor;
  _drawToEdge = drawToEdge;
  _currentFrameIndex = 0;
  _isPixelated = isPixelated;
  // TODO(renzhongyue): Remove the 'capInsets' from 'LynxUI' and all related classes. It is not a
  // standard CSS property and its value is always zero.
  _capInsets = insets;

  // not gradient only now, clear all possible gradients on layer.
  if (_isGradientOnly != isGradientOnly) {
    if (!isGradientOnly) {
      [self detachAllGradientLayers];
    } else {
      self.contents = self.currentBackgroundImage = nil;
    }
    _isGradientOnly = isGradientOnly;
  }

  [self stopAnimation];
  [self clearAnimatedImage];

  if (!_isDirty && !self.contentsUpdating) {
    //     First data update in the frame interval. Draw bitmap directly.
    self.contentsUpdating = YES;
    [self onContentsUpdate];
  } else {
    // Already have image to display for next v-sync, mark dirty.
    // Image will be drawn in next v-sync interval.
    _isDirty = YES;
    if (![self needsDisplay]) {
      [self setNeedsDisplay];
    }
  }
}

// override set image array here. If a new image array is added, we should remove the layers in old
// array.
- (void)setImageArray:(NSMutableArray<LynxBackgroundImageLayerInfo*>*)imageArray {
  if (self.imageArray) {
    [self detachAllGradientLayers];
  }
  [super setImageArray:imageArray];
}

- (void)startAnimation {
  if (!_displayLink) {
    // The displayLink will retain the target object. Using weakProxy here to avoid the layer
    // retained by the displayLink. Using weakProxy can let the lifecircle of background layer
    // independent of the displayLink. The displayLink will be cancelled when background layer
    // dealloc.
    LynxWeakProxy* weakProxy = [LynxWeakProxy proxyWithTarget:self];
    _displayLink = [CADisplayLink displayLinkWithTarget:weakProxy selector:@selector(updateFrame:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }

  [self clearAnimatedImage];

  _needCalculateInterval = false;

  // Fixed refresh rate 30Hz.
  NSInteger fps = MAX(_frameCount / _animatedImageDuration, 1);

  if (@available(iOS 10.0, *)) {
    if (_frameCount < _animatedImageDuration) {
      fps = 30;
      _needCalculateInterval = true;
      _singleFrameDuration = _animatedImageDuration / _frameCount;
      _elapsedDuration = _singleFrameDuration;
    }
    self.displayLink.preferredFramesPerSecond = MIN(30, fps);
  } else {
    // Fallback on earlier versions
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    self.displayLink.frameInterval = MAX(2, 60 / fps);
#pragma clang diagnostic pop
  }

  // start animation
  if (self.displayLink.isPaused) {
    [_displayLink setPaused:NO];
  }
}

- (void)applyStaticBackground {
  // Only has gradient, use CAGradientLayer.
  if ([self isGradientOnly]) {
    [self updateGradient];
    return;
  }

  // Draw with CoreGraphics in rendering thread.
  __weak LynxBackgroundSubBackgroundLayer* weakSelf = self;

  // deep copy thread safe.
  NSArray* imageArrayInfo = [[NSArray alloc] initWithArray:self.imageArray];
  UIColor* backgroundColorCopy = [_layerBackgroundColor copy];

  lynx_async_get_background_image_block_t displayBlock = ^{
    __strong LynxBackgroundSubBackgroundLayer* strongSelf = weakSelf;
    if (strongSelf) {
      return LynxGetBackgroundImageWithClip(
          strongSelf->_viewSize, strongSelf->_cornerRadii, strongSelf->_borderInsets,
          [backgroundColorCopy CGColor], NO, imageArrayInfo, strongSelf -> _backgroundColorClip,
          strongSelf -> _paddingWidth, strongSelf -> _isPixelated);
    }
    return (UIImage*)nil;
  };

  lynx_async_display_completion_block_t completionBlock = ^(UIImage* frame) {
    __strong LynxBackgroundSubBackgroundLayer* strongSelf = weakSelf;
    if (strongSelf && strongSelf.type == LynxBgTypeComplex) {
      [strongSelf autoAdjustInsetsForContents:frame];
    }
  };

  LynxBackgroundManager* manager = self.delegate;
  if (manager) {
    [manager.ui displayComplexBackgroundAsynchronouslyWithDisplay:displayBlock
                                                       completion:completionBlock];
  }
}

/**
 * The background props is updated, and should draw new static or animated image.
 */
- (void)onContentsUpdate {
  if (_isAnimated) {
    if (!_frameQueue) {
      // Ensure will not expand opacity in other thread to make thread safty.
      _frameQueue = [[NSMutableArray alloc] initWithCapacity:2 * kFrameQueueCapacity];
    }
    [self startAnimation];
  } else {
    [self applyStaticBackground];
  }
}

/**
 * Pause the displayLink to stop the animated image.
 */

- (void)stopAnimation {
  if (self.displayLink && !self.displayLink.isPaused) {
    self.displayLink.paused = YES;
  }
}

/**
 * All frames can be cached, no need to redraw.
 */
- (BOOL)canCacheAllFrames {
  return self.frameCount <= kFrameQueueCapacity;
}

/**
 *  Apply the next frame to layer.contents, and cache next few frames to buffer if needed.
 */
- (void)updateFrame:(CADisplayLink*)sender {
  if (@available(iOS 10.0, *)) {
    // Manually control the frame duration, when target frame rate is less than 1.
    if (_needCalculateInterval && _elapsedDuration < _singleFrameDuration) {
      _elapsedDuration += sender.targetTimestamp - CACurrentMediaTime();
      return;
    }
  }

  if ([self canCacheAllFrames]) {
    if ([_frameQueue count] == 0 && !_isPreRendering) {
      [self enqueueFrames:[self frameCount]];
    }
    if (!_isPreRendering) {
      _currentFrameIndex = _currentFrameIndex % _frameCount;
      if (_currentFrameIndex < [_frameQueue count]) {
        // If rendering error occurs, enqueue task will not add images in to _frameQueue, prevent
        // index out of bounds exception here.
        [self autoAdjustInsetsForContents:[_frameQueue objectAtIndex:_currentFrameIndex]];
      }
      ++_currentFrameIndex;
    }
  } else {
    // Pop the next frame
    UIImage* currentFrame;
    @synchronized(_frameQueue) {
      if ([_frameQueue count] > 0) {
        currentFrame = [_frameQueue objectAtIndex:0];
        [_frameQueue removeObjectAtIndex:0];
      }
    }
    if (currentFrame) {
      [self autoAdjustInsetsForContents:currentFrame];
    }

    // Cache the next kFrameQueueCapacity frames.
    if (!_isPreRendering && [_frameQueue count] < kFrameQueueCapacity / 2) {
      [self enqueueFrames:kFrameQueueCapacity];
    }
  }
}

- (dispatch_block_t)createFrameCacheTask:(NSInteger)count {
  __weak LynxBackgroundSubBackgroundLayer* weakSelf = self;

  // Deep copy thread safe.
  NSArray* imageArrayInfo = [[NSArray alloc] initWithArray:self.imageArray];
  UIColor* backgroundColorCopy = [_layerBackgroundColor copy];

  return dispatch_block_create(DISPATCH_BLOCK_ASSIGN_CURRENT, ^{
    __strong LynxBackgroundSubBackgroundLayer* strongSelf = weakSelf;
    if (strongSelf) {
      // TODO: make this async and concurrent.
      // Working with LynxBackgroundImageDrawable, each LynxGetBackgroundImage call will return the
      // next frame.
      for (NSInteger i = 0; i < count; ++i) {
        @autoreleasepool {
          UIImage* frame = LynxGetBackgroundImageWithClip(
              strongSelf->_viewSize, strongSelf->_cornerRadii, strongSelf->_borderInsets,
              [backgroundColorCopy CGColor], NO, imageArrayInfo, strongSelf -> _backgroundColorClip,
              strongSelf -> _paddingWidth, strongSelf -> _isPixelated);
          if (frame) {
            // prevent add nil exception
            @synchronized(strongSelf->_frameQueue) {
              [strongSelf->_frameQueue addObject:frame];
            }
          }
        }
      }
      strongSelf->_isPreRendering = NO;
    }
  });
}

/**
 * Append next few frames asynchronously.
 */
- (void)enqueueFrames:(NSInteger)count {
  _isPreRendering = YES;
  mDispatchTask = [self createFrameCacheTask:count];
  dispatch_async([[self class] concurrentDispatchQueue], mDispatchTask);
}

/**
 * Set up animation related attributes from the target image.
 */
- (void)setAnimatedPropsWithImage:(UIImage*)image {
  if (image && image.images) {
    _isAnimated = YES;
    _frameCount = [image.images count];

    // If duration is 0 for an animated image, use default value.
    _animatedImageDuration = image.duration == 0 ? _frameCount / 60.0 : image.duration;
  } else {
    _isAnimated = NO;
    _frameCount = 1;
    _animatedImageDuration = 0;
  }
}

///**
// * Let the prop updating controlled by v-sync.
// * Multiple updates within 1 v-sync should only trigger once drawing.
// */

- (void)display {
  if (_isDirty) {
    // Data changes are not applied, should launch a new render task.
    _isDirty = NO;
    [self onContentsUpdate];
  } else {
    // No data change and rendering completed.
    self.contentsUpdating = NO;
  }
  if (self.currentBackgroundImage) {
    adjustInsets(self.currentBackgroundImage, self, _capInsets);
  }
}

- (void)dealloc {
  [_displayLink invalidate];
}

- (void)layoutSublayers {
  [super layoutSublayers];
  // The size of the backgroundLayer's actual frame needs to be adjusted with the transform.
  // Regenerate the shadow with the updated bounds here. Normally, the frame would immediately
  // reflect the transform. However, when the CALayer is not part of the viewTree, the frame
  // does not change with transform3D until it is added. Adjust the size of the shadow here.
  LynxBackgroundManager* manager = self.delegate;
  // Check if the current bounds size is the same as the previous shadow size (_shadowsBounds.size)
  // and ensure that manager and manager.shadowArray exist
  if (!(CGSizeEqualToSize(self.bounds.size, _shadowsBounds.size) &&
        CGPointEqualToPoint(self.bounds.origin, _shadowsBounds.origin)) &&
      manager && manager.shadowArray) {
    // shadows bounds will be updated inside update shadow function for reducing redundant shadow
    // updating.
    [manager updateShadow];
  }
}

- (void)updateGradient {
  // The background color should at bottom.
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  if (_layerBackgroundColor) {
    if (!_colorLayer) {
      _colorLayer = [CAShapeLayer layer];
    }
    _colorLayer.frame = self.bounds;
    _colorLayer.fillColor = _layerBackgroundColor.CGColor;
    // create clipPath according to background-clip value for background-color
    UIEdgeInsets borderInsets;
    switch (_backgroundColorClip) {
      case LynxBackgroundClipPaddingBox:
        borderInsets = LynxGetEdgeInsets(self.bounds, _borderInsets, 1.0);
        break;
      case LynxBackgroundClipBorderBox:
        borderInsets = UIEdgeInsetsZero;
        break;
      case LynxBackgroundClipContentBox:
        borderInsets = UIEdgeInsetsMake(
            _borderInsets.top + _paddingWidth.top, _borderInsets.left + _paddingWidth.left,
            _borderInsets.bottom + _paddingWidth.bottom, _borderInsets.right + _paddingWidth.right);
        break;
    }

    CGPathRef clipPath = [LynxBackgroundUtils createBezierPathWithRoundedRect:self.bounds
                                                                  borderRadii:_cornerRadii
                                                                   edgeInsets:borderInsets];
    _colorLayer.path = clipPath;
    CGPathRelease(clipPath);
    [self addSublayer:_colorLayer];
  }

  NSEnumerator* reverseEnum = [self.imageArray reverseObjectEnumerator];
  for (LynxBackgroundImageLayerInfo* info in reverseEnum) {
    if ([info prepareGradientLayers]) {
      [self addSublayer:[(LynxBackgroundGradientDrawable*)info.item verticalRepeatLayer]];
    }
  }
  [CATransaction commit];
}

- (void)detachAllGradientLayers {
  if (_colorLayer) {
    [_colorLayer removeFromSuperlayer];
  }
  for (LynxBackgroundImageLayerInfo* info in self.imageArray) {
    if ([info.item type] >= LynxBackgroundImageLinearGradient) {
      [[(LynxBackgroundGradientDrawable*)info.item gradientLayer] removeFromSuperlayer];
    }
  }
}

@end
