// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUI.h"
#import "LynxUIMethodProcessor.h"

NS_ASSUME_NONNULL_BEGIN

typedef id<NSObject> _Nullable (^lynx_async_operation_block_t)(void);
typedef void (^lynx_async_operation_completion_block_t)(id _Nullable value, BOOL canceled);
typedef BOOL (^lynx_iscancelled_block_t)(void);
typedef void (^lynx_async_display_completion_block_t)(UIImage *image);
typedef UIImage *_Nullable (^lynx_async_get_background_image_block_t)(void);

FOUNDATION_EXPORT short const OVERFLOW_XY_VAL;
FOUNDATION_EXPORT short const OVERFLOW_HIDDEN_VAL;
@class LynxGestureArenaManager;

@interface LynxUI () {
 @package
  volatile int32_t _displaySentinel;
  BOOL _asyncDisplayFromTTML;
  // See LynxUI+Fluency.h for more details.
  BOOL _enableScrollMonitor;
  // See LynxUI+Fluency.h for more details.
  NSString *_scrollMonitorTagName;
}

@property(nonatomic, strong) NSMutableDictionary *lynxProps;
@property(nonatomic, weak, readwrite) LynxUIContext *context;
@property(nonatomic, readwrite) short overflow;

// TODO:(liyanbo) move this into CSSInfos.
@property(nonatomic, assign) BOOL isFirstAnimatedReady;

// Default value is false. When setting _simultaneousTouch as true and clicking to the ui or its sub
// ui, the lynx touch gestures will not fail.
@property(nonatomic, readonly) BOOL enableSimultaneousTouch;

// exposure screen border
@property(nonatomic) CGFloat exposureMarginTop;
@property(nonatomic) CGFloat exposureMarginBottom;
@property(nonatomic) CGFloat exposureMarginLeft;
@property(nonatomic) CGFloat exposureMarginRight;

@property(nonatomic) NSString *exposureUIMarginTop;
@property(nonatomic) NSString *exposureUIMarginBottom;
@property(nonatomic) NSString *exposureUIMarginLeft;
@property(nonatomic) NSString *exposureUIMarginRight;

@property(nonatomic) NSString *exposureArea;

@property(nonatomic, readonly) CGFloat hitSlopTop;
@property(nonatomic, readonly) CGFloat hitSlopBottom;
@property(nonatomic, readonly) CGFloat hitSlopLeft;
@property(nonatomic, readonly) CGFloat hitSlopRight;

@property(nonatomic) int32_t pseudoStatus;

@property(nonatomic, assign, readonly) BOOL alignHeight;
@property(nonatomic, assign, readonly) BOOL alignWidth;

@property(nonatomic) uint32_t nodeIndex;

@property(nonatomic, strong)
    NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *_Nullable gestureMap;

- (void)dispatchMoveToWindow:(UIWindow *)window;

- (BOOL)containsPoint:(CGPoint)point;
- (BOOL)containsPoint:(CGPoint)point inHitTestFrame:(CGRect)frame;
- (BOOL)childrenContainPoint:(CGPoint)point;

- (CGRect)getHitTestFrame;
- (CGRect)getHitTestFrameWithFrame:(CGRect)frame;
- (CGPoint)getHitTestPoint:(CGPoint)inPoint;

- (BOOL)enableExposureUIMargin;

- (void)setImplicitAnimation;

- (void)scrollIntoViewWithSmooth:(BOOL)isSmooth
                       blockType:(NSString *)blockType
                      inlineType:(NSString *)inlineType
                        callback:(LynxUIMethodCallbackBlock)callback;

/// Fixme: give internal LynxUI subclass a way to disable async-display
- (void)setAsyncDisplayFromTTML:(BOOL)async;

- (void)updateFrameWithoutLayoutAnimation:(CGRect)frame
                              withPadding:(UIEdgeInsets)padding
                                   border:(UIEdgeInsets)border
                                   margin:(UIEdgeInsets)margin;

- (void)onLayoutAnimationStart:(CGRect)frame;
- (void)onLayoutAnimationEnd:(CGRect)frame;

- (CGRect)getRectToWindow;

- (BOOL)didSizeChanged;
- (BOOL)shouldReDoTransform;

- (void)setName:(NSString *)name;

- (BOOL)isVisible;

- (LynxGestureArenaManager *)getGestureArenaManager;

- (void)markNeedDisplay;

// accessibility-related
@property(nonatomic, assign) BOOL useDefaultAccessibilityLabel;
@property(nonatomic, readonly) BOOL enableAccessibilityByDefault;
@property(nonatomic, readonly) UIAccessibilityTraits accessibilityTraitsByDefault;

@end

@interface LynxUI (AsyncDisplay)

- (void)displayAsynchronously;

- (BOOL)enableAsyncDisplay;

- (void)displayAsyncWithCompletionBlock:(lynx_async_display_completion_block_t)block;

- (id)drawParameter;

- (void)displayComplexBackgroundAsynchronouslyWithDisplay:
            (lynx_async_get_background_image_block_t)displayBlock
                                               completion:(lynx_async_display_completion_block_t)
                                                              completionBlock;

+ (void)drawRect:(CGRect)bounds withParameters:(id)drawParameters;

typedef void (^LynxCGContextImageDrawingActions)(CGContextRef context);

+ (UIImage *)imageWithActionBlock:(LynxCGContextImageDrawingActions)action
                           opaque:(BOOL)opaque
                            scale:(CGFloat)scale
                             size:(CGSize)size;

@end

NS_ASSUME_NONNULL_END
