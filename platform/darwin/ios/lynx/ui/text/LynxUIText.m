// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxTouchEvent.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIText.h>
#import <Lynx/LynxUIUnitUtils.h>
#import <Lynx/LynxUnitUtils.h>
#import <Lynx/LynxView+Internal.h>

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

static const NSUInteger kLynxTextServiceEventTargetInfoSize = 3;
static const NSUInteger kLynxTextServiceEventTargetSignIndex = 0;
static const NSUInteger kLynxTextServiceEventTargetMaskIndex = 1;
static const NSUInteger kLynxTextServiceEventTargetInlineViewIndex = 2;

typedef NS_OPTIONS(NSUInteger, LynxTextServiceEventTargetMask) {
  LynxTextServiceEventTargetTap = 1u << 0,
  LynxTextServiceEventTargetClick = 1u << 1,
  LynxTextServiceEventTargetLongPress = 1u << 2,
  LynxTextServiceEventTargetTouchStart = 1u << 3,
  LynxTextServiceEventTargetTouchMove = 1u << 4,
  LynxTextServiceEventTargetTouchEnd = 1u << 5,
  LynxTextServiceEventTargetTouchCancel = 1u << 6,
};

static void LynxTextServiceAddEventSpec(NSMutableDictionary<NSString *, LynxEventSpec *> *eventSet,
                                        NSUInteger eventMask,
                                        LynxTextServiceEventTargetMask targetMask,
                                        NSString *eventName) {
  if ((eventMask & targetMask) == 0) {
    return;
  }
  NSString *rawEvent = [eventName stringByAppendingString:@"(bindEvent)"];
  eventSet[eventName] = [[LynxEventSpec alloc] initWithRawEvent:rawEvent withJSEvent:YES];
}

static NSDictionary<NSString *, LynxEventSpec *> *LynxTextServiceBuildEventSet(
    NSUInteger eventMask) {
  if (eventMask == 0) {
    return nil;
  }
  NSMutableDictionary<NSString *, LynxEventSpec *> *eventSet = [NSMutableDictionary new];
  LynxTextServiceAddEventSpec(eventSet, eventMask, LynxTextServiceEventTargetTap, LynxEventTap);
  LynxTextServiceAddEventSpec(eventSet, eventMask, LynxTextServiceEventTargetClick, LynxEventClick);
  LynxTextServiceAddEventSpec(eventSet, eventMask, LynxTextServiceEventTargetLongPress,
                              LynxEventLongPress);
  LynxTextServiceAddEventSpec(eventSet, eventMask, LynxTextServiceEventTargetTouchStart,
                              LynxEventTouchStart);
  LynxTextServiceAddEventSpec(eventSet, eventMask, LynxTextServiceEventTargetTouchMove,
                              LynxEventTouchMove);
  LynxTextServiceAddEventSpec(eventSet, eventMask, LynxTextServiceEventTargetTouchEnd,
                              LynxEventTouchEnd);
  LynxTextServiceAddEventSpec(eventSet, eventMask, LynxTextServiceEventTargetTouchCancel,
                              LynxEventTouchCancel);
  return eventSet.count > 0 ? eventSet : nil;
}

@interface LynxTextServiceEventTarget : NSObject <LynxEventTarget>

- (instancetype)initWithSign:(NSInteger)sign eventMask:(NSUInteger)eventMask;
- (void)setParentEventTarget:(id<LynxEventTarget>)parent;

@end

@implementation LynxTextServiceEventTarget {
  NSInteger _sign;
  __weak id<LynxEventTarget> _parent;
  NSDictionary *_dataset;
  NSDictionary<NSString *, LynxEventSpec *> *_eventSet;
  int32_t _pseudoStatus;
}

- (instancetype)initWithSign:(NSInteger)sign eventMask:(NSUInteger)eventMask {
  self = [super init];
  if (self) {
    _sign = sign;
    _dataset = @{};
    _eventSet = LynxTextServiceBuildEventSet(eventMask);
  }
  return self;
}

- (NSInteger)signature {
  return _sign;
}

- (int32_t)pseudoStatus {
  return _pseudoStatus;
}

- (void)setParentEventTarget:(id<LynxEventTarget>)parent {
  _parent = parent;
}

- (nullable id<LynxEventTarget>)parentTarget {
  __strong id<LynxEventTarget> parent = _parent;
  return parent;
}

- (nullable id<LynxEventTargetBase>)parentResponder {
  id<LynxEventTarget> parent = [self parentTarget];
  if ([parent conformsToProtocol:@protocol(LynxEventTargetBase)]) {
    return (id<LynxEventTargetBase>)parent;
  }
  return nil;
}

- (nullable NSDictionary *)getDataset {
  return _dataset;
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  return self;
}

- (BOOL)containsPoint:(CGPoint)point {
  return YES;
}

- (nullable NSDictionary<NSString *, LynxEventSpec *> *)eventSet {
  return _eventSet;
}

- (nullable NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureMap {
  return nil;
}

- (BOOL)shouldHitTest:(CGPoint)point withEvent:(nullable UIEvent *)event {
  return YES;
}

- (BOOL)ignoreFocus {
  id<LynxEventTarget> parent = [self parentTarget];
  return parent != nil ? [parent ignoreFocus] : NO;
}

- (BOOL)consumeSlideEvent:(CGFloat)angle {
  return NO;
}

- (BOOL)blockNativeEvent:(UIGestureRecognizer *)gestureRecognizer {
  return NO;
}

- (BOOL)eventThrough:(CGPoint)point {
  id<LynxEventTarget> parent = [self parentTarget];
  return parent != nil ? [parent eventThrough:point] : NO;
}

- (enum LynxPointerEventsValue)pointerEvents {
  id<LynxEventTarget> parent = [self parentTarget];
  return parent != nil ? [parent pointerEvents] : kLynxPointerEventsValueAuto;
}

- (enum LynxPanInterceptDirection)panInterceptDirection {
  return kLynxPanInterceptDirectionNone;
}

- (enum LynxPanInterceptScope)panInterceptScope {
  return kLynxPanInterceptScopeNone;
}

- (BOOL)enableTouchPseudoPropagation {
  return YES;
}

- (void)onPseudoStatusFrom:(int32_t)preStatus changedTo:(int32_t)currentStatus {
  _pseudoStatus = currentStatus;
}

- (BOOL)dispatchTouch:(NSString *const)touchType
              touches:(NSSet<UITouch *> *)touches
            withEvent:(UIEvent *)event {
  return NO;
}

- (BOOL)dispatchEvent:(LynxEventDetail *)event {
  return NO;
}

- (void)onResponseChain {
}

- (void)offResponseChain {
}

- (BOOL)isOnResponseChain {
  return NO;
}

- (NSInteger)getGestureArenaMemberId {
  return -1;
}

- (id<LynxEventTarget>)parentLynxPageUI {
  return nil;
}

- (void)setParentLynxPageUI:(id<LynxEventTarget>)ui {
}

- (NSMutableDictionary *)childrenLynxPageUI {
  return nil;
}

- (void)setChildrenLynxPageUI:(NSMutableDictionary *)dict {
}

- (id<LynxEventTarget>)rootLynxPageUI {
  return nil;
}

- (void)setEventID:(int64_t)eventID {
}

- (void)startEventCapture:(int64_t)eventID {
}

- (void)onEventCapture:(BOOL)isCatch withEventID:(int64_t)eventID {
}

- (void)startEventBubble:(int64_t)eventID {
}

- (void)onEventBubble:(BOOL)isCatch withEventID:(int64_t)eventID {
}

- (void)startEventFire:(BOOL)isStop withEventID:(int64_t)eventID {
}

- (void)onEventFire:(BOOL)isStop withEventID:(int64_t)eventID {
}

- (UIView *)view {
  return nil;
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
  BOOL _isDirty;
  BOOL _textGradientOptExperiment;
  BOOL _didDispatchLayoutEvent;
  NSMutableDictionary<NSString *, LynxTextServiceEventTarget *> *_textServiceEventTargetCache;
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
  _textGradientOptExperiment = context.enableTextGradientOpt;
}

- (LynxTextView *)createView {
  LynxTextView *view = [LynxTextView new];
  view.opaque = NO;
  view.contentMode = UIViewContentModeScaleAspectFit;
  view.ui = self;
  return view;
}

- (void)_lynxUIRequestDisplay {
  if (self.renderer == nil || ((self.frame.size.width <= 0 || self.frame.size.height <= 0) &&
                               self.overflow == OVERFLOW_HIDDEN_VAL)) {
    return;
  }

  [self calcOverflowLayerFrame];
  [self requestDisplayAsynchronously];
}

- (void)frameDidChange {
  [super frameDidChange];
  _isDirty = true;
  [self requestDisplay];
}

- (LynxUIMeaningfulContentStatus)meaningfulContentStatus {
  if (self.view.textRenderer) {
    // TODO(@zhouzhuangzhuang): Criteria for determining when text drawing is complete.
    return kLynxUIMeaningfulContentStatusPresented;
  }
  return kLynxUIMeaningfulContentStatusPending;
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  if ([self.context.uiOwner isLayoutInElementModeOn]) {
    LynxTextRenderer *renderer = [self.context.uiOwner.textRenderManager takeTextRender:self.sign];
    [self onReceiveUIOperation:renderer];
    if (renderer && !_didDispatchLayoutEvent && [self.eventSet objectForKey:@"layout"] &&
        self.context.eventEmitter) {
      [self.context.uiOwner.textRenderManager
          dispatchLayoutEventWithRenderer:renderer
                                     sign:self.sign
                             eventEmitter:self.context.eventEmitter];
      _didDispatchLayoutEvent = YES;
    }
  }
  [super updateFrame:frame
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with];

  [self requestDisplay];
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
    LynxTextRenderer *renderer = (LynxTextRenderer *)value;
    if (renderer != _renderer) {
      _didDispatchLayoutEvent = NO;
      [_textServiceEventTargetCache removeAllObjects];
    }
    _isHasSubSpan = false;
    _isDirty = true;
    _renderer = renderer;
    // TODO: remove this config after experiment on stability.
    _renderer.isGradientOpt = _textGradientOptExperiment;

    if (self.useDefaultAccessibilityLabel) {
      self.view.accessibilityLabel = _renderer.attrStr.string;
    }
    self.view.textRenderer = _renderer;
    if (!self.view.selectionChangeEventCallback &&
        [self.eventSet objectForKey:@"selectionchange"]) {
      __weak typeof(self) weakSelf = self;
      self.view.selectionChangeEventCallback = ^(NSDictionary *detail) {
        LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:@"selectionchange"
                                                            targetSign:[weakSelf sign]
                                                                detail:detail];
        [weakSelf.context.eventEmitter dispatchCustomEvent:event];
      };
    }
  }
}

- (void)requestDisplayAsynchronously {
  __weak typeof(self) weakSelf = self;
  [self displayAsyncWithCompletionBlock:^(UIImage *_Nonnull image) {
    CALayer *layer = [weakSelf getOverflowLayer];
    if (weakSelf.overflow != OVERFLOW_HIDDEN_VAL) {
      CGPoint offset = [weakSelf overflowLayerOffset];
      layer.frame = CGRectMake(-offset.x, -offset.y, image.size.width, image.size.height);
    } else {
      layer.frame = CGRectMake(0.f, 0.f, image.size.width, image.size.height);
    }
    layer.contents = (id)image.CGImage;
  }];
}

- (CGSize)frameSize {
  if (self.overflow != OVERFLOW_HIDDEN_VAL) {
    CGRect textBoundingRect = [self.renderer textBoundingRect];
    CGSize size = CGSizeMake(
        MAX(self.frame.size.width, textBoundingRect.size.width + self.padding.left +
                                       self.padding.right + self.border.left + self.border.right),
        MAX(self.frame.size.height,
            textBoundingRect.size.height + self.padding.top + self.border.top));
    return size;
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
  if (self.overflow == OVERFLOW_HIDDEN_VAL || _renderer == nil) {
    return CGPointZero;
  }

  CGRect textBoundingRect = [self.renderer textBoundingRect];
  return CGPointMake(0.f, -textBoundingRect.origin.y);
}

+ (void)drawRect:(CGRect)bounds withParameters:(id)drawParameters {
  LynxUITextDrawParameter *param = drawParameters;
  LynxTextRenderer *renderer = param.renderer;
  UIEdgeInsets padding = param.padding;
  UIEdgeInsets border = param.border;
  bounds.origin = CGPointMake(param.overflowLayerOffset.x, param.overflowLayerOffset.y);
  [renderer drawRect:bounds padding:padding border:border];
}

- (nullable LynxRendererContext *)textServiceRendererContext {
  UIView *view = self.view;
  while (view != nil) {
    if ([view respondsToSelector:@selector(rendererContext)]) {
      LynxRendererContext *context = [(id<LynxRendererHost>)view rendererContext];
      if (context != nil) {
        return context;
      }
    }
    view = view.superview;
  }

  UIView *rootView = self.context.rootView;
  if ([rootView respondsToSelector:@selector(rendererContext)]) {
    return [(id<LynxRendererHost>)rootView rendererContext];
  }
  return nil;
}

- (nullable LynxUI *)textServiceInlineViewWithSign:(NSInteger)sign {
  for (LynxUI *child in self.children) {
    if (child.sign == sign) {
      return child;
    }
  }
  return nil;
}

- (nullable id<LynxEventTarget>)hitTestTextServiceInlineViewWithSign:(NSInteger)sign
                                                               point:(CGPoint)point
                                                           withEvent:(UIEvent *)event {
  LynxUI *child = [self textServiceInlineViewWithSign:sign];
  if (child == nil || ![child shouldHitTest:point withEvent:event] || child.view.isHidden) {
    return nil;
  }
  CALayer *parentLayer = self.view.layer.presentationLayer ?: self.view.layer.modelLayer;
  CALayer *childLayer = child.view.layer.presentationLayer ?: child.view.layer.modelLayer;
  CGPoint targetPoint = [parentLayer convertPoint:point toLayer:childLayer];
  return [child hitTest:targetPoint withEvent:event];
}

- (LynxTextServiceEventTarget *)textServiceEventTargetWithSign:(NSInteger)sign
                                                     eventMask:(NSUInteger)eventMask {
  if (_textServiceEventTargetCache == nil) {
    _textServiceEventTargetCache = [NSMutableDictionary new];
  }
  NSString *cacheKey = [NSString stringWithFormat:@"%ld:%lu", (long)sign, (unsigned long)eventMask];
  LynxTextServiceEventTarget *target = _textServiceEventTargetCache[cacheKey];
  if (target == nil) {
    target = [[LynxTextServiceEventTarget alloc] initWithSign:sign eventMask:eventMask];
    _textServiceEventTargetCache[cacheKey] = target;
  }
  [target setParentEventTarget:self];
  return target;
}

- (nullable id<LynxEventTarget>)hitTestTextServicePage:(CGPoint)point withEvent:(UIEvent *)event {
  LynxRendererContext *rendererContext = [self textServiceRendererContext];
  void *page = [rendererContext getTextBundle:(int32_t)self.sign];
  if (page == NULL) {
    return nil;
  }

  id<LynxServiceTextProtocol> textService = LynxService(LynxServiceTextProtocol);
  if (textService == nil) {
    return nil;
  }

  CGPoint pointInTextRect = CGPointMake(point.x - self.padding.left - self.border.left,
                                        point.y - self.padding.top - self.border.top);
  NSArray<NSNumber *> *targets = [textService getHitTestEventTargetsOfPage:page
                                                           ByTouchPosition:pointInTextRect];
  if (targets.count < kLynxTextServiceEventTargetInfoSize) {
    return nil;
  }

  NSInteger sign = [targets[kLynxTextServiceEventTargetSignIndex] integerValue];
  NSUInteger eventMask = [targets[kLynxTextServiceEventTargetMaskIndex] unsignedIntegerValue];
  BOOL isInlineView = [targets[kLynxTextServiceEventTargetInlineViewIndex] boolValue];

  if (isInlineView) {
    id<LynxEventTarget> inlineViewTarget =
        [self hitTestTextServiceInlineViewWithSign:sign point:pointInTextRect withEvent:event];
    if (inlineViewTarget != nil) {
      return inlineViewTarget;
    }
  } else if (sign == self.sign) {
    return self;
  }

  return [self textServiceEventTargetWithSign:sign eventMask:eventMask];
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  id<LynxEventTarget> textServiceTarget = [self hitTestTextServicePage:point withEvent:event];
  if (textServiceTarget != nil) {
    return textServiceTarget;
  }

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
  [super onNodeReady];

  [self.view initSelectionGesture];
}

- (void)requestDisplay {
  if (!_isDirty) {
    return;
  }
  [self updateAttachmentsFrame];

  self.view.border = self.border;
  self.view.padding = self.padding;

  [_overflow_layer setContents:nil];
  [self.view setNeedsDisplay];

  if ([self enableLayerRender]) {
    if (self.overflow != OVERFLOW_HIDDEN_VAL) {
      CALayer *layer = [self getOverflowLayer];
      [layer setNeedsDisplay];
    }
    [self calcOverflowLayerFrame];
  } else {
    [self _lynxUIRequestDisplay];
  }

  _isDirty = false;
}

- (void)calcOverflowLayerFrame {
  CGPoint overflowOffset = [self overflowLayerOffset];
  [self.view setOverflowOffset:overflowOffset];

  if (!_overflow_layer) {
    return;
  }

  CGSize size = [self frameSize];
  CGRect overflowLayerFrame =
      CGRectMake(-overflowOffset.x, -overflowOffset.y, size.width, size.height);
  _overflow_layer.frame = overflowLayerFrame;
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
 * @param showStartHandle Whether to show start handle
 * @param showEndHandle Whether to show end handle
 * @return The bounding boxes of each line
 */
LYNX_UI_METHOD(setTextSelection) {
  NSNumber *startX = [params objectForKey:@"startX"];
  NSNumber *startY = [params objectForKey:@"startY"];
  NSNumber *endX = [params objectForKey:@"endX"];
  NSNumber *endY = [params objectForKey:@"endY"];
  id showStartHandle = [params objectForKey:@"showStartHandle"];
  id showEndHandle = [params objectForKey:@"showEndHandle"];
  if (!startX || !startY || !endX || !endY) {
    callback(kUIMethodParamInvalid, @"parameter is invalid");
    return;
  }

  NSArray *boxes =
      [self.view setTextSelection:[startX floatValue]
                           startY:[startY floatValue]
                             endX:[endX floatValue]
                             endY:[endY floatValue]
                  showStartHandle:showStartHandle == nil ? YES : [showStartHandle boolValue]
                    showEndHandle:showEndHandle == nil ? YES : [showEndHandle boolValue]];
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
