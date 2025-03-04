// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxForegroundProtocol.h"
#import "LynxModule.h"
#import "LynxUI.h"
#import "LynxUIContext.h"
#import "LynxUIMethodProcessor.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxView;
@class LynxRootUI;
@class LynxEventHandler;
@class LynxComponentScopeRegistry;
@class LynxWeakProxy;
@class LynxGestureDetectorDarwin;
@class LynxTemplateRender;
@class LynxGestureArenaManager;

@protocol LynxBaseInspectorOwner;
@protocol LynxForegroundProtocol;
@protocol LUIErrorHandling;
@protocol LUIBodyView;

@interface LynxUIContext (Internal)
@property(nonatomic, weak, nullable, readwrite) id<LynxImageFetcher> imageFetcher;
@property(nonatomic, weak, nullable, readwrite) LynxEventHandler* eventHandler;
@property(nonatomic, weak, nullable, readwrite) LynxEventEmitter* eventEmitter;
@property(nonatomic, weak, nullable, readonly) UIView* rootView;
@property(nonatomic, strong, nullable, readwrite) NSDictionary* keyframesDict;
- (void)mergeKeyframesWithLynxKeyframes:(LynxKeyframes*)keyframes forKey:(NSString*)name;
@end

@interface LynxUIOwner : NSObject

@property(nonatomic, readonly) LynxUIContext* uiContext;
@property(nonatomic, readonly) LynxRootUI* rootUI;
@property(nonatomic, readonly) LynxFontFaceContext* fontFaceContext;
@property(nonatomic, readonly) id<LynxBaseInspectorOwner> baseInspectOwner;
@property(nonatomic, strong) LynxGestureArenaManager* _Nullable gestureArenaManager;
@property(nonatomic, weak, readonly) LynxTemplateRender* templateRender;

- (void)attachLynxView:(LynxView* _Nonnull)containerView;
- (instancetype)initWithContainerView:(LynxView*)containerView
                       templateRender:(LynxTemplateRender*)templateRender
                    componentRegistry:(LynxComponentScopeRegistry*)registry
                        screenMetrics:(LynxScreenMetrics*)screenMetrics;

- (instancetype)initWithContainerView:(UIView<LUIBodyView>*)containerView
                       templateRender:(nullable LynxTemplateRender*)templateRender
                    componentRegistry:(LynxComponentScopeRegistry*)registry
                        screenMetrics:(LynxScreenMetrics*)screenMetrics
                         errorHandler:(id<LUIErrorHandling> _Nullable)errorHandler;

- (LynxUI*)findUIBySign:(NSInteger)sign;
- (LynxUI*)findUIByComponentId:(NSString*)componentId;

- (LynxUI*)findUIByIdSelector:(NSString*)idSelector withinUI:(LynxUI*)ui;
- (LynxUI*)findUIByIdSelectorInParent:(NSString*)idSelector child:(LynxUI*)child;

- (LynxUI*)findUIByRefId:(NSString*)refId withinUI:(LynxUI*)ui;

- (NSSet<NSString*>*)componentSet;
- (void)componentStatistic:(NSString*)componentName;

- (void)createUIWithSign:(NSInteger)sign
                 tagName:(nullable NSString*)tagName
                eventSet:(nullable NSSet<NSString*>*)eventSet
           lepusEventSet:(nullable NSSet<NSString*>*)lepusEventSet
                   props:(nullable NSDictionary*)props
               nodeIndex:(uint32_t)nodeIndex
      gestureDetectorSet:(nullable NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet;

- (void)updateUIWithSign:(NSInteger)sign
                   props:(NSDictionary*)props
                eventSet:(NSSet<NSString*>*)eventSet
           lepusEventSet:(nullable NSSet<NSString*>*)lepusEventSet
      gestureDetectorSet:(nullable NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet;

- (void)insertNode:(NSInteger)childSign toParent:(NSInteger)parentSign atIndex:(NSInteger)index;

- (void)listWillReuseNode:(NSInteger)sign withItemKey:(NSString*)itemKey;
- (void)listCellWillAppear:(NSInteger)sign withItemKey:(NSString*)itemKey;
- (void)ListCellDisappear:(NSInteger)sign exist:(BOOL)isExist withItemKey:(NSString*)itemKey;

- (void)detachNode:(NSInteger)sign;

- (void)recycleNode:(NSInteger)sign;

- (void)updateUI:(NSInteger)sign
      layoutLeft:(CGFloat)left
             top:(CGFloat)top
           width:(CGFloat)width
          height:(CGFloat)height
         padding:(UIEdgeInsets)padding
          border:(UIEdgeInsets)border
          margin:(UIEdgeInsets)margin
          sticky:(nullable NSArray*)sticky;

- (void)updateUI:(NSInteger)sign
      layoutLeft:(CGFloat)left
             top:(CGFloat)top
           width:(CGFloat)width
          height:(CGFloat)height
         padding:(UIEdgeInsets)padding
          border:(UIEdgeInsets)border;

- (void)invokeUIMethod:(NSString*)method
                params:(NSDictionary*)params
              callback:(LynxUIMethodCallbackBlock)callback
              fromRoot:(NSString*)componentId
               toNodes:(NSArray*)nodes;

- (void)invokeUIMethodForSelectorQuery:(NSString*)method
                                params:(NSDictionary*)params
                              callback:(LynxUIMethodCallbackBlock)callback
                                toNode:(int)sign;

- (void)willContainerViewMoveToWindow:(UIWindow*)window;
- (void)onReceiveUIOperation:(id)value onUI:(NSInteger)sign;

// layoutDidFinish is called only LayoutRecursively is actually executed
// finishLayoutOperation on the other hand, is always being called, and it is called before
// layoutDidFinish
// TODO: merge layoutDidFinished to finishLayoutOperation in layout_context.cc
- (void)layoutDidFinish;
- (void)finishLayoutOperation:(int64_t)operationID componentID:(NSInteger)componentID;
- (void)onNodeReady:(NSInteger)sign;
- (void)onNodeRemoved:(NSInteger)sign;
- (void)onNodeReload:(NSInteger)sign;

- (nullable LynxUI*)uiWithName:(NSString*)name;
- (nullable LynxUI*)uiWithIdSelector:(NSString*)idSelector;
- (nullable LynxWeakProxy*)weakLynxUIWithName:(NSString*)name;

- (void)reset;

- (void)pauseRootLayoutAnimation;
- (void)resumeRootLayoutAnimation;
/**
 * Called during cell reuse, in prepareForReuse, and on successful reuse, call restart.
 */
- (void)resetAnimation;
- (void)restartAnimation;

// When LynxView enter foreground, call this function to resume keyframe animation.
- (void)resumeAnimation;

// When LynxView enter foreground
- (void)onEnterForeground;

// When LynxView enter background
- (void)onEnterBackground;

- (void)registerForegroundListener:(id<LynxForegroundProtocol>)listener;

- (void)unRegisterForegroundListener:(id<LynxForegroundProtocol>)listener;

- (void)updateFontFaceWithDictionary:(NSDictionary*)dic;

- (LynxComponentScopeRegistry*)getComponentRegistry;

- (void)didMoveToWindow:(BOOL)windowIsNil;

- (void)updateAnimationKeyframes:(NSDictionary*)keyframesDict;

#pragma mark - A11y
- (NSArray<LynxUI*>*)uiWithA11yID:(NSString*)a11yID;
- (NSArray<UIView*>*)viewsWithA11yID:(NSString*)a11yID;

- (NSInteger)getTagInfo:(NSString*)tagName;

- (void)updateScrollInfo:(NSInteger)containerID
         estimatedOffset:(float)estimatedOffset
                  smooth:(bool)smooth
               scrolling:(bool)scrolling;

- (void)insertListComponent:(NSInteger)listSign componentSign:(NSInteger)componentSign;

- (void)removeListComponent:(NSInteger)listSign componentSign:(NSInteger)componentSign;

- (void)updateContentOffsetForListContainer:(NSInteger)containerID
                                contentSize:(float)contentSize
                                     deltaX:(float)deltaX
                                     deltaY:(float)deltaY;

- (void)initNewGestureInUIThread:(BOOL)enableNewGesture;

- (NSInteger)getRootSign;
@end

NS_ASSUME_NONNULL_END
