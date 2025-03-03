// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUI.h"
#import "AbsLynxUIScroller.h"
#import "LynxAnimationTransformRotation.h"
#import "LynxBackgroundDrawable.h"
#import "LynxBackgroundUtils.h"
#import "LynxBaseGestureHandler.h"
#import "LynxBasicShape.h"
#import "LynxBoxShadowManager.h"
#import "LynxCSSType.h"
#import "LynxColorUtils.h"
#import "LynxContext.h"
#import "LynxConverter+LynxCSSType.h"
#import "LynxConverter+Transform.h"
#import "LynxConverter+UI.h"
#import "LynxEvent.h"
#import "LynxEventHandler+Internal.h"
#import "LynxFeatureCounter.h"
#import "LynxFilterUtil.h"
#import "LynxGestureArenaManager.h"
#import "LynxGestureArenaMember.h"
#import "LynxGestureDetectorDarwin.h"
#import "LynxGlobalObserver.h"
#import "LynxHeroTransition.h"
#import "LynxKeyframeAnimator.h"
#import "LynxLayoutAnimationManager.h"
#import "LynxLog.h"
#import "LynxPropsProcessor.h"
#import "LynxRootUI.h"
#import "LynxService.h"
#import "LynxServiceSystemInvokeProtocol.h"
#import "LynxSizeValue.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTransformOriginRaw.h"
#import "LynxTransformRaw.h"
#import "LynxTransitionAnimationManager.h"
#import "LynxUI+Accessibility.h"
#import "LynxUI+Gesture.h"
#import "LynxUI+Internal.h"
#import "LynxUI+Private.h"
#import "LynxUICollection.h"
#import "LynxUIContext+Internal.h"
#import "LynxUIIntersectionObserver.h"
#import "LynxUIListContainer.h"
#import "LynxUIMethodProcessor.h"
#import "LynxUIScroller.h"
#import "LynxUIUnitUtils.h"
#import "LynxUnitUtils.h"
#import "LynxVersion.h"
#import "LynxVersionUtils.h"
#import "LynxView+Internal.h"
#import "LynxView.h"
#import "UIView+Lynx.h"

static const short OVERFLOW_X_VAL = 0x01;
static const short OVERFLOW_Y_VAL = 0x02;
short const OVERFLOW_XY_VAL = 0x03;
short const OVERFLOW_HIDDEN_VAL = 0x00;

#define IS_ZERO(num) (fabs(num) < 0.0000000001)

@interface LynxUI () <NSCopying>
// transition animation
@property(nonatomic, strong, nullable) LynxTransitionAnimationManager* transitionAnimationManager;
// layout animation
@property(nonatomic, strong, nullable) LynxLayoutAnimationManager* layoutAnimationManager;
@property(nonatomic, assign) CGRect lastUpdatedFrame;
@property(nonatomic, assign) BOOL didTransformChanged;
@property(nonatomic, strong) Class accessibilityAttachedCellClass;
@property(nonatomic, strong) UIView* accessibilityAttachedCell;
@property(nonatomic, assign) BOOL accessibilityAutoScroll;
@property(nonatomic, strong) NSArray* accessibilityBeingExclusiveFocusedNodes;
@property(nonatomic, assign) NSInteger gestureArenaMemberId;

// accessibility
@property(nonatomic, nullable, strong) NSString* lynxAccessibilityStatus;
@property(nonatomic, strong) NSString* lynxAccessibilityLabel;

- (void)prepareKeyframeManager;
- (void)prepareLayoutAnimationManager;
- (void)prepareTransitionAnimationManager;

@end

@implementation LynxUI {
  UIView* _view;
  __weak LynxUIContext* _context;
  __weak CAShapeLayer* _overflowMask;

  // Indicate whether the UI needs to trigger a redraw.
  BOOL _needDisplay;

  BOOL _userInteractionEnabled;
  // angles to consume slide events
  NSMutableArray<NSArray<NSNumber*>*>* _angleArray;
  // block all native event of this element
  BOOL _blockNativeEvent;
  // block native event at some areas of this element
  NSArray<NSArray<LynxSizeValue*>*>* _blockNativeEventAreas;
  // Default value is false. When setting _simultaneousTouch as true and clicking to the ui or its
  // sub ui, the lynx touch gestures will not fail.
  BOOL _enableSimultaneousTouch;
  BOOL _enableTouchPseudoPropagation;
  enum LynxEventPropStatus _ignoreFocus;
  double _touchSlop;
  BOOL _onResponseChain;
  enum LynxEventPropStatus _eventThrough;
  enum LynxPropStatus _enableExposureUIMargin;
  NSDictionary<NSString*, LynxEventSpec*>* _eventSet;
  NSDictionary<NSNumber*, LynxBaseGestureHandler*>* _gestureHandlers;
  CGFloat _filter_amount;
  LynxFilterType _filter_type;
  BOOL _autoResumeAnimation;
  BOOL _enableReuseAnimationState;
  NSMutableArray<LynxAnimationInfo*>* _animationInfos;
}

- (id)copyWithZone:(nullable NSZone*)zone {
  LynxUI* ui = [[self.class alloc] initWithView:nil];
  [self.children
      enumerateObjectsUsingBlock:^(LynxUI* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        [ui insertChild:[obj copy] atIndex:idx];
      }];

  if (self.lynxProps && self.lynxProps.count != 0) {
    for (NSString* key in self.lynxProps) {
      [LynxPropsProcessor updateProp:self.lynxProps[key] withKey:key forUI:ui];
    }
  }
  [ui.lynxProps addEntriesFromDictionary:self.lynxProps];
  [ui propsDidUpdate];

  [ui updateFrameWithoutLayoutAnimation:self.frame
                            withPadding:self.padding
                                 border:self.border
                                 margin:self.margin];
  [ui layoutDidFinished];

  [ui onNodeReady];

  return ui;
}

- (void)initProperties {
  _lynxProps = [NSMutableDictionary dictionary];
  _sign = NSNotFound;
  _backgroundManager = [[LynxBackgroundManager alloc] initWithUI:self];
  _fontSize = 14;
  _needDisplay = YES;
  _userInteractionEnabled = YES;
  _angleArray = nil;
  _blockNativeEvent = NO;
  _blockNativeEventAreas = nil;
  _enableSimultaneousTouch = NO;
  _enableTouchPseudoPropagation = YES;
  _ignoreFocus = kLynxEventPropUndefined;
  _eventThrough = kLynxEventPropUndefined;
  // touch slop's default value is 8 the same as Android.
  _touchSlop = 8;
  _onResponseChain = NO;
  _transformRaw = nil;
  _transformOriginRaw = nil;
  _asyncDisplayFromTTML = YES;
  _dataset = [NSDictionary dictionary];
  _useDefaultAccessibilityLabel = YES;
  _isFirstAnimatedReady = YES;
  _filter_amount = -1;
  _filter_type = LynxFilterTypeNone;
  _updatedFrame = CGRectZero;
  _lastUpdatedFrame = CGRectZero;
  _overflow = OVERFLOW_HIDDEN_VAL;
  _lastTransformRotation = [[LynxAnimationTransformRotation alloc] init];
  _lastTransformWithoutRotate = CATransform3DIdentity;
  _lastTransformWithoutRotateXY = CATransform3DIdentity;
  _autoResumeAnimation = YES;
  _enableNewTransformOrigin = YES;
  _enableReuseAnimationState = YES;
  _enableExposureUIMargin = kLynxPropUndefined;
  _animationInfos = nil;
}

- (instancetype)init {
  return [self initWithView:nil];
}

- (instancetype)initWithView:(UIView*)view {
  self = [super init];
  if (self) {
    [self initProperties];
    _view = view ? view : [self createView];
    _view.isAccessibilityElement = self.enableAccessibilityByDefault;
    _view.accessibilityTraits = self.accessibilityTraitsByDefault;
    _view.accessibilityLabel = nil;
    _view.lynxClickable = self.accessibilityClickable;
  }
  return self;
}

- (instancetype)initWithoutView {
  self = [super init];
  if (self) {
    [self initProperties];
  }
  return self;
}

- (UIView*)view {
  return _view;
}

- (void)setView:(UIView*)view {
  _view = view;
  _view.lynxSign = [NSNumber numberWithInteger:self.sign];
}

- (UIView*)createView {
  NSAssert(false, @"You must override %@ in a subclass", NSStringFromSelector(_cmd));
  return nil;
}

- (void)updateCSSDefaultValue {
  if (_context.cssAlignWithLegacyW3c) {
    [_backgroundManager makeCssDefaultValueToFitW3c];
  }
}

- (void)setSign:(NSInteger)sign {
  _sign = sign;
  self.view.lynxSign = [NSNumber numberWithInteger:sign];
}

- (void)setContext:(LynxUIContext*)context {
  _context = context;
  _autoResumeAnimation = _context.defaultAutoResumeAnimation;
  _enableNewTransformOrigin = _context.defaultEnableNewTransformOrigin;
}

- (LynxUIContext*)context {
  return _context;
}

- (void)dispatchMoveToWindow:(UIWindow*)window {
  if (self.view.window != window) {
    [self willMoveToWindow:window];
    for (LynxUI* child in self.children) {
      [child dispatchMoveToWindow:window];
    }
  }
}

- (CGPoint)contentOffset {
  return CGPointZero;
}

- (BOOL)isScrollContainer {
  return NO;
}

- (BOOL)isOverlay {
  return NO;
}

- (void)setContentOffset:(CGPoint)contentOffset {
}

/**
 * Leaf node or container that has custom layout may need padding
 */
- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  if ([_context isTouchMoving] && !CGRectIsEmpty(self.updatedFrame) &&
      (fabs(frame.origin.x - self.updatedFrame.origin.x) > CGFLOAT_EPSILON ||
       fabs(frame.origin.y - self.updatedFrame.origin.y) > CGFLOAT_EPSILON)) {
    // The front-end modify the layout properties, causing the UI to slide.
    [_context onPropsChangedByUI:self];
  }
  if (!CGRectEqualToRect(self.updatedFrame, frame) ||
      !UIEdgeInsetsEqualToEdgeInsets(_padding, padding) ||
      !UIEdgeInsetsEqualToEdgeInsets(_border, border)) {
    self.updatedFrame = frame;
    // remove layout ani before next updateFrame
    // Do not remove transition transform animation here.
    [_layoutAnimationManager removeAllLayoutAnimation];
    if (with) {
      [self updateFrameWithLayoutAnimation:frame withPadding:padding border:border margin:margin];
    } else {
      [_transitionAnimationManager removeAllLayoutTransitionAnimation];
      [self updateFrameWithoutLayoutAnimation:frame
                                  withPadding:padding
                                       border:border
                                       margin:margin];
    }
  }
  [self sendLayoutChangeEvent];
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
    withLayoutAnimation:(BOOL)with {
  [self updateFrame:frame
              withPadding:padding
                   border:border
                   margin:UIEdgeInsetsZero
      withLayoutAnimation:with];
}

- (void)setEnableNested:(BOOL)value requestReset:(BOOL)requestReset {
  // override by subclasses
}

- (void)updateSticky:(NSArray*)info {
  if (info == nil || [info count] < 4) {
    _sticky = nil;
    return;
  }
  LynxUI* uiParent = (LynxUI*)self.parent;
  if ([uiParent isKindOfClass:[LynxUIScroller class]]) {
    LynxUIScroller* parent = (LynxUIScroller*)uiParent;
    parent.enableSticky = YES;
    _sticky = info;
  } else {
    return;
  }
}

- (void)checkStickyOnParentScroll:(CGFloat)offsetX withOffsetY:(CGFloat)offsetY {
  if (_sticky == nil) {
    return;
  }
  LynxUIScroller* parent = self.parent;
  CGFloat left = self.frame.origin.x;
  CGFloat top = self.frame.origin.y;
  CGPoint trans = CGPointZero;
  if (top - offsetY <= [_sticky[1] floatValue]) {
    trans.y = offsetY + [_sticky[1] floatValue] - top;
  } else {
    CGFloat scrollHeight = parent.frame.size.height;
    CGFloat bottom = scrollHeight - top - self.frame.size.height;
    if (bottom + offsetY <= [_sticky[3] floatValue]) {
      trans.y = offsetY + bottom - [_sticky[3] floatValue];
    } else {
      trans.y = 0;
    }
  }
  if (left - offsetX <= [_sticky[0] floatValue]) {
    trans.x = offsetX + [_sticky[0] floatValue] - left;
  } else {
    CGFloat scrollWidth = parent.frame.size.width;
    CGFloat right = scrollWidth - left - self.frame.size.width;
    if (right + offsetX <= [_sticky[2] floatValue]) {
      trans.x = offsetX + right - [_sticky[2] floatValue];
    } else {
      trans.x = 0;
    }
  }
  [self.backgroundManager setPostTranslate:trans];
}

- (void)updateFrameWithoutLayoutAnimation:(CGRect)frame
                              withPadding:(UIEdgeInsets)padding
                                   border:(UIEdgeInsets)border
                                   margin:(UIEdgeInsets)margin {
  _padding = padding;
  _border = border;
  _margin = margin;
  self.frame = frame;
  [self frameDidChange];
}

- (void)onLayoutAnimationStart:(CGRect)frame {
}

- (void)onLayoutAnimationEnd:(CGRect)frame {
}

- (void)updateManagerRelated {
  _backgroundManager.backgroundInfo.paddingWidth = _padding;
  [_backgroundManager applyEffect];
  if (_filter_type != LynxFilterTypeNone) {
    id filter = [self getFilterWithType:LynxFilterTypeGrayScale];
    if (filter) {
      [_backgroundManager setFilters:@[ filter ]];
    }
  }
}

- (void)propsDidUpdate {
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui.context addUIToExposedMap:ui];
  }];
  // Notify property have changed.
  [_context.observer notifyProperty:nil];
  [self updateManagerRelated];
}

- (void)setAsyncDisplayFromTTML:(BOOL)async {
  _asyncDisplayFromTTML = async;
}

- (void)onAnimationNodeReady {
  // 1. Apply transform
  if ([self shouldReDoTransform]) {
    [self applyTransform];
  }
  // 2. Apply transition
  if (_transitionAnimationManager) {
    [_transitionAnimationManager applyTransitionAnimation];
  }
  // 3. Apply keyframe
  if (nil != _animationManager) {
    [_animationManager notifyAnimationUpdated];
  }
}

- (void)afterAnimatedNodeReady {
  // Update some properties related to animation
  if (_isFirstAnimatedReady) {
    _isFirstAnimatedReady = NO;
  }
  _didTransformChanged = NO;
  _lastUpdatedFrame = _updatedFrame;
}

- (void)markNeedDisplay {
  _needDisplay = YES;
}

- (void)onNodeReadyForUIOwner {
  if (_readyBlockArray) {
    NSArray* blockArray = [_readyBlockArray copy];
    for (dispatch_block_t ready in blockArray) {
      ready();
    }
  }
  [_readyBlockArray removeAllObjects];
  // to override if need to watch onNodeReady and remember to call super after override

  if (_nodeReadyBlockArray) {
    NSArray* blockArray = [_nodeReadyBlockArray copy];
    [_nodeReadyBlockArray removeAllObjects];
    for (LynxNodeReadyBlock ready in blockArray) {
      ready(self);
    }
  }

  // to override if need to watch onNodeReady and remember to call super after override

  if (_needDisplay) {
    _needDisplay = NO;
    [self.view setNeedsDisplay];
  }
  // If override, need to check _needDisplay and call [self.view setNeedsDisplay].

  [self handleAccessibility:self.accessibilityAttachedCell autoScroll:self.accessibilityAutoScroll];

  [self onAnimationNodeReady];
  [self afterAnimatedNodeReady];

  [self onNodeReady];
}

- (void)propsDidUpdateForUIOwner {
  if (_propsDidUpdateBlockArray) {
    NSArray* array = [_propsDidUpdateBlockArray copy];
    [_propsDidUpdateBlockArray removeAllObjects];
    for (LynxPropsDidUpdateBlockReadyBlock block in array) {
      block(self);
    }
  }

  [self propsDidUpdate];
}

- (void)onNodeReady {
}

- (void)onNodeReload {
}

- (void)clearOverflowMask {
  if (_overflowMask != nil) {
    _overflowMask = self.view.layer.mask = nil;
  }
}

- (bool)updateLayerMaskOnFrameChanged {
  // TODO(renzhongyue): modify this function such that it can run on the async thread.
  LYNX_ASSERT_ON_MAIN_THREAD;

  if (_clipPath) {
    CAShapeLayer* mask = [[CAShapeLayer alloc] init];
    UIBezierPath* path = [_clipPath pathWithFrameSize:self.frameSize];
    mask.path = path.CGPath;
    _overflowMask = self.view.layer.mask = mask;
    return true;
  }
  if (CGSizeEqualToSize(self.frame.size, CGSizeZero)) {
    return false;
  }

  if (_overflow == OVERFLOW_XY_VAL) {
    self.view.clipsToBounds = NO;
    if (_overflowMask != nil) {
      _overflowMask = self.view.layer.mask = nil;
    }
    return true;
  }

  bool hasDifferentRadii = false;
  if (_overflow == 0) {
    hasDifferentRadii = [self.backgroundManager hasDifferentBorderRadius];
    if (!hasDifferentRadii) {
      self.view.clipsToBounds = YES;
      if (_overflowMask != nil) {
        _overflowMask = self.view.layer.mask = nil;
      }
      return true;
    }
  }

  if (self.view.layer.mask != nil && self.view.layer.mask != _overflowMask) {
    // mask is used, we could not set overflow
    return false;
  }

  self.view.clipsToBounds = FALSE;

  CGPathRef pathRef = nil;
  if (_overflow == 0 && hasDifferentRadii) {
    pathRef = [LynxBackgroundUtils
        createBezierPathWithRoundedRect:CGRectMake(self.contentOffset.x, self.contentOffset.y,
                                                   self.frame.size.width, self.frame.size.height)
                            borderRadii:self.backgroundManager.borderRadius];

  } else {
    const CGSize screenSize = _context.screenMetrics.screenSize;
    CGFloat x = 0, y = 0, width = self.frame.size.width, height = self.frame.size.height;
    if ((_overflow & OVERFLOW_X_VAL) != 0) {
      x -= screenSize.width;
      width += 2 * screenSize.width;
    }
    if ((_overflow & OVERFLOW_Y_VAL) != 0) {
      y -= screenSize.height;
      height += 2 * screenSize.height;
    }
    pathRef = CGPathCreateWithRect(CGRectMake(self.contentOffset.x + x, self.contentOffset.y + y,
                                              MAX(width, 0), MAX(height, 0)),
                                   nil);
  }

  CAShapeLayer* shapeLayer = [[CAShapeLayer alloc] init];
  shapeLayer.path = pathRef;
  CGPathRelease(pathRef);

  _overflowMask = self.view.layer.mask = shapeLayer;

  return true;
}

- (void)willMoveToWindow:(UIWindow*)window {
  if (_context.eventEmitter && _eventSet && _eventSet.count != 0) {
    static NSString* LynxEventAttach = @"attach";
    static NSString* LynxEventDettach = @"detach";
    if (window && [_eventSet valueForKey:LynxEventAttach]) {
      [_context.eventEmitter
          dispatchCustomEvent:[[LynxCustomEvent alloc] initWithName:LynxEventAttach
                                                         targetSign:_sign]];
    } else if (!window && [_eventSet valueForKey:LynxEventDettach]) {
      [_context.eventEmitter
          dispatchCustomEvent:[[LynxCustomEvent alloc] initWithName:LynxEventDettach
                                                         targetSign:_sign]];
    }
  }
  [_backgroundManager updateShadow];

  if (window) {
    if ([self respondsToSelector:@selector(targetOnScreen)]) {
      [self targetOnScreen];
    }
    [_animationManager resumeAnimation];
  } else {
    if ([self respondsToSelector:@selector(targetOffScreen)]) {
      [self targetOffScreen];
    }
  }
}

- (void)frameDidChange {
  if (!self.parent || ![self.parent hasCustomLayout]) {
    if (!_backgroundManager.implicitAnimation) {
      [CATransaction begin];
      [CATransaction setDisableActions:YES];
    }
    if (CATransform3DIsIdentity(self.view.layer.transform)) {
      self.view.frame = self.frame;
    } else {
      CGRect bounds = self.frame;
      bounds.origin = self.view.bounds.origin;
      self.view.bounds = bounds;
      if (!_enableNewTransformOrigin) {
        self.view.center = CGPointMake(self.frame.origin.x + self.frame.size.width / 2,
                                       self.frame.origin.y + self.frame.size.height / 2);
      } else {
        CGFloat newCenterX, newCenterY;
        newCenterX = self.frame.origin.x + self.frame.size.width * self.view.layer.anchorPoint.x;
        newCenterY = self.frame.origin.y + self.frame.size.height * self.view.layer.anchorPoint.y;
        self.view.center = CGPointMake(newCenterX, newCenterY);
      }
    }

    [self updateLayerMaskOnFrameChanged];
    [self updateManagerRelated];
    if (!_backgroundManager.implicitAnimation) {
      [CATransaction commit];
    }
  }
}

- (BOOL)childrenContainPoint:(CGPoint)point {
  BOOL contain = NO;
  if (_context.enableEventRefactor) {
    for (LynxUI* ui in self.children) {
      CALayer* parentLayer = self.view.layer.presentationLayer ?: self.view.layer.modelLayer;
      CALayer* childLayer = ui.view.layer.presentationLayer ?: ui.view.layer.modelLayer;
      CGPoint newPoint = [parentLayer convertPoint:point toLayer:childLayer];
      if ([ui shouldHitTest:newPoint withEvent:nil]) {
        contain = contain || [ui containsPoint:newPoint];
      }
    }
    return contain;
  }

  CGPoint offset = self.frame.origin;
  CGPoint newPoint = CGPointMake(point.x + self.contentOffset.x - offset.x - [self getTransationX],
                                 point.y + self.contentOffset.y - offset.y - [self getTransationY]);
  for (LynxUI* ui in self.children) {
    if ([ui shouldHitTest:newPoint withEvent:nil]) {
      contain = contain || [ui containsPoint:newPoint];
    }
  }
  return contain;
}

- (CGPoint)getHitTestPoint:(CGPoint)inPoint {
  return CGPointMake(inPoint.x + self.contentOffset.x - self.getTransationX - self.frame.origin.x,
                     inPoint.y + self.contentOffset.y - self.getTransationY - self.frame.origin.y);
}

- (CGRect)getHitTestFrameWithFrame:(CGRect)frame {
  // frame should calculate translate and scale
  float scaleX = self.scaleX;
  float scaleY = self.scaleY;
  float centerX = frame.origin.x + frame.size.width / 2.0f;
  float centerY = frame.origin.y + frame.size.height / 2.0f;
  float rectX = centerX - frame.size.width * scaleX / 2.0f + self.getTransationX;
  float rectY = centerY - frame.size.height * scaleY / 2.0f + self.getTransationY;
  CGRect transFrame =
      CGRectMake(rectX, rectY, frame.size.width * scaleX, frame.size.height * scaleY);

  if (transFrame.size.width + _hitSlopLeft + _hitSlopRight >= CGFLOAT_EPSILON &&
      transFrame.size.height + _hitSlopTop + _hitSlopBottom >= CGFLOAT_EPSILON) {
    transFrame.origin.x -= _hitSlopLeft;
    transFrame.origin.y -= _hitSlopTop;
    transFrame.size.width += _hitSlopLeft + _hitSlopRight;
    transFrame.size.height += _hitSlopTop + _hitSlopBottom;
  }

  return transFrame;
}

- (CGRect)getHitTestFrame {
  return [self getHitTestFrameWithFrame:self.frame];
}

- (LynxUI*)hitTest:(CGPoint)point withEvent:(UIEvent*)event onUIWithCustomLayout:(LynxUI*)ui {
  UIView* view = [ui.view hitTest:point withEvent:event];
  if (view == ui.view || !view) {
    return nil;
  }

  UIView* targetViewWithUI = view;
  while (view.superview != ui.view) {
    view = view.superview;
    if (view.lynxSign) {
      targetViewWithUI = view;
    }
  }
  for (LynxUI* child in ui.children) {
    if (child.view == targetViewWithUI) {
      return child;
    }
  }
  return nil;
}

- (void)insertChild:(LynxUI*)child atIndex:(NSInteger)index {
  // main layer and its super layer
  CALayer* mainLayer = child.view.layer;
  CALayer* superLayer = self.view.layer;
  LynxBackgroundManager* mgr = [child backgroundManager];

  // insert the child & its view into the proper position;
  [self didInsertChild:child atIndex:index];
  [self.view insertSubview:[child view] atIndex:index];

  // adjust its layer's position
  // if the current LynxUI is not at the beginning of children
  if (index > 0) {
    LynxUI* siblingUI = [self.children objectAtIndex:index - 1];

    if (!siblingUI) {
      LLogError(@"siblingUI at index%ld is nil", (long)(index - 1));
    }

    // check if the index of the left neighbor's rightmost layer(aka. the top layer) is greater than
    // the index of the 'mainLayer' if so, we need to move the 'mainLayer' to the right
    if ([superLayer.sublayers indexOfObject:[siblingUI topLayer]] >
        [superLayer.sublayers indexOfObject:mainLayer]) {
      // view operations
      [child.view removeFromSuperview];
      [self.view insertSubview:child.view aboveSubview:siblingUI.view];
      // layer operations
      [mainLayer removeFromSuperlayer];
      [superLayer insertSublayer:mainLayer above:[siblingUI topLayer]];
    }
  }

  // if the current LynxUI is not at the end of children
  if ((NSUInteger)index < [self.children count] - 1) {
    LynxUI* siblingUI = [self.children objectAtIndex:index + 1];

    if (!siblingUI) {
      LLogError(@"siblingUI at index%ld is nil", (long)(index + 1));
    }

    // check if the index of the right neighbor's leftmost layer(aka. the bottom layer) is less than
    // the index of the 'mainLayer' if so, we need to move the 'mainLayer' to the left
    if ([superLayer.sublayers indexOfObject:[siblingUI bottomLayer]] <
        [superLayer.sublayers indexOfObject:mainLayer]) {
      // view operations
      [child.view removeFromSuperview];
      [self.view insertSubview:child.view belowSubview:siblingUI.view];
      // layer operations
      [mainLayer removeFromSuperlayer];
      [superLayer insertSublayer:mainLayer below:[siblingUI bottomLayer]];
    }
  }

  // append the borderLayer & backgroundLayer
  if (mgr) {
    // if the borderLayer exists:
    if (mgr.borderLayer) {
      [mgr.borderLayer removeFromSuperlayer];
      if (OVERFLOW_HIDDEN_VAL == child.overflow) {
        [superLayer insertSublayer:mgr.borderLayer above:mainLayer];
      } else {
        // Border should below content to enable subview overflow the bounds.
        [superLayer insertSublayer:mgr.borderLayer below:mainLayer];
      }
    }

    // if the backgroundLayer exists:
    if (mgr.backgroundLayer) {
      [mgr.backgroundLayer removeFromSuperlayer];
      if (OVERFLOW_HIDDEN_VAL != child.overflow && mgr.borderLayer) {
        // backgroundLayer | borderLayer | mainLayer
        // To enable overflow.
        [superLayer insertSublayer:mgr.backgroundLayer below:mgr.borderLayer];
      } else {
        // backgroundLayer | mainLayer | <optional> borderLayer
        // overflow: hidden.
        [superLayer insertSublayer:mgr.backgroundLayer below:mainLayer];
      }
    }
  }
}

- (void)didInsertChild:(LynxUI*)child atIndex:(NSInteger)index {
  [super insertChild:child atIndex:index];
}

- (void)willRemoveComponent:(LynxUI*)child {
  [[child view] removeFromSuperview];
}

- (void)willMoveToSuperComponent:(LynxUI*)newSuperUI {
  [super willMoveToSuperComponent:newSuperUI];
  [self dispatchMoveToWindow:newSuperUI ? newSuperUI.view.window : nil];

  // the insertion (of associated layers) will be handled inside LynxUI::insertchild
  // deletion is handled here
  if (!newSuperUI) {
    [_backgroundManager removeAssociateLayers];
  }

  if (!newSuperUI) {
    [_context.intersectionManager removeAttachedIntersectionObserver:self];
  }
}

- (void)onReceiveUIOperation:(id)value {
}

- (void)layoutDidFinished {
}

- (void)finishLayoutOperation {  // before layoutDidFinished
}

- (BOOL)hasCustomLayout {
  return NO;
}

- (BOOL)hasTranslateDiff:(NSArray*)transform {
  NSArray<LynxTransformRaw*>* oldTransform = [LynxTransformRaw toTransformRaw:transform];
  BOOL translateXDiff = fabs([LynxTransformRaw getTranslateX:_transformRaw] -
                             [LynxTransformRaw getTranslateX:oldTransform]) > CGFLOAT_EPSILON;
  BOOL translateYDiff = fabs([LynxTransformRaw getTranslateY:_transformRaw] -
                             [LynxTransformRaw getTranslateY:oldTransform]) > CGFLOAT_EPSILON;
  return translateXDiff || translateYDiff;
}

- (CGRect)frameFromParent {
  CGRect result = self.frame;
  LynxUI* parent = self.parent;
  while (parent != nil) {
    result.origin.x = result.origin.x + parent.frame.origin.x;
    result.origin.y = result.origin.y + parent.frame.origin.y;
    parent = parent.parent;
  }
  return result;
}

- (void)setRawEvents:(NSSet<NSString*>*)events andLepusRawEvents:(NSSet<NSString*>*)lepusEvents {
  _eventSet = [LynxEventSpec convertRawEvents:events andRwaLepusEvents:lepusEvents];
  [self eventDidSet];
}

#pragma mark - LynxNewGesture

- (void)setGestureDetectorState:(NSInteger)gestureId state:(LynxGestureState)state {
  [[self getGestureArenaManager] setGestureDetectorState:gestureId
                                                memberId:[self getGestureArenaMemberId]
                                                   state:state];
}

// Handle whether internal lynxUI of the current gesture node consume the gesture and whether
// native view outside the current node (outside of lynxView) consume the gesture.
- (void)consumeGesture:(NSInteger)gestureId params:(NSDictionary*)params {
  BOOL inner = [(params[@"inner"] ?: @(YES)) boolValue];
  BOOL consume = [(params[@"consume"] ?: @(YES)) boolValue];
  if (inner) {
    [self consumeInternalGesture:consume];
  }
}

- (void)consumeInternalGesture:(BOOL)consume {
  // Override by sub class
}

- (LynxGestureArenaManager*)getGestureArenaManager {
  return [self.context.uiOwner gestureArenaManager];
}

- (void)setGestureDetectors:(NSSet<LynxGestureDetectorDarwin*>*)detectors {
  // Check if detectors are provided and not empty.
  if (detectors == nil || detectors.count == 0) {
    return;
  }

  // Initialize a dictionary to store gesture detectors.
  NSMutableDictionary<NSNumber*, LynxGestureDetectorDarwin*>* gestureMap =
      [NSMutableDictionary dictionary];

  // Populate the gesture map with detectors using their IDs as keys.
  for (LynxGestureDetectorDarwin* detector in detectors) {
    NSNumber* key = @(detector.gestureID);
    gestureMap[key] = detector;
  }

  // Update the gesture map and trigger the gestureDidSet method.
  _gestureMap = gestureMap;
  [self gestureDidSet];
}

- (void)eventDidSet {
}

- (void)gestureDidSet {
  if (!self.context.enableNewGesture) {
    return;
  }

  LynxGestureArenaManager* manager = [self getGestureArenaManager];
  if (![manager isMemberExist:[self getGestureArenaMemberId]]) {
    _gestureArenaMemberId = [manager addMember:self];
    [manager registerGestureDetectors:_gestureArenaMemberId detectorMap:_gestureMap];
  }
}
- (float)getScrollX __attribute__((deprecated("Do not use this after lynx 2.5"))) {
  return 0;
}

- (float)getScrollY __attribute__((deprecated("Do not use this after lynx 2.5"))) {
  return 0;
}

- (void)resetContentOffset {
  // override this in scroll-view related classes
}

- (void)applyRTL:(BOOL)rtl {
  // override by subclasses
}

- (LynxUI*)getParent {
  return self.parent;
}

- (float)getTransationX {
  return [[[self getPresentationLayer] valueForKeyPath:@"transform.translation.x"] floatValue];
}

- (float)getTransationY {
  return [[[self getPresentationLayer] valueForKeyPath:@"transform.translation.y"] floatValue];
}

- (float)getTransationZ {
  return [[[self getPresentationLayer] valueForKeyPath:@"transform.translation.z"] floatValue];
}

- (float)scaleX {
  return [[[self getPresentationLayer] valueForKeyPath:@"transform.scale.x"] floatValue];
}

- (float)scaleY {
  return [[[self getPresentationLayer] valueForKeyPath:@"transform.scale.y"] floatValue];
}

- (NSMutableArray*)readyBlockArray {
  if (!_readyBlockArray) {
    _readyBlockArray = [NSMutableArray array];
  }
  return _readyBlockArray;
}

- (NSMutableArray*)nodeReadyBlockArray {
  if (!_nodeReadyBlockArray) {
    _nodeReadyBlockArray = [NSMutableArray array];
  }
  return _nodeReadyBlockArray;
}

- (NSMutableArray*)propsDidUpdateBlockArray {
  if (!_propsDidUpdateBlockArray) {
    _propsDidUpdateBlockArray = [NSMutableArray array];
  }
  return _propsDidUpdateBlockArray;
}

- (CALayer*)getPresentationLayer {
  if (self.view.layer.presentationLayer != nil) {
    return self.view.layer.presentationLayer;
  }
  return self.view.layer;
}

- (LynxUI*)getExposeReceiveTarget {
  return self;
}

- (CGRect)getBoundingClientRectToScreen {
  CALayer* layer = self.view.layer.presentationLayer ?: self.view.layer.modelLayer;
  return [layer convertRect:layer.bounds toLayer:nil];
}

- (void)removeChildrenExposureUI {
}

- (CGRect)getBoundingClientRect {
  UIView* rootView = ((LynxUI*)self.context.rootUI).view;
  int left = 0;
  int top = 0;
  if (rootView == NULL) {
    return CGRectMake(left, top, self.frame.size.width, self.frame.size.height);
  }
  CGRect rect = [self.view convertRect:self.view.bounds toView:rootView];
  return rect;
}

- (LynxUI*)getRelativeUI:(NSString*)relativeID {
  LynxUI* uiParent = self.parent;
  while (uiParent && ![uiParent isKindOfClass:[LynxRootUI class]]) {
    if ([uiParent.idSelector isEqualToString:relativeID]) {
      return uiParent;
    }
    uiParent = uiParent.parent;
  }

  return nil;
}

- (CGRect)getRelativeBoundingClientRect:(NSDictionary*)params {
  NSString* relativeID = [params objectForKey:@"relativeTo"];
  LynxUI* relativeUI =
      [self getRelativeUI:relativeID] ?: [self.context.uiOwner uiWithIdSelector:relativeID];
  if ([[params objectForKey:@"relativeTo"] isEqualToString:@"screen"]) {
    return [self.view convertRect:self.view.bounds toView:nil];
  } else if (relativeUI != nil) {
    CGRect rect = [self.view convertRect:self.view.bounds toView:relativeUI.view];
    if ([relativeUI.view isKindOfClass:[UIScrollView class]]) {
      rect.origin.x -= ((UIScrollView*)relativeUI.view).contentOffset.x;
      rect.origin.y -= ((UIScrollView*)relativeUI.view).contentOffset.y;
    }
    return rect;
  } else {
    UIView* rootView = ((LynxUI*)self.context.rootUI).view;
    if (rootView == NULL) {
      return CGRectMake(0, 0, self.frame.size.width, self.frame.size.height);
    }
    return [self.view convertRect:self.view.bounds toView:rootView];
  }
}

- (TransOffset)getTransformValueWithLeft:(float)left
                                   right:(float)right
                                     top:(float)top
                                  bottom:(float)bottom {
  TransOffset res;
  UIView* root_view = [[UIApplication sharedApplication] keyWindow];
  CALayer* layer = self.view.layer;
  CGFloat width = layer.bounds.size.width;
  CGFloat height = layer.bounds.size.height;
  if ([self.view isKindOfClass:[UIScrollView class]]) {
    CGPoint contentOffset = ((UIScrollView*)self.view).contentOffset;
    left += contentOffset.x;
    right += contentOffset.x;
    top += contentOffset.y;
    bottom += contentOffset.y;
  }
  res.left_top = [self.view convertPoint:CGPointMake(left, top) toView:root_view];
  res.right_top = [self.view convertPoint:CGPointMake(width + right, top) toView:root_view];
  res.right_bottom = [self.view convertPoint:CGPointMake(width + right, height + bottom)
                                      toView:root_view];
  res.left_bottom = [self.view convertPoint:CGPointMake(left, height + bottom) toView:root_view];
  return res;
}

- (CGRect)getRectToWindow {
  UIWindow* window = [[[UIApplication sharedApplication] delegate] window];
  CGRect rect = [self.view convertRect:self.view.bounds toView:window];
  return rect;
}

LYNX_UI_METHOD(boundingClientRect) {
  CGRect rect = [self getRelativeBoundingClientRect:params];
  callback(
      kUIMethodSuccess, @{
        @"id" : _idSelector ?: @"",
        @"dataset" : self.dataset,
        @"left" : @(rect.origin.x),
        @"right" : @(rect.origin.x + rect.size.width),
        @"top" : @(rect.origin.y),
        @"bottom" : @(rect.origin.y + rect.size.height),
        @"width" : @(rect.size.width),
        @"height" : @(rect.size.height)
      });
}

// TODO(zhixuan): Deprecated API, remove me.
LYNX_UI_METHOD(requestUIInfo) {
  NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
  if ([[params allKeys] containsObject:@"node"]) {
    [dict setObject:@{} forKey:@"node"];
  }
  if ([[params allKeys] containsObject:@"id"]) {
    [dict setObject:_idSelector ?: @"" forKey:@"id"];
  }
  if ([[params allKeys] containsObject:@"dataset"]) {
    [dict setObject:_dataset ?: @{} forKey:@"dataset"];
  }
  // Same as boundingClientRect query callback
  if ([[params allKeys] containsObject:@"rect"] || [[params allKeys] containsObject:@"size"]) {
    CGRect rect = [self getBoundingClientRect];
    if ([[params allKeys] containsObject:@"rect"]) {
      [dict addEntriesFromDictionary:@{
        @"left" : @(rect.origin.x),
        @"right" : @(rect.origin.x + rect.size.width),
        @"top" : @(rect.origin.y),
        @"bottom" : @(rect.origin.y + rect.size.height),
      }];
    }

    if ([[params allKeys] containsObject:@"size"]) {
      [dict addEntriesFromDictionary:@{
        @"width" : @(rect.size.width),
        @"height" : @(rect.size.height)
      }];
    }
  }
  // The node selected is <scroll-view> to get the position of scroll vertical and scroll landscape.
  // Otherwise, the two scrolling values will always be 0,0.
  if ([[params allKeys] containsObject:@"scrollOffset"]) {
    if ([[self view] isKindOfClass:UIScrollView.class]) {
      UIScrollView* scrollView = [self view];
      [dict addEntriesFromDictionary:@{
        @"scrollTop" : @(scrollView.contentOffset.y),
        @"scrollLeft" : @(scrollView.contentOffset.x)
      }];
    } else {
      [dict addEntriesFromDictionary:@{@"scrollTop" : @(0), @"scrollLeft" : @(0)}];
    }
  }
  callback(kUIMethodSuccess, dict.copy);
}

LYNX_UI_METHOD(scrollIntoView) {
  NSString* behavior = @"auto";
  NSString* blockType = @"start";
  NSString* inlineType = @"nearest";
  NSDictionary* scrollIntoViewOptions;
  if ([[params allKeys] containsObject:@"scrollIntoViewOptions"]) {
    scrollIntoViewOptions = ((NSDictionary*)[params objectForKey:@"scrollIntoViewOptions"]);
  }
  if (scrollIntoViewOptions == nil) {
    if (callback) {
      callback(kUIMethodParamInvalid, @"missing the param of `scrollIntoViewOptions`");
    }
    return;
  }
  if ([[scrollIntoViewOptions allKeys] containsObject:@"behavior"]) {
    behavior = ((NSString*)[scrollIntoViewOptions objectForKey:@"behavior"]);
  }
  if ([[scrollIntoViewOptions allKeys] containsObject:@"block"]) {
    blockType = ((NSString*)[scrollIntoViewOptions objectForKey:@"block"]);
  }
  if ([[scrollIntoViewOptions allKeys] containsObject:@"inline"]) {
    inlineType = ((NSString*)[scrollIntoViewOptions objectForKey:@"inline"]);
  }

  [self scrollIntoViewWithSmooth:[behavior isEqualToString:@"smooth"]
                       blockType:blockType
                      inlineType:inlineType
                        callback:callback];
}

- (void)scrollIntoViewWithSmooth:(BOOL)isSmooth
                       blockType:(NSString*)blockType
                      inlineType:(NSString*)inlineType
                        callback:(LynxUIMethodCallbackBlock)callback {
  BOOL scrollFlag = false;
  LynxUI* uiParent = (LynxUI*)self.parent;
  while (uiParent != nil) {
    if ([uiParent isKindOfClass:[AbsLynxUIScroller class]]) {
      [((AbsLynxUIScroller*)uiParent) scrollInto:self
                                        isSmooth:isSmooth
                                       blockType:blockType
                                      inlineType:inlineType];
      scrollFlag = true;
      break;
    }
    uiParent = (LynxUI*)uiParent.parent;
  }
  if (!scrollFlag) {
    LLogWarn(@"scrollIntoView not supported for nodeId:%ld", self.sign);
    if (callback) {
      callback(
          kUIMethodOperationError,
          [NSString stringWithFormat:@"scrollIntoView not supported for nodeId:%ld", self.sign]);
    }
  } else {
    if (callback) {
      callback(kUIMethodSuccess, nil);
    }
  }
}

LYNX_UI_METHOD(takeScreenshot) {
  if (!_view || _view.frame.size.width <= 0 || _view.frame.size.height <= 0) {
    return callback(kUIMethodNoUiForNode, @{});
  }
  bool usePng = false;
  if ([[params allKeys] containsObject:@"format"]) {
    NSString* foramt = ((NSString*)[params objectForKey:@"format"]);
    if ([foramt isEqualToString:@"png"]) {
      usePng = true;
    }
  }
  CGFloat scale = 1.f;
  if ([[params allKeys] containsObject:@"scale"]) {
    scale = ((NSNumber*)[params objectForKey:@"scale"]).floatValue;
  }

  id<LynxServiceSystemInvokeProtocol> invoker = LynxService(LynxServiceSystemInvokeProtocol);
  UIImage* image;
  if (invoker) {
    image = [invoker takeScreenshot:_view
                withBackgroundColor:_backgroundManager.backgroundColor
                              scale:[UIScreen mainScreen].scale * scale];
  } else {
    UIGraphicsBeginImageContextWithOptions(_view.frame.size, NO,
                                           [UIScreen mainScreen].scale * scale);
    if (_backgroundManager.backgroundColor &&
        ![_backgroundManager.backgroundColor isEqual:[UIColor clearColor]]) {
      CGRect rect = CGRectMake(0, 0, _view.frame.size.width, _view.frame.size.height);
      [_backgroundManager.backgroundColor setFill];
      UIRectFill(rect);
    }
    [_view drawViewHierarchyInRect:_view.bounds afterScreenUpdates:NO];
    image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
  }
  if (!image) {
    return callback(kUIMethodUnknown, @{});
  }

  NSData* data = usePng ? UIImagePNGRepresentation(image) : UIImageJPEGRepresentation(image, 1.0);
  NSString* header = usePng ? @"data:image/png;base64," : @"data:image/jpeg;base64,";
  NSString* str = [header stringByAppendingString:[data base64EncodedStringWithOptions:0]];

  callback(
      kUIMethodSuccess, @{
        @"width" : @(image.size.width * image.scale),
        @"height" : @(image.size.height * image.scale),
        @"data" : str
      });
}

LYNX_PROPS_GROUP_DECLARE(
    LYNX_PROP_DECLARE("accessibility-exclusive-focus", setAccessibilityExclusiveFocus, BOOL),
    LYNX_PROP_DECLARE("copyable", copyable, BOOL),
    LYNX_PROP_DECLARE("hit-slop", setHitSlop, NSObject*),
    LYNX_PROP_DECLARE("accessibility-auto-scroll-if-focused", setAccessibilityAutoScrollIfFocused,
                      BOOL),
    LYNX_PROP_DECLARE("accessibility-attached-cell-class", setAccessibilityAttachedCellClass,
                      NSString*),
    LYNX_PROP_DECLARE("should-rasterize-shadow", setShouldRasterizeShadow, BOOL),
    LYNX_PROP_DECLARE("transform", setTransform, NSArray*),
    LYNX_PROP_DECLARE("transform-origin", setTransformOrigin, NSArray*),
    LYNX_PROP_DECLARE("async-display", setAsyncDisplay, BOOL),
    LYNX_PROP_DECLARE("clip-radius", enableClipOnCornerRadius, NSString*),
    LYNX_PROP_DECLARE("mask-image", setMaskImage, NSArray*),
    LYNX_PROP_DECLARE("mask-origin", setMaskOrigin, NSArray*),
    LYNX_PROP_DECLARE("mask-position", setMaskPosition, NSArray*),
    LYNX_PROP_DECLARE("mask-repeat", setMaskRepeat, NSArray*),
    LYNX_PROP_DECLARE("mask-size", setMaskSize, NSArray*),
    LYNX_PROP_DECLARE("mask-clip", setMaskClip, NSArray*),
    LYNX_PROP_DECLARE("background-image", setBackgroundImage, NSArray*),
    LYNX_PROP_DECLARE("background", setBackground, NSString*),
    LYNX_PROP_DECLARE("background-color", setBackgroundColor, UIColor*),
    LYNX_PROP_DECLARE("background-origin", setBackgroundOrigin, NSArray*),
    LYNX_PROP_DECLARE("background-position", setBackgroundPosition, NSArray*),
    LYNX_PROP_DECLARE("background-repeat", setBackgroundRepeat, NSArray*),
    LYNX_PROP_DECLARE("background-size", setBackgroundSize, NSArray*),
    LYNX_PROP_DECLARE("background-clip", setBackgroundClip, NSArray*),
    LYNX_PROP_DECLARE("background-capInsets", setBackgroundCapInsets, NSString*),
    LYNX_PROP_DECLARE("clip-path", setClipPath, NSArray*),
    LYNX_PROP_DECLARE("opacity", setOpacity, CGFloat),
    LYNX_PROP_DECLARE("visibility", setVisibility, LynxVisibilityType),
    LYNX_PROP_DECLARE("direction", setLynxDirection, LynxDirectionType),
    LYNX_PROP_DECLARE("border-radius", setBorderRadius, NSArray*),
    LYNX_PROP_DECLARE("border-top-left-radius", setBorderTopLeftRadius, NSArray*),
    LYNX_PROP_DECLARE("border-bottom-left-radius", setBorderBottomLeftRadius, NSArray*),
    LYNX_PROP_DECLARE("border-top-right-radius", setBorderTopRightRadius, NSArray*),
    LYNX_PROP_DECLARE("border-bottom-right-radius", setBorderBottomRightRadius, NSArray*),
    LYNX_PROP_DECLARE("border-top-width", setBorderTopWidth, CGFloat),
    LYNX_PROP_DECLARE("border-left-width", setBorderLeftWidth, CGFloat),
    LYNX_PROP_DECLARE("border-bottom-width", setBorderBottomWidth, CGFloat),
    LYNX_PROP_DECLARE("border-right-width", setBorderRightWidth, CGFloat),
    LYNX_PROP_DECLARE("outline-width", setOutlineWidth, CGFloat),
    LYNX_PROP_DECLARE("border-top-color", setBorderTopColor, UIColor*),
    LYNX_PROP_DECLARE("border-left-color", setBorderLeftColor, UIColor*),
    LYNX_PROP_DECLARE("border-bottom-color", setBorderBottomColor, UIColor*),
    LYNX_PROP_DECLARE("border-right-color", setBorderRightColor, UIColor*),
    LYNX_PROP_DECLARE("outline-color", setOutlineColor, UIColor*),
    LYNX_PROP_DECLARE("border-left-style", setBorderLeftStyle, LynxBorderStyle),
    LYNX_PROP_DECLARE("border-right-style", setBorderRightStyle, LynxBorderStyle),
    LYNX_PROP_DECLARE("border-top-style", setBorderTopStyle, LynxBorderStyle),
    LYNX_PROP_DECLARE("border-bottom-style", setBorderBottomStyle, LynxBorderStyle),
    LYNX_PROP_DECLARE("outline-style", setOutlineStyle, LynxBorderStyle),
    LYNX_PROP_DECLARE("name", setName, NSString*),
    LYNX_PROP_DECLARE("idSelector", setIdSelector, NSString*),
    LYNX_PROP_DECLARE("accessibility-label", setAccessibilityLabel, NSString*),
    LYNX_PROP_DECLARE("accessibility-traits", setAccessibilityTraits, NSString*),
    LYNX_PROP_DECLARE("accessibility-element", setAccessibilityElement, BOOL),
    LYNX_PROP_DECLARE("accessibility-value", setAccessibilityValue, NSString*),
    LYNX_PROP_DECLARE("box-shadow", setBoxShadow, NSArray*),
    LYNX_PROP_DECLARE("implicit-animation", setImplicitAnimationFiber, BOOL),
    LYNX_PROP_DECLARE("layout-animation-create-duration", setLayoutAnimationCreateDuration,
                      NSTimeInterval),
    LYNX_PROP_DECLARE("layout-animation-create-delay", setLayoutAnimationCreateDelay,
                      NSTimeInterval),
    LYNX_PROP_DECLARE("layout-animation-create-property", setLayoutAnimationCreateProperty,
                      LynxAnimationProp),
    LYNX_PROP_DECLARE("layout-animation-create-timing-function",
                      setLayoutAnimationCreateTimingFunction, CAMediaTimingFunction*),
    LYNX_PROP_DECLARE("layout-animation-update-duration", setLayoutAnimationUpdateDuration,
                      NSTimeInterval),
    LYNX_PROP_DECLARE("layout-animation-update-delay", setLayoutAnimationUpdateDelay,
                      NSTimeInterval),
    LYNX_PROP_DECLARE("layout-animation-update-property", setLayoutAnimationUpdateProperty,
                      LynxAnimationProp),
    LYNX_PROP_DECLARE("layout-animation-update-timing-function",
                      setLayoutAnimationUpdateTimingFunction, CAMediaTimingFunction*),
    LYNX_PROP_DECLARE("layout-animation-delete-duration", setLayoutAnimationDeleteDuration,
                      NSTimeInterval),
    LYNX_PROP_DECLARE("layout-animation-delete-delay", setLayoutAnimationDeleteDelay,
                      NSTimeInterval),
    LYNX_PROP_DECLARE("layout-animation-delete-property", setLayoutAnimationDeleteProperty,
                      LynxAnimationProp),
    LYNX_PROP_DECLARE("layout-animation-delete-timing-function",
                      setLayoutAnimationDeleteTimingFunction, CAMediaTimingFunction*),
    LYNX_PROP_DECLARE("font-size", setFontSize, CGFloat),
    LYNX_PROP_DECLARE("lynx-test-tag", setTestTag, NSString*),
    LYNX_PROP_DECLARE("user-interaction-enabled", setUserInteractionEnabled, BOOL),
    LYNX_PROP_DECLARE("native-interaction-enabled", setNativeInteractionEnabled, BOOL),
    LYNX_PROP_DECLARE("allow-edge-antialiasing", setAllowEdgeAntialiasing, BOOL),
    LYNX_PROP_DECLARE("overflow-x", setOverflowX, LynxOverflowType),
    LYNX_PROP_DECLARE("overflow-y", setOverflowY, LynxOverflowType),
    LYNX_PROP_DECLARE("overflow", setOverflow, LynxOverflowType),
    LYNX_PROP_DECLARE("caret-color", setCaretColor, NSString*),
    LYNX_PROP_DECLARE("consume-slide-event", setConsumeSlideEvent, NSArray*),
    LYNX_PROP_DECLARE("block-native-event", setBlockNativeEvent, BOOL),
    LYNX_PROP_DECLARE("block-native-event-areas", setBlockNativeEventAreas, NSArray*),
    LYNX_PROP_DECLARE("ios-enable-simultaneous-touch", setEnableSimultaneousTouch, BOOL),
    LYNX_PROP_DECLARE("enable-touch-pseudo-propagation", setEnableTouchPseudoPropagation, BOOL),
    LYNX_PROP_DECLARE("event-through", setEventThrough, BOOL),
    LYNX_PROP_DECLARE("ignore-focus", setIgnoreFocus, BOOL),
    LYNX_PROP_DECLARE("react-ref", setRefId, NSString*),
    LYNX_PROP_DECLARE("dataset", setDataset, NSDictionary*),
    LYNX_PROP_DECLARE("intersection-observers", setIntersectionObservers, NSArray*),
    LYNX_PROP_DECLARE("perspective", setPerspective, NSArray*),
    LYNX_PROP_DECLARE("auto-resume-animation", setAutoResumeAnimation, BOOL),
    LYNX_PROP_DECLARE("enable-new-transform-origin", setEnableNewTransformOrigin, BOOL),
    LYNX_PROP_DECLARE("overlap-ios", setOverlapRendering, BOOL),
    LYNX_PROP_DECLARE("background-shape-layer", setUseBackgroundShapeLayer, BOOL),
    LYNX_PROP_DECLARE("enable-nested-scroll", setEnableNested, BOOL),
    LYNX_PROP_DECLARE("enable-reuse-animation-state", setEnableReuseAnimationState, BOOL),
    LYNX_PROP_DECLARE("image-rendering", setImageRendering, NSInteger))

#pragma mark - Transform

LYNX_PROP_DEFINE("transform", setTransform, NSArray*) {
  if (requestReset) {
    value = nil;
  }
  _didTransformChanged = YES;
  _hasTranslateDiff = [self hasTranslateDiff:value];
  _transformRaw = [LynxTransformRaw toTransformRaw:value];

  if (nil != _animationManager) {
    [_animationManager notifyPropertyUpdated:[LynxKeyframeAnimator kTransformStr]
                                       value:_transformRaw];
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    if ([ui.context isTouchMoving] && ui.hasTranslateDiff) {
      // The front-end modify the animation properties, causing the UI to slide.
      [ui.context onPropsChangedByUI:ui];
    }
  }];
}

LYNX_PROP_DEFINE("transform-origin", setTransformOrigin, NSArray*) {
  if (requestReset) {
    value = nil;
  }
  _didTransformChanged = YES;
  _transformOriginRaw = [LynxTransformOriginRaw convertToLynxTransformOriginRaw:value];
}

LYNX_PROP_DEFINE("perspective", setPerspective, NSArray*) {
  if (requestReset) {
    _perspective = nil;
    return;
  }
  _perspective = value;
}

LYNX_PROP_DEFINE("async-display", setAsyncDisplay, BOOL) {
  if (requestReset) {
    _asyncDisplayFromTTML = YES;
    return;
  }
  _asyncDisplayFromTTML = value;
}

- (void)applyTransformOrigin {
  // TODO(renzhongyue): modify this function such that it can run on the async thread.
  LYNX_ASSERT_ON_MAIN_THREAD;

  CGFloat anchorX = 0, anchorY = 0;
  CGFloat oldAnchorX = self.view.layer.anchorPoint.x;
  CGFloat oldAnchorY = self.view.layer.anchorPoint.y;
  if (self.transformOriginRaw == nil) {
    anchorX = 0.5;
    anchorY = 0.5;
  } else {
    if ([self.transformOriginRaw isP0Percent]) {
      anchorX = self.transformOriginRaw.p0;
    } else {
      if (self.updatedFrame.size.width == 0) {
        anchorX = 0.5;
      } else {
        anchorX = self.transformOriginRaw.p0 / self.updatedFrame.size.width;
      }
    }
    if ([self.transformOriginRaw isP1Percent]) {
      anchorY = self.transformOriginRaw.p1;
    } else {
      if (self.updatedFrame.size.height == 0) {
        anchorY = 0.5;
      } else {
        anchorY = self.transformOriginRaw.p1 / self.updatedFrame.size.height;
      }
    }
  }

  CGFloat newCenterX = self.view.center.x + (anchorX - oldAnchorX) * self.updatedFrame.size.width;
  CGFloat newCenterY = self.view.center.y + (anchorY - oldAnchorY) * self.updatedFrame.size.height;
  self.view.center = CGPointMake(newCenterX, newCenterY);
  self.view.layer.anchorPoint = CGPointMake(anchorX, anchorY);
  self.backgroundManager.transformOrigin = CGPointMake(anchorX, anchorY);
}

- (void)applyTransform {
  // TODO(renzhongyue): modify this function such that it can run on the async thread.
  LYNX_ASSERT_ON_MAIN_THREAD;

  [_transitionAnimationManager removeTransitionAnimation:TRANSITION_TRANSFORM];
  if (_enableNewTransformOrigin) {
    [self applyTransformOrigin];
  }
  char rotationType = LynxTransformRotationNone;
  CGFloat currentRotationX = 0;
  CGFloat currentRotationY = 0;
  CGFloat currentRotationZ = 0;
  CATransform3D transformWithoutRotate = CATransform3DIdentity;
  CATransform3D transformWithoutRotateXY = CATransform3DIdentity;
  CATransform3D transform3D = [LynxConverter toCATransform3D:_transformRaw
                                                          ui:self
                                                    newFrame:_updatedFrame
                                      transformWithoutRotate:&transformWithoutRotate
                                    transformWithoutRotateXY:&transformWithoutRotateXY
                                                rotationType:&rotationType
                                                   rotationX:&currentRotationX
                                                   rotationY:&currentRotationY
                                                   rotationZ:&currentRotationZ];

  LynxAnimationTransformRotation* oldTransformRotation = _lastTransformRotation;
  LynxAnimationTransformRotation* newTransformRotation =
      [[LynxAnimationTransformRotation alloc] init];
  newTransformRotation.rotationX = currentRotationX;
  newTransformRotation.rotationY = currentRotationY;
  newTransformRotation.rotationZ = currentRotationZ;

  if (_didTransformChanged && !_isFirstAnimatedReady && _transitionAnimationManager &&
      ([_transitionAnimationManager isTransitionTransform:_view.layer.transform
                                             newTransform:transform3D] ||
       [_transitionAnimationManager isTransitionTransformRotation:oldTransformRotation
                                             newTransformRotation:newTransformRotation])) {
    __weak LynxUI* weakSelf = self;
    [_transitionAnimationManager
        performTransitionAnimationsWithTransform:transform3D
                          transformWithoutRotate:transformWithoutRotate
                        transformWithoutRotateXY:transformWithoutRotateXY
                                        rotation:newTransformRotation
                                        callback:^(BOOL finished) {
                                          weakSelf.view.layer.transform = transform3D;
                                          weakSelf.backgroundManager.transform = transform3D;
                                          weakSelf.lastTransformRotation = newTransformRotation;
                                          weakSelf.lastTransformWithoutRotate =
                                              transformWithoutRotate;
                                          weakSelf.lastTransformWithoutRotateXY =
                                              transformWithoutRotateXY;
                                          [weakSelf.view setNeedsDisplay];
                                        }];
  } else {
    if (!CATransform3DEqualToTransform(_view.layer.transform, transform3D) ||
        [_lastTransformRotation isEqualToTransformRotation:newTransformRotation]) {
      // Transform will be apply on background manager
      _view.layer.transform = transform3D;
      _backgroundManager.transform = transform3D;
      self.lastTransformRotation = newTransformRotation;
      self.lastTransformWithoutRotate = transformWithoutRotate;
      self.lastTransformWithoutRotateXY = transformWithoutRotateXY;
      [self.view setNeedsDisplay];
      // Static transform animation changes the UI‘s layout.
      [self.context.observer notifyLayout:nil];
    }
  }
}

// This informs whether clipping will take place if a subview overflows a superview's border-radius
// part.
LYNX_PROP_DEFINE("clip-radius", enableClipOnCornerRadius, NSString*) {
  if (requestReset) {
    if ([LynxVersionUtils compareLeft:[_context targetSdkVersion]
                            withRight:LYNX_TARGET_SDK_VERSION_1_5] < 0) {
      _clipOnBorderRadius = YES;
    } else {
      _clipOnBorderRadius = NO;
    }
  }

  if ((value && [value caseInsensitiveCompare:@"no"] == NSOrderedSame) ||
      (value && [value caseInsensitiveCompare:@"false"] == NSOrderedSame)) {
    _clipOnBorderRadius = NO;
  } else if ((value && [value caseInsensitiveCompare:@"yes"] == NSOrderedSame) ||
             (value && [value caseInsensitiveCompare:@"true"] == NSOrderedSame)) {
    _clipOnBorderRadius = YES;
  }

  [self markNeedDisplay];
}

- (void)setBackgroundOrMaskWithDrawable:(NSMutableArray*)drawable
                                  reset:(BOOL)reset
                                  value:(NSArray*)value {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  if (reset) {
    value = [NSArray new];
  }
  [drawable removeAllObjects];
  for (NSUInteger i = 0; i < [value count]; i++) {
    NSUInteger type = [LynxConverter toNSUInteger:[value objectAtIndex:i]];
    if (type == LynxBackgroundImageURL) {
      i++;
      [drawable addObject:[[LynxBackgroundImageDrawable alloc]
                              initWithString:[LynxConverter toNSString:value[i]]]];
    } else if (type == LynxBackgroundImageLinearGradient) {
      i++;
      [drawable addObject:[[LynxBackgroundLinearGradientDrawable alloc] initWithArray:value[i]]];
    } else if (type == LynxBackgroundImageRadialGradient) {
      i++;
      [drawable addObject:[[LynxBackgroundRadialGradientDrawable alloc] initWithArray:value[i]]];
    } else if (type == LynxBackgroundImageNone) {
      i++;
      [drawable addObject:[LynxBackgroundNoneDrawable new]];
    }
  }
  [self markNeedDisplay];
}

LYNX_PROP_DEFINE("mask-image", setMaskImage, NSArray*) {
  [self setBackgroundOrMaskWithDrawable:_backgroundManager.maskDrawable
                                  reset:requestReset
                                  value:value];
  [_backgroundManager markMaskDirty];
}

LYNX_PROP_DEFINE("background-image", setBackgroundImage, NSArray*) {
  [self setBackgroundOrMaskWithDrawable:_backgroundManager.backgroundDrawable
                                  reset:requestReset
                                  value:value];
  [_backgroundManager markBackgroundDirty];
}

LYNX_PROP_DEFINE("background", setBackground, NSString*) {
  LLogWarn(@"setBackground is deprecated, call this method has no effect");
}

LYNX_PROP_DEFINE("background-color", setBackgroundColor, UIColor*) {
  if (requestReset) {
    value = nil;
  }

  if (nil != _animationManager) {
    [_animationManager notifyPropertyUpdated:[LynxKeyframeAnimator kBackgroundColorStr]
                                       value:(id)value.CGColor];
  }

  if (_transitionAnimationManager &&
      [_transitionAnimationManager maybeUpdateBackgroundWithTransitionAnimation:value]) {
    return;
  }

  _backgroundManager.backgroundColor = value;

  [self markNeedDisplay];
}

- (void)setBackgroundOrMaskWithOrigin:(NSMutableArray*)origin
                                reset:(BOOL)reset
                                value:(NSArray*)value {
  [origin removeAllObjects];
  if (reset) {
    value = [NSArray new];
  }
  for (NSNumber* type in value) {
    NSUInteger backgroundOrigin = [type unsignedIntegerValue];
    if (backgroundOrigin > LynxBackgroundOriginContentBox) {
      backgroundOrigin = LynxBackgroundOriginBorderBox;
    }
    [origin addObject:@(backgroundOrigin)];
  }

  [self markNeedDisplay];
}

LYNX_PROP_DEFINE("background-origin", setBackgroundOrigin, NSArray*) {
  [self setBackgroundOrMaskWithOrigin:_backgroundManager.backgroundOrigin
                                reset:requestReset
                                value:value];
  [_backgroundManager markBackgroundDirty];
}

LYNX_PROP_DEFINE("mask-origin", setMaskOrigin, NSArray*) {
  [self setBackgroundOrMaskWithOrigin:_backgroundManager.maskOrigin reset:requestReset value:value];
  [_backgroundManager markMaskDirty];
}

- (void)setBackgroundOrMaskWithPosition:(NSMutableArray*)position
                                  reset:(BOOL)reset
                                  value:(NSArray*)value {
  [position removeAllObjects];
  if (reset || value.count % 2 != 0) {
    value = [NSArray new];
  }
  for (NSUInteger i = 0; i < [value count]; i += 2) {
    NSUInteger type = [LynxConverter toNSUInteger:[value objectAtIndex:i + 1]];
    LynxBackgroundPosition* backgroundPosition = NULL;
    LynxPlatformLength* len = [[LynxPlatformLength alloc] initWithValue:[value objectAtIndex:i]
                                                                   type:type];
    backgroundPosition = [[LynxBackgroundPosition alloc] initWithValue:len];
    [position addObject:backgroundPosition];
  }

  [self markNeedDisplay];
}

LYNX_PROP_DEFINE("background-position", setBackgroundPosition, NSArray*) {
  [self setBackgroundOrMaskWithPosition:_backgroundManager.backgroundPosition
                                  reset:requestReset
                                  value:value];
  [_backgroundManager markBackgroundDirty];
}

LYNX_PROP_DEFINE("mask-position", setMaskPosition, NSArray*) {
  [self setBackgroundOrMaskWithPosition:_backgroundManager.maskPosition
                                  reset:requestReset
                                  value:value];
  [_backgroundManager markMaskDirty];
}

- (void)setBackgroundOrMaskWithRepeat:(NSMutableArray*)repeat
                                reset:(BOOL)reset
                                value:(NSArray*)value {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  [repeat removeAllObjects];
  if (reset) {
    value = [NSArray array];
  }
  for (NSNumber* number in value) {
    [repeat addObject:@([number integerValue])];
  }

  [self markNeedDisplay];
}

LYNX_PROP_DEFINE("background-repeat", setBackgroundRepeat, NSArray*) {
  [self setBackgroundOrMaskWithRepeat:_backgroundManager.backgroundRepeat
                                reset:requestReset
                                value:value];

  [_backgroundManager markBackgroundDirty];
}

LYNX_PROP_DEFINE("mask-repeat", setMaskRepeat, NSArray*) {
  [self setBackgroundOrMaskWithRepeat:_backgroundManager.maskRepeat reset:requestReset value:value];
  [_backgroundManager markMaskDirty];
}

- (void)setBackgroundOrMaskWithSize:(NSMutableArray*)size reset:(BOOL)reset value:(NSArray*)value {
  [size removeAllObjects];
  if (reset || value.count % 2 != 0) {
    value = [NSArray new];
  }
  for (NSUInteger i = 0; i < [value count]; i += 2) {
    LynxPlatformLength* length =
        [[LynxPlatformLength alloc] initWithValue:[value objectAtIndex:i]
                                             type:[[value objectAtIndex:i + 1] unsignedIntValue]];
    [size addObject:[[LynxBackgroundSize alloc] initWithLength:length]];
  }

  [self markNeedDisplay];
}

LYNX_PROP_DEFINE("background-size", setBackgroundSize, NSArray*) {
  [self setBackgroundOrMaskWithSize:_backgroundManager.backgroundImageSize
                              reset:requestReset
                              value:value];
  [_backgroundManager markBackgroundDirty];
}

LYNX_PROP_DEFINE("mask-size", setMaskSize, NSArray*) {
  [self setBackgroundOrMaskWithSize:_backgroundManager.maskSize reset:requestReset value:value];
  [_backgroundManager markMaskDirty];
}

- (void)setBackgroundOrMaskWithClip:(NSMutableArray*)clip reset:(BOOL)reset value:(NSArray*)value {
  [clip removeAllObjects];
  if (reset) {
    value = [NSArray new];
  }
  for (NSNumber* type in value) {
    [clip addObject:@([type integerValue])];
  }

  [self markNeedDisplay];
}

LYNX_PROP_DEFINE("background-clip", setBackgroundClip, NSArray*) {
  [self setBackgroundOrMaskWithClip:_backgroundManager.backgroundClip
                              reset:requestReset
                              value:value];
  [_backgroundManager markBackgroundDirty];
}

LYNX_PROP_DEFINE("mask-clip", setMaskClip, NSArray*) {
  [self setBackgroundOrMaskWithClip:_backgroundManager.maskClip reset:requestReset value:value];
  [_backgroundManager markMaskDirty];
}

LYNX_PROP_DEFINE("opacity", setOpacity, CGFloat) {
  if (requestReset) {
    value = 1;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    if (nil != ui.animationManager) {
      [ui.animationManager notifyPropertyUpdated:[LynxKeyframeAnimator kOpacityStr]
                                           value:[NSNumber numberWithFloat:value]];
    }

    if (ui.transitionAnimationManager &&
        [ui.transitionAnimationManager maybeUpdateOpacityWithTransitionAnimation:value]) {
      return;
    }

    [ui view].layer.opacity = value;
    ui.backgroundManager.opacity = value;
  }];

  [self markNeedDisplay];
}

LYNX_PROP_DEFINE("visibility", setVisibility, LynxVisibilityType) {
  if (requestReset) {
    value = (int)LynxVisibilityVisible;
  }
  LynxVisibilityType type = (LynxVisibilityType)value;
  BOOL isVisible = (type == LynxVisibilityVisible);
  BOOL isHidden = (type == LynxVisibilityHidden);

  if (_transitionAnimationManager) {
    [_transitionAnimationManager removeTransitionAnimation:TRANSITION_VISIBILITY];
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    if (ui.transitionAnimationManager &&
        ([ui.transitionAnimationManager isTransitionVisibility:self.view.hidden
                                                      newState:isHidden])) {
      __weak LynxUI* weakSelf = ui;
      [ui.transitionAnimationManager
          performTransitionAnimationsWithVisibility:isHidden
                                           callback:^(BOOL finished) {
                                             __strong LynxUI* strongSelf = weakSelf;
                                             if (strongSelf) {
                                               if ([strongSelf view].hidden == false && isHidden) {
                                                 [strongSelf view].hidden = true;
                                                 [strongSelf backgroundManager].hidden = true;
                                               } else if ([strongSelf view].hidden == true &&
                                                          (isVisible || requestReset)) {
                                                 [strongSelf view].hidden = false;
                                                 [strongSelf backgroundManager].hidden = false;
                                               }
                                             }
                                           }];

    } else {
      if ([ui view].hidden == false && isHidden) {
        [ui view].hidden = true;
        [ui backgroundManager].hidden = true;
      } else if ([ui view].hidden == true && (isVisible || requestReset)) {
        [ui view].hidden = false;
        [ui backgroundManager].hidden = false;
      }
    }
  }];
}

LYNX_PROP_DEFINE("direction", setLynxDirection, LynxDirectionType) {
  if (requestReset) {
    _directionType = LynxDirectionLtr;
  } else {
    _directionType = value;
    // this block will be called in onNodeReady
    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      [ui resetContentOffset];
      [ui applyRTL:value == LynxDirectionRtl];
    }];
  }
}

- (LynxBorderUnitValue)toBorderUnitValue:(NSArray*)unitValue index:(int)index {
  LynxBorderUnitValue ret = {
      .val = [LynxConverter toCGFloat:[unitValue objectAtIndex:index]],
      .unit = (LynxPlatformLengthUnit)
                          [LynxConverter toNSUInteger:[unitValue objectAtIndex:index + 1]] ==
                      LynxPlatformLengthUnitNumber
                  ? LynxBorderValueUnitDefault
                  : LynxBorderValueUnitPercent};
  return ret;
}

#define LYNX_CORNER_POSITION_TOP_LEFT_X 0
#define LYNX_CORNER_POSITION_TOP_LEFT_Y 1
#define LYNX_CORNER_POSITION_TOP_RIGHT_X 2
#define LYNX_CORNER_POSITION_TOP_RIGHT_Y 3
#define LYNX_CORNER_POSITION_BOTTOM_RIGHT_X 4
#define LYNX_CORNER_POSITION_BOTTOM_RIGHT_Y 5
#define LYNX_CORNER_POSITION_BOTTOM_LEFT_X 6
#define LYNX_CORNER_POSITION_BOTTOM_LEFT_Y 7

#define SET_BORDER_UNIT_VAL(dst, src, dstLength, srcLength, offset)                         \
  {                                                                                         \
    newLength = NULL;                                                                       \
    LBRGetBorderValueOrLength(value, offset, &newValue, &newLength);                        \
    if (!isBorderUnitEqualA(dst, src,                                                       \
                            _backgroundManager.backgroundInfo->borderRadiusCalc[dstLength], \
                            srcLength)) {                                                   \
      dst = src;                                                                            \
      _backgroundManager.backgroundInfo->borderRadiusCalc[dstLength] = srcLength;           \
      changed = true;                                                                       \
    }                                                                                       \
  }

#define LYNX_PROP_SET_BORDER_RADIUS(position, index)                                 \
  {                                                                                  \
    if (requestReset) {                                                              \
      value = @[ @0, @0, @0, @0 ];                                                   \
    }                                                                                \
    LynxBorderRadii borderRadius = _backgroundManager.borderRadius;                  \
    bool changed = false;                                                            \
                                                                                     \
    LynxBorderUnitValue newValue = {0, LynxBorderValueUnitDefault};                  \
    LynxPlatformLength* newLength = NULL;                                            \
                                                                                     \
    SET_BORDER_UNIT_VAL(borderRadius.position##X, newValue, index, newLength, 0)     \
    SET_BORDER_UNIT_VAL(borderRadius.position##Y, newValue, index + 1, newLength, 2) \
                                                                                     \
    if (changed) {                                                                   \
      _backgroundManager.borderRadiusRaw = borderRadius;                             \
      _backgroundManager.borderRadius = borderRadius;                                \
      [self markNeedDisplay];                                                        \
    }                                                                                \
  }

//@see computed_css_style.cc#borderRadiusToLepus.
LYNX_PROP_DEFINE("border-radius", setBorderRadius, NSArray*) {
  if (requestReset) {
    value = @[ @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0 ];
  }
  LynxBorderRadii borderRadius = _backgroundManager.borderRadius;
  bool changed = false;

  LynxBorderUnitValue newValue = {0, LynxBorderValueUnitDefault};
  LynxPlatformLength* newLength = NULL;

  SET_BORDER_UNIT_VAL(borderRadius.topLeftX, newValue, LYNX_CORNER_POSITION_TOP_LEFT_X, newLength,
                      0)
  SET_BORDER_UNIT_VAL(borderRadius.topLeftY, newValue, LYNX_CORNER_POSITION_TOP_LEFT_Y, newLength,
                      2)
  SET_BORDER_UNIT_VAL(borderRadius.topRightX, newValue, LYNX_CORNER_POSITION_TOP_RIGHT_X, newLength,
                      4)
  SET_BORDER_UNIT_VAL(borderRadius.topRightY, newValue, LYNX_CORNER_POSITION_TOP_RIGHT_Y, newLength,
                      6)
  SET_BORDER_UNIT_VAL(borderRadius.bottomRightX, newValue, LYNX_CORNER_POSITION_BOTTOM_RIGHT_X,
                      newLength, 8)
  SET_BORDER_UNIT_VAL(borderRadius.bottomRightY, newValue, LYNX_CORNER_POSITION_BOTTOM_RIGHT_Y,
                      newLength, 10)
  SET_BORDER_UNIT_VAL(borderRadius.bottomLeftX, newValue, LYNX_CORNER_POSITION_BOTTOM_LEFT_X,
                      newLength, 12)
  SET_BORDER_UNIT_VAL(borderRadius.bottomLeftY, newValue, LYNX_CORNER_POSITION_BOTTOM_LEFT_Y,
                      newLength, 14)

  if (changed) {
    _backgroundManager.borderRadius = borderRadius;
    _backgroundManager.borderRadiusRaw = borderRadius;
    [self markNeedDisplay];
  }
}

LYNX_PROP_DEFINE("border-top-left-radius", setBorderTopLeftRadius,
                 NSArray*){LYNX_PROP_SET_BORDER_RADIUS(topLeft, LYNX_CORNER_POSITION_TOP_LEFT_X)}

LYNX_PROP_DEFINE("border-bottom-left-radius", setBorderBottomLeftRadius, NSArray*){
    LYNX_PROP_SET_BORDER_RADIUS(bottomLeft, LYNX_CORNER_POSITION_BOTTOM_LEFT_X)}

LYNX_PROP_DEFINE("border-top-right-radius", setBorderTopRightRadius,
                 NSArray*){LYNX_PROP_SET_BORDER_RADIUS(topRight, LYNX_CORNER_POSITION_TOP_RIGHT_X)}

LYNX_PROP_DEFINE("border-bottom-right-radius", setBorderBottomRightRadius, NSArray*){
    LYNX_PROP_SET_BORDER_RADIUS(bottomRight, LYNX_CORNER_POSITION_BOTTOM_RIGHT_X)}
#undef LYNX_CORNER_POSITION_TOP_LEFT_X
#undef LYNX_CORNER_POSITION_TOP_LEFT_Y
#undef LYNX_CORNER_POSITION_TOP_RIGHT_X
#undef LYNX_CORNER_POSITION_TOP_RIGHT_Y
#undef LYNX_CORNER_POSITION_BOTTOM_RIGHT_X
#undef LYNX_CORNER_POSITION_BOTTOM_RIGHT_Y
#undef LYNX_CORNER_POSITION_BOTTOM_LEFT_X
#undef LYNX_CORNER_POSITION_BOTTOM_LEFT_Y
#undef LYNX_PROP_SET_BORDER_RADIUS
#undef SET_BORDER_UNIT_VAL

#define LYNX_PROP_SET_BORDER_WIDTH(side)                     \
  UIEdgeInsets borderWidth = _backgroundManager.borderWidth; \
  if (value != borderWidth.side) {                           \
    borderWidth.side = value;                                \
    _backgroundManager.borderWidth = borderWidth;            \
    [self markNeedDisplay];                                  \
  }

LYNX_PROP_DEFINE("border-top-width", setBorderTopWidth, CGFloat){LYNX_PROP_SET_BORDER_WIDTH(top)}

LYNX_PROP_DEFINE("border-left-width", setBorderLeftWidth, CGFloat){LYNX_PROP_SET_BORDER_WIDTH(left)}

LYNX_PROP_DEFINE("border-bottom-width", setBorderBottomWidth,
                 CGFloat){LYNX_PROP_SET_BORDER_WIDTH(bottom)}

LYNX_PROP_DEFINE("border-right-width", setBorderRightWidth,
                 CGFloat){LYNX_PROP_SET_BORDER_WIDTH(right)}
#define LYNX_PROP_TOUICOLOR(colorStr) \
  [LynxConverter toUIColor:[NSNumber numberWithInt:[colorStr intValue]]]

LYNX_PROP_DEFINE("border-top-color", setBorderTopColor, UIColor*) {
  if (requestReset) {
    value = nil;
  }
  [_backgroundManager updateBorderColor:LynxBorderTop value:value];
}

LYNX_PROP_DEFINE("border-left-color", setBorderLeftColor, UIColor*) {
  if (requestReset) {
    value = nil;
  }
  [_backgroundManager updateBorderColor:LynxBorderLeft value:value];
}

LYNX_PROP_DEFINE("border-bottom-color", setBorderBottomColor, UIColor*) {
  if (requestReset) {
    value = nil;
  }
  [_backgroundManager updateBorderColor:LynxBorderBottom value:value];
}

LYNX_PROP_DEFINE("border-right-color", setBorderRightColor, UIColor*) {
  if (requestReset) {
    value = nil;
  }
  [_backgroundManager updateBorderColor:LynxBorderRight value:value];
}

LYNX_PROP_DEFINE("border-left-style", setBorderLeftStyle, LynxBorderStyle) {
  if (requestReset) {
    value = LynxBorderStyleSolid;
  }
  if ([_backgroundManager updateBorderStyle:LynxBorderLeft value:value]) {
    [self markNeedDisplay];
  }
}

LYNX_PROP_DEFINE("border-right-style", setBorderRightStyle, LynxBorderStyle) {
  if (requestReset) {
    value = LynxBorderStyleSolid;
  }
  if ([_backgroundManager updateBorderStyle:LynxBorderRight value:value]) {
    [self markNeedDisplay];
  }
}

LYNX_PROP_DEFINE("border-top-style", setBorderTopStyle, LynxBorderStyle) {
  if (requestReset) {
    value = LynxBorderStyleSolid;
  }
  if ([_backgroundManager updateBorderStyle:LynxBorderTop value:value]) {
    [self markNeedDisplay];
  }
}

LYNX_PROP_DEFINE("border-bottom-style", setBorderBottomStyle, LynxBorderStyle) {
  if (requestReset) {
    value = LynxBorderStyleSolid;
  }
  if ([_backgroundManager updateBorderStyle:LynxBorderBottom value:value]) {
    [self markNeedDisplay];
  }
}

LYNX_PROP_DEFINE("outline-width", setOutlineWidth, CGFloat) {
  const CGFloat val = (requestReset ? 0 : value);
  if ([_backgroundManager updateOutlineWidth:val]) {
    [self markNeedDisplay];
  }
}

LYNX_PROP_DEFINE("outline-color", setOutlineColor, UIColor*) {
  if (requestReset) {
    value = nil;
  }
  if ([_backgroundManager updateOutlineColor:value]) {
    [self markNeedDisplay];
  }
}

LYNX_PROP_DEFINE("outline-style", setOutlineStyle, LynxBorderStyle) {
  if (requestReset) {
    value = LynxBorderStyleNone;
  }

  if ([_backgroundManager updateOutlineStyle:value]) {
    [self markNeedDisplay];
  }
}

LYNX_PROP_DEFINE("name", setName, NSString*) {
  if (requestReset) {
    value = @"";
  }
  [self setName:value];
}

- (void)setName:(NSString* _Nonnull)name {
  _name = name;
}

// this prop only be used in native and should not be used in ttml
LYNX_PROP_DEFINE("idSelector", setIdSelector, NSString*) {
  if (requestReset) {
    value = @"";
  }
  _idSelector = value;
}

LYNX_PROP_DEFINE("accessibility-label", setAccessibilityLabel, NSString*) {
  if (requestReset) {
    value = @"";
    self.useDefaultAccessibilityLabel = YES;
  }

  if (![value isEqualToString:@""]) {
    self.useDefaultAccessibilityLabel = NO;
  }
  self.lynxAccessibilityLabel = value;

  NSString* label = [self concatA11yStatus:self.lynxAccessibilityStatus
                                     label:self.lynxAccessibilityLabel];
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.accessibilityLabel = label;
  }];
}

LYNX_PROP_DEFINE("accessibility-traits", setAccessibilityTraits, NSString*) {
  UIAccessibilityTraits traits = requestReset ? self.accessibilityTraitsByDefault
                                              : [LynxConverter toAccessibilityTraits:value];
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.accessibilityTraits = traits;
  }];
}

LYNX_PROP_DEFINE("accessibility-value", setAccessibilityValue, NSString*) {
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.accessibilityValue = value;
  }];
}

LYNX_PROP_DEFINE("accessibility-element", setAccessibilityElement, BOOL) {
  if (requestReset) {
    value = self.enableAccessibilityByDefault;
  }
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.isAccessibilityElement = value;
  }];
}

LYNX_PROP_SETTER("accessibility-elements", setAccessibilityElements, NSString*) {
  self.accessibilityElementsIds = [value componentsSeparatedByString:@","];
}

LYNX_PROP_SETTER("accessibility-elements-a11y", setAccessibilityElementsA11y, NSString*) {
  self.accessibilityElementsA11yIds = [value componentsSeparatedByString:@","];
}

LYNX_PROP_SETTER("ios-platform-accessibility-id", setPlatformAccessibilityId, NSString*) {
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.accessibilityIdentifier = value;
  }];
}

LYNX_PROP_SETTER("accessibility-elements-hidden", setAccessibilityElementsHidden, BOOL) {
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.accessibilityElementsHidden = value;
  }];
}

LYNX_PROP_SETTER("accessibility-actions", setAccessibilityActions, NSArray*) {
  if (@available(iOS 14.0, *)) {
    NSMutableArray* customActions = [NSMutableArray array];

    for (NSString* obj in value) {
      [customActions
          addObject:[[UIAccessibilityCustomAction alloc]
                         initWithName:obj
                        actionHandler:^BOOL(UIAccessibilityCustomAction* _Nonnull customAction) {
                          [self.context.eventEmitter
                              sendCustomEvent:[[LynxDetailEvent alloc]
                                                  initWithName:@"accessibilityaction"
                                                    targetSign:[self sign]
                                                        detail:@{@"name" : obj}]];
                          return YES;
                        }]];
    }

    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      ui.view.accessibilityCustomActions = customActions;
    }];
  }
}

LYNX_PROP_SETTER("accessibility-status", setAccessibilityRole, NSString*) {
  if (value.length) {
    self.lynxAccessibilityStatus = value;
    self.useDefaultAccessibilityLabel = NO;
  } else {
    self.lynxAccessibilityStatus = nil;
  }

  NSString* label = [self concatA11yStatus:self.lynxAccessibilityStatus
                                     label:self.lynxAccessibilityLabel];
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.accessibilityLabel = label;
  }];
}

- (nullable NSString*)concatA11yStatus:(NSString*)status label:(NSString*)label {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  NSMutableString* ret = [NSMutableString string];

  if (status) {
    [ret appendString:status];
  }

  if (label) {
    if (ret.length) {
      [ret appendString:@", "];
    }
    [ret appendString:label];
  }

  return ret;
}

#pragma mark - Box Shadow

LYNX_PROP_DEFINE("box-shadow", setBoxShadow, NSArray*) {
  if (requestReset) {
    value = nil;
  }

  [self.backgroundManager setShadowArray:[LynxConverter toLynxBoxShadow:value]];
}

LYNX_PROP_DEFINE("implicit-animation", setImplicitAnimationFiber, BOOL) {
  if (requestReset) {
    value = _context.defaultImplicitAnimation;
  }
  if (value) {
    [LynxFeatureCounter count:LynxFeatureObjcImplicitAnimation instanceId:[_context instanceId]];
  }
  _backgroundManager.implicitAnimation = value;
}

LYNX_PROP_DEFINE("auto-resume-animation", setAutoResumeAnimation, BOOL) {
  if (requestReset) {
    value = _context.defaultAutoResumeAnimation;
  }
  if (!value) {
    [LynxFeatureCounter count:LynxFeatureObjcAutoResumeAnimation instanceId:[_context instanceId]];
  }
  _autoResumeAnimation = value;
  _animationManager.autoResumeAnimation = value;
}

LYNX_PROP_DEFINE("enable-new-transform-origin", setEnableNewTransformOrigin, BOOL) {
  if (requestReset) {
    value = _context.defaultEnableNewTransformOrigin;
  }
  if (!value) {
    [LynxFeatureCounter count:LynxFeatureObjcDisableNewTransformOriginIos
                   instanceId:[_context instanceId]];
  }
  _enableNewTransformOrigin = value;
}

LYNX_PROP_DEFINE("enable-reuse-animation-state", setEnableReuseAnimationState, BOOL) {
  if (requestReset) {
    value = YES;
  }
  if (!value) {
    [LynxFeatureCounter count:LynxFeatureObjcDisableReuseAnimationState
                   instanceId:[_context instanceId]];
  }
  _enableReuseAnimationState = value;
}

LYNX_PROP_DEFINE("image-rendering", setImageRendering, LynxImageRenderingType) {
  if (requestReset) {
    value = LynxImageRenderingAuto;
  }
  if (value == LynxImageRenderingPixelated) {
    _backgroundManager.isPixelated = YES;
  } else {
    _backgroundManager.isPixelated = NO;
  }
  [_backgroundManager markMaskDirty];
  [_backgroundManager markBackgroundDirty];

  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    if (value == LynxImageRenderingPixelated) {
      ui.view.layer.minificationFilter = kCAFilterNearest;
      ui.view.layer.magnificationFilter = kCAFilterNearest;
    } else {
      ui.view.layer.minificationFilter = kCAFilterLinear;
      ui.view.layer.magnificationFilter = kCAFilterLinear;
    }
  }];
}

#pragma mark - Layout animation

// layout for layout-animation
- (void)updateFrameWithLayoutAnimation:(CGRect)newFrame
                           withPadding:(UIEdgeInsets)padding
                                border:(UIEdgeInsets)border
                                margin:(UIEdgeInsets)margin {
  LYNX_ASSERT_ON_MAIN_THREAD;

  if (!_layoutAnimationManager && !_transitionAnimationManager) {
    [self updateFrameWithoutLayoutAnimation:newFrame
                                withPadding:padding
                                     border:border
                                     margin:margin];
    return;
  }

  if (_layoutAnimationManager &&
      [_layoutAnimationManager maybeUpdateFrameWithLayoutAnimation:newFrame
                                                       withPadding:padding
                                                            border:border
                                                            margin:margin]) {
    LLogInfo(@"LynxUI do layoutAnimation");
  } else if (_transitionAnimationManager &&
             [_transitionAnimationManager maybeUpdateFrameWithTransitionAnimation:newFrame
                                                                      withPadding:padding
                                                                           border:border
                                                                           margin:margin]) {
    LLogInfo(@"LynxUI do transitionAnimation");
  } else {
    LLogInfo(@"LynxUI don't do any layout related animation.");
    [self updateFrameWithoutLayoutAnimation:newFrame
                                withPadding:padding
                                     border:border
                                     margin:margin];
  }
}

// init
- (void)prepareKeyframeManager {
  _backgroundManager.implicitAnimation = false;
  if (nil == _animationManager) {
    _animationManager = [[LynxKeyframeManager alloc] initWithUI:self];
    _animationManager.autoResumeAnimation = _autoResumeAnimation;
  }
}

- (void)prepareLayoutAnimationManager {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  _backgroundManager.implicitAnimation = false;
  if (!_layoutAnimationManager) {
    _layoutAnimationManager = [[LynxLayoutAnimationManager alloc] initWithLynxUI:self];
  }
}

- (void)prepareTransitionAnimationManager {
  _backgroundManager.implicitAnimation = false;
  if (!_transitionAnimationManager) {
    _transitionAnimationManager = [[LynxTransitionAnimationManager alloc] initWithLynxUI:self];
  }
}

// create
LYNX_PROP_DEFINE("layout-animation-create-duration", setLayoutAnimationCreateDuration,
                 NSTimeInterval) {
  if (requestReset) {
    value = 0;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationCreate instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  if (IS_ZERO(value)) {
    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      [ui.view.layer removeAllAnimations];
    }];
    [self.backgroundManager removeAllAnimations];
  }
  _layoutAnimationManager.createConfig.duration = value;
}

LYNX_PROP_DEFINE("layout-animation-create-delay", setLayoutAnimationCreateDelay, NSTimeInterval) {
  if (requestReset) {
    value = 0;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationCreate instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.createConfig.delay = value;
}

LYNX_PROP_DEFINE("layout-animation-create-property", setLayoutAnimationCreateProperty,
                 LynxAnimationProp) {
  if (requestReset) {
    value = NONE;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationCreate instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.createConfig.prop = value;
}

LYNX_PROP_DEFINE("layout-animation-create-timing-function", setLayoutAnimationCreateTimingFunction,
                 CAMediaTimingFunction*) {
  if (requestReset) {
    value = [LynxConverter toCAMediaTimingFunction:nil];
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationCreate instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.createConfig.timingFunction = value;
}

// update
LYNX_PROP_DEFINE("layout-animation-update-duration", setLayoutAnimationUpdateDuration,
                 NSTimeInterval) {
  if (requestReset) {
    value = 0;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationUpdate instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  if (IS_ZERO(value)) {
    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      [ui.view.layer removeAllAnimations];
    }];
    [self.backgroundManager removeAllAnimations];
  }
  _layoutAnimationManager.updateConfig.duration = value;
}

LYNX_PROP_DEFINE("layout-animation-update-delay", setLayoutAnimationUpdateDelay, NSTimeInterval) {
  if (requestReset) {
    value = 0;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationUpdate instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.updateConfig.delay = value;
}

LYNX_PROP_DEFINE("layout-animation-update-property", setLayoutAnimationUpdateProperty,
                 LynxAnimationProp) {
  if (requestReset) {
    value = NONE;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationUpdate instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.updateConfig.prop = value;
}

LYNX_PROP_DEFINE("layout-animation-update-timing-function", setLayoutAnimationUpdateTimingFunction,
                 CAMediaTimingFunction*) {
  if (requestReset) {
    value = [LynxConverter toCAMediaTimingFunction:nil];
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationUpdate instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.updateConfig.timingFunction = value;
}

// delete
LYNX_PROP_DEFINE("layout-animation-delete-duration", setLayoutAnimationDeleteDuration,
                 NSTimeInterval) {
  if (requestReset) {
    value = 0.0;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationDelete instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  if (IS_ZERO(value)) {
    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      [ui.view.layer removeAllAnimations];
    }];
    [self.backgroundManager removeAllAnimations];
  }
  _layoutAnimationManager.deleteConfig.duration = value;
}

LYNX_PROP_DEFINE("layout-animation-delete-delay", setLayoutAnimationDeleteDelay, NSTimeInterval) {
  if (requestReset) {
    value = 0.0;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationDelete instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.deleteConfig.delay = value;
}

LYNX_PROP_DEFINE("layout-animation-delete-property", setLayoutAnimationDeleteProperty,
                 LynxAnimationProp) {
  if (requestReset) {
    value = NONE;
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationDelete instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.deleteConfig.prop = value;
}

LYNX_PROP_DEFINE("layout-animation-delete-timing-function", setLayoutAnimationDeleteTimingFunction,
                 CAMediaTimingFunction*) {
  if (requestReset) {
    value = [LynxConverter toCAMediaTimingFunction:nil];
  }
  [LynxFeatureCounter count:LynxFeatureObjcLayoutAnimationDelete instanceId:[_context instanceId]];
  [self prepareLayoutAnimationManager];
  _layoutAnimationManager.deleteConfig.timingFunction = value;
}

LYNX_PROP_DEFINE("font-size", setFontSize, CGFloat) {
  if (requestReset) {
    value = 14;
  }
  if (_fontSize != value) {
    _fontSize = value;
  }
}

LYNX_PROP_DEFINE("lynx-test-tag", setTestTag, NSString*) {
  if (requestReset) {
    value = @"";
  }
  if (_context.isDev) {
    self.useDefaultAccessibilityLabel = NO;

    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      ui.view.isAccessibilityElement = YES;
      // TODO: use accessibilityIdentifier instead of accessibilityLabel
      ui.view.accessibilityLabel = value;
    }];
  }
}

- (void)setOverflowMask:(short)mask withValue:(LynxOverflowType)val {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  short newVal = _overflow;
  if (val == LynxOverflowVisible) {
    newVal |= mask;
  } else {
    newVal &= ~mask;
  }

  BOOL needUpdateLayer = (newVal != _overflow);
  BOOL needForceCLip = (newVal == 0);

  _overflow = newVal;
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.clipsToBounds = needForceCLip;
    if (needUpdateLayer) {
      [ui updateLayerMaskOnFrameChanged];
    }
  }];
}

- (void)setImplicitAnimation {
  LYNX_MAYBE_ON_ASYNC_THREAD;
  _backgroundManager.implicitAnimation = _context.defaultImplicitAnimation;
}

- (short)overflow {
  return _overflow;
}

- (CGSize)frameSize {
  return self.frame.size;
}

LYNX_PROP_DEFINE("user-interaction-enabled", setUserInteractionEnabled, BOOL) {
  if (requestReset) {
    value = YES;
  }
  _userInteractionEnabled = value;
}

LYNX_PROP_DEFINE("native-interaction-enabled", setNativeInteractionEnabled, BOOL) {
  if (requestReset) {
    value = YES;
  }
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.userInteractionEnabled = value;
  }];
}

LYNX_PROP_DEFINE("allow-edge-antialiasing", setAllowEdgeAntialiasing, BOOL) {
  if (requestReset) {
    value = NO;
  }

  _backgroundManager.allowsEdgeAntialiasing = value;

  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    ui.view.layer.allowsEdgeAntialiasing = value;
  }];
}

LYNX_PROP_DEFINE("overflow-x", setOverflowX, LynxOverflowType) {
  if (requestReset) {
    value = [self getInitialOverflowType];
  }
  [self setOverflowMask:OVERFLOW_X_VAL withValue:value];
}

LYNX_PROP_DEFINE("overflow-y", setOverflowY, LynxOverflowType) {
  if (requestReset) {
    value = [self getInitialOverflowType];
  }
  [self setOverflowMask:OVERFLOW_Y_VAL withValue:value];
}
LYNX_PROP_DEFINE("overflow", setOverflow, LynxOverflowType) {
  if (requestReset) {
    value = [self getInitialOverflowType];
  }
  [self setOverflowMask:OVERFLOW_XY_VAL withValue:value];
}

LYNX_PROP_DEFINE("caret-color", setCaretColor, NSString*) {
  // implemented by components
}

LYNX_PROP_DEFINE("consume-slide-event", setConsumeSlideEvent, NSArray*) {
  // If requestReset, let value be nil. If the value is not nil, check each item of the value to see
  // if it is NSArray and the first two items are NSNumber. If it meets the conditions, put it into
  // _angleArray. Otherwise, skip the item. If _angleArray is not empty, needCheckConsumeSlideEvent
  // is executed to indicate that consumeSlideEvent detection is needed.
  if (requestReset) {
    value = nil;
  }
  _angleArray = [[NSMutableArray alloc] init];
  for (id obj in value) {
    if (![obj isKindOfClass:[NSArray class]]) {
      continue;
    }
    NSArray* ary = (NSArray*)obj;
    if (ary.count >= 2 && [ary[0] isKindOfClass:[NSNumber class]] &&
        [ary[1] isKindOfClass:[NSNumber class]]) {
      [_angleArray addObject:ary];
    }
  }
  if ([_angleArray count] > 0) {
    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      [ui.context.eventHandler needCheckConsumeSlideEvent];
    }];
  }
}

- (BOOL)consumeSlideEvent:(CGFloat)angle {
  LYNX_ASSERT_ON_MAIN_THREAD;

  // Traverse `_angleArray` and check if the given angle falls within each angle interval. If the
  // condition is met, return YES, indicating that the current LynxUI needs to consume slide events.
  // Otherwise, return NO indicating that the current LynxUI does not need to consume slide events.
  __block BOOL res = NO;
  [_angleArray enumerateObjectsUsingBlock:^(NSArray<NSNumber*>* _Nonnull obj, NSUInteger idx,
                                            BOOL* _Nonnull stop) {
    if (angle >= obj[0].doubleValue && angle <= obj[1].doubleValue) {
      res = YES;
      *stop = YES;
    }
  }];
  return res;
}

LYNX_PROP_DEFINE("block-native-event", setBlockNativeEvent, BOOL) {
  if (requestReset) {
    value = false;
  }
  _blockNativeEvent = value;
}

LYNX_PROP_DEFINE("block-native-event-areas", setBlockNativeEventAreas, NSArray*) {
  if (requestReset) {
    value = nil;
  }
  _blockNativeEventAreas = nil;
  if (![value isKindOfClass:[NSArray class]]) {
    LLogWarn(@"block-native-event-areas: type err: %@", value);
    return;
  }
  // Supports two types: `30px` and `50%`
  NSMutableArray<NSArray<LynxSizeValue*>*>* blockNativeEventAreas = [NSMutableArray array];
  [value enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
    if ([obj isKindOfClass:[NSArray class]] && [(NSArray*)obj count] == 4) {
      NSArray* area = obj;
      LynxSizeValue* x = [LynxSizeValue sizeValueFromCSSString:area[0]];
      LynxSizeValue* y = [LynxSizeValue sizeValueFromCSSString:area[1]];
      LynxSizeValue* w = [LynxSizeValue sizeValueFromCSSString:area[2]];
      LynxSizeValue* h = [LynxSizeValue sizeValueFromCSSString:area[3]];
      if (x && y && w && h) {
        [blockNativeEventAreas addObject:@[ x, y, w, h ]];
      } else {
        LLogWarn(@"block-native-event-areas: %luth type err", (unsigned long)idx);
      }
    } else {
      LLogWarn(@"block-native-event-areas: %luth type err, size != 4", (unsigned long)idx);
    }
  }];
  if ([blockNativeEventAreas count] > 0) {
    _blockNativeEventAreas = [blockNativeEventAreas copy];
  } else {
    LLogWarn(@"block-native-event-areas: empty areas");
  }
}

// Temp setting for 2022 Spring Festival activities. Remove this later, and solve the similar
// problems by implementing flexible handling of conflicts between Lynx gestures and Native gestures
// in the future.
LYNX_PROP_DEFINE("ios-enable-simultaneous-touch", setEnableSimultaneousTouch, BOOL) {
  if (requestReset) {
    value = false;
  }
  _enableSimultaneousTouch = value;
}

- (BOOL)enableSimultaneousTouch {
  return _enableSimultaneousTouch;
}

LYNX_PROP_DEFINE("enable-touch-pseudo-propagation", setEnableTouchPseudoPropagation, BOOL) {
  if (requestReset) {
    value = YES;
  }
  _enableTouchPseudoPropagation = value;
}

- (BOOL)enableTouchPseudoPropagation {
  return _enableTouchPseudoPropagation;
}

- (void)onPseudoStatusFrom:(int32_t)preStatus changedTo:(int32_t)currentStatus {
  _pseudoStatus = currentStatus;
}

LYNX_PROP_DEFINE("event-through", setEventThrough, BOOL) {
  // If requestReset, the _eventThrough will be Undefined.
  enum LynxEventPropStatus res = kLynxEventPropUndefined;
  if (requestReset) {
    _eventThrough = res;
    return;
  }
  _eventThrough = value ? kLynxEventPropEnable : kLynxEventPropDisable;
}

- (BOOL)eventThrough {
  LYNX_ASSERT_ON_MAIN_THREAD;

  // If _eventThrough == Enable, return YES. If _eventThrough == Disable, return NO.
  // If _eventThrough == Undefined && parent not nil, return parent.eventThrough.
  if (_eventThrough == kLynxEventPropEnable) {
    return YES;
  } else if (_eventThrough == kLynxEventPropDisable) {
    return NO;
  }

  id<LynxEventTarget> parent = [self parentTarget];
  if (parent != nil) {
    // when parent is root ui, return false.
    if ([parent isKindOfClass:[LynxRootUI class]]) {
      return NO;
    }
    return [parent eventThrough];
  }
  return NO;
}

- (BOOL)blockNativeEvent:(UIGestureRecognizer*)gestureRecognizer {
  LYNX_ASSERT_ON_MAIN_THREAD;

  BOOL blockNativeEventAll = _blockNativeEvent;
  if (blockNativeEventAll) {
    return YES;
  }
  if (!_blockNativeEventAreas) {
    return NO;
  }
  CGPoint p = [gestureRecognizer locationInView:self.view];
  CGSize size = self.view.bounds.size;

  __block BOOL blockNativeEventThisPoint = NO;
  [_blockNativeEventAreas enumerateObjectsUsingBlock:^(NSArray<LynxSizeValue*>* _Nonnull obj,
                                                       NSUInteger idx, BOOL* _Nonnull stop) {
    if ([obj count] == 4) {
      CGFloat left = [obj[0] convertToDevicePtWithFullSize:size.width];
      CGFloat top = [obj[1] convertToDevicePtWithFullSize:size.height];
      CGFloat right = left + [obj[2] convertToDevicePtWithFullSize:size.width];
      CGFloat bottom = top + [obj[3] convertToDevicePtWithFullSize:size.height];
      blockNativeEventThisPoint = p.x >= left && p.x < right && p.y >= top && p.y < bottom;
      if (blockNativeEventThisPoint) {
        LLogInfo(@"blocked this point!");
        *stop = YES;
      }
    }
  }];
  return blockNativeEventThisPoint;
}

LYNX_PROP_SETTER("a11y-id", setA11yID, NSString*) { self.a11yID = value; }

LYNX_PROP_DEFINE("ignore-focus", setIgnoreFocus, BOOL) {
  // If requestReset, the _ignoreFocus will be Undefined.
  enum LynxEventPropStatus res = kLynxEventPropUndefined;
  if (requestReset) {
    _ignoreFocus = res;
    return;
  }
  _ignoreFocus = value ? kLynxEventPropEnable : kLynxEventPropDisable;
}

LYNX_PROP_SETTER("exposure-scene", setExposureScene, NSString*) {
  if (requestReset) {
    value = nil;
  }
  // TODO(hexionghui): Put removeUIFromExposedMap caused by exposure-related properties in
  // onPropsUpdated for processing.
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui.context removeUIFromExposedMap:ui];
    ui.exposureScene = value;
  }];
}

LYNX_PROP_SETTER("exposure-id", setExposureID, NSString*) {
  if (requestReset) {
    value = nil;
  }
  // TODO(hexionghui): Put removeUIFromExposedMap caused by exposure-related properties in
  // onPropsUpdated for processing.
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui.context removeUIFromExposedMap:ui];
    ui.exposureID = value;
  }];
}

LYNX_PROP_SETTER("exposure-screen-margin-top", setExposureScreenMarginTop, NSString*) {
  if (requestReset) {
    value = nil;
  }
  // TODO(hexionghui): Put removeUIFromExposedMap caused by exposure-related properties in
  // onPropsUpdated for processing.
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui.context removeUIFromExposedMap:ui];
    ui.exposureMarginTop = [LynxUnitUtils toPtFromUnitValue:value];
  }];
}

LYNX_PROP_SETTER("exposure-screen-margin-bottom", setExposureScreenMarginBottom, NSString*) {
  if (requestReset) {
    value = nil;
  }
  // TODO(hexionghui): Put removeUIFromExposedMap caused by exposure-related properties in
  // onPropsUpdated for processing.
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui.context removeUIFromExposedMap:ui];
    ui.exposureMarginBottom = [LynxUnitUtils toPtFromUnitValue:value];
  }];
}

LYNX_PROP_SETTER("exposure-screen-margin-left", setExposureScreenMarginLeft, NSString*) {
  if (requestReset) {
    value = nil;
  }
  // TODO(hexionghui): Put removeUIFromExposedMap caused by exposure-related properties in
  // onPropsUpdated for processing.
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui.context removeUIFromExposedMap:ui];
    ui.exposureMarginLeft = [LynxUnitUtils toPtFromUnitValue:value];
  }];
}

LYNX_PROP_SETTER("exposure-screen-margin-right", setExposureScreenMarginRight, NSString*) {
  if (requestReset) {
    value = nil;
  }
  // TODO(hexionghui): Put removeUIFromExposedMap caused by exposure-related properties in
  // onPropsUpdated for processing.
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui.context removeUIFromExposedMap:ui];
    ui.exposureMarginRight = [LynxUnitUtils toPtFromUnitValue:value];
  }];
}

LYNX_PROP_SETTER("enable-exposure-ui-margin", setEnableExposureUIMargin, BOOL) {
  enum LynxPropStatus res = kLynxPropUndefined;
  if (requestReset) {
    _enableExposureUIMargin = res;
    return;
  }
  _enableExposureUIMargin = value ? kLynxPropEnable : kLynxPropDisable;
}

- (BOOL)enableExposureUIMargin {
  if (_enableExposureUIMargin == kLynxPropEnable) {
    return true;
  } else if (_enableExposureUIMargin == kLynxPropDisable) {
    return false;
  }
  // read from pageConfig
  return [_context enableExposureUIMargin];
}

LYNX_PROP_SETTER("exposure-ui-margin-top", setExposureUIMarginTop, NSString*) {
  if (requestReset) {
    value = nil;
  }
  _exposureUIMarginTop = value;
}

LYNX_PROP_SETTER("exposure-ui-margin-bottom", setExposureUIMarginBottom, NSString*) {
  if (requestReset) {
    value = nil;
  }
  _exposureUIMarginBottom = value;
}

LYNX_PROP_SETTER("exposure-ui-margin-left", setExposureUIMarginLeft, NSString*) {
  if (requestReset) {
    value = nil;
  }
  _exposureUIMarginLeft = value;
}

LYNX_PROP_SETTER("exposure-ui-margin-right", setExposureUIMarginRight, NSString*) {
  if (requestReset) {
    value = nil;
  }
  _exposureUIMarginRight = value;
}

LYNX_PROP_SETTER("exposure-area", setExposureArea, NSString*) {
  if (requestReset) {
    value = nil;
  }
  _exposureArea = value;
}

LYNX_PROP_SETTER("block-list-event", setBlockListEvent, BOOL) { self.blockListEvent = value; }

LYNX_PROP_SETTER("align-height", setAlignHeight, BOOL) { _alignHeight = value; }

LYNX_PROP_SETTER("align-width", setAlignWidth, BOOL) { _alignWidth = value; }

// this prop only be used in "ref" within ReactLynx and should not be explicitly used in ttml by
// user
LYNX_PROP_DEFINE("react-ref", setRefId, NSString*) {
  if (requestReset) {
    value = @"";
  }
  _refId = value;
}

LYNX_PROP_DEFINE("dataset", setDataset, NSDictionary*) {
  if (requestReset) {
    value = [NSDictionary dictionary];
  }

  _dataset = value;
}

LYNX_PROP_DEFINE("intersection-observers", setIntersectionObservers, NSArray*) {
  if (requestReset) {
    value = [NSArray array];
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui.context.intersectionManager removeAttachedIntersectionObserver:ui];
    if (!value || [value count] == 0) {
      return;
    }
    for (NSUInteger idx = 0; idx < [value count]; idx++) {
      NSDictionary* propsObject = value[idx];
      if (propsObject) {
        LynxUIIntersectionObserver* observer =
            [[LynxUIIntersectionObserver alloc] initWithOptions:propsObject
                                                        manager:ui.context.intersectionManager
                                                     attachedUI:ui];
        [ui.context.intersectionManager addIntersectionObserver:observer];
      }
    }
  }];
}

LYNX_PROP_SETTER("enable-scroll-monitor", setEnableScrollMonitor, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _enableScrollMonitor = value;
}

LYNX_PROP_SETTER("scroll-monitor-tag", setScrollMonitorTag, NSString*) {
  if (requestReset) {
    value = nil;
  }
  _scrollMonitorTagName = value;
}

LYNX_PROP_SETTER("filter", setFilter, NSArray*) {
  float amount = .0f;

  if (requestReset || [value count] != 3) {
    amount = .0f;
    _filter_type = LynxFilterTypeNone;
  } else {
    _filter_type = [(NSNumber*)[value objectAtIndex:0] intValue];
    amount = [(NSNumber*)[value objectAtIndex:1] floatValue];
  }

  switch (_filter_type) {
    case LynxFilterTypeGrayScale:
      amount = 1 - amount;
      _filter_amount = [LynxUnitUtils clamp:amount min:0.0f max:1.0f];
      break;
    case LynxFilterTypeBlur:
    default:
      _filter_amount = amount;
  }

  id filter = [self getFilterWithType:_filter_type];
  if (filter) {
    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      ui.view.layer.filters = @[ filter ];
    }];
    [_backgroundManager setFilters:@[ filter ]];
  } else {
    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      ui.view.layer.filters = nil;
    }];
    [_backgroundManager setFilters:nil];
  }
}

LYNX_PROP_DEFINE("overlap-ios", setOverlapRendering, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _backgroundManager.overlapRendering = value;
}

LYNX_PROP_DEFINE("ios-background-shape-layer", setUseBackgroundShapeLayer, BOOL) {
  LynxBgShapeLayerProp enabled =
      requestReset ? LynxBgShapeLayerPropUndefine
                   : (value ? LynxBgShapeLayerPropEnabled : LynxBgShapeLayerPropDisabled);
  [_backgroundManager setUiBackgroundShapeLayerEnabled:enabled];
}

- (id)getFilterWithType:(LynxFilterType)type {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  if (_filter_type == LynxFilterTypeNone) {
    return nil;
  }

  return [LynxFilterUtil getFilterWithType:type filterAmount:_filter_amount];
}

// override by subclass
- (void)onAnimationStart:(NSString*)type
              startFrame:(CGRect)startFrame
              finalFrame:(CGRect)finalFrame
                duration:(NSTimeInterval)duration {
}

// override by subclass
- (void)onAnimationEnd:(NSString*)type
            startFrame:(CGRect)startFrame
            finalFrame:(CGRect)finalFrame
              duration:(NSTimeInterval)duratio {
}

- (void)resetAnimation {
  if (nil != _animationManager) {
    [_animationManager resetAnimation];
  }

  [self.children
      enumerateObjectsUsingBlock:^(LynxUI* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        [obj resetAnimation];
      }];
}

- (void)restartAnimation {
  if (nil != _animationManager) {
    [_animationManager restartAnimation];
  }
  [self.children
      enumerateObjectsUsingBlock:^(LynxUI* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        [obj restartAnimation];
      }];
}

- (void)removeAnimationForReuse {
  if (_layoutAnimationManager) {
    [_layoutAnimationManager removeAllLayoutAnimation];
  }
  if (_transitionAnimationManager) {
    [_transitionAnimationManager removeAllTransitionAnimation];
  }
}

- (void)setAnimation:(NSArray*)value {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  if ([value isEqual:[NSNull null]] || value == nil) {
    _animationInfos = nil;
    [_animationManager endAllAnimation];
    _animationManager = nil;
    return;
  }
  [self prepareKeyframeManager];
  NSMutableArray<LynxAnimationInfo*>* infos = [[NSMutableArray alloc] init];
  for (id v in value) {
    if ([v isKindOfClass:[NSArray class]]) {
      LynxAnimationInfo* info = [LynxConverter toKeyframeAnimationInfo:v];
      if (info != nil) {
        [infos addObject:info];
      }
    }
  }
  _animationInfos = infos;
  [_animationManager setAnimations:infos];
}

- (void)setTransition:(NSArray*)value {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  if ([value isEqual:[NSNull null]] || value == nil) {
    value = nil;
  }
  [self prepareTransitionAnimationManager];
  if ([value isEqual:[NSNull null]] || value == nil || value.count == 0) {
    [_transitionAnimationManager removeAllTransitionAnimation];
    _transitionAnimationManager = nil;
    return;
  }
  [_transitionAnimationManager assembleTransitions:value];
}

- (void)sendLayoutChangeEvent {
  NSString* layoutChangeFunctionName = @"layoutchange";
  if ([self eventSet] && [[self eventSet] valueForKey:layoutChangeFunctionName]) {
    CGRect rect = [self getBoundingClientRect];
    NSDictionary* data = @{
      @"id" : _idSelector ?: @"",
      @"dataset" : _dataset,
      @"left" : @(rect.origin.x),
      @"right" : @(rect.origin.x + rect.size.width),
      @"top" : @(rect.origin.y),
      @"bottom" : @(rect.origin.y + rect.size.height),
      @"width" : @(rect.size.width),
      @"height" : @(rect.size.height)
    };
    LynxCustomEvent* event = [[LynxDetailEvent alloc] initWithName:layoutChangeFunctionName
                                                        targetSign:[self sign]
                                                            detail:data];
    [self.context.eventEmitter sendCustomEvent:event];
  }
}

/* EventTarget Section Begin */
- (NSInteger)signature {
  return self.sign;
}

- (int32_t)pseudoStatus {
  return _pseudoStatus;
}

- (nullable id<LynxEventTarget>)parentTarget {
  if ([self.parent conformsToProtocol:@protocol(LynxEventTarget)]) {
    return self.parent;
  }
  return nil;
}

- (nullable id<LynxEventTargetBase>)parentResponder {
  if ([self.parent conformsToProtocol:@protocol(LynxEventTargetBase)]) {
    return self.parent;
  }
  return nil;
}

- (nullable NSDictionary*)getDataset {
  return _dataset;
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  LynxUI* guard = nil;
  // this is parent response to translate Point coordinate
  if ([self hasCustomLayout]) {
    guard = [self hitTest:point withEvent:event onUIWithCustomLayout:self];
    point = [self.view convertPoint:point toView:guard.view];
  } else {
    CGPoint childPoint = CGPointZero;
    for (LynxUI* child in [self.children reverseObjectEnumerator]) {
      if (![child shouldHitTest:point withEvent:event] || [child.view isHidden]) {
        continue;
      }

      CALayer* parentLayer = self.view.layer.presentationLayer ?: self.view.layer.modelLayer;
      CALayer* childLayer = child.view.layer.presentationLayer ?: child.view.layer.modelLayer;
      CGPoint targetPoint = [parentLayer convertPoint:point toLayer:childLayer];
      bool contain = false;
      if (_context.enableEventRefactor) {
        contain = [child containsPoint:targetPoint];
      } else {
        contain = [child containsPoint:point];
      }

      if (contain) {
        if (child.isOnResponseChain) {
          guard = child;
          childPoint = targetPoint;
          break;
        }
        if (guard == nil || guard.getTransationZ < child.getTransationZ) {
          guard = child;
          childPoint = targetPoint;
        }
      }
    }
    if (_context.enableEventRefactor) {
      point = childPoint;
    } else {
      point = [guard getHitTestPoint:point];
    }
  }
  if (guard == nil) {
    // no new result
    return self;
  }
  return [guard hitTest:point withEvent:event];
}

- (BOOL)containsPoint:(CGPoint)point inHitTestFrame:(CGRect)frame {
  bool contain = NO;
  if (_context.enableEventRefactor) {
    frame =
        CGRectMake(frame.origin.x - self.touchSlop, frame.origin.y - self.touchSlop,
                   frame.size.width + 2 * self.touchSlop, frame.size.height + 2 * self.touchSlop);
    contain = CGRectContainsPoint(frame, point);
    if (!contain && _overflow != 0) {
      if (_overflow == OVERFLOW_X_VAL) {
        if (!(frame.origin.y - self.touchSlop < point.y &&
              frame.origin.y + frame.size.height + self.touchSlop > point.y)) {
          return contain;
        }
      } else if (_overflow == OVERFLOW_Y_VAL) {
        if (!(frame.origin.x - self.touchSlop < point.x &&
              frame.origin.x + frame.size.width + self.touchSlop > point.x)) {
          return contain;
        }
      }
      contain = [self childrenContainPoint:point];
    }
    return contain;
  }

  frame = CGRectMake(frame.origin.x - self.touchSlop, frame.origin.y - self.touchSlop,
                     frame.size.width + 2 * self.touchSlop, frame.size.height + 2 * self.touchSlop);
  contain = CGRectContainsPoint(frame, point);
  if (!contain && _overflow != 0) {
    if (_overflow == OVERFLOW_X_VAL) {
      if (!(frame.origin.y < point.y && frame.origin.y + frame.size.height > point.y)) {
        return contain;
      }
    } else if (_overflow == OVERFLOW_Y_VAL) {
      if (!(frame.origin.x < point.x && frame.origin.x + frame.size.width > point.x)) {
        return contain;
      }
    }
    contain = [self childrenContainPoint:point];
  }
  return contain;
}

- (BOOL)containsPoint:(CGPoint)point {
  CGRect frame = CGRectZero;
  if (_context.enableEventRefactor) {
    frame = self.view.bounds;
    if (frame.size.width + _hitSlopLeft + _hitSlopRight >= CGFLOAT_EPSILON &&
        frame.size.height + _hitSlopTop + _hitSlopBottom >= CGFLOAT_EPSILON) {
      frame.origin.x -= _hitSlopLeft;
      frame.origin.y -= _hitSlopTop;
      frame.size.width += _hitSlopLeft + _hitSlopRight;
      frame.size.height += _hitSlopTop + _hitSlopBottom;
    }
  } else {
    frame = [self getHitTestFrame];
  }
  return [self containsPoint:point inHitTestFrame:frame];
}

- (nullable NSDictionary<NSString*, LynxEventSpec*>*)eventSet {
  return _eventSet;
}

- (nullable NSDictionary<NSNumber*, LynxGestureDetectorDarwin*>*)gestureMap {
  return _gestureMap;
}

- (BOOL)shouldHitTest:(CGPoint)point withEvent:(nullable UIEvent*)event {
  // If set user-interaction-enabled="{{false}}" or visibility: hidden, this ui will not be on the
  // response chain.
  return _userInteractionEnabled && !self.view.hidden && self.view.window;
}

- (BOOL)isVisible {
  UIView* view = [self view];
  if (view == nil) {
    return NO;
  }

  // if view is hidden, return NO
  if (view.isHidden) {
    return NO;
  }

  // if view's alpha == 0, return NO
  if (view.alpha == 0) {
    return NO;
  }

  // if view's size == 0 and clipsToBounds is true, return NO
  if (view.frame.size.width == 0 || view.frame.size.height == 0) {
    if (view.clipsToBounds) {
      return NO;
    }
  }

  // if list cell is offscreen, return NO. see issue:#7727
  // Not only list inside lynxView, but also UICollectionView outside lynxView
  if ([view.superview.superview isKindOfClass:[UICollectionViewCell class]]) {
    UICollectionViewCell* cellView = (UICollectionViewCell*)view.superview.superview;
    UIView* listView = cellView.superview;
    if ([listView isKindOfClass:[UICollectionView class]]) {
      NSArray<UICollectionViewCell*>* visibleCells = ((UICollectionView*)listView).visibleCells;
      if (![visibleCells containsObject:cellView]) {
        return NO;
      }
    }
  }

  // if foldview cell is offscreen, return NO, like issue:#7727.
  // Not only foldview inside lynxView, but also UITableView outside lynxView
  if ([view.superview.superview.superview.superview.superview.superview.superview
          isKindOfClass:[UITableViewCell class]]) {
    UITableViewCell* cellView =
        (UITableViewCell*)
            view.superview.superview.superview.superview.superview.superview.superview;
    UIView* listView = cellView.superview;
    if ([listView isKindOfClass:[UITableView class]]) {
      NSArray<UITableViewCell*>* visibleCells = ((UITableView*)listView).visibleCells;
      if (![visibleCells containsObject:cellView]) {
        return NO;
      }
    }
  }

  // if view's window is nil, return NO
  return view.window != nil;
}

- (BOOL)ignoreFocus {
  LYNX_ASSERT_ON_MAIN_THREAD;

  // If _ignoreFocus == Enable, return YES. If _ignoreFocus == Disable, return NO.
  // If _ignoreFocus == Undefined && parent not nil, return parent.ignoreFocus.
  if (_ignoreFocus == kLynxEventPropEnable) {
    return YES;
  } else if (_ignoreFocus == kLynxEventPropDisable) {
    return NO;
  }

  id<LynxEventTarget> parent = [self parentTarget];
  if (parent != nil) {
    // when parent is root ui, return false.
    if ([parent isKindOfClass:[LynxRootUI class]]) {
      return NO;
    }
    return [parent ignoreFocus];
  }
  return NO;
}

// only include touches and event, don't care Lynx frontend event
- (BOOL)dispatchTouch:(NSString* const)touchType
              touches:(NSSet<UITouch*>*)touches
            withEvent:(UIEvent*)event {
  return NO;
}

// include target point and Lynx frontend event
- (BOOL)dispatchEvent:(LynxEventDetail*)event {
  return NO;
}

- (void)onResponseChain {
  _onResponseChain = YES;
}

- (void)offResponseChain {
  _onResponseChain = NO;
}

- (BOOL)isOnResponseChain {
  return _onResponseChain;
}

- (double)touchSlop {
  if (_onResponseChain) {
    return _touchSlop;
  }
  return 0;
}

/* EventTarget Section End */

// this can be overriden if a UI typically has more than three layers
- (CALayer*)topLayer {
  if (_backgroundManager) {
    if (_backgroundManager.maskLayer) {
      return _backgroundManager.maskLayer;
    }
    if (_backgroundManager.borderLayer && self.overflow == OVERFLOW_HIDDEN_VAL) {
      return _backgroundManager.borderLayer;
    }
  }
  return self.view.layer;
}

- (CALayer*)bottomLayer {
  if (_backgroundManager) {
    if (_backgroundManager.backgroundLayer) {
      return _backgroundManager.backgroundLayer;
    }
  }
  return self.view.layer;
}

- (BOOL)isRtl {
  if (_directionType == LynxDirectionRtl) {
    [LynxFeatureCounter count:LynxFeatureObjcPageRtl instanceId:[self.context instanceId]];
    return YES;
  }
  return NO;
}

- (BOOL)enableAccessibilityByDefault {
  return NO;
}

- (BOOL)accessibilityClickable {
  if (!_context.eventEmitter || !_eventSet || _eventSet.count == 0) {
    return NO;
  }
  static NSString* LynxEventTap = @"tap";
  if ([_eventSet valueForKey:LynxEventTap]) {
    return YES;
  } else {
    return NO;
  }
}

- (UIAccessibilityTraits)accessibilityTraitsByDefault {
  return UIAccessibilityTraitNone;
}

- (BOOL)didSizeChanged {
  return !CGRectEqualToRect(_updatedFrame, _lastUpdatedFrame);
}

- (BOOL)shouldReDoTransform {
  return _didTransformChanged ||
         (([LynxTransformRaw hasPercent:_transformRaw] || [_transformOriginRaw isValid]) &&
          [self didSizeChanged]);
}

- (LynxOverflowType)getInitialOverflowType {
  return LynxOverflowHidden;
}

#pragma mark native storage in list

- (NSMutableDictionary*)listNativeStateCache {
  return nil;
}

- (NSMutableDictionary*)getNativeStorageFromList:(LynxUI*)list {
  return list.listNativeStateCache;
}

- (void)removeKeyFromNativeStorage:(LynxUI*)list key:(NSString*)key {
  [list.listNativeStateCache removeObjectForKey:key];
}

- (void)storeKeyToNativeStorage:(LynxUI*)list key:(NSString*)key value:(id)value {
  if (![list isKindOfClass:LynxUICollection.class] &&
      ![list isKindOfClass:LynxUIListContainer.class]) {
    return;
  }
  [list.listNativeStateCache setObject:value forKey:key];
}

- (BOOL)initialPropsFlushed:(NSString*)initialPropKey cacheKey:(NSString*)cacheKey {
  return YES;
}

- (void)setInitialPropsHasFlushed:(NSString*)initialPropKey cacheKey:(NSString*)cacheKey {
}

- (void)restoreKeyframeStateFromStorage:(NSString*)itemKey withList:(LynxUI*)list {
  if (!_enableReuseAnimationState || !itemKey) {
    return;
  }
  if (![list isKindOfClass:LynxUICollection.class] &&
      ![list isKindOfClass:LynxUIListContainer.class]) {
    return;
  }

  // When onListCellAppear is triggered, mAnimationInfos has already been updated and is the latest.
  if (_animationInfos != nil) {
    NSString* uniqueAnimationCacheKey =
        [NSString stringWithFormat:@"Animation_%@_%@_%@", self.tagName, itemKey, self.idSelector];
    id animationManager =
        [[self getNativeStorageFromList:list] objectForKey:uniqueAnimationCacheKey];
    if (animationManager) {
      [self removeKeyFromNativeStorage:list key:uniqueAnimationCacheKey];
      // Before onListCellAppear is triggered, onNodeReady may have already been triggered, which
      // would initiate a complete animation execution from the beginning. We need to clear this
      // animation first.
      if (_animationManager) {
        [_animationManager detachFromUI];
        _animationManager = nil;
      }

      [animationManager attachToUI:self];
      _animationManager = animationManager;
    } else {
      [self prepareKeyframeManager];
    }
    [_animationManager setAnimations:_animationInfos];
    [_animationManager notifyAnimationUpdated];
  }
}

- (void)saveKeyframeStateToStorage:(NSString*)itemKey exist:(BOOL)isExist withList:(LynxUI*)list {
  if (!_enableReuseAnimationState || !itemKey) {
    return;
  }
  if ((isExist && self.animationManager != nil) || !isExist) {
    NSString* uniqueAnimationCacheKey =
        [NSString stringWithFormat:@"Animation_%@_%@_%@", self.tagName, itemKey, self.idSelector];
    if (!isExist) {
      [self removeKeyFromNativeStorage:list key:uniqueAnimationCacheKey];
    } else {
      [self storeKeyToNativeStorage:list key:uniqueAnimationCacheKey value:self.animationManager];
    }
    [_animationManager detachFromUI];
    _animationManager = nil;
  }
}

- (void)onListCellAppear:(NSString*)itemKey withList:(LynxUI*)list {
  for (LynxUI* child in self.children) {
    if (!child.blockListEvent) {
      [child onListCellAppear:itemKey withList:list];
    }
  }
  [self restoreKeyframeStateFromStorage:itemKey withList:list];
}

- (void)onListCellDisappear:(NSString*)itemKey exist:(BOOL)isExist withList:(LynxUI*)list {
  for (LynxUI* child in self.children) {
    if (!child.blockListEvent) {
      [child onListCellDisappear:itemKey exist:isExist withList:list];
    }
  }
  [self saveKeyframeStateToStorage:itemKey exist:isExist withList:list];
}

- (void)onListCellPrepareForReuse:(NSString*)itemKey withList:(LynxUI*)list {
  for (LynxUI* child in self.children) {
    if (!child.blockListEvent) {
      [child onListCellPrepareForReuse:itemKey withList:list];
    }
  }
}

- (BOOL)notifyParent {
  return NO;
}

- (CGFloat)toPtWithUnitValue:(NSString*)unitValue fontSize:(CGFloat)fontSize {
  LynxUI* rootUI = (LynxUI*)self.context.rootUI;
  return [LynxUnitUtils toPtWithScreenMetrics:self.context.screenMetrics
                                    unitValue:unitValue
                                 rootFontSize:rootUI.fontSize
                                  curFontSize:fontSize
                                    rootWidth:CGRectGetWidth(rootUI.frame)
                                   rootHeight:CGRectGetHeight(rootUI.frame)
                                withDefaultPt:0];
}

- (UIView*)accessibilityAttachedCell {
  if (!self.accessibilityAttachedCellClass) {
    return nil;
  }
  if (!_accessibilityAttachedCell) {
    // find attached cell from lower to upper
    UIView* view = self.view;
    while (view && ![view isKindOfClass:self.accessibilityAttachedCellClass]) {
      view = view.superview;
    }
  }
  return _accessibilityAttachedCell;
}

- (void)autoScrollIfFocusedChanged:(NSNotification*)notification {
  if (@available(iOS 9.0, *)) {
    UIView* currentFocusedElement = notification.userInfo[UIAccessibilityFocusedElementKey];
    if ([currentFocusedElement isKindOfClass:UIView.class]) {
      NSArray* array;
      UIView* cell = [self accessibilityAttachedCell];
      if (cell) {
        array = cell.accessibilityElements;
      } else {
        array = self.view.accessibilityElements;
      }

      // Automatically scroll to the focused item
      if ([array containsObject:currentFocusedElement]) {
        UIScrollView* scrollView = [self accessibilityFindScrollView:currentFocusedElement];
        if (scrollView) {
          // handle both horizontal and vertical
          if (scrollView.contentSize.width > scrollView.contentSize.height) {
            // scroll to center horizontally
            CGPoint pointCenterInScrollView =
                [currentFocusedElement convertPoint:currentFocusedElement.center toView:scrollView];
            CGPoint targetOffset = CGPointMake(0, scrollView.contentOffset.y);
            targetOffset.x =
                pointCenterInScrollView.x - currentFocusedElement.frame.size.width / 2.0;
            // we can not scroll beyond bounces
            targetOffset.x =
                MAX(-scrollView.contentInset.left,
                    MIN(targetOffset.x, scrollView.contentSize.width - scrollView.frame.size.width +
                                            scrollView.contentInset.right));
            [scrollView setContentOffset:targetOffset];
          } else if (scrollView.contentSize.width < scrollView.contentSize.height) {
            // scroll to center vertically
            CGPoint pointCenterInScrollView =
                [currentFocusedElement convertPoint:currentFocusedElement.center toView:scrollView];
            CGPoint targetOffset = CGPointMake(scrollView.contentOffset.x, 0);
            targetOffset.y =
                pointCenterInScrollView.y - currentFocusedElement.frame.size.height / 2.0;
            // we can not scroll beyond bounces
            targetOffset.y = MAX(
                -scrollView.contentInset.top,
                MIN(targetOffset.y, scrollView.contentSize.height - scrollView.frame.size.height +
                                        scrollView.contentInset.bottom));
            [scrollView setContentOffset:targetOffset];
          }
        }
      }
    }
  }
}

- (UIScrollView*)accessibilityFindScrollView:(UIView*)child {
  // find the first LynxScrollView which contains the target view
  UIView* view = child;
  Class cls = NSClassFromString(@"LynxScrollView");
  while (view && ![view isKindOfClass:cls]) {
    view.accessibilityElementsHidden = NO;
    view = view.superview;
  }
  return (UIScrollView*)view;
}

/// General API to set value via KVC for all layers, which are owned by this UI and parallel to the
/// LynxUI.view.layer, in `LynxBackgroundManager`. Currently backgroundLayer, borderLayer and
/// maskLayer.
///
/// - Parameters:
///   - value: value for key.
///   - keyPath: key path for for layer's property.
///   - forAllLayers: true to set the value for all layers related to the UI, and parallel to the
///   view.layer. (background layer, border layer, mask layer)
- (void)setLayerValue:(id)value
           forKeyPath:(nonnull NSString*)keyPath
         forAllLayers:(BOOL)forAllLayers {
  [self.view.layer setValue:value forKeyPath:keyPath];

  if (forAllLayers) {
    [self.backgroundManager.backgroundLayer setValue:value forKeyPath:keyPath];
    [self.backgroundManager.borderLayer setValue:value forKeyPath:keyPath];
    [self.backgroundManager.maskLayer setValue:value forKeyPath:keyPath];
  }
}

/**
 * @name: should-rasterize-shadow
 * @description: Use bitmap backend to draw shadows, avoiding off-screen rendering.
 * @note: box-shadow
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.8
 **/
LYNX_PROP_DEFINE("should-rasterize-shadow", setShouldRasterizeShadow, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _backgroundManager.shouldRasterizeShadow = value;
}

/**
 * @name: accessibility-attached-cell-class
 * @description: Identifying the class name of a UITableViewCell. If the LynxView is in a
 *UITableViewCell, we have to set accessibilityElements to the cell itself, so that accessibility
 *changes can be responded.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.8
 **/
LYNX_PROP_DEFINE("accessibility-attached-cell-class", setAccessibilityAttachedCellClass,
                 NSString*) {
  // Rest the class and the cell, and will fetch the cell later in `onNodeReady`
  self.accessibilityAttachedCellClass = NSClassFromString(value);
  _accessibilityAttachedCell = nil;
}

/**
 * @name: accessibility-auto-scroll-if-focused
 * @description: Automatically scroll to focused accessibility element if it is focused by code.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.8
 **/
LYNX_PROP_DEFINE("accessibility-auto-scroll-if-focused", setAccessibilityAutoScrollIfFocused,
                 BOOL) {
  self.accessibilityAutoScroll = value;
}

/// This is a standard CSS property `clip-path`
/// - Parameter basicShape: <basic-shape-function> = [function type, params]
LYNX_PROP_DEFINE("clip-path", setClipPath, NSArray*) {
  if (requestReset || !value || [value count] < 1) {
    _clipPath = nil;
    // reset the layer mask.
    [self updateLayerMaskOnFrameChanged];
    return;
  }
  LynxBasicShapeType type = [[value objectAtIndex:0] intValue];
  switch (type) {
    case LynxBasicShapeTypeCircle:
    case LynxBasicShapeTypeInset:
      _clipPath = LBSCreateBasicShapeFromArray(value);
      break;
    case LynxBasicShapeTypePath: {
      if ([value count] != 2) {
        // Native parse error occurss. Reset the path.
        _clipPath = nil;
        break;
      }
      id data = [value objectAtIndex:1];
      if (![data isKindOfClass:[NSString class]]) {
        _clipPath = nil;
        break;
      }
      _clipPath = LBSCreateBasicShapeFromPathData((NSString*)data);
      break;
    }
    default:
      _clipPath = nil;
  }
  [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
    [ui updateLayerMaskOnFrameChanged];
  }];
}

/**
 * @name: copyable
 * @description: Allowing copy LynxUI from one to another at native
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.11
 **/
LYNX_PROP_DEFINE("copyable", copyable, BOOL) { _copyable = value; }

- (void)dealloc {
  [self clearExclusiveAccessibilityElements:self.accessibilityBeingExclusiveFocusedNodes];
}

/**
 * @name: accessibility-exclusive-focus
 * @description: When set to true, only accessible elements within the current subtree can be
 *focused.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.12
 **/
LYNX_PROP_DEFINE("accessibility-exclusive-focus", setAccessibilityExclusiveFocus, BOOL) {
  if (UIAccessibilityIsVoiceOverRunning()) {
    [self.nodeReadyBlockArray addObject:^(LynxUI* ui) {
      ui.accessibilityBeingExclusiveFocusedNodes =
          [ui setExclusiveAccessibilityElements:value
                                        subTree:ui
                                  previousNodes:ui.accessibilityBeingExclusiveFocusedNodes];
    }];
  }
}

- (NSInteger)getGestureArenaMemberId {
  return _gestureArenaMemberId;
}

- (BOOL)canConsumeGesture:(CGPoint)delta {
  return YES;
}

- (BOOL)getGestureBorder:(BOOL)start {
  return NO;
}

- (NSDictionary<NSNumber*, LynxGestureDetectorDarwin*>*)getGestureDetectorMap {
  return _gestureMap;
}

- (NSDictionary<NSNumber*, LynxBaseGestureHandler*>*)getGestureHandlers {
  if (!_gestureHandlers) {
    _gestureHandlers = [LynxBaseGestureHandler convertToGestureHandler:self.sign
                                                               context:self.context
                                                                member:self
                                                      gestureDetectors:_gestureMap];
  }

  return _gestureHandlers;
}

- (CGFloat)getMemberScrollX {
  return 0.0;
}

- (CGFloat)getMemberScrollY {
  return 0.0;
}

- (void)onGestureScrollBy:(CGPoint)delta {
}

- (NSArray<NSNumber*>*)scrollBy:(CGFloat)deltaX deltaY:(CGFloat)deltaY {
  return @[ @0, @0 ];
}

/**
 * @name: hit-slop
 * @description: Change the margin of ui for hit-test. A positive value means to expand the
 * boundary, and a negative value means to shrink the boundary. The changes in the four directions
 * can be controlled by top, bottom, left, and right. When no direction is specified, it means that
 * the four directions are changed at the same time.
 * @category: stable
 * @standardAction: keep
 * @supportVersion: 2.14
 **/
LYNX_PROP_DEFINE("hit-slop", setHitSlop, NSObject*) {
  if (value == nil ||
      !([value isKindOfClass:[NSDictionary class]] || [value isKindOfClass:[NSString class]])) {
    return;
  }

  if ([value isKindOfClass:[NSDictionary class]] && [(NSDictionary*)value count] > 0) {
    NSArray* keySet = @[ @"top", @"bottom", @"left", @"right" ];
    NSDictionary* dict = (NSDictionary*)value;
    for (NSString* key in dict) {
      id keyValue = [dict valueForKey:key];
      if ([keySet containsObject:key] && [keyValue isKindOfClass:[NSString class]]) {
        CGFloat ptValue = [LynxUnitUtils toPtFromUnitValue:keyValue];
        switch ([keySet indexOfObject:key]) {
          case 0:
            _hitSlopTop = _hitSlopTop != ptValue ? ptValue : _hitSlopTop;
            break;
          case 1:
            _hitSlopBottom = _hitSlopBottom != ptValue ? ptValue : _hitSlopBottom;
            break;
          case 2:
            _hitSlopLeft = _hitSlopLeft != ptValue ? ptValue : _hitSlopLeft;
            break;
          case 3:
            _hitSlopRight = _hitSlopRight != ptValue ? ptValue : _hitSlopRight;
            break;
          default:
            break;
        };
      }
    }
    return;
  }

  CGFloat ptValue = [LynxUnitUtils toPtFromUnitValue:(NSString*)value];
  if (_hitSlopTop != ptValue || _hitSlopBottom != ptValue || _hitSlopLeft != ptValue ||
      _hitSlopRight != ptValue) {
    _hitSlopTop = _hitSlopBottom = _hitSlopLeft = _hitSlopRight = ptValue;
  }
}

@end
