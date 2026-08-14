// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxSVGRenderer.h"

#import <Lynx/LynxError.h>
#import <Lynx/LynxEvent.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxResourceRequest.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxUIContext.h>
#import <ServalSVG/SrSVG.h>
#import <UIKit/UIKit.h>

@interface LynxSVGRenderer () {
  NSString *_src;
  NSString *_content;
  NSString *_color;
  CGSize _lastHostSize;
  NSUInteger _renderGeneration;
  BOOL _renderScheduled;
  SrSVG *_srSvg;
  NSMutableDictionary<NSString *, id> *_imageHolder;
}

@end

static CGSize LynxSVGDeviceSize(UIView *host, CGSize hostSize) {
  CGFloat scale = host.traitCollection.displayScale;
  if (scale == 0) {
    scale = UIScreen.mainScreen.scale;
  }
  if (scale == 0) {
    scale = 1.0;
  }
  return CGSizeMake(hostSize.width * scale, hostSize.height * scale);
}

static void LynxSVGSetHostImage(UIView *host, UIImage *image) {
  host.layer.contents = (__bridge id)image.CGImage;
  host.layer.contentsScale = host.traitCollection.displayScale;
}

static dispatch_queue_t LynxSVGRenderQueue(void) {
  static dispatch_queue_t queue = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    queue = dispatch_queue_create("com.lynx.svg.renderer.renderQueue", DISPATCH_QUEUE_SERIAL);
    dispatch_set_target_queue(queue, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0));
  });
  return queue;
}

@implementation LynxSVGRenderer

- (void)updateAttributes:(NSDictionary *)props {
  if (![props isKindOfClass:NSDictionary.class]) {
    return;
  }

  BOOL shouldRender = NO;

  id nextSrc = props[@"src"];
  if ([nextSrc isKindOfClass:NSString.class]) {
    NSString *src = nextSrc;
    if ((src.length != 0 || _src != nil) && ![src isEqualToString:_src]) {
      _src = [src copy];
      shouldRender = YES;
    }
  }

  id nextContent = props[@"content"];
  if ([nextContent isKindOfClass:NSString.class]) {
    NSString *content = nextContent;
    if ((content.length != 0 || _content != nil) && ![content isEqualToString:_content]) {
      _content = [content copy];
      shouldRender = YES;
    }
  }

  id nextColor = props[@"current-color"];
  if (nextColor != nil) {
    NSString *color = nil;
    if ([nextColor isKindOfClass:NSString.class]) {
      color = [nextColor
          stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
      color = color.length == 0 ? nil : color;
    }
    if ((color == nil && _color != nil) || (color != nil && ![color isEqualToString:_color])) {
      _color = [color copy];
      shouldRender = YES;
    }
  }

  if (shouldRender) {
    [self requestRender];
  }
}

- (void)onSetFrame:(CGRect)frame {
  if (CGSizeEqualToSize(frame.size, _lastHostSize)) {
    return;
  }
  _lastHostSize = frame.size;
  [self requestRenderImmediately];
}

- (void)requestRender {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf requestRender];
    });
    return;
  }

  ++_renderGeneration;
  if (_renderScheduled) {
    return;
  }
  _renderScheduled = YES;

  __weak typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil || !strongSelf->_renderScheduled) {
      return;
    }
    strongSelf->_renderScheduled = NO;
    [strongSelf renderCurrentState:strongSelf->_renderGeneration];
  });
}

- (void)requestRenderImmediately {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf requestRenderImmediately];
    });
    return;
  }

  _renderScheduled = NO;
  NSUInteger generation = ++_renderGeneration;
  [self renderCurrentState:generation];
}

- (void)renderCurrentState:(NSUInteger)generation {
  CGSize deviceSize = LynxSVGDeviceSize(self.rendererHost, _lastHostSize);
  UIView *host = self.rendererHost;

  if (deviceSize.width <= 0 || deviceSize.height <= 0) {
    LynxSVGSetHostImage(host, nil);
    return;
  }

  NSString *src = [_src copy];
  if (src != nil) {
    if (src.length == 0) {
      LynxSVGSetHostImage(host, nil);
      return;
    }
    NSString *urlString = [src hasPrefix:@"./"] ? [src substringFromIndex:2] : src;
    if ([NSURL URLWithString:urlString] == nil) {
      LynxError *lynxError = [LynxError lynxErrorWithCode:ECLynxResourceImagePicSource
                                                  message:@"SVG cannot parse this src"
                                            fixSuggestion:@"Please check the validity of the src"
                                                    level:LynxErrorLevelError
                                               customInfo:@{@"src" : src}];
      [self.context.uiContext reportError:lynxError];
      return;
    }

    __weak typeof(self) weakSelf = self;
    LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:src
                                                                       type:LynxResourceTypeSVG];
    [self.context.uiContext.genericResourceFetcher
        fetchResource:request
           onComplete:^(NSData *_Nullable svgData, NSError *_Nullable error) {
             __strong typeof(weakSelf) strongSelf = weakSelf;
             if (strongSelf == nil || strongSelf->_renderGeneration != generation ||
                 svgData.length == 0) {
               return;
             }
             [strongSelf renderSVGData:svgData withSize:deviceSize generation:generation];
           }];
    return;
  }

  NSString *content = [_content copy];
  if (content.length == 0) {
    LynxSVGSetHostImage(host, nil);
    return;
  }
  NSData *svgData = [[content stringByReplacingOccurrencesOfString:@"&quot;" withString:@"\""]
      dataUsingEncoding:NSUTF8StringEncoding];
  [self renderSVGData:svgData withSize:deviceSize generation:generation];
}

- (void)renderSVGData:(NSData *)svgData
             withSize:(CGSize)deviceSize
           generation:(NSUInteger)generation {
  if (svgData.length == 0) {
    return;
  }

  SrSVG *srSvg = _srSvg ?: (_srSvg = [[SrSVG alloc] init]);
  NSString *color = [_color copy];
  __weak typeof(self) weakSelf = self;
  dispatch_async(LynxSVGRenderQueue(), ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil) {
      return;
    }
    UIImage *image =
        [srSvg getSrSvgDrawImageWithData:svgData
                                 andSize:deviceSize
                                andColor:color
                             andCallback:^UIImage *_Nullable(NSString *_Nullable href) {
                               return [strongSelf loadImageFromHref:href];
                             }];

    dispatch_async(dispatch_get_main_queue(), ^{
      __strong typeof(weakSelf) innerStrongSelf = weakSelf;
      if (innerStrongSelf == nil || innerStrongSelf->_renderGeneration != generation ||
          image == nil) {
        return;
      }
      LynxSVGSetHostImage([innerStrongSelf rendererHost], image);
      NSDictionary *params = @{
        @"height" : @(image.size.height),
        @"width" : @(image.size.width),
      };
      [innerStrongSelf.context.uiContext.eventEmitter
          dispatchCustomEvent:[[LynxDetailEvent alloc] initWithName:@"load"
                                                         targetSign:innerStrongSelf.sign
                                                             detail:params]];
    });
  });
}

- (UIImage *)loadImageFromHref:(NSString *)href {
  if (href.length == 0) {
    return nil;
  }

  NSString *key = [href copy];
  @synchronized(self) {
    if (_imageHolder == nil) {
      _imageHolder = [NSMutableDictionary dictionary];
    }
    id image = _imageHolder[key];
    if (image == NSNull.null) {
      return nil;
    }
    if (image != nil) {
      return image;
    }
    _imageHolder[key] = NSNull.null;
  }

  __weak typeof(self) weakSelf = self;
  [self.context.uiContext.mediaResourceFetcher
      fetchUIImage:[[LynxResourceRequest alloc] initWithUrl:key]
        onComplete:^(UIImage *_Nullable image, NSError *_Nullable error) {
          if (image == nil) {
            return;
          }
          __strong typeof(weakSelf) strongSelf = weakSelf;
          if (strongSelf == nil) {
            return;
          }
          @synchronized(strongSelf) {
            strongSelf->_imageHolder[key] = image;
          }
          [strongSelf requestRender];
        }];

  return nil;
}

@end
