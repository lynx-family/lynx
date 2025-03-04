// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxUIListProtocol.h>
#import "LynxEventEmitter.h"
#import "LynxGenericResourceFetcher.h"
#import "LynxImageFetcher.h"
#import "LynxLifecycleDispatcher.h"
#import "LynxMediaResourceFetcher.h"
#import "LynxScreenMetrics.h"
#import "LynxScrollListener.h"
#import "LynxTemplateResourceFetcher.h"

NS_ASSUME_NONNULL_BEGIN
@class LynxRootUI;
@class LynxFontFaceContext;

@class LynxEventHandler;
@class LynxLifecycleDispatcher;
@class LynxShadowNodeOwner;
@class LynxUIOwner;
@class LynxUIExposure;
@class LynxUIIntersectionObserverManager;
@class LynxGlobalObserver;
@class LynxContext;
@protocol LUIErrorHandling;

@interface LynxUIContext : NSObject
@property(nonatomic, weak, nullable, readwrite) LynxContext* lynxContext;
@property(nonatomic, weak, nullable, readwrite) id<LynxImageFetcher> imageFetcher;
@property(nonatomic, weak, nullable, readwrite) id<LynxResourceFetcher> resourceFetcher;
@property(nonatomic, strong, nullable, readwrite) id<LynxResourceFetcher> resourceServiceFetcher;
@property(nonatomic, weak, nullable) id<LynxListLayoutProtocol> customizedListLayout;
@property(nonatomic, weak, nullable) id<LUIErrorHandling> errorHandler;
@property(nonatomic, weak, nullable, readwrite) id<LynxScrollListener> scrollListener;
@property(nonatomic, weak, nullable, readonly) LynxEventHandler* eventHandler;
@property(nonatomic, weak, nullable, readonly) LynxEventEmitter* eventEmitter;
@property(nonatomic, weak, nullable, readonly) UIView<LUIBodyView>* rootView;
@property(nonatomic, weak, nullable, readwrite) LynxRootUI* rootUI;
@property(nonatomic, weak, nullable, readwrite) LynxFontFaceContext* fontFaceContext;
@property(nonatomic, weak, nullable, readwrite) LynxShadowNodeOwner* nodeOwner;
@property(nonatomic, weak, nullable, readwrite) LynxUIOwner* uiOwner;
@property(nonatomic, weak) id lynxModuleExtraData;
@property(nonatomic, assign, readwrite) int64_t shellPtr;
@property(nonatomic, readwrite) LynxScreenMetrics* screenMetrics;
@property(nonatomic, readonly) LynxUIIntersectionObserverManager* intersectionManager;
@property(nonatomic) LynxUIExposure* uiExposure;
@property(nonatomic, strong, nullable, readonly) NSDictionary* keyframesDict;
@property(nonatomic, nullable) NSDictionary* contextDict;
@property(nonatomic) LynxGlobalObserver* observer;

// generic resource fetcher
@property(nonatomic, strong, nullable) id<LynxGenericResourceFetcher> genericResourceFetcher;
@property(nonatomic, strong, nullable) id<LynxMediaResourceFetcher> mediaResourceFetcher;
@property(nonatomic, strong, nullable) id<LynxTemplateResourceFetcher> templateResourceFetcher;

// settings
@property(nonatomic, readonly) BOOL defaultOverflowVisible;
@property(nonatomic, readonly) BOOL defaultImplicitAnimation;
@property(nonatomic, readonly) BOOL enableTextRefactor;
@property(nonatomic, readonly) BOOL defaultAutoResumeAnimation;
@property(nonatomic, readonly) BOOL defaultEnableNewTransformOrigin;
@property(nonatomic, readonly) BOOL enableEventRefactor;
@property(nonatomic, readonly) BOOL enableA11yIDMutationObserver;
@property(nonatomic, readonly) BOOL enableTextOverflow;
@property(nonatomic, readonly) BOOL enableNewClipMode;
@property(nonatomic, readonly) BOOL enableEventThrough;
@property(nonatomic, readonly) BOOL enableBackgroundShapeLayer;
@property(nonatomic, readonly) BOOL enableFiberArch;
@property(nonatomic, readonly) BOOL enableExposureUIMargin;
@property(nonatomic, readonly) BOOL enableTextLayerRender;
@property(nonatomic, readonly) BOOL enableTextLanguageAlignment;
@property(nonatomic, readonly) BOOL enableXTextLayoutReused;
@property(nonatomic, readonly) BOOL enableNewGesture;
@property(nonatomic, readonly) BOOL cssAlignWithLegacyW3c;
@property(nonatomic, readonly) NSString* targetSdkVersion;
@property(nonatomic, readonly) BOOL enableTextNonContiguousLayout;
@property(nonatomic, readonly) BOOL enableImageDownsampling;
@property(nonatomic, readonly) BOOL enableNewImage;
@property(nonatomic, readonly) BOOL trailUseNewImage;
@property(nonatomic, readonly) NSInteger logBoxImageSizeWarningThreshold;

- (instancetype)initWithScreenMetrics:(LynxScreenMetrics*)screenMetrics;
- (void)updateScreenSize:(CGSize)screenSize;
- (void)onGestureRecognized;
- (void)onGestureRecognizedByUI:(LynxUI*)ui;
- (void)onPropsChangedByUI:(LynxUI*)ui;
- (BOOL)isTouchMoving;
- (NSNumber*)getLynxRuntimeId;
/// Deprecated: use didReceiveResourceError:withSource:type: instead
- (void)didReceiveResourceError:(NSError*)error;

- (void)reportError:(nonnull NSError*)error;

- (void)didReceiveException:(NSException*)exception
                withMessage:(NSString*)message
                      forUI:(LynxUI*)ui;

- (BOOL)isDev;
- (void)addUIToExposedMap:(LynxUI*)ui;
- (void)addUIToExposedMap:(LynxUI*)ui
     withUniqueIdentifier:(NSString* _Nullable)uniqueID
                extraData:(NSDictionary* _Nullable)data
               useOptions:(NSDictionary* _Nullable)options;
- (void)removeUIFromExposedMap:(LynxUI*)ui;
- (void)removeUIFromExposedMap:(LynxUI*)ui withUniqueIdentifier:(NSString* _Nullable)uniqueID;
- (void)removeUIFromIntersectionManager:(LynxUI*)ui;

- (nullable id<LynxResourceFetcher>)getGenericResourceFetcher;

@end
NS_ASSUME_NONNULL_END
