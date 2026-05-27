// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxView.h>
#import <ServalSVG/SrSVG.h>
#import <XElement/LynxUISVG.h>

@interface LynxUISVG ()
@property(nonatomic, strong) SrSVG *srSvg;
@end

@implementation LynxUISVG

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
  self.srSvg = [[SrSVG alloc] init];
  view.clipsToBounds = YES;
  [view setTranslatesAutoresizingMaskIntoConstraints:YES];
  return view;
}

LYNX_PROP_SETTER("src", setSrc, NSString *) {
  if ([value isKindOfClass:[NSString class]]) {
    if (value.length != 0 || _src != nil) {
      // src is not empty or src is changed to empty
      _src = value;
      [self.view invalidate];
    }
  }
}

LYNX_PROP_SETTER("content", setContent, NSString *) {
  if ([value isKindOfClass:[NSString class]]) {
    if (value.length != 0 || _content != nil) {
      // content is not empty or content is changed to empty
      _content = value;
      [self.view invalidate];
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
  if ((nextColor == nil && _color == nil) || [nextColor isEqualToString:_color]) {
    return;
  }
  _color = [nextColor copy];
  [self invalidateViewOnMainThread];
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

- (UIImage *)loadImageFromHref:(NSString *)href withSize:(CGSize)devSize {
  if (self.imageHolder == nil) {
    self.imageHolder = [LynxThreadSafeDictionary new];
  }
  __weak typeof(self) weakSelf = self;
  UIImage *image = self.imageHolder[href];
  if (!image) {
    [self.imageHolder setObject:[NSNull null] forKey:href];

    [self fetchSVGImage:href
               withSize:devSize
               complete:^(UIImage *_Nullable uiImage, NSError *_Nullable error) {
                 if (uiImage) {
                   [weakSelf.imageHolder setObject:uiImage forKey:href];
                   if ([NSThread isMainThread]) {
                     [weakSelf.view invalidate];
                   } else {
                     dispatch_async(dispatch_get_main_queue(), ^{
                       [weakSelf.view invalidate];
                     });
                   }
                 }
               }];
  }
  if ((NSNull *)image != [NSNull null]) {
    return image;
  }
  return nil;
}

- (UIImage *)processSVGData:(NSData *)data withSize:(CGSize)devSize {
  if (data == nil || data.length == 0) {
    return nil;
  }
  __weak typeof(self) weakSelf = self;

  return [self.srSvg getSrSvgDrawImageWithData:data
                                       andSize:devSize
                                      andColor:self.color
                                   andCallback:^UIImage *_Nullable(NSString *_Nullable href) {
                                     return [weakSelf loadImageFromHref:href withSize:devSize];
                                   }];
}

- (void)updateLayoutIfNeed {
  LynxSVGView *imageView = self.view;
  // Image inside could be blurry due to the screen resolution after scaling.
  CGSize devSize = imageView.frame.size;
  devSize.width *= [UIScreen mainScreen].scale;
  devSize.height *= [UIScreen mainScreen].scale;
  if (devSize.width == 0 || devSize.height == 0) {
    // Clear the dirty mark before return.
    [self.view setImage:nil];
    return;
  }

  __weak typeof(self) weakSelf = self;
  if (_src) {
    if ([_src length] == 0) {
      [imageView setImage:nil];
      return;
    }
    NSURL *fileUrl = nil;
    NSURL *baseUrl = nil;
    if ([_src hasPrefix:@"./"]) {
      if ([self.context.rootView isKindOfClass:[LynxView class]]) {
        baseUrl = [NSURL URLWithString:[(LynxView *)self.context.rootView url]];
      }
      fileUrl = [NSURL URLWithString:[_src substringFromIndex:2]];
    } else {
      fileUrl = [NSURL URLWithString:_src];
    }
    if (!fileUrl) {
      LynxError *lynxError = [LynxError lynxErrorWithCode:ECLynxResourceImagePicSource
                                                  message:@"SVG cannot parse this src"
                                            fixSuggestion:@"Please check the validity of the src"
                                                    level:LynxErrorLevelError
                                               customInfo:@{@"src" : _src}];
      [self.context reportError:lynxError];
      return;
    }

    void (^completionBlock)(NSData *_Nullable svgData, NSError *_Nullable error) =
        ^(NSData *_Nullable svgData, NSError *_Nullable error) {
          if (!svgData) {
            return;
          }
          [self
              displayComplexBackgroundAsynchronouslyWithDisplay:^UIImage *() {
                __strong typeof(weakSelf) strongSelf = weakSelf;
                return [strongSelf processSVGData:svgData withSize:devSize];
              }
              completion:^(UIImage *_Nonnull image) {
                __strong typeof(weakSelf) strongSelf = weakSelf;
                [strongSelf applyImage:image];
              }];
        };

    [self fetchSVGResource:_src complete:completionBlock];

  } else if (_content) {
    if ([_content length] == 0) {
      [imageView setImage:nil];
      return;
    }
    NSData *svgData = [[_content stringByReplacingOccurrencesOfString:@"&quot;" withString:@"\""]
        dataUsingEncoding:NSUTF8StringEncoding];
    [self
        displayComplexBackgroundAsynchronouslyWithDisplay:^UIImage *() {
          __strong typeof(weakSelf) strongSelf = weakSelf;
          return [strongSelf processSVGData:svgData withSize:devSize];
        }
        completion:^(UIImage *_Nonnull image) {
          __strong typeof(weakSelf) strongSelf = weakSelf;
          [strongSelf applyImage:image];
        }];
  } else {
    [self.view setImage:nil];
  }
}

- (void)fetchSVGResource:(NSString *)src
                complete:(LynxGenericResourceCompletionBlock _Nonnull)callback {
  LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:src
                                                                     type:LynxResourceTypeSVG];
  [self.context.genericResourceFetcher fetchResource:request onComplete:callback];
}

- (void)fetchSVGImage:(NSString *_Nonnull)url
             withSize:(CGSize)devSize
             complete:(LynxMediaResourceCompletionBlock _Nonnull)callback {
  (void)devSize;
  [self.context.mediaResourceFetcher fetchUIImage:[[LynxResourceRequest alloc] initWithUrl:url]
                                       onComplete:callback];
}

- (void)layoutDidFinished {
  [super layoutDidFinished];
  [self invalidateViewOnMainThread];
}

@end
