// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxView+Internal.h>
#import "LynxPageReloadHelper.h"

#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"

NS_ASSUME_NONNULL_BEGIN

@interface DevToolPlatformDarwinDelegate : NSObject

- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view
                             withUIOwner:(nullable LynxUIOwner *)owner;

- (std::shared_ptr<lynx::devtool::DevToolPlatformFacade>)getNativePtr;

- (void)scrollIntoView:(int)node_index;

- (int)findNodeIdForLocationWithX:(float)x withY:(float)y mode:(NSString *)mode;

- (void)setLynxInspectorConsoleDelegate:(id _Nonnull)delegate;

- (void)getConsoleObject:(NSString *)objectId
           needStringify:(BOOL)stringify
           resultHandler:(void (^)(NSString *detail))handler;

- (void)onConsoleMessage:(const std::string &)message;

- (void)onConsoleObject:(const std::string &)detail callbackId:(int)callbackId;

- (void)attachLynxView:(LynxView *)lynxView;

- (void)startCasting:(int)quality
               width:(int)max_width
              height:(int)max_height
                mode:(NSString *)screenshot_mode;

- (void)sendScreenCast:(NSString *)data
           andMetadata:(std::shared_ptr<lynx::devtool::ScreenMetadata>)metadata;

- (NSArray<NSNumber *> *)getTransformValue:(NSInteger)sign
                 withPadBorderMarginLayout:(NSArray<NSNumber *> *)padBorderMarginLayout;

- (void)dispatchScreencastVisibilityChanged:(BOOL)status;

- (void)onAckReceived;

- (void)stopCasting;

- (void)continueCasting;

- (void)pauseCasting;

- (void)sendCardPreview;

- (void)sendCardPreviewData:(NSString *)data;

- (std::vector<float>)getRectToWindow;

- (void)onReceiveTemplateFragment:(const std::string &)data eof:(bool)eof;

- (void)setReloadHelper:(nullable LynxPageReloadHelper *)reloadHelper;

- (std::vector<int32_t>)getViewLocationOnScreen;

- (void)sendEventToVM:(NSDictionary *)event;

- (void)setDevToolCallback:(void (^)(NSDictionary *))callback;

- (NSString *)getLynxUITree;

- (NSString *)getUINodeInfo:(int)id;

- (int)setUIStyle:(int)id withStyleName:(NSString *)name withStyleContent:(NSString *)content;

- (lynx::lepus::Value *)getLepusValueFromTemplateData;

- (std::string)getSystemModelName;

- (std::string)getTemplateJsInfo:(int32_t)offset size:(int32_t)size;

- (std::string)getLepusDebugInfo:(const std::string &)url;

- (void)setLepusDebugInfoUrl:(const std::string &)url;

- (NSString *)getLepusDebugInfoUrl;

- (void)emulateTouch:(std::shared_ptr<lynx::devtool::MouseEvent>)input;
- (void)emulateTouch:(nonnull NSString *)type
         coordinateX:(int)x
         coordinateY:(int)y
              button:(nonnull NSString *)button
              deltaX:(CGFloat)dx
              deltaY:(CGFloat)dy
           modifiers:(int)modifiers
          clickCount:(int)clickCount;

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(NSString *)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size;

- (void)sendConsoleEvent:(NSString *)message
               withLevel:(int32_t)level
           withTimeStamp:(int64_t)timeStamp;

- (void)sendLayerTreeDidChangeEvent;

@end

NS_ASSUME_NONNULL_END
