// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LUIErrorHandling.h>

typedef void (^attachLynxPageUI)(__weak NSObject* _Nonnull ui);

/// This protocol is a collection of abilities a root view of Lynx UI tree is required.
///
///
/// LUIErrorHandling: abilities to handle error occurs during rendering UIs.
@protocol LUIBodyView <NSObject, LUIErrorHandling>

@required
///  If `enableAsyncDisplay` is YES, sub-nodes' contents (text, image, SVG, border, etc) will be
///  drawn asynchronously.
/// Set `enableAsyncDisplay` is NO if it is not wanted. Default to YES.
@property(nonatomic, readwrite, assign) BOOL enableAsyncDisplay;

/// Basic info that logged out when error occurs. If you don't care about the debug info, an empty
/// string is acceptable.
@required
@property(nonatomic, readonly, nullable) NSString* url;

/// An unique id of the Lynx UI tree. Some global events are depends on this id to distinguish the
/// Lynx UI tree instance (e.g. Scroll Fluency Monitor). If you don't care about the reporting info,
/// an default value -1 is acceptable.
@required
@property(nonatomic, readonly, assign) int32_t instanceId;

/// Some GlobalEvents are sending to pipeline via this method (e.g. UIExposure, accessibility).
/// - Parameters:
///   - name: name of the event.
///   - params: params of the event.
@required
- (void)sendGlobalEvent:(nonnull NSString*)name withParams:(nullable NSArray*)params;

// LynxUI lib use this methods to notify the rootView the size of the LynxUI tree is modified.
@required
- (void)setIntrinsicContentSize:(CGSize)size;

/// This is an optional optimization config for text. Default to YES in LynxUIContext.
/// This configuration will be removed soon and the optimization will always take effect.
@required
- (BOOL)enableTextNonContiguousLayout;

/// This is the interface used to set the parent-child relationship of Lynx pages. The parent page
/// uses this interface to set the relevant callback, and the child page will call the callback at
/// the appropriate time.
/// - Parameters:
///   - callback: the callback used to set the parent-child relationship of Lynx pages.
@optional
- (void)setAttachLynxPageUICallback:(attachLynxPageUI _Nonnull)callback;

/// This interface is used to identify whether the Lynx page is a child page in the Lynx page nested
/// scenario.
@optional
@property(nonatomic) BOOL isChildLynxPage;

@end
