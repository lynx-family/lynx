// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "DevToolPlatformDarwinDelegate.h"
#include <vector>

#import <Lynx/LynxPageReloadHelper+Internal.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <sys/utsname.h>
#import "ConsoleDelegateManager.h"
#import "Helper/LynxEmulateTouchHelper.h"
#import "Helper/LynxUITreeHelper.h"
#import "LepusDebugInfoHelper.h"
#import "LynxDevToolToast.h"
#import "LynxDeviceInfoHelper.h"
#import "LynxDevtoolEnv.h"
#import "LynxScreenCastHelper.h"

#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "devtool/lynx_devtool/element/element_inspector.h"

#pragma mark - DevToolPlatformDarwin
namespace lynx {
namespace devtool {
class DevToolPlatformDarwin : public DevToolPlatformFacade {
 public:
  DevToolPlatformDarwin(DevToolPlatformDarwinDelegate* darwin) { _darwin = darwin; }

  int FindNodeIdForLocation(float x, float y, std::string screen_shot_mode) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      return
          [darwin findNodeIdForLocationWithX:x
                                       withY:y
                                        mode:[NSString stringWithCString:screen_shot_mode.c_str()]];
    } else {
      return 0;
    }
  }

  void ScrollIntoView(int node_index) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin scrollIntoView:node_index];
    }
  }

  void OnConsoleMessage(const std::string& message) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin onConsoleMessage:message];
    }
  }

  void OnConsoleObject(const std::string& detail, int callback_id) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin onConsoleObject:detail callbackId:callback_id];
    }
  }

  virtual void StartScreenCast(ScreenshotRequest request) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      std::string mode = lynx::devtool::DevToolStatus::GetInstance().GetStatus(
          lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
          lynx::devtool::DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);
      [darwin startCasting:request.quality_
                     width:(int)request.max_width_
                    height:(int)request.max_height_
                      mode:[NSString stringWithCString:mode.c_str()]];
    }
  }

  virtual void StopScreenCast() override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin stopCasting];
    }
  }

  virtual void GetLynxScreenShot() override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin sendCardPreview];
    }
  }

  virtual void OnAckReceived() override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      [darwin onAckReceived];
    }
  }

  virtual std::string GetUINodeInfo(int id) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      NSString* res = [darwin getUINodeInfo:id];
      if (res != nil) {
        return std::string([res UTF8String]);
      }
    }
    return "";
  }

  virtual std::string GetLynxUITree() override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      NSString* res = [darwin getLynxUITree];
      if (res != nil) {
        return std::string([res UTF8String]);
      }
    }
    return "";
  }

  virtual int SetUIStyle(int id, std::string name, std::string content) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      return [darwin setUIStyle:id
                  withStyleName:[NSString stringWithUTF8String:name.c_str()]
               withStyleContent:[NSString stringWithUTF8String:content.c_str()]];
    }
    return -1;
  }

  void SetDevToolSwitch(const std::string& key, bool value) override {
    [LynxDevtoolEnv.sharedInstance setSwitchMask:value
                                          forKey:[NSString stringWithUTF8String:key.c_str()]];
  }

  std::vector<float> GetRectToWindow() const override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      return [darwin getRectToWindow];
    } else {
      return {0, 0, 0, 0};
    }
  }

  std::string GetLynxVersion() const override {
    return [[LynxDeviceInfoHelper getLynxVersion] UTF8String];
  }

  void OnReceiveTemplateFragment(const std::string& data, bool eof) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin onReceiveTemplateFragment:data eof:eof];
    }
  }

  std::vector<int32_t> GetViewLocationOnScreen() const override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      return [darwin getViewLocationOnScreen];
    }
    return {-1, -1};
  }

  void SendEventToVM(const std::string& vm_type, const std::string& event_name,
                     const std::string& data) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin sendEventToVM:@{
        @"type" : [NSString stringWithUTF8String:event_name.c_str()],
        @"data" : [NSString stringWithUTF8String:data.c_str()],
        // Note this will be checked by `TemplateAssembler::GetContextProxy`, accepted values are
        // from
        // `lynx::runtime::ContextProxy::Type` Currently, only `DevTool` will send message to VM.
        @"origin" : @"Devtool",
        @"target" : [NSString stringWithUTF8String:vm_type.c_str()]
      }];
    }
  }

  virtual lynx::lepus::Value* GetLepusValueFromTemplateData() override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      return [darwin getLepusValueFromTemplateData];
    }
    return nullptr;
  }

  std::string GetLepusDebugInfo(const std::string& url) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      return [darwin getLepusDebugInfo:url];
    }
    return "";
  }

  std::string GetTemplateJsInfo(int32_t offset, int32_t size) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      return [darwin getTemplateJsInfo:offset size:size];
    }
    return "";
  }

  virtual void EmulateTouch(std::shared_ptr<lynx::devtool::MouseEvent> input) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      [darwin emulateTouch:input];
    }
  }

  void PageReload(bool ignore_cache, std::string template_binary = "",
                  bool from_template_fragments = false, int32_t template_size = 0) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      NSString* nsBinary = nil;
      if (!template_binary.empty()) {
        nsBinary = [NSString stringWithCString:template_binary.c_str()
                                      encoding:NSUTF8StringEncoding];
      }
      [darwin reloadLynxView:ignore_cache
                withTemplate:nsBinary
               fromFragments:from_template_fragments
                    withSize:template_size];
    }
  }

  void Navigate(const std::string& url) override {}

  std::vector<double> GetBoxModel(Element* element) override {
    if (element->GetTag() == "x-overlay-ng") {
      return ElementInspector::GetOverlayNGBoxModel(element);
    }
    auto box_model = GetBoxModelInGeneralPlatform(element);
    return box_model;
  }

  std::vector<float> GetTransformValue(
      int identifier, const std::vector<float>& pad_border_margin_layout) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      NSArray<NSNumber*>* padBorderMarginLayout = VectorToNSArray(pad_border_margin_layout);

      NSArray<NSNumber*>* result = [darwin getTransformValue:identifier
                                   withPadBorderMarginLayout:padBorderMarginLayout];
      return NSArrayToVector(result);
    }
    return std::vector<float>();
  }

 private:
  std::vector<float> NSArrayToVector(NSArray<NSNumber*>* array) {
    std::vector<float> result;
    result.reserve(array.count);
    for (NSNumber* num in array) {
      result.push_back(num.floatValue);
    }
    return result;
  }

  NSArray<NSNumber*>* VectorToNSArray(const std::vector<float>& vec) {
    NSMutableArray<NSNumber*>* result = [NSMutableArray arrayWithCapacity:vec.size()];
    for (float value : vec) {
      [result addObject:@(value)];
    }
    return [result copy];
  }

  void SetLepusDebugInfoUrl(const std::string& url) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin setLepusDebugInfoUrl:url];
    }
  }

 private:
  __weak DevToolPlatformDarwinDelegate* _darwin;
};
}  // namespace devtool
}  // namespace lynx

@implementation DevToolPlatformDarwinDelegate {
  // LynxView
  __weak LynxView* _lynxView;

  // UITree
  LynxUITreeHelper* _uiTreeHelper;

  // EmulateTouch
  LynxEmulateTouchHelper* _touchHelper;

  // PageReload
  LynxPageReloadHelper* _reloadHelper;

  // ConsoleDelegateManager
  ConsoleDelegateManager* _consoleDelegateManager;
  LepusDebugInfoHelper* _lepusDebugInfoHelper;

  LynxScreenCastHelper* _castHelper;
  void (^_devtoolCallback)(NSDictionary*);

  std::shared_ptr<lynx::devtool::DevToolPlatformFacade> devtool_platform_facade_;
}

- (nonnull instancetype)initWithLynxView:(nullable LynxView*)view
                             withUIOwner:(nullable LynxUIOwner*)owner {
  _uiTreeHelper = [[LynxUITreeHelper alloc] init];
  [_uiTreeHelper attachLynxUIOwner:owner];

  _lynxView = view;
  _touchHelper = [[LynxEmulateTouchHelper alloc] initWithLynxView:view];

  _castHelper = [[LynxScreenCastHelper alloc] initWithLynxView:view withPlatformDelegate:self];

  devtool_platform_facade_ = std::make_shared<lynx::devtool::DevToolPlatformDarwin>(self);

  _consoleDelegateManager =
      [[ConsoleDelegateManager alloc] initWithDevToolPlatformFacade:devtool_platform_facade_];
  _lepusDebugInfoHelper = [[LepusDebugInfoHelper alloc] init];

  return self;
}

- (std::shared_ptr<lynx::devtool::DevToolPlatformFacade>)getNativePtr {
  return devtool_platform_facade_;
}

- (void)scrollIntoView:(int)node_index {
  if (_uiTreeHelper) {
    return [_uiTreeHelper scrollIntoView:node_index];
  }
}

- (int)findNodeIdForLocationWithX:(float)x withY:(float)y mode:(NSString*)mode {
  if (_uiTreeHelper) {
    return [_uiTreeHelper findNodeIdForLocationWithX:x withY:y mode:mode];
  }
  return 0;
}

- (NSArray<NSNumber*>*)getTransformValue:(NSInteger)sign
               withPadBorderMarginLayout:(NSArray<NSNumber*>*)padBorderMarginLayout {
  if (_uiTreeHelper) {
    return [_uiTreeHelper getTransformValue:sign withPadBorderMarginLayout:padBorderMarginLayout];
  }
  return @[];
}

- (void)setLynxInspectorConsoleDelegate:(id)delegate {
  [_consoleDelegateManager setLynxInspectorConsoleDelegate:delegate];
}

- (void)getConsoleObject:(NSString*)objectId
           needStringify:(BOOL)stringify
           resultHandler:(void (^)(NSString* _Nonnull detail))handler {
  [_consoleDelegateManager getConsoleObject:objectId needStringify:stringify resultHandler:handler];
}

- (void)onConsoleMessage:(const std::string&)message {
  [_consoleDelegateManager onConsoleMessage:message];
}

- (void)onConsoleObject:(const std::string&)detail callbackId:(int)callbackId {
  [_consoleDelegateManager onConsoleObject:detail callbackId:callbackId];
}

- (void)attachLynxView:(LynxView*)lynxView {
  _lynxView = lynxView;
  [_castHelper attachLynxView:lynxView];
  [_touchHelper attachLynxView:lynxView];
}

- (void)startCasting:(int)quality
               width:(int)max_width
              height:(int)max_height
                mode:(NSString*)screenshot_mode {
  [_castHelper startCasting:quality width:max_width height:max_height mode:screenshot_mode];
}

- (void)sendScreenCast:(NSString*)data
           andMetadata:(std::shared_ptr<lynx::devtool::ScreenMetadata>)metadata {
  if (data != nil && devtool_platform_facade_) {
    devtool_platform_facade_->SendPageScreencastFrameEvent([data UTF8String], metadata);
  }
}

- (void)dispatchScreencastVisibilityChanged:(BOOL)status {
  if (devtool_platform_facade_) {
    devtool_platform_facade_->SendPageScreencastVisibilityChangedEvent(status);
  }
}

- (void)onAckReceived {
  [_castHelper onAckReceived];
}

- (void)stopCasting {
  [_castHelper stopCasting];
}

- (void)continueCasting {
  [_castHelper continueCasting];
}

- (void)pauseCasting {
  [_castHelper pauseCasting];
}

- (void)sendCardPreview {
  __weak __typeof(_castHelper) weakCastHelper = _castHelper;
  // Delay for 1500ms to allow time for rendering remote resources
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, (int64_t)ScreenshotPreviewDelayTime * NSEC_PER_MSEC),
      dispatch_get_main_queue(), ^{
        __strong __typeof(weakCastHelper) castHelper = weakCastHelper;
        [castHelper sendCardPreview];
      });
}

- (void)sendCardPreviewData:(NSString*)data {
  if (data != nil && devtool_platform_facade_) {
    devtool_platform_facade_->SendLynxScreenshotCapturedEvent([data UTF8String]);
  }
}

- (std::vector<float>)getRectToWindow {
  if (_uiTreeHelper) {
    CGRect rect = [_uiTreeHelper getRectToWindow];
    return {(float)rect.origin.x, (float)rect.origin.y, (float)rect.size.width,
            (float)rect.size.height};
  }
  return {};
}

- (void)onReceiveTemplateFragment:(const std::string&)data eof:(bool)eof {
  [_reloadHelper onReceiveTemplateFragment:[NSString stringWithCString:data.c_str()
                                                              encoding:NSUTF8StringEncoding]
                                   withEof:eof];
}

- (void)setReloadHelper:(nullable LynxPageReloadHelper*)reloadHelper {
  _reloadHelper = reloadHelper;
}

- (std::vector<int32_t>)getViewLocationOnScreen {
  CGPoint point = [_uiTreeHelper getViewLocationOnScreen];
  if (point.x >= 0 && point.y >= 0) {
    return {static_cast<int32_t>(roundf(point.x)), static_cast<int32_t>(roundf(point.y))};
  }
  return {0, 0};
}

- (void)sendEventToVM:(NSDictionary*)event {
  if (_devtoolCallback == nil) {
    return;
  }
  _devtoolCallback(event);
}

- (void)setDevToolCallback:(void (^)(NSDictionary*))callback {
  _devtoolCallback = callback;
}

- (NSString*)getLynxUITree {
  NSString* res;
  if (_uiTreeHelper) {
    res = [_uiTreeHelper getLynxUITree];
  }
  return res;
}

- (NSString*)getUINodeInfo:(int)id {
  NSString* res;
  if (_uiTreeHelper) {
    res = [_uiTreeHelper getUINodeInfo:id];
  }
  return res;
}

- (int)setUIStyle:(int)id withStyleName:(NSString*)name withStyleContent:(NSString*)content {
  if (_uiTreeHelper) {
    return [_uiTreeHelper setUIStyle:id withStyleName:name withStyleContent:content];
  } else {
    return -1;
  }
}

- (lynx::lepus::Value*)getLepusValueFromTemplateData {
  LynxTemplateData* template_data = _reloadHelper ? [_reloadHelper getTemplateData] : nullptr;
  if (template_data) {
    lynx::lepus::Value* value = LynxGetLepusValueFromTemplateData(template_data);
    return value;
  }
  return nullptr;
}

- (std::string)getSystemModelName {
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString* deviceModel = [NSString stringWithCString:systemInfo.machine
                                             encoding:NSUTF8StringEncoding];

  return [deviceModel UTF8String];
}

- (std::string)getTemplateJsInfo:(int32_t)offset size:(int32_t)size {
  if (_reloadHelper != nil) {
    NSString* str = [_reloadHelper getTemplateJsInfo:offset withSize:size];
    return str ? std::string([str UTF8String]) : "";
  }
  return "";
}

- (std::string)getLepusDebugInfo:(const std::string&)url {
  return [_lepusDebugInfoHelper getDebugInfo:url];
}

- (void)setLepusDebugInfoUrl:(const std::string&)url {
  [_lepusDebugInfoHelper setDebugInfoUrl:[NSString stringWithUTF8String:url.c_str()]];
}

- (NSString*)getLepusDebugInfoUrl {
  return [_lepusDebugInfoHelper debugInfoUrl];
}

- (void)emulateTouch:(std::shared_ptr<lynx::devtool::MouseEvent>)input {
  NSString* type = [NSString stringWithCString:input->type_.c_str()
                                      encoding:[NSString defaultCStringEncoding]];
  NSString* button = [NSString stringWithCString:input->button_.c_str()
                                        encoding:[NSString defaultCStringEncoding]];

  [self emulateTouch:type
         coordinateX:input->x_
         coordinateY:input->y_
              button:button
              deltaX:input->delta_x_
              deltaY:input->delta_y_
           modifiers:input->modifiers_
          clickCount:input->click_count_];
}

- (void)emulateTouch:(nonnull NSString*)type
         coordinateX:(int)x
         coordinateY:(int)y
              button:(nonnull NSString*)button
              deltaX:(CGFloat)dx
              deltaY:(CGFloat)dy
           modifiers:(int)modifiers
          clickCount:(int)clickCount {
  if (_touchHelper != nil) {
    std::string mode = lynx::devtool::DevToolStatus::GetInstance().GetStatus(
        lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
        lynx::devtool::DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);
    [_touchHelper emulateTouch:type
                   coordinateX:x
                   coordinateY:y
                        button:button
                        deltaX:dx
                        deltaY:dy
                     modifiers:modifiers
                    clickCount:clickCount
                screenshotMode:[NSString stringWithCString:mode.c_str()]];
  }
}

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(NSString*)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size {
  [LynxDevToolToast showToast:@"Start to download & reload..."];
  [_reloadHelper reloadLynxView:ignoreCache
                   withTemplate:templateBin
                  fromFragments:fromFragments
                       withSize:size];
}

- (void)sendConsoleEvent:(NSString*)message
               withLevel:(int32_t)level
           withTimeStamp:(int64_t)timeStamp {
  if (message != nil && devtool_platform_facade_) {
    devtool_platform_facade_->SendConsoleEvent({[message UTF8String], level, timeStamp});
  }
}

- (void)sendLayerTreeDidChangeEvent {
  if (devtool_platform_facade_) {
    devtool_platform_facade_->SendLayerTreeDidChangeEvent();
  }
}

@end
