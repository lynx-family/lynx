// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIFilterImage.h"
#import "LynxBackgroundUtils.h"
#import "LynxBlurImageProcessor.h"
#import "LynxColorUtils.h"
#import "LynxComponentRegistry.h"
#import "LynxConvertUtils.h"
#import "LynxFatImageProcessor.h"
#import "LynxImageLoader.h"
#import "LynxImageProcessor.h"
#import "LynxPropsProcessor.h"
#import "LynxService.h"
#import "LynxServiceImageProtocol.h"
#import "LynxSubErrorCode.h"
#import "LynxUIContext+Internal.h"
#import "LynxUnitUtils.h"

@interface LynxUIFilterImage ()
@property(nonatomic, assign) UIViewContentMode resizeMode;
@property(nonatomic) NSString* src;
@property(nonatomic) UIImage* originalImage;
@end

@implementation LynxUIFilterImage {
  UIViewContentMode _resizeMode;
  NSString* _src;
  UIImage* _originalImage;
  UIImageView* _imageView;
  CGPoint _shadowOffset;
  UIColor* _shadowColor;
  CGFloat _shadowRadius;
  CGFloat _blurRadius;
  BOOL _isDirty;
}

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("filter-image")
#else
LYNX_REGISTER_UI("filter-image")
#endif

- (instancetype)init {
  self = [super init];
  if (self) {
    _resizeMode = UIViewContentModeScaleToFill;
    _isDirty = NO;
  }
  return self;
}

- (UIView*)createView {
  UIImageView* image = [UIImageView new];
  image.clipsToBounds = YES;
  // Default contentMode UIViewContentModeScaleToFill
  image.contentMode = UIViewContentModeScaleToFill;
  return image;
}

- (void)upateFrameWithoutLayerMask {
  // Update bounds
  const UIEdgeInsets border = self.backgroundManager.borderWidth;
  CGFloat x = self.padding.left + border.left;
  CGFloat y = self.padding.top + border.top;
  CGFloat width =
      self.frame.size.width - self.padding.left - self.padding.right - border.left - border.right;
  CGFloat height =
      self.frame.size.height - self.padding.top - self.padding.bottom - border.top - border.bottom;
  CGPoint origin = [self view].frame.origin;
  // change bound is not affect to image origin, so we change frame directly
  [self view].frame = CGRectMake(origin.x + x, origin.y + y, width, height);
}

- (bool)updateLayerMaskOnFrameChanged {
  // we do not need to run super, as overflow is not used for image,
  // border-radius will be processed by myself
  if (CGSizeEqualToSize(self.frame.size, CGSizeZero)) {
    return false;
  }
  const bool hasRadii = LynxHasBorderRadii(self.backgroundManager.borderRadius);
  if (hasRadii) {
    UIEdgeInsets borderWidth = self.backgroundManager.borderWidth;
    CGPathRef pathRef = [LynxBackgroundUtils
        createBezierPathWithRoundedRect:CGRectMake(self.contentOffset.x, self.contentOffset.y,
                                                   self.frame.size.width, self.frame.size.height)
                            borderRadii:self.backgroundManager.borderRadius
                             edgeInsets:borderWidth];
    CAShapeLayer* shapeLayer = [[CAShapeLayer alloc] init];
    shapeLayer.path = pathRef;
    CGPathRelease(pathRef);
    self.view.layer.mask = shapeLayer;
  } else if (!(UIEdgeInsetsEqualToEdgeInsets(self.backgroundManager.borderWidth,
                                             UIEdgeInsetsZero) &&
               UIEdgeInsetsEqualToEdgeInsets(self.padding, UIEdgeInsetsZero))) {
    [self upateFrameWithoutLayerMask];
  }

  return true;
}

- (void)frameDidChange {
  [super frameDidChange];
  [self requestImage];
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  [self requestImage];
}

- (void)requestImage {
  if (!_src || self.frame.size.width == 0 || self.frame.size.height == 0) {
    return;
  }
  NSURL* url = [[NSURL alloc] initWithString:_src];
  LynxFatImageProcessor* fatImageProcessor =
      [[LynxFatImageProcessor alloc] initWithSize:self.frame.size
                                          padding:self.padding
                                           border:self.backgroundManager.borderWidth
                                      contentMode:_resizeMode];
  LynxBlurImageProcessor* blurImageProcessor =
      [[LynxBlurImageProcessor alloc] initWithBlurRadius:_blurRadius];
  NSArray* processorChain = @[ fatImageProcessor, blurImageProcessor ];
  __weak typeof(self) weakSelf = self;
  static NSString* LynxImageEventError = @"error";
  static NSString* LynxImageEventLoad = @"load";
  // FIXME(linxs:) change to imageService once imageService is ready!
  LynxURL* requestUrl = [[LynxURL alloc] init];
  requestUrl.url = url;
  [[LynxImageLoader sharedInstance]
      loadImageFromLynxURL:requestUrl
                      size:self.frame.size
               contextInfo:@{LynxImageFetcherContextKeyUI : self}
                processors:processorChain
              imageFetcher:self.context.imageFetcher
               LynxUIImage:nil
      enableGenericFetcher:NO
                 completed:^(UIImage* _Nullable image, NSError* _Nullable error,
                             NSURL* _Nullable imageURL) {
                   typeof(weakSelf) strongSelf = weakSelf;
                   if (!error) {
                     strongSelf.view.image = image;
                     if ([strongSelf.eventSet valueForKey:LynxImageEventLoad]) {
                       NSDictionary* detail = @{
                         @"height" : [NSNumber numberWithFloat:image.size.height],
                         @"width" : [NSNumber numberWithFloat:image.size.width]
                       };
                       [strongSelf.context.eventEmitter
                           dispatchCustomEvent:[[LynxDetailEvent alloc]
                                                   initWithName:LynxImageEventLoad
                                                     targetSign:self.sign
                                                         detail:detail]];
                     }
                   } else {
                     strongSelf.view.image = nil;
                     NSString* errorDetail = [NSString stringWithFormat:@"%@", [error userInfo]];
                     NSNumber* errorCode = [error.userInfo valueForKey:@"error_num"]
                                               ?: [NSNumber numberWithInteger:error.code];
                     NSNumber* categorizedErrorCode = [LynxService(LynxServiceImageProtocol)
                         getMappedCategorizedPicErrorCode:errorCode];
                     if ([strongSelf.eventSet valueForKey:LynxImageEventError]) {
                       NSDictionary* detail = @{@"errMsg" : errorDetail};
                       [strongSelf.context.eventEmitter
                           dispatchCustomEvent:[[LynxDetailEvent alloc]
                                                   initWithName:LynxImageEventError
                                                     targetSign:self.sign
                                                         detail:detail]];
                     }
                     NSInteger subErrCode = categorizedErrorCode
                                                ? [categorizedErrorCode integerValue]
                                                : ECLynxResourceImageException;
                     NSString* errMsg = @"Error when load filter-image";
                     // pass LynxError to LynxLifecycleDispatcher
                     LynxError* err = [LynxError lynxErrorWithCode:subErrCode
                                                           message:errMsg
                                                     fixSuggestion:@""
                                                             level:LynxErrorLevelError];
                     [err setRootCause:errorDetail];
                     [strongSelf.context didReceiveResourceError:err
                                                      withSource:url ? url.absoluteString : @""
                                                            type:@"image"];
                   }
                 }];
}

LYNX_PROP_SETTER("src", setSrc, NSString*) {
  if (requestReset) {
    self.view.image = nil;
    _src = nil;
    return;
  }
  if (_src != value) {
    _src = value;
  }
}

LYNX_PROP_SETTER("mode", setMode, UIViewContentMode) {
  if (requestReset) {
    value = UIViewContentModeScaleAspectFill;
  }
  if (_resizeMode != value || self.view.contentMode != value) {
    _resizeMode = value;
  }
}

LYNX_PROP_SETTER("blur-radius", setBlurRadius, NSString*) {
  if (requestReset) {
    _blurRadius = 0;
  } else {
    LynxUI* rootUI = (LynxUI*)self.context.rootUI;
    UIView* rootView = self.context.rootView;
    LynxScreenMetrics* screenMetrics = self.context.screenMetrics;
    _blurRadius = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                             unitValue:value
                                          rootFontSize:rootUI.fontSize
                                           curFontSize:self.fontSize
                                             rootWidth:rootView.frame.size.width
                                            rootHeight:rootView.frame.size.height
                                         withDefaultPt:0];
  }
}

LYNX_PROP_SETTER("drop-shadow", setDropShadow, NSString*) {
  if (requestReset) {
    self.view.layer.shadowOffset = CGSizeMake(0, 0);
    self.view.layer.shadowRadius = 0;
    self.view.layer.shadowOpacity = 0.f;
  } else {
    NSArray<NSString*>* items = [value componentsSeparatedByString:@" "];
    LynxScreenMetrics* screenMetrics = self.context.screenMetrics;
    if (items.count == 4) {
      LynxUI* rootUI = (LynxUI*)self.context.rootUI;
      UIView* rootView = self.context.rootView;
      CGFloat offsetX = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                                   unitValue:items[0]
                                                rootFontSize:rootUI.fontSize
                                                 curFontSize:self.fontSize
                                                   rootWidth:rootView.frame.size.width
                                                  rootHeight:rootView.frame.size.height
                                               withDefaultPt:0];
      CGFloat offsetY = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                                   unitValue:items[1]
                                                rootFontSize:rootUI.fontSize
                                                 curFontSize:self.fontSize
                                                   rootWidth:rootView.frame.size.width
                                                  rootHeight:rootView.frame.size.height
                                               withDefaultPt:0];
      CGFloat shadowRadius = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                                        unitValue:items[2]
                                                     rootFontSize:rootUI.fontSize
                                                      curFontSize:self.fontSize
                                                        rootWidth:rootView.frame.size.width
                                                       rootHeight:rootView.frame.size.height
                                                    withDefaultPt:0];
      UIColor* shadowColor = [LynxColorUtils convertNSStringToUIColor:items[3]];
      self.view.layer.shadowColor = shadowColor.CGColor;
      self.view.layer.shadowOffset = CGSizeMake(offsetX, offsetY);
      self.view.layer.shadowRadius = shadowRadius;
      self.view.layer.shadowOpacity = 1.f;
    }
  }
}

@end
