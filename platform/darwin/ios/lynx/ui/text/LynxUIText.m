// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIText.h"
#import "LynxComponentRegistry.h"
#import "LynxEnv.h"
#import "LynxPropsProcessor.h"
#import "LynxUI+Internal.h"
#import "LynxUIUnitUtils.h"
#import "LynxUnitUtils.h"
#import "LynxView+Internal.h"

@interface LynxUITextDrawParameter : NSObject

@property(nonatomic) LynxTextRenderer *renderer;
@property(nonatomic) UIEdgeInsets padding;
@property(nonatomic) UIEdgeInsets border;
@property(nonatomic) CGPoint overflowLayerOffset;

@end

@interface LynxCALayerDelegate : NSObject <CALayerDelegate>

@end

@implementation LynxCALayerDelegate

- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event {
  return (id)[NSNull null];
}

@end

@implementation LynxUITextDrawParameter

@end

@implementation LynxUIText {
  LynxTextRenderer *_renderer;
  LynxLinearGradient *_gradient;
  LynxTextOverflowLayer *_overflow_layer;
  LynxCALayerDelegate *_delegate;
  BOOL _isHasSubSpan;
}

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("text")
#else
LYNX_REGISTER_UI("text")
#endif

LYNX_PROPS_GROUP_DECLARE(
    LYNX_PROP_DECLARE("text-selection", setEnableTextSelection, BOOL),
    LYNX_PROP_DECLARE("custom-context-menu", setEnableCustomContextMenu, BOOL),
    LYNX_PROP_DECLARE("custom-text-selection", setEnableCustomTextSelection, BOOL),
    LYNX_PROP_DECLARE("selection-background-color", setSelectionBackgroundColor, UIColor *),
    LYNX_PROP_DECLARE("selection-handle-color", setSelectionHandleColor, UIColor *),
    LYNX_PROP_DECLARE("selection-handle-size", setSelectionHandleSize, CGFloat))

- (instancetype)initWithView:(LynxTextView *)view {
  self = [super initWithView:view];
  if (self != nil) {
    // disable text async-display by default
    // user can enable this by adding async-display property on ttml element
    [self setAsyncDisplayFromTTML:NO];
  }
  return self;
}

- (void)setContext:(LynxUIContext *)context {
  [super setContext:context];
  if (self.context.enableTextOverflow) {
    self.overflow = OVERFLOW_XY_VAL;
    self.view.clipsToBounds = NO;
  }
}

- (LynxTextView *)createView {
  LynxTextView *view = [LynxTextView new];
  view.opaque = NO;
  view.contentMode = UIViewContentModeScaleAspectFit;
  view.ui = self;
  return view;
}

- (void)_lynxUIRequestDisplay {
  if (self.renderer == nil || self.frame.size.width <= 0 || self.frame.size.height <= 0) {
    return;
  }
  self.view.layer.contents = nil;
  [self.view.contentLayer setContents:nil];
  [_overflow_layer setContents:nil];
  [self requestDisplayAsynchronsly];
}

- (void)adjustContentLayerPosition {
  CGPoint offset = [self overflowLayerOffset];
  self.view.contentLayer.frame =
      CGRectMake(-offset.x, -offset.y, self.frameSize.width, self.frameSize.height);
  [self.view setOverflowOffset:offset];
}

- (void)frameDidChange {
  [super frameDidChange];
  self.view.contentLayer.frame = CGRectMake(0, 0, self.frameSize.width, self.frameSize.height);
  [self updateAttachmentsFrame];
  if ([self enableLayerRender]) {
    self.view.border = self.border;
    self.view.padding = self.padding;
    [self adjustContentLayerPosition];
    [self.view.contentLayer setNeedsDisplay];
  } else {
    [self _lynxUIRequestDisplay];
  }
}

- (void)updateAttachmentsFrame {
  for (LynxTextAttachmentInfo *attachment in _renderer.attachments) {
    [self.children
        enumerateObjectsUsingBlock:^(LynxUI *_Nonnull child, NSUInteger idx, BOOL *_Nonnull stop) {
          if (child.sign == attachment.sign) {
            CGFloat scale = [UIScreen mainScreen].scale;
            if (attachment.nativeAttachment) {
              if (CGRectIsEmpty(attachment.frame) && ![child.view isHidden]) {
                [child.view setHidden:YES];
                [child.backgroundManager setHidden:YES];
              } else if (!CGRectIsEmpty(attachment.frame) && [child.view isHidden]) {
                [child.view setHidden:NO];
                [child.backgroundManager setHidden:NO];
              }
            } else {
              CGRect frame = attachment.frame;
              frame.origin.x =
                  round(frame.origin.x * scale) / scale + self.padding.left + self.border.left;
              frame.origin.y =
                  round(frame.origin.y * scale) / scale + self.padding.top + self.border.top;
              frame.size.width = round(frame.size.width * scale) / scale;
              frame.size.height = round(frame.size.height * scale) / scale;
              [child updateFrame:frame
                          withPadding:UIEdgeInsetsZero
                               border:UIEdgeInsetsZero
                  withLayoutAnimation:NO];
            }

            *stop = true;
          }
        }];
  }
}

- (void)onReceiveUIOperation:(id)value {
  if (value && [value isKindOfClass:LynxTextRenderer.class]) {
    _isHasSubSpan = false;
    _renderer = value;

    [self updateAttachmentsFrame];

    if (self.useDefaultAccessibilityLabel) {
      self.view.accessibilityLabel = _renderer.attrStr.string;
    }
    self.view.textRenderer = _renderer;
    if ([self enableLayerRender]) {
      [self adjustContentLayerPosition];
      [self.view.contentLayer setNeedsDisplay];
    } else {
      [self _lynxUIRequestDisplay];
    }
    if (!self.view.selectionChangeEventCallback &&
        [self.eventSet objectForKey:@"selectionchange"]) {
      __weak typeof(self) weakSelf = self;
      self.view.selectionChangeEventCallback = ^(NSDictionary *detail) {
        LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:@"selectionchange"
                                                            targetSign:[self sign]
                                                                detail:detail];
        [weakSelf.context.eventEmitter dispatchCustomEvent:event];
      };
    }
  }
}

- (void)requestDisplayAsynchronsly {
  __weak typeof(self) weakSelf = self;
  [self displayAsyncWithCompletionBlock:^(UIImage *_Nonnull image) {
    CALayer *layer = nil;
    if (weakSelf.overflow != OVERFLOW_HIDDEN_VAL) {
      layer = [weakSelf getOverflowLayer];
      CGPoint offset = [weakSelf overflowLayerOffset];
      layer.frame = CGRectMake(-offset.x, -offset.y, image.size.width, image.size.height);
    } else {
      layer = weakSelf.view.contentLayer;
    }
    layer.contents = (id)image.CGImage;
    layer.contentsScale = [LynxUIUnitUtils screenScale];
  }];
}

- (CGSize)frameSize {
  if (self.overflow != OVERFLOW_HIDDEN_VAL) {
    CGSize size = [_renderer textsize];
    CGFloat width = size.width > self.frame.size.width ? size.width : self.frame.size.width;
    CGFloat height = size.height > self.frame.size.height ? size.height : self.frame.size.height;
    CGPoint offset = [self overflowLayerOffset];
    return CGSizeMake(width + 2 * offset.x, height + 2 * offset.y);
  }
  return self.frame.size;
}

- (void)addOverflowLayer {
  _overflow_layer = [[LynxTextOverflowLayer alloc] initWithView:self.view];
  if (_delegate == nil) {
    _delegate = [[LynxCALayerDelegate alloc] init];
  }
  _overflow_layer.delegate = _delegate;
  [self.view.layer addSublayer:_overflow_layer];
}

- (CALayer *)getOverflowLayer {
  if (!_overflow_layer) {
    [self addOverflowLayer];
  }
  CGPoint offset = self.overflowLayerOffset;
  _overflow_layer.frame =
      CGRectMake(-offset.x, -offset.y, self.frameSize.width, self.frameSize.height);
  return _overflow_layer;
}

- (LynxTextRenderer *)renderer {
  return _renderer;
}

- (NSString *)accessibilityText {
  return _renderer.attrStr.string;
}

- (void)dealloc {
  // TODO refactor
  if (_overflow_layer) {
    if ([NSThread isMainThread]) {
      _overflow_layer.delegate = nil;
    } else {
      LynxTextOverflowLayer *overflow_layer = _overflow_layer;
      dispatch_async(dispatch_get_main_queue(), ^{
        overflow_layer.delegate = nil;
      });
    }
  }
}

- (id)drawParameter {
  LynxUITextDrawParameter *para = [[LynxUITextDrawParameter alloc] init];
  para.renderer = self.renderer;
  para.border = self.backgroundManager.borderWidth;
  para.padding = self.padding;
  para.overflowLayerOffset = [self overflowLayerOffset];
  return para;
}

- (CGPoint)overflowLayerOffset {
  if (self.overflow == 0x00 || _renderer == nil) {
    return CGPointZero;
  }
  // TODO use a more suitable offset?
  if ([self enableLayerRender]) {
    // we use half fontSize to adjust the layer ,for avoiding text clip issue
    return CGPointMake(0, _renderer.maxFontSize / 2);
  }

  return CGPointMake(0, _renderer.maxFontSize);
}

+ (void)drawRect:(CGRect)bounds withParameters:(id)drawParameters {
  LynxUITextDrawParameter *param = drawParameters;
  LynxTextRenderer *renderer = param.renderer;
  UIEdgeInsets padding = param.padding;
  UIEdgeInsets border = param.border;
  bounds.origin = CGPointMake(param.overflowLayerOffset.x, param.overflowLayerOffset.y);
  [renderer drawRect:bounds padding:padding border:border];
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  if (!_isHasSubSpan) {
    [self.renderer genSubSpan];
    _isHasSubSpan = true;
  }
  CGPoint pointInTextRect = CGPointMake(point.x - self.padding.left - self.border.left,
                                        point.y - self.padding.top - self.border.top);
  for (LynxEventTargetSpan *span in _renderer.subSpan) {
    if ([span containsPoint:pointInTextRect]) {
      [span setParentEventTarget:self];
      return span;
    }
  }
  return [super hitTest:point withEvent:event];
}

- (BOOL)enableAccessibilityByDefault {
  return YES;
}

- (UIAccessibilityTraits)accessibilityTraitsByDefault {
  return UIAccessibilityTraitStaticText;
}

- (BOOL)enableAsyncDisplay {
  BOOL isIOSAppOnMac = NO;
  if (@available(iOS 14.0, *)) {
    // https://github.com/firebase/firebase-ios-sdk/issues/6969
    isIOSAppOnMac = ([[NSProcessInfo processInfo] respondsToSelector:@selector(isiOSAppOnMac)] &&
                     [NSProcessInfo processInfo].isiOSAppOnMac);
  }
  // https://t.wtturl.cn/R91Suay/
  // if running on Mac with M1 chip, disable async render
  return [super enableAsyncDisplay] && !isIOSAppOnMac;
}

- (BOOL)enableLayerRender {
  return self.context.enableTextLayerRender;
}

- (void)onNodeReady {
  [self.view initSelectionGesture];
}

#pragma mark prop setter

LYNX_PROP_DEFINE("text-selection", setEnableTextSelection, BOOL) {
  if (requestReset) {
    value = NO;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    ((LynxUIText *)ui).view.enableTextSelection = value;
  }];
}

LYNX_PROP_DEFINE("custom-context-menu", setEnableCustomContextMenu, BOOL) {
  if (requestReset) {
    value = NO;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    ((LynxUIText *)ui).view.enableCustomContextMenu = value;
  }];
}

LYNX_PROP_DEFINE("custom-text-selection", setEnableCustomTextSelection, BOOL) {
  if (requestReset) {
    value = NO;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    ((LynxUIText *)ui).view.enableCustomTextSelection = value;
  }];
}

LYNX_PROP_DEFINE("selection-background-color", setSelectionBackgroundColor, UIColor *) {
  if (requestReset) {
    value = nil;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [((LynxUIText *)ui).view updateSelectionColor:value];
  }];
}

LYNX_PROP_DEFINE("selection-handle-color", setSelectionHandleColor, UIColor *) {
  if (requestReset) {
    value = nil;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [((LynxUIText *)ui).view updateHandleColor:value];
  }];
}

LYNX_PROP_DEFINE("selection-handle-size", setSelectionHandleSize, CGFloat) {
  if (requestReset) {
    value = 0.f;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [((LynxUIText *)ui).view updateHandleSize:value];
  }];
}

#pragma mark UI_Method

/**
 * @brief Returns the bounding box, boundingRect  of the specified text range.
 * @param params
 * start: The start position of the specified range.
 * end: The end position of the specified range.
 * @param callback
 * boundingRect: The bounding box of the selected text
 * boxes: The bounding boxes for each line
 */
LYNX_UI_METHOD(getTextBoundingRect) {
  NSNumber *start = [params objectForKey:@"start"];
  NSNumber *end = [params objectForKey:@"end"];
  if (!start || !end || [start intValue] < 0 || [end intValue] < 0) {
    callback(kUIMethodParamInvalid, @"parameter is invalid");
    return;
  }

  NSArray *boxes = [self.view getTextBoundingBoxes:[start integerValue] withEnd:[end integerValue]];
  if (boxes.count > 0) {
    CGRect rect = [self getRelativeBoundingClientRect:params];
    NSDictionary *result = [self getTextBoundingRectFromBoxes:boxes textRect:rect];
    callback(kUIMethodSuccess, [result copy]);
    return;
  }

  callback(kUIMethodUnknown, @"Can not find text bounding rect.");
}

LYNX_UI_METHOD(getSelectedText) {
  NSString *selectedText = [self.view getSelectedText];
  callback(kUIMethodSuccess, @{@"selectedText" : selectedText});
}

/**
 * @brief Set text selection.
 * @param startX The x-coordinate of the start of the selected text relative to the text component
 * @param startY The y-coordinate of the start of the selected text relative to the text component
 * @param endX The x-coordinate of the end of the selected text relative to the text component
 * @param endY The y-coordinate of the end of the selected text relative to the text component
 * @return The bounding boxes of each line
 */
LYNX_UI_METHOD(setTextSelection) {
  NSNumber *startX = [params objectForKey:@"startX"];
  NSNumber *startY = [params objectForKey:@"startY"];
  NSNumber *endX = [params objectForKey:@"endX"];
  NSNumber *endY = [params objectForKey:@"endY"];
  if (!startX || !startY || !endX || !endY) {
    callback(kUIMethodParamInvalid, @"parameter is invalid");
    return;
  }

  NSArray *boxes = [self.view setTextSelection:[startX floatValue]
                                        startY:[startY floatValue]
                                          endX:[endX floatValue]
                                          endY:[endY floatValue]];
  if (boxes.count == 0) {
    callback(kUIMethodSuccess, @{});
  } else {
    CGRect rect = [self getRelativeBoundingClientRect:params];
    NSMutableDictionary *result = [self getTextBoundingRectFromBoxes:boxes textRect:rect];
    NSArray *handles = [self.view getHandlesInfo];
    NSMutableArray *handleArray = [NSMutableArray array];
    for (NSArray *handle in handles) {
      [handleArray addObject:[self getHandleMap:handle textRect:rect]];
    }
    result[@"handles"] = handleArray;
    callback(kUIMethodSuccess, [result copy]);
  }
}

- (NSDictionary *)getHandleMap:(NSArray *)handle textRect:(CGRect)rect {
  NSMutableDictionary *ret = [NSMutableDictionary dictionary];
  ret[@"x"] = @(rect.origin.x + [handle[0] floatValue] + self.padding.left + self.border.left);
  ret[@"y"] = @(rect.origin.y + [handle[1] floatValue] + self.padding.top + self.border.top);
  ret[@"radius"] = handle[2];

  return ret;
}

- (NSMutableDictionary *)getTextBoundingRectFromBoxes:(NSArray *)boxes textRect:(CGRect)textRect {
  NSMutableDictionary *result = [NSMutableDictionary dictionary];
  if (boxes.count == 0) {
    return result;
  }

  CGRect boundingRect = [boxes[0] CGRectValue];
  for (NSUInteger i = 0; i < boxes.count; i++) {
    boundingRect = CGRectUnion(boundingRect, [boxes[i] CGRectValue]);
  }

  result[@"boundingRect"] = [self getMapFromRect:textRect lineBox:boundingRect];

  NSMutableArray *boxList = [NSMutableArray array];
  for (NSValue *value in boxes) {
    CGRect lineBox = [value CGRectValue];
    [boxList addObject:[self getMapFromRect:textRect lineBox:lineBox]];
  }
  result[@"boxes"] = boxList;

  return result;
}

- (NSDictionary *)getMapFromRect:(CGRect)textRect lineBox:(CGRect)lineBox {
  NSMutableDictionary *map = [NSMutableDictionary dictionary];

  map[@"left"] =
      @(textRect.origin.x + CGRectGetMinX(lineBox) + self.padding.left + self.border.left);
  map[@"top"] = @(textRect.origin.y + CGRectGetMinY(lineBox) + self.padding.top + self.border.top);
  map[@"right"] =
      @(textRect.origin.x + CGRectGetMaxX(lineBox) + self.padding.left + self.border.left);
  map[@"bottom"] =
      @(textRect.origin.y + CGRectGetMaxY(lineBox) + self.padding.top + self.border.top);
  map[@"width"] = @(CGRectGetWidth(lineBox));
  map[@"height"] = @(CGRectGetHeight(lineBox));

  return map;
}

@end
