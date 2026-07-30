// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#include <cmath>
#include <vector>

#import <BaseDevTool/DevToolToast.h>
#import <Lynx/LynxPageReloadHelper+Internal.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTouchEvent.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <Lynx/LynxUIRenderer.h>
#import <Lynx/LynxUIRendererProtocol.h>
#import <LynxDevtool/ConsoleDelegateManager.h>
#import <LynxDevtool/LepusDebugInfoHelper.h>
#import <LynxDevtool/LynxDeviceInfoHelper.h>
#import <LynxDevtool/LynxDevtoolEnv.h>
#import <LynxDevtool/LynxEmulateTouchHelper.h>
#import <LynxDevtool/LynxScreenCastHelper.h>
#import <LynxDevtool/LynxUITreeHelper.h>
#import <sys/utsname.h>

#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "devtool/lynx_devtool/agent/input/input_event_target.h"

@interface DevToolPlatformDarwinDelegate ()
- (nullable UIView*)firstResponderInView:(nullable UIView*)view;
- (NSString*)effectiveScreenshotMode;
@end

@interface LynxEmulateTouchHelper (PointerEventLifecycle)
- (void)cancelCurrentPointerSequence;
@end

#pragma mark - DevToolPlatformDarwin
namespace lynx {
namespace devtool {
namespace {
bool ToDarwinPointerEventType(input::PointerEventType type,
                              LynxDevToolPointerEventType* darwin_type) {
  if (!darwin_type) {
    return false;
  }
  switch (type) {
    case input::PointerEventType::kDown:
      *darwin_type = LynxDevToolPointerEventTypeDown;
      return true;
    case input::PointerEventType::kMove:
      *darwin_type = LynxDevToolPointerEventTypeMove;
      return true;
    case input::PointerEventType::kUp:
      *darwin_type = LynxDevToolPointerEventTypeUp;
      return true;
    case input::PointerEventType::kCancel:
      *darwin_type = LynxDevToolPointerEventTypeCancel;
      return true;
    case input::PointerEventType::kScroll:
      *darwin_type = LynxDevToolPointerEventTypeScroll;
      return true;
  }
  return false;
}

class DarwinInputEventTarget : public input::InputEventTarget {
 public:
  explicit DarwinInputEventTarget(DevToolPlatformDarwinDelegate* delegate) : delegate_(delegate) {}

  input::PointerCapabilities GetPointerCapabilities() const override {
    input::PointerCapabilities capabilities;
    capabilities.default_source_type = input::PointerSourceType::kTouch;
    capabilities.supports_touch = true;
    return capabilities;
  }

  bool InjectPointerEvent(const input::PointerEvent& event) override {
    if (event.source_type != input::PointerSourceType::kTouch || event.pointers.size() != 1) {
      return false;
    }
    __strong typeof(delegate_) delegate = delegate_;
    return delegate != nil && [delegate injectPointerEvent:event];
  }

 private:
  __weak DevToolPlatformDarwinDelegate* delegate_;
};
}  // namespace

class DevToolPlatformDarwin : public DevToolPlatformFacade {
 public:
  explicit DevToolPlatformDarwin(DevToolPlatformDarwinDelegate* darwin)
      : _darwin(darwin), input_event_target_(std::make_shared<DarwinInputEventTarget>(darwin)) {}

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

  std::string GetDebugInfoByUrl(const std::string& url) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      NSString* debugInfo = [darwin getDebugInfoByUrl:[NSString stringWithCString:url.c_str()]];
      if (debugInfo) {
        return [debugInfo UTF8String];
      }
    }
    return DevToolStatus::NO_DEBUG_INFO_FOUND_BY_URL;
  }

  void ScrollIntoView(int node_index) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin scrollIntoView:node_index];
    }
  }

  void Focus(int node_index) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin) {
      [darwin focus:node_index];
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
                      mode:[NSString stringWithUTF8String:mode.c_str()]
                    format:[NSString stringWithUTF8String:request.format_.c_str()]];
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
    // deprecated since 3.8
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

  std::shared_ptr<input::InputEventTarget> GetInputEventTarget() override {
    return input_event_target_;
  }

  void InsertText(const std::string& text) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      NSString* nsText = [[NSString alloc] initWithBytes:text.data()
                                                  length:text.size()
                                                encoding:NSUTF8StringEncoding];
      [darwin insertText:nsText];
    }
  }

  void PageReload(bool ignore_cache, const std::string& template_binary,
                  const std::string& reload_url, bool from_template_fragments = false,
                  int32_t template_size = 0) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      NSString* nsBinary = nil;
      if (!template_binary.empty()) {
        nsBinary = [NSString stringWithCString:template_binary.c_str()
                                      encoding:NSUTF8StringEncoding];
      }
      NSString* nsReloadUrl = nil;
      if (!reload_url.empty()) {
        nsReloadUrl = [NSString stringWithCString:reload_url.c_str() encoding:NSUTF8StringEncoding];
      }
      [darwin reloadLynxView:ignore_cache
                withTemplate:nsBinary
               fromFragments:from_template_fragments
                    withSize:template_size
               withReloadUrl:nsReloadUrl];
    }
  }

  void Navigate(const std::string& url) override {
    __strong typeof(_darwin) darwin = _darwin;
    if (darwin != nil) {
      [darwin navigateLynxView:[NSString stringWithUTF8String:url.c_str()]];
    }
  }

 protected:
  bool SupportsOverlayBoxModel() const override { return true; }

 public:
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

 private:
  __weak DevToolPlatformDarwinDelegate* _darwin;
  std::shared_ptr<input::InputEventTarget> input_event_target_;
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

  // DebugInfoRecorder
  id<LynxDebugInfoRecorderProtocol> _debugInfoRecorder;

  // PageReload
  LynxPageReloadHelper* _reloadHelper;

  // ConsoleDelegateManager
  ConsoleDelegateManager* _consoleDelegateManager;
  LepusDebugInfoHelper* _lepusDebugInfoHelper;

  LynxScreenCastHelper* _castHelper;
  void (^_devtoolCallback)(NSDictionary*);

  std::shared_ptr<lynx::devtool::DevToolPlatformFacade> devtool_platform_facade_;
  NSString* _effectiveScreenshotMode;
  NSInteger _injectedTouchTargetTag;
  CGPoint _injectedTouchStartPoint;
  BOOL _injectedTouchMoved;
}

- (nonnull instancetype)initWithLynxView:(nullable LynxView*)view {
  _uiTreeHelper = [[LynxUITreeHelper alloc] init];

  _lynxView = view;
  _debugInfoRecorder = nil;
  _touchHelper = [[LynxEmulateTouchHelper alloc] initWithLynxView:view];
  _injectedTouchTargetTag = -1;
  _injectedTouchMoved = NO;

  _castHelper = [[LynxScreenCastHelper alloc] initWithLynxView:view withPlatformDelegate:self];

  devtool_platform_facade_ = std::make_shared<lynx::devtool::DevToolPlatformDarwin>(self);

  _consoleDelegateManager =
      [[ConsoleDelegateManager alloc] initWithDevToolPlatformFacade:devtool_platform_facade_];
  _lepusDebugInfoHelper = [[LepusDebugInfoHelper alloc] init];

  return self;
}

- (void)dealloc {
  if ([_touchHelper respondsToSelector:@selector(cancelCurrentPointerSequence)]) {
    [_touchHelper cancelCurrentPointerSequence];
  }
}

- (void)attachLynxUIOwner:(nullable LynxUIOwner*)owner {
  [_uiTreeHelper attachLynxUIOwner:owner];
}

- (std::shared_ptr<lynx::devtool::DevToolPlatformFacade>)getNativePtr {
  return devtool_platform_facade_;
}

- (void)scrollIntoView:(int)node_index {
  if (_uiTreeHelper) {
    return [_uiTreeHelper scrollIntoView:node_index];
  }
}

- (void)focus:(int)node_index {
  if (_uiTreeHelper) {
    [_uiTreeHelper focus:node_index];
  }
}

- (int)findNodeIdForLocationWithX:(float)x withY:(float)y mode:(NSString*)mode {
  __strong typeof(_lynxView) lynxView = _lynxView;
  NSString* effectiveMode = _effectiveScreenshotMode.length > 0 ? _effectiveScreenshotMode : mode;
  return [lynxView.templateRender.lynxUIRenderer findNodeIdForLocationWithX:x
                                                                      withY:y
                                                                       mode:effectiveMode];
}

- (NSString*)getDebugInfoByUrl:(NSString*)url {
  if (_debugInfoRecorder) {
    return [_debugInfoRecorder getDebugInfo:url];
  }
  return @"NO_DEBUG_INFO_FOUND_BY_URL";
}

- (NSArray<NSNumber*>*)getTransformValue:(NSInteger)sign
               withPadBorderMarginLayout:(NSArray<NSNumber*>*)padBorderMarginLayout {
  __strong typeof(_lynxView) lynxView = _lynxView;
  return [lynxView.templateRender.lynxUIRenderer getTransformValue:sign
                                         withPadBorderMarginLayout:padBorderMarginLayout];
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
  if ([_touchHelper respondsToSelector:@selector(cancelCurrentPointerSequence)]) {
    [_touchHelper cancelCurrentPointerSequence];
  }
  _injectedTouchTargetTag = -1;
  _injectedTouchMoved = NO;
  _lynxView = lynxView;
  [_castHelper attachLynxView:lynxView];
  [_touchHelper attachLynxView:lynxView];
}

- (void)startCasting:(int)quality
               width:(int)max_width
              height:(int)max_height
                mode:(NSString*)screenshot_mode
              format:(NSString*)format {
  __strong typeof(_lynxView) lynxView = _lynxView;
  NSString* requestedMode = screenshot_mode.length > 0 ? screenshot_mode : ScreenshotModeFullScreen;
  _effectiveScreenshotMode = [lynxView.templateRender.lynxUIRenderer isFullScreenShotSupported]
                                 ? [requestedMode copy]
                                 : ScreenshotModeLynxView;
  // Keep shared inspector coordinate consumers aligned with the actual capture mode.
  lynx::devtool::DevToolStatus::GetInstance().SetStatus(
      lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      [_effectiveScreenshotMode UTF8String]);
  [_castHelper startCasting:quality
                      width:max_width
                     height:max_height
                       mode:_effectiveScreenshotMode
                     format:format];
}

- (NSString*)effectiveScreenshotMode {
  if (_effectiveScreenshotMode.length > 0) {
    return _effectiveScreenshotMode;
  }
  std::string mode = lynx::devtool::DevToolStatus::GetInstance().GetStatus(
      lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      lynx::devtool::DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);
  NSString* result = [NSString stringWithCString:mode.c_str() encoding:NSUTF8StringEncoding];
  return result.length > 0 ? result : ScreenshotModeFullScreen;
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

- (void)setDebugInfoInterceptor:(nonnull id<LynxDebugInfoRecorderProtocol>)debugInfoRecorder {
  _debugInfoRecorder = debugInfoRecorder;
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

- (NSString*)getLepusDebugInfoUrl:(NSString*)filename {
  std::string url;
  if (devtool_platform_facade_ != nullptr) {
    url = devtool_platform_facade_->GetLepusDebugInfoUrl([filename UTF8String]);
  }
  return [NSString stringWithUTF8String:url.c_str()];
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

- (void)insertText:(nullable NSString*)text {
  if (text == nil) {
    return;
  }
  __strong typeof(_lynxView) lynxView = _lynxView;
  UIView* firstResponder = [self firstResponderInView:lynxView];
  if ([firstResponder conformsToProtocol:@protocol(UITextInput)] &&
      [firstResponder respondsToSelector:@selector(insertText:)]) {
    [(id<UITextInput>)firstResponder insertText:text];
  }
}

- (BOOL)injectPointerEvent:(const lynx::input::PointerEvent&)inputEvent {
  __strong typeof(_lynxView) lynxView = _lynxView;
  if (lynxView == nil) {
    return NO;
  }
  const auto* pointer = inputEvent.FindPointer(inputEvent.changed_pointer_id);
  LynxDevToolPointerEventType darwinType;
  if (pointer == nullptr || !std::isfinite(pointer->x) || !std::isfinite(pointer->y) ||
      !std::isfinite(inputEvent.delta_x) || !std::isfinite(inputEvent.delta_y) ||
      !lynx::devtool::ToDarwinPointerEventType(inputEvent.type, &darwinType)) {
    return NO;
  }

  const CGFloat x = pointer->x;
  const CGFloat y = pointer->y;
  NSString* screenshotMode = [self effectiveScreenshotMode];
  if ([_touchHelper respondsToSelector:@selector
                    (injectPointerEvent:coordinateX:coordinateY:deltaX:deltaY:screenshotMode:)] &&
      [_touchHelper injectPointerEvent:darwinType
                           coordinateX:x
                           coordinateY:y
                                deltaX:inputEvent.delta_x
                                deltaY:inputEvent.delta_y
                        screenshotMode:screenshotMode]) {
    return YES;
  }

  NSString* eventName = nil;
  switch (inputEvent.type) {
    case lynx::input::PointerEventType::kDown:
      if (_injectedTouchTargetTag > 0) {
        return NO;
      }
      _injectedTouchTargetTag = [self findNodeIdForLocationWithX:x withY:y mode:screenshotMode];
      if (_injectedTouchTargetTag <= 0) {
        _injectedTouchTargetTag = -1;
        return NO;
      }
      _injectedTouchStartPoint = CGPointMake(pointer->x, pointer->y);
      _injectedTouchMoved = NO;
      eventName = LynxEventTouchStart;
      break;
    case lynx::input::PointerEventType::kMove: {
      const CGFloat dx = pointer->x - _injectedTouchStartPoint.x;
      const CGFloat dy = pointer->y - _injectedTouchStartPoint.y;
      _injectedTouchMoved = _injectedTouchMoved || dx * dx + dy * dy > 25.f;
      eventName = LynxEventTouchMove;
      break;
    }
    case lynx::input::PointerEventType::kUp:
      eventName = LynxEventTouchEnd;
      break;
    case lynx::input::PointerEventType::kCancel:
      eventName = LynxEventTouchCancel;
      break;
    case lynx::input::PointerEventType::kScroll:
      return NO;
  }
  if (_injectedTouchTargetTag <= 0) {
    return NO;
  }

  id<LynxUIRendererProtocol> renderer = lynxView.templateRender.lynxUIRenderer;
  UIView* rootView = [renderer eventHandlerRootView];
  LynxUI* target = [renderer findUIBySign:_injectedTouchTargetTag];
  if (rootView == nil || target == nil) {
    _injectedTouchTargetTag = -1;
    _injectedTouchMoved = NO;
    return NO;
  }

  CGPoint clientPoint = CGPointMake(x, y);
  CGPoint pagePoint;
  NSString* lynxViewMode =
      [NSString stringWithCString:lynx::devtool::DevToolStatus::SCREENSHOT_MODE_LYNXVIEW
                         encoding:NSUTF8StringEncoding];
  if ([screenshotMode isEqualToString:lynxViewMode]) {
    pagePoint = clientPoint;
    clientPoint = [rootView convertPoint:pagePoint toView:nil];
  } else {
    pagePoint = [[LynxUIKitAPIAdapter getKeyWindow] convertPoint:clientPoint toView:rootView];
  }
  CGPoint viewPoint = [rootView convertPoint:pagePoint toView:target.view];
  LynxTouchEvent* event = [[LynxTouchEvent alloc] initWithName:eventName
                                                     targetTag:_injectedTouchTargetTag
                                                   clientPoint:clientPoint
                                                     pagePoint:pagePoint
                                                     viewPoint:viewPoint];
  event.eventTarget = target;
  event.timestamp = [[NSDate date] timeIntervalSince1970];
  [lynxView sendTouchEvent:event];
  if (inputEvent.type == lynx::input::PointerEventType::kUp && !_injectedTouchMoved) {
    LynxTouchEvent* tapEvent = [[LynxTouchEvent alloc] initWithName:LynxEventTap
                                                          targetTag:_injectedTouchTargetTag
                                                        clientPoint:clientPoint
                                                          pagePoint:pagePoint
                                                          viewPoint:viewPoint];
    tapEvent.eventTarget = target;
    tapEvent.timestamp = event.timestamp;
    [lynxView sendTouchEvent:tapEvent];
  }
  if (inputEvent.type == lynx::input::PointerEventType::kUp ||
      inputEvent.type == lynx::input::PointerEventType::kCancel) {
    _injectedTouchTargetTag = -1;
    _injectedTouchMoved = NO;
  }
  return YES;
}

- (nullable UIView*)firstResponderInView:(nullable UIView*)view {
  if (view == nil) {
    return nil;
  }
  if (view.isFirstResponder) {
    return view;
  }
  for (UIView* subview in view.subviews) {
    UIView* firstResponder = [self firstResponderInView:subview];
    if (firstResponder != nil) {
      return firstResponder;
    }
  }
  return nil;
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
    [_touchHelper emulateTouch:type
                   coordinateX:x
                   coordinateY:y
                        button:button
                        deltaX:dx
                        deltaY:dy
                     modifiers:modifiers
                    clickCount:clickCount
                screenshotMode:[self effectiveScreenshotMode]];
  }
}

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(NSString*)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size
         withReloadUrl:(NSString*)reload_url {
  [DevToolToast showToast:@"Start to download & reload..."];
  [_reloadHelper reloadLynxView:ignoreCache
                   withTemplate:templateBin
                  fromFragments:fromFragments
                       withSize:size
                  withReloadUrl:reload_url];
}

- (void)navigateLynxView:(nonnull NSString*)url {
  [_reloadHelper navigateLynxView:url];
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

- (void)sendCDPEvent:(NSString*)message {
  if (devtool_platform_facade_) {
    devtool_platform_facade_->SendCDPEvent([message UTF8String]);
  }
}

@end
