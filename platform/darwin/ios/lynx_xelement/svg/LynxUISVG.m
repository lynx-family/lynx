// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <ServalSVG/SrSVG.h>
#import <XElement/LynxUISVG.h>

#import <XElement/LynxSVGRenderEngine.h>

@interface LynxUISVG ()
@property(nonatomic, strong) LynxSVGRenderEngine *engine;
@end

@implementation LynxUISVG

@dynamic src, content, color;

- (void)invalidateViewOnMainThread {
  if ([NSThread isMainThread]) {
    [self.view invalidate];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self.view invalidate];
    });
  }
}

- (LynxSVGView *)createView {
  LynxSVGView *view = [[LynxSVGView alloc] initWithFrame:CGRectZero ui:self];
  view.clipsToBounds = YES;
  [view setTranslatesAutoresizingMaskIntoConstraints:YES];
  return view;
}

- (void)setContext:(LynxUIContext *)context {
  [super setContext:context];

  if (self.engine) {
    return;
  }

  __weak typeof(self) weakSelf = self;
  self.engine = [[LynxSVGRenderEngine alloc] initWithSrSVG:[[SrSVG alloc] init]
      color:nil
      genericFetcher:self.context.genericResourceFetcher
      mediaFetcher:self.context.mediaResourceFetcher
      asyncExecutor:^void(LynxSVGRenderBlock renderBlock, LynxSVGRenderCompletion completion) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
          if (completion) {
            completion(nil);
          }
          return;
        }
        [strongSelf displayComplexBackgroundAsynchronouslyWithDisplay:renderBlock
                                                           completion:completion];
      }
      reloadBlock:^{
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }
        if ([NSThread isMainThread]) {
          [strongSelf.view invalidate];
        } else {
          dispatch_async(dispatch_get_main_queue(), ^{
            [strongSelf.view invalidate];
          });
        }
      }];
}

#pragma mark - Deprecated property getters/setters

- (NSString *)src {
  return self.engine.src;
}

- (void)setSrc:(NSString *)src {
  self.engine.src = src;
  [self.view invalidate];
}

- (NSString *)content {
  return self.engine.content;
}

- (void)setContent:(NSString *)content {
  self.engine.content = content;
  [self.view invalidate];
}

- (NSString *)color {
  return self.engine.color;
}

- (void)setColor:(NSString *)color {
  self.engine.color = color;
  [self invalidateViewOnMainThread];
}

LYNX_PROP_SETTER("src", setSrc, NSString *) {
  if ([value isKindOfClass:[NSString class]]) {
    if (value.length != 0 || self.src != nil) {
      self.src = value;
    }
  }
}

LYNX_PROP_SETTER("content", setContent, NSString *) {
  if ([value isKindOfClass:[NSString class]]) {
    if (value.length != 0 || self.content != nil) {
      self.content = value;
    }
  }
}

LYNX_PROP_SETTER("current-color", setColor, NSString *) {
  NSString *nextColor = nil;
  if ([value isKindOfClass:[NSString class]]) {
    NSString *trimmedColor = [(NSString *)value
        stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    nextColor = trimmedColor.length != 0 ? trimmedColor : nil;
  }
  if ((nextColor == nil && self.color == nil) || [nextColor isEqualToString:self.color]) {
    return;
  }
  self.color = nextColor;
}

// Call on main thread;
- (void)applyImage:(UIImage *)image {
  if (image == nil || ![NSThread isMainThread]) {
    return;
  }
  [self.view setImage:image];
  static NSString *LynxImageEventLoad = @"load";
  if ([self.eventSet valueForKey:LynxImageEventLoad]) {
    NSDictionary *detail = @{
      @"height" : [NSNumber numberWithFloat:image.size.height],
      @"width" : [NSNumber numberWithFloat:image.size.width]
    };
    [self.context.eventEmitter
        dispatchCustomEvent:[[LynxDetailEvent alloc] initWithName:LynxImageEventLoad
                                                       targetSign:self.sign
                                                           detail:detail]];
  }
}

- (LynxUIMeaningfulContentStatus)meaningfulContentStatus {
  if (self.view.image) {
    return kLynxUIMeaningfulContentStatusPresented;
  }
  return kLynxUIMeaningfulContentStatusPending;
}

- (void)updateLayoutIfNeed {
  LynxSVGView *imageView = self.view;
  CGSize devSize = imageView.frame.size;
  devSize.width *= [UIScreen mainScreen].scale;
  devSize.height *= [UIScreen mainScreen].scale;
  if (devSize.width == 0 || devSize.height == 0) {
    [imageView setImage:nil];
    return;
  }

  __weak typeof(self) weakSelf = self;
  [self.engine renderWithSize:devSize
      completion:^(UIImage *_Nullable image) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }
        [strongSelf applyImage:image];
      }
      errorHandler:^(LynxError *error) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }
        [strongSelf.context reportError:error];
      }];
}

- (void)layoutDidFinished {
  [super layoutDidFinished];
  [self invalidateViewOnMainThread];
}

- (UIImage *)processSVGData:(NSData *)data withSize:(CGSize)devSize {
  return [self.engine renderSVGData:data withSize:devSize];
}

- (UIImage *)loadImageFromHref:(NSString *)href withSize:(CGSize)devSize {
  return [self.engine loadImageFromHref:href withSize:devSize];
}

@end
