// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxEventTarget.h"
#import "LynxExtensionModule.h"
#import "LynxImageFetcher.h"
#import "LynxResourceFetcher.h"
#import "LynxScrollListener.h"
#import "LynxTemplateRenderDelegateExternal.h"
#import "LynxTemplateRenderProtocol.h"
#import "LynxUIListProtocol.h"
#import "LynxViewEnum.h"

@class LynxDevtool;
@class LynxTemplateBundle;
@class LynxUIIntersectionObserverManager;
@class LynxUI;

@interface LynxTemplateRender : NSObject <LynxTemplateRenderProtocol>

// Layout, must call invalidateIntrinsicContentSize after change layout props
// If you use view.frame to set view frame, the layout mode will all be
// specified
@property(nonatomic, assign) LynxViewSizeMode layoutWidthMode;
@property(nonatomic, assign) LynxViewSizeMode layoutHeightMode;
@property(nonatomic, assign) CGFloat preferredMaxLayoutWidth;
@property(nonatomic, assign) CGFloat preferredMaxLayoutHeight;
@property(nonatomic, assign) CGFloat preferredLayoutWidth;
@property(nonatomic, assign) CGFloat preferredLayoutHeight;
@property(nonatomic, assign) CGRect frameOfLynxView;
@property(nonatomic, assign) BOOL isDestroyed;
@property(nonatomic, assign) BOOL hasRendered;
@property(nonatomic, strong, readonly, nullable) NSString* url;
@property(nonatomic, assign) BOOL enableJSRuntime;
@property(nonatomic, strong, nonnull) LynxDevtool* devTool;

@property(nonatomic, assign) BOOL enablePrePainting;
@property(nonatomic, assign) BOOL enableDumpElement;
@property(nonatomic, assign) BOOL enableRecycleTemplateBundle;
@property(nonatomic, strong, nullable) NSMutableDictionary<NSString*, id>* lepusModulesClasses;
@property(nonatomic, weak, nullable) id<LynxResourceFetcher> resourceFetcher;

- (nullable id<LynxEventTarget>)hitTestInEventHandler:(CGPoint)point
                                            withEvent:(UIEvent* _Nonnull)event;
- (void)willMoveToWindow:(nonnull UIWindow*)newWindow;

/**
 Accepts a custom layout from the client to replace the default layoutManager in the list. This can
 only be used with a new list container.
 */
- (void)setCustomizedLayoutInUIContext:(id<LynxListLayoutProtocol> _Nullable)customizedListLayout;
- (void)setScrollListener:(id<LynxScrollListener> _Nullable)scrollListener;
- (void)setImageFetcherInUIOwner:(id<LynxImageFetcher> _Nullable)imageFetcher;
- (void)setResourceFetcherInUIOwner:(id<LynxResourceFetcher> _Nullable)resourceFetcher;
- (void)setNeedPendingUIOperation:(BOOL)needPendingUIOperation;
- (BOOL)enableAirStrictMode;
- (BOOL)enableLayoutOnly;
- (BOOL)enableFiberArch;
- (BOOL)enableBackgroundShapeLayer;
- (BOOL)isLayoutFinish;

- (void)resetLayoutStatus;

- (void)preloadLazyBundles:(NSArray* _Nonnull)urls;
- (BOOL)registerLazyBundle:(nonnull NSString*)url bundle:(nonnull LynxTemplateBundle*)bundle;

- (void)setEnableUserBytecode:(BOOL)enableUserBytecode url:(nonnull NSString*)url;

// Thread
- (void)syncFlush;
- (void)attachEngineToUIThread;
- (void)detachEngineFromUIThread;

- (nullable LynxUI*)findUIBySign:(NSInteger)sign;
- (nullable UIView*)findViewWithName:(nonnull NSString*)name;
- (nullable LynxUI*)uiWithName:(nonnull NSString*)name;
- (nullable LynxUI*)uiWithIdSelector:(nonnull NSString*)idSelector;
- (nullable UIView*)viewWithIdSelector:(nonnull NSString*)idSelector;
// prefer using `findViewWithName:` than `viewWithName:`.
// `viewWithName:` will be marked deprecated in 1.6
- (nullable UIView*)viewWithName:(nonnull NSString*)name;

- (void)setImageDownsampling:(BOOL)enableImageDownsampling;
- (BOOL)enableImageDownsampling;
- (BOOL)enableNewImage;
- (BOOL)trailNewImage;

- (float)rootWidth;
- (float)rootHeight;

#pragma mark - preload

- (void)attachLynxView:(LynxView* _Nonnull)lynxView;
- (BOOL)processRender:(LynxView* _Nonnull)lynxView;

- (void)processLayout:(nonnull NSData*)tem
              withURL:(nonnull NSString*)url
             initData:(nullable LynxTemplateData*)data;

- (void)processLayoutWithTemplateBundle:(nonnull LynxTemplateBundle*)bundle
                                withURL:(nonnull NSString*)url
                               initData:(nullable LynxTemplateData*)data;

- (void)processLayoutWithSSRData:(nonnull NSData*)tem
                         withURL:(nonnull NSString*)url
                        initData:(nullable LynxTemplateData*)data;

// It is only used to accept callbacks that calculate the height when lynxView has not been
// attached. AttachLynxView will override this delegate.
- (void)setTemplateRenderDelegate:(LynxTemplateRenderDelegateExternal* _Nonnull)delegate;

@end
