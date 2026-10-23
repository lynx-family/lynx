// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "platform/darwin/macos/common/LynxUIRenderer.h"
#include "clay/common/service/service_manager.h"
#include "clay/fml/logging.h"
#include "clay/lynx_adaptor/native_module/lynx_module_factory.h"
#include "clay/lynx_adaptor/native_platform_view.h"
#include "clay/lynx_adaptor/native_view_service_embedder.h"
#include "clay/lynx_adaptor/resource_loader_embedder.h"
#include "clay/net/loader/resource_loader_creator_service.h"
#include "clay/shell/common/services/instrumentation_service.h"
#include "clay/shell/platform/darwin/macos/framework/Source/ClayViewProvider_Internal.h"
#include "clay/shell/platform/darwin/macos/framework/Source/FlutterEngine_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterTextInputPlugin.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "platform/embedder/interfaces/renderer_factory.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

@interface LynxUIRendererMac : NSObject

@property(nonatomic) ClayViewProvider* clayViewProvider;

@end

@implementation LynxUIRendererMac : NSObject

- (instancetype)init {
  self = [super init];

  if (self) {
    _clayViewProvider = [[ClayViewProvider alloc] init];
  }
  return self;
}

- (void)dealloc {
}

- (void)setFrame:(CGRect)frame {
  [self.clayViewProvider setFrame:frame];
}

- (void)setParent:(NativeWindow)parent {
  if (!parent) {
    return;
  }
  [self.clayViewProvider setParent:(__bridge NSView*)parent];
}

- (void)RegisterIMEHandler:(void*)handler arg:(void*)arg {
  [self.clayViewProvider requestIME:handler arg:arg];
}

- (NSEvent*)generateMouseDownEvent:(NSPoint)locationInWindow {
  NSEvent* downEvent =
      [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
                         location:locationInWindow
                    modifierFlags:0
                        timestamp:0
                     windowNumber:[[self.clayViewProvider.flutterView window] windowNumber]
                          context:nil
                      eventNumber:0
                       clickCount:1
                         pressure:1];
  return downEvent;
}

- (NSEvent*)generateMouseUpEvent:(NSPoint)locationInWindow {
  NSEvent* upEvent =
      [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp
                         location:locationInWindow
                    modifierFlags:0
                        timestamp:0
                     windowNumber:[[self.clayViewProvider.flutterView window] windowNumber]
                          context:nil
                      eventNumber:0
                       clickCount:1
                         pressure:1];
  return upEvent;
}

- (NSEvent*)generateMouseEnterEvent:(NSPoint)locationInWindow {
  NSEvent* enterEvent =
      [NSEvent enterExitEventWithType:NSEventTypeMouseEntered
                             location:locationInWindow
                        modifierFlags:0
                            timestamp:0
                         windowNumber:[[self.clayViewProvider.flutterView window] windowNumber]
                              context:nil
                          eventNumber:0
                       trackingNumber:0
                             userData:NULL];
  return enterEvent;
}

- (NSEvent*)generateMouseExitEvent:(NSPoint)locationInWindow {
  NSEvent* exitEvent =
      [NSEvent enterExitEventWithType:NSEventTypeMouseExited
                             location:locationInWindow
                        modifierFlags:0
                            timestamp:0
                         windowNumber:[[self.clayViewProvider.flutterView window] windowNumber]
                              context:nil
                          eventNumber:0
                       trackingNumber:0
                             userData:NULL];
  return exitEvent;
}

- (NSEvent*)generateMouseMoveEvent:(NSPoint)locationInWindow {
  NSEvent* moveEvent =
      [NSEvent mouseEventWithType:NSEventTypeMouseMoved
                         location:locationInWindow
                    modifierFlags:0
                        timestamp:0
                     windowNumber:[[self.clayViewProvider.flutterView window] windowNumber]
                          context:nil
                      eventNumber:0
                       clickCount:0
                         pressure:0];
  return moveEvent;
}

- (void)emulateMouseEntered:(nonnull NSEvent*)event {
  if ([self.clayViewProvider respondsToSelector:@selector(mouseEntered:)]) {
    [self.clayViewProvider performSelector:@selector(mouseEntered:) withObject:event];
  }
}
- (void)emulateMouseExited:(nonnull NSEvent*)event {
  if ([self.clayViewProvider respondsToSelector:@selector(mouseExited:)]) {
    [self.clayViewProvider performSelector:@selector(mouseExited:) withObject:event];
  }
}
- (void)emulateMouseMoved:(nonnull NSEvent*)event {
  if ([self.clayViewProvider respondsToSelector:@selector(mouseMoved:)]) {
    [self.clayViewProvider performSelector:@selector(mouseMoved:) withObject:event];
  }
}
- (void)emulateMouseDown:(NSEvent*)event {
  if ([self.clayViewProvider respondsToSelector:@selector(mouseDown:)]) {
    [self.clayViewProvider performSelector:@selector(mouseDown:) withObject:event];
  }
}

- (void)emulateMouseUp:(NSEvent*)event {
  if ([self.clayViewProvider respondsToSelector:@selector(mouseUp:)]) {
    [self.clayViewProvider performSelector:@selector(mouseUp:) withObject:event];
  }
}

@end

namespace lynx {
namespace embedder {

namespace {

void DispatchEmulatedMouseEvent(LynxUIRendererMac* lynx_ui_renderer, const std::string& name,
                                float x, float y) {
  if (!lynx_ui_renderer) {
    return;
  }

  NSPoint locationInView = NSMakePoint(x, y);
  NSPoint locationInWindow =
      [lynx_ui_renderer.clayViewProvider.flutterView convertPoint:locationInView toView:nil];
  if (name == "mouseclick") {
    [lynx_ui_renderer emulateMouseDown:[lynx_ui_renderer generateMouseDownEvent:locationInWindow]];
    [lynx_ui_renderer emulateMouseUp:[lynx_ui_renderer generateMouseUpEvent:locationInWindow]];
  } else if (name == "mousemove") {
    [lynx_ui_renderer emulateMouseMoved:[lynx_ui_renderer generateMouseMoveEvent:locationInWindow]];
  } else if (name == "mouseenter" || name == "mouseover") {
    [lynx_ui_renderer
        emulateMouseEntered:[lynx_ui_renderer generateMouseEnterEvent:locationInWindow]];
  } else if (name == "mouseleave") {
    [lynx_ui_renderer
        emulateMouseExited:[lynx_ui_renderer generateMouseExitEvent:locationInWindow]];
  } else if (name == "mousedown") {
    [lynx_ui_renderer emulateMouseDown:[lynx_ui_renderer generateMouseDownEvent:locationInWindow]];
  } else if (name == "mouseup") {
    [lynx_ui_renderer emulateMouseUp:[lynx_ui_renderer generateMouseUpEvent:locationInWindow]];
  }
}

}  // namespace

std::unique_ptr<LynxUIRenderer> CreateDefaultUIRenderer(RendererConfig config) {
  return std::make_unique<LynxUIRendererImpl>(std::move(config));
}

LynxUIRendererImpl::LynxUIRendererImpl(RendererConfig config)
    : LynxUIRendererClay(config),
      frame_timing_listener_(std::make_shared<FrameTimingListenerImpl>()) {
  auto lynx_ui_renderer = [[LynxUIRendererMac alloc] init];
  auto* view_context =
      reinterpret_cast<clay::ViewContext*>(lynx_ui_renderer.clayViewProvider.clayViewContext);
  if (config.default_background_color) {
    view_context->GetPageView()->SetDefaultBackgroundColor(
        clay::Color(*config.default_background_color));
  }
  auto module_factory = std::unique_ptr<lynx::runtime::NativeModuleFactory>(
      lynx::LynxModuleFactory::CreateModuleFactory(view_context));
  ui_delegate_ =
      std::make_unique<lynx::tasm::UIDelegateClay>(view_context, std::move(module_factory));
  lynx_ui_renderer_ = (__bridge_retained void*)lynx_ui_renderer;
  CGRect frame = CGRectMake(0, 0, config.width, config.height);
  [lynx_ui_renderer setFrame:frame];
  [lynx_ui_renderer setParent:config.parent];

  clay::NativeViewServiceEmbedder::SetViewFactories(view_context,
                                                    std::move(config.native_view_creators));

  if (config.resource_provider) {
    resource_provider_ = std::move(config.resource_provider);
    std::weak_ptr<ResourceProvider> weak_provider = resource_provider_;
    [lynx_ui_renderer.clayViewProvider
        setInterceptUrlCallback:^NSString*(NSString* originUrl, BOOL shouldDecode,
                                           NSInteger maxPathLength) {
          auto provider = weak_provider.lock();
          if (!provider) return originUrl;
          auto redirected = provider->Intercept([originUrl UTF8String], shouldDecode);
          NSString* redirectedUrl = [NSString stringWithUTF8String:redirected.c_str()];
          if (!redirectedUrl) {
            return nil;
          }
          return redirectedUrl;
        }];
    view_context->GetServiceManager()->RegisterService<clay::ResourceLoaderCreatorService>(
        std::make_shared<clay::ResourceLoaderCreatorService>(
            [weak_provider](fml::RefPtr<fml::TaskRunner> task_runner,
                            std::shared_ptr<clay::ResourceLoaderIntercept> intercept)
                -> std::shared_ptr<clay::ResourceLoader> {
              auto provider = weak_provider.lock();
              if (!provider) return nullptr;
              return std::make_shared<clay::ResourceLoaderEmbedder>(task_runner, intercept,
                                                                    provider);
            }));
  }

  std::shared_ptr<clay::ServiceManager> service_manager = view_context->GetServiceManager();
  if (service_manager) {
    clay::Puppet<clay::Owner::kPlatform, clay::InstrumentationService> instrumentation_service =
        service_manager->GetService<clay::InstrumentationService>();
    instrumentation_service.Act([frame_timing_listener = frame_timing_listener_](auto& impl) {
      impl.AddFrameTimingListener(frame_timing_listener);
    });
  }
}

LynxUIRendererImpl::~LynxUIRendererImpl() {
  if (lynx_ui_renderer_) {
    LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
    [lynx_ui_renderer.clayViewProvider setInterceptUrlCallback:nil];
    [lynx_ui_renderer.clayViewProvider setParent:nil];
  }
  ui_delegate_.reset();
  if (lynx_ui_renderer_) {
    (void)(__bridge_transfer id)lynx_ui_renderer_;
  }
  frame_timing_listener_->SetClient(nullptr);
}

void LynxUIRendererImpl::SetParent(NativeWindow parent) {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  [lynx_ui_renderer setParent:parent];
}

NativeWindow LynxUIRendererImpl::GetNativeWindow() {
  if (!lynx_ui_renderer_) {
    return nullptr;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  return (__bridge NativeWindow)lynx_ui_renderer.clayViewProvider.flutterView;
}

void LynxUIRendererImpl::SetFrame(float x, float y, float width, float height) {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  CGRect frame = CGRectMake(x, y, width, height);
  [lynx_ui_renderer setFrame:frame];
}

void LynxUIRendererImpl::Reset() {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  auto* view_context =
      reinterpret_cast<clay::ViewContext*>(lynx_ui_renderer.clayViewProvider.clayViewContext);
  if (view_context) {
    view_context->ResetPageView();
  }
}

void LynxUIRendererImpl::OnEnterForeground() {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  [lynx_ui_renderer.clayViewProvider onEnterForeground];
}

void LynxUIRendererImpl::OnEnterBackground() {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  [lynx_ui_renderer.clayViewProvider onEnterBackground];
}

void LynxUIRendererImpl::EmulateMouseEvent(const char* event_name, float x, float y, float delta_x,
                                           float delta_y) {
  (void)delta_x;
  (void)delta_y;
  if (!lynx_ui_renderer_ || !event_name) {
    return;
  }

  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  DispatchEmulatedMouseEvent(lynx_ui_renderer, event_name, x, y);
}

void LynxUIRendererImpl::Focus(int node_id) {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  auto* view_context =
      reinterpret_cast<clay::ViewContext*>(lynx_ui_renderer.clayViewProvider.clayViewContext);
  if (!view_context) {
    return;
  }
  view_context->InvokeUIMethod(node_id, "focus", {},
                               [node_id](clay::LynxUIMethodResult code, clay::Value) {
                                 if (code != clay::LynxUIMethodResult::kSuccess) {
                                   FML_LOG(WARNING) << "DOM.focus failed for nodeId: " << node_id
                                                    << ", code: " << static_cast<int>(code);
                                 }
                               });
}

void LynxUIRendererImpl::InsertText(const std::string& text) {
  if (!lynx_ui_renderer_) {
    return;
  }
  NSString* ns_text = [[NSString alloc] initWithBytes:text.data()
                                               length:text.size()
                                             encoding:NSUTF8StringEncoding];
  if (!ns_text) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  FlutterTextInputPlugin* text_input_plugin = lynx_ui_renderer.clayViewProvider.textInputPlugin;
  if (![text_input_plugin isFirstResponder]) {
    return;
  }
  [text_input_plugin insertText:ns_text replacementRange:NSMakeRange(NSNotFound, 0)];
}

void LynxUIRendererImpl::InjectBubbleEvent(const char* params) {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;

  rapidjson::Document dom;
  dom.Parse(params);
  if (dom.HasParseError()) {
    return;
  }
  std::string name;
  float x = 0.0f;
  float y = 0.0f;
  if (dom.HasMember("name") && dom["name"].IsString()) {
    name = dom["name"].GetString();
  }
  if (dom.HasMember("params")) {
    if (dom["params"].HasMember("pageX") && dom["params"]["pageX"].IsNumber()) {
      x = dom["params"]["pageX"].GetDouble();
    }
    if (dom["params"].HasMember("pageY") && dom["params"]["pageY"].IsNumber()) {
      y = dom["params"]["pageY"].GetDouble();
    }
  }
  DispatchEmulatedMouseEvent(lynx_ui_renderer, name, x, y);
}

void LynxUIRendererImpl::RegisterNativeView(const char* name, lynx_native_view_creator creator,
                                            void* opaque) {
  if (!lynx_ui_renderer_) {
    return;
  }

  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  auto* view_context =
      reinterpret_cast<clay::ViewContext*>(lynx_ui_renderer.clayViewProvider.clayViewContext);
  clay::NativeViewServiceEmbedder::AddViewFactory(view_context, name, creator, opaque);
  // A native view factory was registered.  If the overlay already exists,
  // re-insert it on top so that a new factory whose view is created later
  // does not end up above the overlay (corner case C2).
  [lynx_ui_renderer.clayViewProvider ensureOverlayOnTopmost:nil];
}

lynx::tasm::UIDelegate* LynxUIRendererImpl::GetUIDelegate() { return ui_delegate_.get(); }

void LynxUIRendererImpl::RegisterIMEHandler(void* handler, void* opaque) {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  [lynx_ui_renderer RegisterIMEHandler:handler arg:opaque];
}

void LynxUIRendererImpl::SendPointerEvent(const ClayPointerEvent& event) {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  [lynx_ui_renderer.clayViewProvider.engine sendPointerEvent:event];
}

void LynxUIRendererImpl::SetFrameTimingClient(FrameTimingClient* client) {
  frame_timing_listener_->SetClient(client);
}

}  // namespace embedder
}  // namespace lynx
