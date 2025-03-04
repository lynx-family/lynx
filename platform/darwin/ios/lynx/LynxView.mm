// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxView.h"
#import "LynxBaseInspectorOwner.h"
#import "LynxBaseLogBoxProxy.h"
#import "LynxContext.h"
#import "LynxDevtool.h"
#import "LynxEngineProxy.h"
#import "LynxEnv.h"
#import "LynxError.h"
#import "LynxErrorBehavior.h"
#import "LynxFeatureCounter.h"
#import "LynxHeroTransition.h"
#import "LynxLazyRegister.h"
#import "LynxLifecycleDispatcher.h"
#import "LynxLog.h"
#import "LynxService.h"
#import "LynxSubErrorCode.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTemplateRenderDelegate.h"
#import "LynxThreadManager.h"
#import "LynxTraceEvent.h"
#import "LynxUIKitAPIAdapter.h"
#import "LynxUIRendererProtocol.h"
#import "LynxView+Protected.h"
#import "LynxWeakProxy.h"

#include "base/include/lynx_actor.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

#define RUN_RENDER_SAFELY(method)           \
  do {                                      \
    if (_templateRender != nil) {           \
      method                                \
    } else {                                \
      _LogW(@"LynxTemplateRender is nil."); \
    }                                       \
  } while (0)

@interface LynxView () <LynxTemplateRenderDelegate>

@end

@implementation LynxView

#pragma mark - Init

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
  NSString* description =
      [NSString stringWithFormat:@"%s in class %@ is unavailable", sel_getName(_cmd), self.class];
  NSAssert(false, description);
#pragma clang diagnostic pop
  return nil;
}

- (instancetype)init {
  return [self initWithBuilderBlock:nil];
}

- (instancetype)initWithFrame:(CGRect)frame {
  return [self initWithBuilderBlock:^(LynxViewBuilder* builder) {
    builder.frame = frame;
  }];
}

- (instancetype)initWithBuilderBlock:(void (^)(NS_NOESCAPE LynxViewBuilder*))block {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxView::initWithBuilderBlock");
  [LynxLazyRegister loadLynxInitTask];
  self = [super initWithFrame:CGRectZero];
  self.accessibilityLabel = @"lynxview";
  _dispatchingIntrinsicContentSizeChange = NO;
  if (self) {
    [self initLifecycleDispatcher];
  }
  _templateRender = [[LynxTemplateRender alloc] initWithBuilderBlock:block lynxView:self];
  _lifecycleDispatcher.instanceId = [_templateRender instanceId];
  return self;
}

- (instancetype)initWithoutRender {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxView::initWithoutRender");
  [LynxLazyRegister loadLynxInitTask];
  self = [super initWithFrame:CGRectZero];
  self.accessibilityLabel = @"lynxview";
  [self initLifecycleDispatcher];

  return self;
}

- (void)initLifecycleDispatcher {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxView::initLifecycleDispatcher");
  _lifecycleDispatcher = [[LynxLifecycleDispatcher alloc] init];
  [_lifecycleDispatcher addLifecycleClient:[LynxEnv sharedInstance].lifecycleDispatcher];
}

#pragma mark - Clean

- (void)dealloc {
  [self clearForDestroy];
}

- (void)clearForDestroy {
  _LogI(@"Lynxview %p: clearForDestroy", self);
  [self clearModuleForDestroy];
  if (_dispatchingIntrinsicContentSizeChange) {
    _LogI(@"Warning!!!! you possibly call clearForDestroy inside of layoutDidFinish call stack");
  }

  // forbidden clearForDestroy not on ui thread
  if (![NSThread isMainThread]) {
    NSString* stack =
        [[[NSThread callStackSymbols] valueForKey:@"description"] componentsJoinedByString:@"\n"];
    NSString* errMsg = [NSString
        stringWithFormat:@"LynxView %p clearForDestroy not on ui thread, thread:%@, stack:%@", self,
                         [NSThread currentThread], stack];
    if (_templateRender) {
      [_templateRender onErrorOccurred:ECLynxThreadWrongThreadDestroyError message:errMsg];
    } else {
      [self dispatchError:[LynxError lynxErrorWithCode:ECLynxThreadWrongThreadDestroyError
                                               message:errMsg]];
    }
    _LogE(@"LynxView %p clearForDestroy not on ui thread, thread:%@", self,
          [NSThread currentThread]);
  }

  if (_templateRender) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::didReportComponentInfo");
  }
  // need set nil here, else call _templateRender after clearForDestroy
  // will cause crash in LynxShell
  _templateRender = nil;
}

- (void)resetViewAndLayer {
  // clear view
  [[self subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
  [[self.layer sublayers] makeObjectsPerformSelector:@selector(removeFromSuperlayer)];
}

- (void)reset {
  // clear view
  _LogI(@"LynxView %p:reset", self);
  [[self subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
  [[self.layer sublayers] makeObjectsPerformSelector:@selector(removeFromSuperlayer)];

  RUN_RENDER_SAFELY([_templateRender reset];);
}

#pragma mark - Load Template

// This method can only be accessed from main thread
- (void)loadTemplate:(LynxLoadMeta*)meta {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxView::loadTemplateWithLynxLoadMeta");
  [self setUpModuleGlobalProps];
  if (_dispatchingIntrinsicContentSizeChange) {
    _LogI(@"Warning!!!! you possibly call loadTemplateBundle inside of layoutDidFinish call stack");
  }
  RUN_RENDER_SAFELY([_templateRender loadTemplate:meta];);
}

- (void)loadTemplateFromURL:(NSString*)url {
  [self loadTemplateFromURL:url initData:nil];
}

- (void)loadTemplateFromURL:(NSString*)url initData:(LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxView::loadTemplateFromURL", "url", [url UTF8String]);
  [self setUpModuleGlobalProps];
  if (_dispatchingIntrinsicContentSizeChange) {
    _LogI(
        @"Warning!!!! you possibly call loadTemplateFromURL inside of layoutDidFinish call stack");
  }
  RUN_RENDER_SAFELY([_templateRender loadTemplateFromURL:url initData:data];);
}

- (void)loadTemplate:(NSData*)tem withURL:(NSString*)url {
  [self loadTemplate:tem withURL:url initData:nil];
}

- (void)loadTemplate:(NSData*)tem withURL:(NSString*)url initData:(LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxView::loadTemplateWithURL", "url", [url UTF8String]);
  [self setUpModuleGlobalProps];
  if (_dispatchingIntrinsicContentSizeChange) {
    _LogI(@"Warning!!!! you possibly call loadTemplate inside of layoutDidFinish call stack");
  }
  RUN_RENDER_SAFELY([_templateRender loadTemplate:tem withURL:url initData:data];);
}

- (void)loadTemplateBundle:(LynxTemplateBundle*)bundle
                   withURL:(NSString*)url
                  initData:(LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxView::loadTemplateBundle", "url", [url UTF8String]);
  [self setUpModuleGlobalProps];
  if (_dispatchingIntrinsicContentSizeChange) {
    _LogI(@"Warning!!!! you possibly call loadTemplateBundle inside of layoutDidFinish call stack");
  }
  RUN_RENDER_SAFELY([_templateRender loadTemplateBundle:bundle withURL:url initData:data];);
}

#pragma mark - SSR

- (void)loadSSRData:(nonnull NSData*)tem
            withURL:(nonnull NSString*)url
           initData:(LynxTemplateData*)data {
  _LogI(@"LynxView %p: start loadSSRData with %@", self, url);
  RUN_RENDER_SAFELY([_templateRender loadSSRData:tem withURL:url initData:data];);
}

- (void)loadSSRDataFromURL:(NSString*)url initData:(LynxTemplateData*)data {
  _LogI(@"LynxView %p: start loadSSRDataFromURL with %@", self, url);
  RUN_RENDER_SAFELY([_templateRender loadSSRDataFromURL:url initData:data];);
}

- (void)ssrHydrate:(nonnull NSData*)tem
           withURL:(nonnull NSString*)url
          initData:(nullable LynxTemplateData*)data {
  _LogI(@"LynxView %p: start ssrHydrate with %@", self, url);
  RUN_RENDER_SAFELY([_templateRender ssrHydrate:tem withURL:url initData:data];);
}

- (void)ssrHydrateFromURL:(NSString*)url initData:(nullable LynxTemplateData*)data {
  _LogI(@"LynxView %p: start ssrHydrateFromURL with %@", self, url);
  RUN_RENDER_SAFELY([_templateRender ssrHydrateFromURL:url initData:data];);
}

#pragma mark - Update Data

- (void)updateDataWithString:(NSString*)data {
  [self updateDataWithString:data processorName:nil];
}

- (void)updateDataWithString:(NSString*)data processorName:(NSString*)name {
  RUN_RENDER_SAFELY([_templateRender updateDataWithString:data processorName:name];);
}

- (void)updateDataWithDictionary:(NSDictionary<NSString*, id>*)data {
  [self updateDataWithDictionary:data processorName:nil];
}

- (void)updateDataWithDictionary:(NSDictionary<NSString*, id>*)data processorName:(NSString*)name {
  RUN_RENDER_SAFELY([_templateRender updateDataWithDictionary:data processorName:name];);
}

- (void)updateDataWithTemplateData:(LynxTemplateData*)data {
  RUN_RENDER_SAFELY([_templateRender updateDataWithTemplateData:data];);
}

- (void)updateMetaData:(LynxUpdateMeta*)meta {
  // TODO(nihao.royal) migrate updateData/updateGlobalProps to this api later~
  RUN_RENDER_SAFELY([_templateRender updateMetaData:meta];);
}

#pragma mark - Global Props

- (void)updateGlobalPropsWithDictionary:(NSDictionary<NSString*, id>*)data {
  RUN_RENDER_SAFELY([_templateRender updateGlobalPropsWithDictionary:data];);
}

- (void)updateGlobalPropsWithTemplateData:(LynxTemplateData*)data {
  RUN_RENDER_SAFELY([_templateRender updateGlobalPropsWithTemplateData:data];);
}

#pragma mark - Reset & Reload

- (void)resetDataWithTemplateData:(LynxTemplateData*)data {
  RUN_RENDER_SAFELY([_templateRender resetDataWithTemplateData:data];);
}

- (void)reloadTemplateWithTemplateData:(LynxTemplateData*)data {
  [self reloadTemplateWithTemplateData:data globalProps:nil];
}

- (void)reloadTemplateWithTemplateData:(nonnull LynxTemplateData*)data
                           globalProps:(nullable LynxTemplateData*)globalProps {
  RUN_RENDER_SAFELY([_templateRender reloadTemplateWithTemplateData:data globalProps:globalProps];);
}

#pragma mark - Storage

- (void)setSessionStorageItem:(NSString*)key withTemplateData:(LynxTemplateData*)data {
  RUN_RENDER_SAFELY([_templateRender setSessionStorageItem:key WithTemplateData:data];);
}

- (void)getSessionStorageItem:(NSString*)key
                 withCallback:(void (^)(id<NSObject> _Nullable))callback {
  RUN_RENDER_SAFELY([_templateRender getSessionStorageItem:key withCallback:callback];);
}

- (double)subscribeSessionStorage:(NSString*)key
                     withCallback:(void (^)(id<NSObject> _Nullable))callback {
  return [_templateRender subscribeSessionStorage:key withCallback:callback];
}

- (void)unSubscribeSessionStorage:(NSString*)key withId:(double)callbackId {
  [_templateRender unSubscribeSessionStorage:key withId:callbackId];
}

#pragma mark - Override

- (void)layoutSubviews {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxView::layoutSubviews", "instanceId",
              [_templateRender instanceId]);
  if (_enableSyncFlush && [self.subviews count] > 0) {
    [self syncFlush];
  }
  [super layoutSubviews];
  RUN_RENDER_SAFELY([_templateRender triggerLayoutInTick];);
}

- (void)didMoveToWindow {
  [super didMoveToWindow];

  RUN_RENDER_SAFELY([_templateRender didMoveToWindow:self.window == nil];);
}

- (void)willMoveToWindow:(UIWindow*)newWindow {
  [super willMoveToWindow:newWindow];

  RUN_RENDER_SAFELY([_templateRender willMoveToWindow:newWindow];);
}

- (void)setFrame:(CGRect)frame {
  // TODO should update viewport here?
  [super setFrame:frame];
  [_templateRender.lynxUIRenderer onSetFrame:frame];
}

- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
    [self showMessageOnConsole:[NSString
                                   stringWithFormat:@"LynxView: hit test for lynx %ld", [self hash]]
                     withLevel:LynxLogBoxLevelInfo];
  }
  _LogI(@"LynxView %p: hitTest with point.x: %f, point.y: %f", self, point.x, point.y);

  if ([_templateRender.lynxUIRenderer needHandleHitTest]) {
    return [_templateRender.lynxUIRenderer hitTest:point withEvent:event];
  }

  id<LynxEventTarget> touchTarget = nil;
  RUN_RENDER_SAFELY(touchTarget = [_templateRender hitTestInEventHandler:point withEvent:event];);
  UIView* view = [super hitTest:point withEvent:event];

  if ([self needEndEditing:view] &&
      ![[[[view superview] superview] superview] isKindOfClass:[UITextView class]] &&
      ![touchTarget ignoreFocus] && ![self tapOnUICalloutBarButton:point withEvent:event]) {
    // To free our touch handler from being blocked, dispatch endEditing asynchronously.
    __weak LynxView* weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf endEditing:true];
    });
  }
  // If target eventThrough, return nil to let event through LynxView.
  if ([touchTarget eventThrough]) {
    return nil;
  } else {
    return view;
  }
}

- (BOOL)needEndEditing:(UIView*)view {
  if ([view isKindOfClass:[UITextField class]] || [view isKindOfClass:[UITextView class]]) {
    return NO;
  }

  // In UITextView case, when user chose "SelectAll", the view hierarchy will be like this:
  // UITextRangeView -> UITextSelectionView -> _UITextContainerView -> LynxTextView
  // However, UITextRangeView is a private class which is not accessible, so we can only
  // use [[[superview]superview]superview] as judge condition to avoid keyboard being folded
  // so that user can adjust cursor positions.
  if ([[[[view superview] superview] superview] isKindOfClass:[UITextView class]]) {
    return NO;
  }

  // In iOS16 & UITextField has the same issue mentioned before, the view hierarchy will be like
  // this: UITextRangeView -> UITextSelectionView -> _UITextLayoutView -> UIFieldEditor ->
  // LynxTextField so use [[[[superview] superview] superview] superview] to handle this
  // situation.
  if (@available(iOS 16.0, *)) {
    if ([[[[[view superview] superview] superview] superview] isKindOfClass:[UITextField class]]) {
      return NO;
    }
  }

  return YES;
}

#pragma mark - View

- (void)updateScreenMetricsWithWidth:(CGFloat)width height:(CGFloat)height {
  [_templateRender updateScreenMetricsWithWidth:width height:height];
}

/**
 UICTContentSizeCategoryXS                     0.824
 UICTContentSizeCategoryS                      0.882
 UICTContentSizeCategoryM                      0.942
 UICTContentSizeCategoryL                      1.0
 UICTContentSizeCategoryXL                     1.1118
 UICTContentSizeCategoryXXL                    1.235
 UICTContentSizeCategoryXXXL                   1.353
 UICTContentSizeCategoryAccessibilityM         1.647
 UICTContentSizeCategoryAccessibilityL         1.941
 UICTContentSizeCategoryAccessibilityXL        2.353
 UICTContentSizeCategoryAccessibilityXXL       2.764
 UICTContentSizeCategoryAccessibilityXXXL      3.118
 */
- (void)updateFontScale:(CGFloat)scale {
  if (_templateRender != nil) {
    [_templateRender updateFontScale:scale];
  }
  [LynxFeatureCounter count:LynxFeatureObjcUpdateFontScale
                 instanceId:(_templateRender != nil ? [_templateRender instanceId]
                                                    : lynx::shell::kUnknownInstanceId)];
}

- (void)pauseRootLayoutAnimation {
  RUN_RENDER_SAFELY([_templateRender pauseRootLayoutAnimation];);
}

- (void)resumeRootLayoutAnimation {
  RUN_RENDER_SAFELY([_templateRender resumeRootLayoutAnimation];);
}

- (void)resetAnimation {
  RUN_RENDER_SAFELY([_templateRender resetAnimation];);
}
- (void)restartAnimation {
  RUN_RENDER_SAFELY([_templateRender restartAnimation];);
}

- (void)setTheme:(LynxTheme*)theme {
  [LynxFeatureCounter count:LynxFeatureObjcSetThemeIos
                 instanceId:(_templateRender != nil ? [_templateRender instanceId]
                                                    : lynx::shell::kUnknownInstanceId)];

  RUN_RENDER_SAFELY([_templateRender setTheme:theme];);
}

- (nullable LynxTheme*)theme {
  [LynxFeatureCounter count:LynxFeatureObjcGetThemeIos
                 instanceId:(_templateRender != nil ? [_templateRender instanceId]
                                                    : lynx::shell::kUnknownInstanceId)];
  if (_templateRender != nil) {
    return [_templateRender theme];
  } else {
    return nil;
  }
}

- (void)setNeedPendingUIOperation:(BOOL)needPendingUIOperation {
  [_templateRender setNeedPendingUIOperation:needPendingUIOperation];
}

- (void)triggerLayout {
  RUN_RENDER_SAFELY([_templateRender triggerLayout];);
}

- (void)addLifecycleClient:(id<LynxViewBaseLifecycle>)lifecycleClient {
  if (lifecycleClient) {
    [_lifecycleDispatcher addLifecycleClient:lifecycleClient];
  }
}

- (void)removeLifecycleClient:(id<LynxViewBaseLifecycle>)lifecycleClient {
  if (lifecycleClient) {
    [_lifecycleDispatcher removeLifecycleClient:lifecycleClient];
  }
}

- (void)updateViewportWithPreferredLayoutWidth:(CGFloat)preferredLayoutWidth
                         preferredLayoutHeight:(CGFloat)preferredLayoutHeight {
  [self updateViewportWithPreferredLayoutWidth:preferredLayoutWidth
                         preferredLayoutHeight:preferredLayoutHeight
                                    needLayout:YES];
}

- (void)updateViewportWithPreferredLayoutWidth:(CGFloat)preferredLayoutWidth
                         preferredLayoutHeight:(CGFloat)preferredLayoutHeight
                                    needLayout:(BOOL)needLayout {
  [self setPreferredLayoutWidth:preferredLayoutWidth];
  [self setPreferredLayoutHeight:preferredLayoutHeight];
  RUN_RENDER_SAFELY([_templateRender updateViewport:needLayout];);
}

#pragma mark - Find Node

- (LynxUI*)findUIByIndex:(int)index {
  if (_templateRender != nil) {
    return [_templateRender findUIBySign:index];
  } else {
    return nil;
  }
}

- (nullable UIView*)findViewWithName:(nonnull NSString*)name {
  if (_templateRender) {
    return [_templateRender findViewWithName:name];
  } else {
    return nil;
  }
}

- (nullable LynxUI*)uiWithName:(nonnull NSString*)name {
  if (_templateRender != nil) {
    return [_templateRender uiWithName:name];
  } else {
    return nil;
  }
}

- (nullable LynxUI*)uiWithIdSelector:(nonnull NSString*)idSelector {
  if (_templateRender != nil) {
    return [_templateRender uiWithIdSelector:idSelector];
  } else {
    return nil;
  }
}

- (nullable UIView*)viewWithIdSelector:(nonnull NSString*)idSelector {
  if (_templateRender != nil) {
    return [_templateRender viewWithIdSelector:idSelector];
  } else {
    return nil;
  }
}

#pragma mark - Setter & Getter

- (LynxTemplateRender*)templateRender {
  return _templateRender;
}

- (void)setEnableTextNonContiguousLayout:(BOOL)enableTextNonContiguousLayout {
  _enableTextNonContiguousLayout = enableTextNonContiguousLayout;
}

- (BOOL)enableTextNonContiguousLayout {
  return _enableTextNonContiguousLayout || [_templateRender enableTextNonContiguousLayout];
}

- (nullable NSString*)url {
  if (_templateRender != nil) {
    return [_templateRender url];
  } else {
    return nil;
  }
}

- (void)setEnableAsyncDisplay:(BOOL)enableAsyncDisplay {
  RUN_RENDER_SAFELY([_templateRender setEnableAsyncDisplay:enableAsyncDisplay];);
}

- (BOOL)enableAsyncDisplay {
  if (_templateRender != nil) {
    return [_templateRender enableAsyncDisplay];
  } else {
    return NO;
  }
}

- (LynxViewSizeMode)layoutWidthMode {
  if (_templateRender != nil) {
    return [_templateRender layoutWidthMode];
  } else {
    return LynxViewSizeModeExact;
  }
}

- (void)setLayoutWidthMode:(LynxViewSizeMode)layoutWidthMode {
  RUN_RENDER_SAFELY(_templateRender.layoutWidthMode = layoutWidthMode;);
}

// get and set of layout property
- (LynxViewSizeMode)layoutHeightMode {
  if (_templateRender != nil) {
    return [_templateRender layoutHeightMode];
  } else {
    return LynxViewSizeModeExact;
  }
}

- (void)setLayoutHeightMode:(LynxViewSizeMode)layoutHeightMode {
  RUN_RENDER_SAFELY(_templateRender.layoutHeightMode = layoutHeightMode;);
}

// return "0" as default value
- (CGFloat)preferredMaxLayoutWidth {
  if (_templateRender != nil) {
    return [_templateRender preferredMaxLayoutWidth];
  } else {
    return 0;
  }
}

- (void)setPreferredMaxLayoutWidth:(CGFloat)preferredMaxLayoutWidth {
  RUN_RENDER_SAFELY(_templateRender.preferredMaxLayoutWidth = preferredMaxLayoutWidth;);
}

- (CGFloat)preferredMaxLayoutHeight {
  if (_templateRender != nil) {
    return [_templateRender preferredMaxLayoutHeight];
  } else {
    return 0;
  }
}

- (void)setPreferredMaxLayoutHeight:(CGFloat)preferredMaxLayoutHeight {
  RUN_RENDER_SAFELY(_templateRender.preferredMaxLayoutHeight = preferredMaxLayoutHeight;);
}

- (CGFloat)preferredLayoutWidth {
  if (_templateRender != nil) {
    return [_templateRender preferredLayoutWidth];
  } else {
    return 0;
  }
}

- (void)setPreferredLayoutWidth:(CGFloat)preferredLayoutWidth {
  RUN_RENDER_SAFELY(_templateRender.preferredLayoutWidth = preferredLayoutWidth;);
}

- (CGFloat)preferredLayoutHeight {
  if (_templateRender != nil) {
    return [_templateRender preferredLayoutHeight];
  } else {
    return 0;
  }
}

- (void)setPreferredLayoutHeight:(CGFloat)preferredLayoutHeight {
  RUN_RENDER_SAFELY(_templateRender.preferredLayoutHeight = preferredLayoutHeight;);
}

- (void)setEnableLayoutOnly:(BOOL)enableLayoutOnly {
  _enableLayoutOnly = enableLayoutOnly;
}

- (void)setEnableSyncFlush:(BOOL)enableSyncFlush {
  _enableSyncFlush = enableSyncFlush;
}

- (void)setScrollListener:(id<LynxScrollListener>)scrollListener {
  _scrollListener = scrollListener;
  RUN_RENDER_SAFELY([_templateRender setScrollListener:scrollListener];);
}

- (void)setCustomizedListLayout:(id<LynxListLayoutProtocol>)customizedListLayout {
  _customizedListLayout = customizedListLayout;
  RUN_RENDER_SAFELY([_templateRender setCustomizedLayoutInUIContext:customizedListLayout];);
}

- (CGSize)intrinsicContentSize {
  return _intrinsicContentSize;
}

- (LynxLifecycleDispatcher*)getLifecycleDispatcher {
  return _lifecycleDispatcher;
};

- (void)setImageFetcher:(id<LynxImageFetcher>)imageFetcher {
  _imageFetcher = imageFetcher;
  RUN_RENDER_SAFELY([_templateRender setImageFetcherInUIOwner:imageFetcher];);
}

- (nullable id<LynxResourceFetcher>)resourceFetcher {
  if (_templateRender != nil) {
    return _templateRender.resourceFetcher;
  }
  return nil;
}

- (void)setResourceFetcher:(nullable id<LynxResourceFetcher>)resourceFetcher {
  RUN_RENDER_SAFELY(_templateRender.resourceFetcher = resourceFetcher;);
}

- (void)setIntrinsicContentSize:(CGSize)size {
  _intrinsicContentSize = size;

  _dispatchingIntrinsicContentSizeChange = YES;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::DidChangeIntrinsicContentSize");
  [_lifecycleDispatcher lynxViewDidChangeIntrinsicContentSize:self];
  _dispatchingIntrinsicContentSizeChange = NO;
}

- (id<LynxBaseInspectorOwner>)baseInspectorOwner {
  if (_templateRender && _templateRender.devTool) {
    return _templateRender.devTool.owner;
  }
  return nil;
}

#pragma mark - Event

- (void)onEnterForeground {
  _attached = YES;
  RUN_RENDER_SAFELY([_templateRender onEnterForeground];);
}

- (void)onEnterBackground {
  _attached = NO;
  RUN_RENDER_SAFELY([_templateRender onEnterBackground];);
}

- (void)onLongPress {
  RUN_RENDER_SAFELY([_templateRender onLongPress];);
}

- (void)sendGlobalEvent:(nonnull NSString*)name withParams:(nullable NSArray*)params {
  if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
    [self showMessageOnConsole:[NSString
                                   stringWithFormat:@"LynxView: send global event %@ for lynx %ld",
                                                    name, [self hash]]
                     withLevel:LynxLogBoxLevelInfo];
  }
  if ([_templateRender enableAirStrictMode]) {
    // In Air mode, send global event by triggerEventBus
    [self triggerEventBus:name withParams:params];
  } else {
    RUN_RENDER_SAFELY([_templateRender sendGlobalEvent:name withParams:params];);
  }
}

- (void)sendGlobalEventToLepus:(nonnull NSString*)name withParams:(nullable NSArray*)params {
  if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
    [self showMessageOnConsole:
              [NSString stringWithFormat:@"LynxView: send global event %@ to lepus for lynx %ld",
                                         name, [self hash]]
                     withLevel:LynxLogBoxLevelInfo];
  }
  RUN_RENDER_SAFELY([_templateRender sendGlobalEventToLepus:name withParams:params];);
}

- (void)triggerEventBus:(nonnull NSString*)name withParams:(nullable NSArray*)params {
  if (name.length) {
    RUN_RENDER_SAFELY([_templateRender triggerEventBus:name withParams:params];);
  }
}

// Since DevToolPlatformDarwinDelegate.mm of LynxDevtool cannot access _templateRenderL and
// LynxEnginProxy, the new method sendTouchEvent is added to LynxView.mm. This method is used to
// call back the sendSyncTouchEvent method after calling emulatetouch.
- (void)sendTouchEvent:(nullable LynxTouchEvent*)event {
  if (_templateRender != nil) {
    [[_templateRender getEngineProxy] sendSyncTouchEvent:event];
  }
}

- (void)dispatchError:(LynxError*)error {
  if (error.code == EBLynxAppBundleLoad) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [_lifecycleDispatcher lynxView:self didLoadFailedWithUrl:self.url error:error];
#pragma clang diagnostic pop
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::didRecieveError");
  [_lifecycleDispatcher lynxView:self didRecieveError:error];
}

- (void)notifyDidChangeIntrinsicContentSize {
  [_lifecycleDispatcher lynxViewDidChangeIntrinsicContentSize:self];
}

#pragma mark - Get Info

- (NSDictionary*)getPageDataByKey:(NSArray*)keys {
  if ([keys count] == 0) {
    _LogI(@"getPageDataByKey called with empty keys.");
    return nil;
  }

  if (_templateRender != nil) {
    return [_templateRender getPageDataByKey:keys];
  } else {
    return nil;
  }
}

- (nullable JSModule*)getJSModule:(nonnull NSString*)name {
  if (_templateRender != nil) {
    return [_templateRender getJSModule:name];
  } else {
    return nil;
  }
}

- (LynxThreadStrategyForRender)getThreadStrategyForRender {
  return [_templateRender getThreadStrategyForRender];
}

- (LynxContext*)getLynxContext {
  return [_templateRender getLynxContext];
}

- (BOOL)isLayoutFinish {
  return [_templateRender isLayoutFinish];
}

- (NSDictionary*)getCurrentData {
  if (_templateRender != nil) {
    return [_templateRender getCurrentData];
  } else {
    return nil;
  }
}

- (float)rootWidth {
  if (_templateRender != nil) {
    return [_templateRender rootWidth];
  } else {
    return 0;
  }
}

- (float)rootHeight {
  if (_templateRender != nil) {
    return [_templateRender rootHeight];
  } else {
    return 0;
  }
}

- (NSDictionary*)getAllJsSource {
  if (_templateRender != nil) {
    return [_templateRender getAllJsSource];
  } else {
    return nil;
  }
}

// Used by LynxUIContext
- (nullable NSNumber*)getLynxRuntimeId {
  if (_templateRender != nil) {
    return [_templateRender getLynxRuntimeId];
  }
  return nil;
}

#pragma mark - Timing

- (void)setExtraTiming:(LynxExtraTiming*)timing {
  [_templateRender setExtraTiming:timing];
}

- (NSDictionary*)getAllTimingInfo {
  return [_templateRender getAllTimingInfo];
}

- (void)setExtraTimingWithDictionary:(NSDictionary*)timing {
  LynxExtraTiming* timingInfo = [[LynxExtraTiming alloc] init];
  timingInfo.openTime = [[timing objectForKey:@"open_time"] unsignedLongLongValue];
  timingInfo.containerInitStart =
      [[timing objectForKey:@"container_init_start"] unsignedLongLongValue];
  timingInfo.containerInitEnd = [[timing objectForKey:@"container_init_end"] unsignedLongLongValue];
  timingInfo.prepareTemplateStart =
      [[timing objectForKey:@"prepare_template_start"] unsignedLongLongValue];
  timingInfo.prepareTemplateEnd =
      [[timing objectForKey:@"prepare_template_end"] unsignedLongLongValue];
  [_templateRender setExtraTiming:timingInfo];
}

- (void)putParamsForReportingEvents:(NSDictionary<NSString*, id>*)params {
  [_templateRender putExtraParamsForReportingEvents:params];
}

#pragma mark - Multi Thread

- (void)runOnTasmThread:(dispatch_block_t)task {
  [_templateRender runOnTasmThread:task];
}

- (void)syncFlush {
  [_templateRender syncFlush];
}

- (void)attachEngineToUIThread {
  [_templateRender attachEngineToUIThread];
}

- (void)detachEngineFromUIThread {
  [_templateRender detachEngineFromUIThread];
}

#pragma mark - Component

- (void)preloadDynamicComponents:(nonnull NSArray*)urls {
  _LogI(@"LynxView %p: preload lazy bundles: %@", self, [urls componentsJoinedByString:@", "]);
  if ([urls count] == 0) {
    return;
  }
  RUN_RENDER_SAFELY([_templateRender preloadLazyBundles:urls];);
}

- (BOOL)registerDynamicComponent:(nonnull NSString*)url bundle:(nonnull LynxTemplateBundle*)bundle {
  _LogI(@"LynxView %p: registerLazyBundle: %@", self, url);
  RUN_RENDER_SAFELY(return [_templateRender registerLazyBundle:url bundle:bundle];);
  return NO;
}

#pragma mark - Bytecode

- (void)setEnableUserBytecode:(BOOL)enableUserBytecode url:(nonnull NSString*)url {
  RUN_RENDER_SAFELY([_templateRender setEnableUserBytecode:enableUserBytecode url:url];);
}

#pragma mark - Internal Extension

- (void)setUpModuleGlobalProps {
  [LynxService(LynxServiceModuleProtocol) initGlobalProps:self];
}

- (void)clearModuleForDestroy {
  [LynxService(LynxServiceModuleProtocol) clearModuleForDestroy:self];
}

- (BOOL)tapOnUICalloutBarButton:(CGPoint)point withEvent:(UIEvent*)event {
  return NO;
}

#pragma mark - LUIBodyView

- (void)didReceiveResourceError:(NSError*)error {
  [[self getLifecycleDispatcher] lynxView:self didRecieveError:error];
}

- (void)didReceiveResourceError:(LynxError*)error
                     withSource:(NSString*)resourceUrl
                           type:(NSString*)type {
  if (!error) {
    return;
  }
  [error addCustomInfo:type forKey:LynxErrorKeyResourceType];
  [error addCustomInfo:resourceUrl forKey:LynxErrorKeyResourceUrl];
  [error setTemplateUrl:[self url]];
  [[self getLifecycleDispatcher] lynxView:self didRecieveError:error];
}

- (void)reportError:(NSError*)error {
  [_templateRender onErrorOccurred:error.code sourceError:error];
}

- (void)reportLynxError:(LynxError*)error {
  [_templateRender onErrorOccurred:error];
}

- (int32_t)instanceId {
  return _templateRender.instanceId;
}

#pragma mark - Preload

- (void)attachTemplateRender:(LynxTemplateRender* _Nullable)templateRender {
  if (_templateRender) {
    _LogW(@"LynxView %p:LynxTemplateRender is already attached", self);
    return;
  }
  _templateRender = templateRender;
  [_templateRender attachLynxView:self];
};

- (void)processRender {
  if (![LynxThreadManager isMainQueue]) {
    __weak LynxView* weakSelf = self;
    [LynxThreadManager runBlockInMainQueue:^{
      [weakSelf processRender];
    }];
    return;
  }

  BOOL isAttachSuccess = [_templateRender processRender:self];
  if (!isAttachSuccess) {
    _LogW(@"LynxView processRender error. url:%@", _templateRender.url);
    return;
  }
  [self updateViewport];
}

- (void)detachRender {
  _templateRender = nil;
}

- (void)startLynxRuntime {
  [_templateRender startLynxRuntime];
}

- (void)updateViewport {
  RUN_RENDER_SAFELY([_templateRender updateViewport];);
}

#pragma mark - LynxTemplateRenderDelegate

- (void)templateRenderOnDataUpdated:(LynxTemplateRender*)templateRender {
  _LogI(@"Lynxview Lifecycle OnDataUpdated in %p", self);
  [_lifecycleDispatcher lynxViewDidUpdate:self];
}

- (void)templateRender:(LynxTemplateRender*)templateRender onPageChanged:(BOOL)isFirstScreen {
  _LogI(@"Lynxview Lifecycle onPageChanged in %p", self);
  __weak typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf) {
      if (isFirstScreen) {
        TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::lynxViewDidFirstScreen");
        [strongSelf->_lifecycleDispatcher lynxViewDidFirstScreen:strongSelf];
      } else {
        TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::lynxViewDidPageUpdate");
        [strongSelf->_lifecycleDispatcher lynxViewDidPageUpdate:strongSelf];
      }
    }
  });
}

- (void)templateRenderOnTasmFinishByNative:(LynxTemplateRender*)templateRender {
  [_lifecycleDispatcher lynxViewOnTasmFinishByNative:self];
}

- (void)templateRender:(LynxTemplateRender*)templateRender onTemplateLoaded:(NSString*)url {
  [[LynxHeroTransition sharedInstance] executeEnterTransition:self];
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::didLoadFinishedWithUrl", "url",
              [url UTF8String]);
  [_lifecycleDispatcher lynxView:self didLoadFinishedWithUrl:url];
}

- (void)templateRenderOnRuntimeReady:(LynxTemplateRender*)templateRender {
  [_lifecycleDispatcher lynxViewDidConstructJSRuntime:self];
}

- (void)templateRender:(LynxTemplateRender*)templateRender
    onReceiveFirstLoadPerf:(LynxPerformance*)perf {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::didReceiveFirstLoadPerf");
  [_lifecycleDispatcher lynxView:self didReceiveFirstLoadPerf:perf];
}

- (void)templateRender:(LynxTemplateRender*)templateRender onUpdatePerf:(LynxPerformance*)perf {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::didReceiveUpdatePerf");
  [_lifecycleDispatcher lynxView:self didReceiveUpdatePerf:perf];
}

- (void)templateRender:(LynxTemplateRender*)templateRender
    onReceiveDynamicComponentPerf:(NSDictionary*)perf {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::didReceiveLazyBundlePerf");
  [_lifecycleDispatcher lynxView:self didReceiveDynamicComponentPerf:perf];
}

- (NSString*)templateRender:(LynxTemplateRender*)templateRender
    translatedResourceWithId:(NSString*)resId
                    themeKey:(NSString*)key {
  if ([self.resourceFetcher
          respondsToSelector:@selector(translatedResourceWithId:theme:themeKey:view:)]) {
    return [self.resourceFetcher translatedResourceWithId:resId
                                                    theme:self.theme
                                                 themeKey:key
                                                     view:self];
  }
  return nil;
}

- (void)templateRender:(LynxTemplateRender*)templateRender
       didInvokeMethod:(NSString*)method
              inModule:(NSString*)module
             errorCode:(int)code {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::didInvokeMethod", "module",
              [module UTF8String], "method", [method UTF8String]);
  [_lifecycleDispatcher lynxView:self didInvokeMethod:method inModule:module errorCode:code];
}

- (void)templateRender:(LynxTemplateRender*)templateRender onErrorOccurred:(LynxError*)error {
  if (error.code == EBLynxAppBundleLoad) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [_lifecycleDispatcher lynxView:self didLoadFailedWithUrl:self.url error:error];
#pragma clang diagnostic pop
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::didRecieveError");
  [_lifecycleDispatcher lynxView:self didRecieveError:error];
}

- (void)templateRenderOnResetViewAndLayer:(LynxTemplateRender*)templateRender {
  [[self subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
  [[self.layer sublayers] makeObjectsPerformSelector:@selector(removeFromSuperlayer)];
}

- (void)templateRenderOnTemplateStartLoading:(LynxTemplateRender*)templateRender {
  LynxPipelineInfo* pipelineInfo = [[LynxPipelineInfo alloc] initWithUrl:[self url]];
  [pipelineInfo addPipelineOrigin:LynxFirstScreen];
  [self templateRenderOnPageStarted:templateRender withPipelineInfo:pipelineInfo];
}

- (void)templateRenderOnPageStarted:(LynxTemplateRender*)templateRender
                   withPipelineInfo:(LynxPipelineInfo*)pipelineInfo {
  _LogI(@"LynxView %p: OnPageStart %@, stage: %ld ", self, templateRender.url ?: @"",
        (long)[pipelineInfo pipelineOrigin]);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::onPageStartedWithLynxView");
  if ([pipelineInfo pipelineOrigin] & LynxFirstScreen) {
    [_lifecycleDispatcher lynxViewDidStartLoading:self];
  }
  [_lifecycleDispatcher onPageStartedWithLynxView:self withPipelineInfo:pipelineInfo];
}

- (void)templateRenderOnFirstScreen:(LynxTemplateRender*)templateRender {
  __weak typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::lynxViewDidFirstScreen");
      [strongSelf->_lifecycleDispatcher lynxViewDidFirstScreen:strongSelf];
    }
  });
}

- (void)templateRenderOnPageUpdate:(LynxTemplateRender*)templateRender {
  __weak typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifecycle::lynxViewDidPageUpdate");
      [strongSelf->_lifecycleDispatcher lynxViewDidPageUpdate:strongSelf];
    }
  });
}

- (void)templateRenderOnDetach:(LynxTemplateRender*)templateRender {
  [self detachRender];
}

- (void)templateRender:(LynxTemplateRender*)templateRender onCallJSBFinished:(NSDictionary*)info {
  [_lifecycleDispatcher lynxView:self onCallJSBFinished:info];
}

- (void)templateRender:(LynxTemplateRender*)templateRender onJSBInvoked:(NSDictionary*)info {
  [_lifecycleDispatcher lynxView:self onJSBInvoked:info];
}

- (void)templateRenderSetLayoutOption:(LynxTemplateRender*)templateRender {
  [self setEnableTextNonContiguousLayout:_enableTextNonContiguousLayout];
  [self setEnableLayoutOnly:_enableLayoutOnly];
}

- (void)templateRenderRequestNeedsLayout:(LynxTemplateRender*)templateRender {
  [self setNeedsLayout];
}

- (void)templateRenderOnTransitionUnregister:(LynxTemplateRender*)templateRender {
  [[LynxHeroTransition sharedInstance] unregisterLynxView:self];
}

- (void)templateRender:(LynxTemplateRender*)templateRender onLynxEvent:(LynxEventDetail*)event {
  [_lifecycleDispatcher onLynxEvent:event];
}

- (void)templateRender:(LynxTemplateRender*)templateRender
    onTemplateBundleReady:(LynxTemplateBundle*)bundle {
  [_lifecycleDispatcher onTemplateBundleReady:bundle];
}

#pragma mark - Deprecated

- (id<LynxViewClient>)client {
  if (_clientWeakProxy) {
    return _clientWeakProxy.target;
  }
  return nil;
}

- (void)setClient:(id<LynxViewClient>)client {
  LynxWeakProxy* clientWeakProxy = [LynxWeakProxy proxyWithTarget:client];

  if ([client conformsToProtocol:@protocol(LynxViewClient)] ||
      [client conformsToProtocol:@protocol(LynxViewLifecycle)]) {
    if (_clientWeakProxy) {
      [_lifecycleDispatcher removeLifecycleClient:(id<LynxViewLifecycle>)_clientWeakProxy];
    }
    [_lifecycleDispatcher addLifecycleClient:(id<LynxViewLifecycle>)clientWeakProxy];
  }

  _clientWeakProxy = clientWeakProxy;

  self.imageFetcher = _clientWeakProxy.target;
  self.resourceFetcher = _clientWeakProxy.target;
  self.scrollListener = _clientWeakProxy.target;
}

- (void)invalidateIntrinsicContentSize {
  [self triggerLayout];
  [super invalidateIntrinsicContentSize];
}

- (void)setEnableRadonCompatible:(BOOL)enableRadonCompatible
    __attribute__((deprecated("Radon diff mode can't be close after lynx 2.3."))) {
}

- (void)requestLayoutWhenSafepointEnable {
}

- (void)setGlobalPropsWithDictionary:(NSDictionary<NSString*, id>*)data {
  RUN_RENDER_SAFELY([_templateRender updateGlobalPropsWithDictionary:data];);
}

- (void)setGlobalPropsWithTemplateData:(LynxTemplateData*)data {
  RUN_RENDER_SAFELY([_templateRender updateGlobalPropsWithTemplateData:data];);
}

- (nullable UIView*)viewWithName:(nonnull NSString*)name {
  if (_templateRender != nil) {
    return [_templateRender viewWithName:name];
  } else {
    return nil;
  }
}

- (NSString*)cardVersion {
  if (_templateRender != nil) {
    return [_templateRender cardVersion];
  } else {
    return nil;
  }
}

- (nonnull LynxConfigInfo*)lynxConfigInfo {
  return [[LynxConfigInfo alloc] init];
}

- (LynxPerformance*)forceGetPerf {
  return nil;
}

- (void)processLayout:(NSData*)tem withURL:(NSString*)url initData:(LynxTemplateData*)data {
  _LogI(@"LynxView %p: start processLayout with %@", self, url);
  [LynxService(LynxServiceModuleProtocol) initGlobalProps:self];
  RUN_RENDER_SAFELY([_templateRender processLayout:tem withURL:url initData:data];);
}

- (void)processLayoutWithTemplateBundle:(LynxTemplateBundle*)bundle
                                withURL:(NSString*)url
                               initData:(LynxTemplateData*)data {
  _LogI(@"LynxView %p: start processLayoutWithTemplateBundle with %@", self, url);
  RUN_RENDER_SAFELY([_templateRender processLayoutWithTemplateBundle:bundle
                                                             withURL:url
                                                            initData:data];);
}

- (void)processLayoutWithSSRData:(nonnull NSData*)tem
                         withURL:(nonnull NSString*)url
                        initData:(nullable LynxTemplateData*)data {
  _LogI(@"LynxView %p: start processLayoutWithSSRData with %@", self, url);
  RUN_RENDER_SAFELY([_templateRender processLayoutWithSSRData:tem withURL:url initData:data];);
}

/**
 * Used to output logs to the console of DevTool. This function is effective only when DevTool is
 * connected.
 * @param msg Message wanted.
 * @param level The log level.
 */
- (void)showMessageOnConsole:(NSString*)msg withLevel:(int32_t)level {
  id<LynxBaseInspectorOwner> inspectorOwner = [self baseInspectorOwner];
  if (!inspectorOwner) {
    return;
  }
  [inspectorOwner showMessageOnConsole:msg withLevel:level];
}

@end
