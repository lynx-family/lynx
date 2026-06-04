// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxSVGRenderer.h>

#import <Lynx/LynxError.h>
#import <Lynx/LynxEvent.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxUIContext.h>
#import <ServalSVG/SrSVG.h>

#import <XElement/LynxSVGRenderEngine.h>
#import <XElement/LynxSVGRendererHost.h>

@interface LynxSVGRenderer ()

@property(nonatomic, strong) LynxSVGRenderEngine *engine;
@property(nonatomic, assign) BOOL isDestroyed;

@end

@implementation LynxSVGRenderer

- (instancetype)initWithRenderHost:(UIView<LynxRendererHost> *)host
                           andSign:(int32_t)sign
                        andContext:(LynxRendererContext *)context {
  self = [super initWithRenderHost:host andSign:sign andContext:context];
  if (self) {
    _isDestroyed = NO;

    __weak __typeof(self) weakSelf = self;
    _engine = [[LynxSVGRenderEngine alloc] initWithSrSVG:[[SrSVG alloc] init]
        color:nil
        genericFetcher:context.uiContext.genericResourceFetcher
        mediaFetcher:context.uiContext.mediaResourceFetcher
        asyncExecutor:^void(LynxSVGRenderBlock renderBlock, LynxSVGRenderCompletion completion) {
          dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            UIImage *image = renderBlock();
            dispatch_async(dispatch_get_main_queue(), ^{
              completion(image);
            });
          });
        }
        reloadBlock:^{
          __strong __typeof(weakSelf) strongSelf = weakSelf;
          if (!strongSelf || strongSelf->_isDestroyed) {
            return;
          }
          [strongSelf checkSizeAndRender];
        }];
  }
  return self;
}

- (void)dealloc {
  _isDestroyed = YES;
  [_engine cancel];
}

#pragma mark - Attributes

- (void)updateAttributes:(NSDictionary *)props {
  if (!props) {
    return;
  }

  BOOL needRender = NO;

  NSString *src = props[@"src"];
  if (src && ![src isEqualToString:self.engine.src]) {
    self.engine.src = [src copy];
    needRender = YES;
  }

  NSString *content = props[@"content"];
  if (content && ![content isEqualToString:self.engine.content]) {
    self.engine.content = [content copy];
    needRender = YES;
  }

  NSString *color = props[@"current-color"];
  if (color) {
    NSString *trimmedColor =
        [color stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    NSString *nextColor = trimmedColor.length != 0 ? trimmedColor : nil;
    if (![nextColor isEqualToString:self.engine.color]) {
      self.engine.color = [nextColor copy];
      needRender = YES;
    }
  }

  if (needRender) {
    [self checkSizeAndRender];
  }
}

#pragma mark - DisplayList

- (void)updateDisplayList:(lynx::tasm::DisplayList *)list {
  [super updateDisplayList:list];

  if (list == nullptr || list->GetContentFloatData() == nullptr ||
      list->GetContentFloatDataSize() < 4) {
    return;
  }

  const float *frame = list->GetContentFloatData();
  CGSize size = CGSizeMake(frame[2], frame[3]);
  CGFloat scale = self.rendererHost.traitCollection.displayScale;
  if (scale == 0) {
    scale = 1.0;
  }
  size.width *= scale;
  size.height *= scale;

  [self renderWithSize:size];
}

#pragma mark - Rendering

- (void)checkSizeAndRender {
  UIView<LynxRendererHost> *host = self.rendererHost;
  if (!host) {
    return;
  }

  CGSize size = host.frame.size;
  CGFloat scale = host.traitCollection.displayScale;
  if (scale == 0) {
    scale = 1.0;
  }
  size.width *= scale;
  size.height *= scale;
  [self renderWithSize:size];
}

- (void)renderWithSize:(CGSize)devSize {
  if (_isDestroyed) {
    return;
  }

  if (devSize.width <= 0 || devSize.height <= 0) {
    return;
  }

  __weak __typeof(self) weakSelf = self;
  [self.engine renderWithSize:devSize
      completion:^(UIImage *_Nullable image) {
        __strong __typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf || strongSelf->_isDestroyed) {
          return;
        }
        [strongSelf applyImage:image];
      }
      errorHandler:^(LynxError *error) {
        __strong __typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf || strongSelf->_isDestroyed) {
          return;
        }
        [strongSelf.context.uiContext reportError:error];
      }];
}

#pragma mark - Image Application

- (void)applyImage:(UIImage *)image {
  if (_isDestroyed) {
    return;
  }

  LynxSVGRendererHost *host = (LynxSVGRendererHost *)self.rendererHost;
  if (!host) {
    return;
  }

  [host setImage:image];

  if (image) {
    [self sendLoadEventWithImage:image];
  }
}

- (void)sendLoadEventWithImage:(UIImage *)image {
  LynxEventEmitter *eventEmitter = self.context.uiContext.eventEmitter;
  if (!eventEmitter) {
    return;
  }

  NSDictionary *params = @{
    @"height" : @(image.size.height),
    @"width" : @(image.size.width),
  };
  LynxCustomEvent *event = [[LynxCustomEvent alloc] initWithName:@"load"
                                                      targetSign:self.sign
                                                          params:params];
  [eventEmitter dispatchCustomEvent:event];
}

@end
