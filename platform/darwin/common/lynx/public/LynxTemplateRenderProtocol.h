// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import "JSModule.h"
#import "LynxLoadMeta.h"
#import "LynxTheme.h"
#import "LynxViewEnum.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxView;
@class LynxViewBuilder;
@class LynxExtraTiming;
@class LynxTemplateBundle;
@class LynxTemplateData;
@class LynxUpdateMeta;
@class LynxContext;
@protocol LynxModule;

@protocol LynxTemplateRenderProtocol <NSObject>

@required
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
@property(nonatomic, strong, nullable) NSMutableDictionary<NSString*, id>* lepusModulesClasses;

#pragma mark - Init

- (nonnull instancetype)initWithBuilderBlock:
                            (void (^_Nullable)(NS_NOESCAPE LynxViewBuilder* _Nonnull))block
                                    lynxView:(LynxView* _Nullable)lynxView;

- (void)loadTemplateFromURL:(NSString* _Nonnull)url initData:(LynxTemplateData* _Nullable)data;

#pragma mark - Clean & Reuse

- (void)reset;

- (void)clearForDestroy;

#pragma mark - Template data

/**
 * Load template data
 */
- (void)loadTemplate:(nonnull LynxLoadMeta*)meta;

- (void)loadTemplate:(nonnull NSData*)tem
             withURL:(nonnull NSString*)url
            initData:(nullable LynxTemplateData*)data;

- (void)loadTemplateBundle:(nonnull LynxTemplateBundle*)bundle
                   withURL:(nonnull NSString*)url
                  initData:(nullable LynxTemplateData*)data;

- (void)loadTemplateWithoutLynxView:(NSData* _Nonnull)tem
                            withURL:(NSString* _Nonnull)url
                           initData:(LynxTemplateData* _Nullable)data;

/**
 * Update template data
 */
- (void)updateMetaData:(nonnull LynxUpdateMeta*)meta;

- (void)updateDataWithString:(nullable NSString*)data processorName:(nullable NSString*)name;

- (void)updateDataWithDictionary:(nullable NSDictionary<NSString*, id>*)data
                   processorName:(nullable NSString*)name;

- (void)updateDataWithTemplateData:(nullable LynxTemplateData*)data;

/**
 * Reset template data
 */
- (void)resetDataWithTemplateData:(nullable LynxTemplateData*)data;

/**
 * Reload template data and global props
 */
- (void)reloadTemplateWithTemplateData:(nullable LynxTemplateData*)data
                           globalProps:(nullable LynxTemplateData*)globalProps;

#pragma mark - SSR

- (void)loadSSRData:(nonnull NSData*)tem
            withURL:(nonnull NSString*)url
           initData:(nullable LynxTemplateData*)data;

- (void)loadSSRDataFromURL:(nonnull NSString*)url initData:(nullable LynxTemplateData*)data;

- (void)ssrHydrate:(nonnull NSData*)tem
           withURL:(nonnull NSString*)url
          initData:(nullable LynxTemplateData*)data;

- (void)ssrHydrateFromURL:(nonnull NSString*)url initData:(nullable LynxTemplateData*)data;

#pragma mark - Storage

- (void)setSessionStorageItem:(nonnull NSString*)key
             WithTemplateData:(nullable LynxTemplateData*)data;

- (void)getSessionStorageItem:(nonnull NSString*)key
                 withCallback:(void (^_Nonnull)(id<NSObject> _Nullable))callback;

- (double)subscribeSessionStorage:(nonnull NSString*)key
                     withCallback:(void (^_Nonnull)(id<NSObject> _Nullable))callback;

- (void)unSubscribeSessionStorage:(nonnull NSString*)key withId:(double)callbackId;

#pragma mark - Global Props

- (void)updateGlobalPropsWithDictionary:(nullable NSDictionary<NSString*, id>*)data;

- (void)updateGlobalPropsWithTemplateData:(nullable LynxTemplateData*)data;

#pragma mark - Event

- (void)sendGlobalEvent:(nonnull NSString*)name withParams:(nullable NSArray*)params;

- (void)sendGlobalEventToLepus:(nonnull NSString*)name withParams:(nullable NSArray*)params;

- (void)triggerEventBus:(nonnull NSString*)name withParams:(nullable NSArray*)params;

- (void)onEnterForeground;
- (void)onEnterBackground;

- (void)onLongPress;

#pragma mark - View

- (void)triggerLayout;
- (void)triggerLayoutInTick;

- (void)updateViewport;
- (void)updateViewport:(BOOL)needLayout;

/**
 * EXPERIMENTAL API!
 * Updating the screen size for lynxview.
 * Updating the screen size does not trigger a re-layout, You should trigger  a re-layout by
 * yourself. It will be useful for the screen size change, like screen rotation. it can make some
 * css properties based on rpx shows better. Multiple views are not supported with different
 * settings!
 * @param width (dp) screen width
 * @param height (dp) screen screen(dp)
 */
- (void)updateScreenMetricsWithWidth:(CGFloat)width height:(CGFloat)height;

- (void)updateFontScale:(CGFloat)scale;

- (void)pauseRootLayoutAnimation;
- (void)resumeRootLayoutAnimation;

- (void)restartAnimation;
- (void)resetAnimation;

- (void)setTheme:(LynxTheme* _Nullable)theme;
- (void)setLocalTheme:(LynxTheme* _Nonnull)theme;
- (nullable LynxTheme*)theme;

#pragma mark - Module

- (void)registerModule:(Class<LynxModule> _Nonnull)module param:(id _Nullable)param;

- (BOOL)isModuleExist:(NSString* _Nonnull)moduleName;

- (nullable JSModule*)getJSModule:(nonnull NSString*)name;

#pragma mark - Setter & Getter

- (void)setEnableAsyncDisplay:(BOOL)enableAsyncDisplay;
- (BOOL)enableAsyncDisplay;

- (BOOL)enableTextNonContiguousLayout;

- (nonnull LynxContext*)getLynxContext;

- (LynxThreadStrategyForRender)getThreadStrategyForRender;

#pragma mark - Get Info

- (nonnull NSDictionary*)getCurrentData;

- (nonnull NSDictionary*)getPageDataByKey:(nonnull NSArray*)keys;

- (NSString* _Nonnull)cardVersion;

- (nonnull NSDictionary*)getAllJsSource;

- (nullable NSNumber*)getLynxRuntimeId;

#pragma mark - Handle error

- (void)onErrorOccurred:(NSInteger)errCode message:(NSString* _Nonnull)errMessage;
- (void)onErrorOccurred:(NSInteger)errCode sourceError:(NSError* _Nonnull)source;

#pragma mark - Perf

- (void)setExtraTiming:(LynxExtraTiming* _Nonnull)timing;

/// Add extra parameters for reporting events, overriding old values if the parameters already
/// exist.
/// - Parameter params: common parameters for report events.
- (void)putExtraParamsForReportingEvents:(NSDictionary<NSString*, id>* _Nonnull)params;

- (nullable NSDictionary*)getAllTimingInfo;

- (nullable NSDictionary*)getExtraInfo;

#pragma mark - Intersection

- (void)notifyIntersectionObservers;

#pragma mark - Runtime

- (void)startLynxRuntime;

@end

NS_ASSUME_NONNULL_END
