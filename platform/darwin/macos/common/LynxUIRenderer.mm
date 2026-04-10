// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "platform/darwin/macos/common/LynxUIRenderer.h"
#include "clay/common/service/service_manager.h"
#include "clay/lynx_adaptor/native_module/lynx_module_factory.h"
#include "clay/lynx_adaptor/native_platform_view.h"
#include "clay/lynx_adaptor/native_view_service_desktop.h"
#include "clay/lynx_adaptor/resource_loader_embedder.h"
#include "clay/net/loader/resource_loader_creator_service.h"
#include "clay/shell/common/services/instrumentation_service.h"
#include "clay/shell/platform/darwin/macos/framework/Headers/ClayViewProvider.h"
#include "clay/ui/component/view_context.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "platform/embedder/lynx_view_clients.h"
#include "platform/embedder/public/capi/lynx_generic_resource_fetcher_capi.h"
#include "platform/embedder/public/capi/lynx_memory_capi.h"
#if ENABLE_TESTBENCH_REPLAY
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"
#endif

@interface LynxUIRendererMac : NSObject

@property(nonatomic) ClayViewProvider* clayViewProvider;

@end

@implementation LynxUIRendererMac : NSObject

- (instancetype)init {
  self = [super init];
#if ENABLE_TESTBENCH_RECORDER
  // Set storage directory for testbench recorder.
  if (lynx::tasm::LynxEnv::GetInstance().GetStorageDirectory().empty()) {
    NSString* filePath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask,
                                                              YES) lastObject];
    if (filePath) {
      lynx::tasm::LynxEnv::GetInstance().SetStorageDirectory([filePath UTF8String]);
    }
  }
#endif
  if (self) {
    _clayViewProvider = [[ClayViewProvider alloc] init];
  }
  return self;
}

- (void)dealloc {
}

- (void)setFrame:(CGRect)frame {
  _clayViewProvider.flutterView.frame = frame;
}

- (void)setParent:(NativeWindow)parent {
  if (!parent) {
    return;
  }
  NSView* view = (__bridge NSView*)parent;
  [view addSubview:_clayViewProvider.flutterView];
}

- (void)RegisterIMEHandler:(void*)handler arg:(void*)arg {
  [self.clayViewProvider requestIME:handler arg:arg];
}

#if ENABLE_TESTBENCH_REPLAY
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
      [NSEvent enterExitEventWithType:NSEventTypeMouseMoved
                             location:locationInWindow
                        modifierFlags:0
                            timestamp:0
                         windowNumber:[[self.clayViewProvider.flutterView window] windowNumber]
                              context:nil
                          eventNumber:0
                       trackingNumber:0
                             userData:NULL];
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
#endif

@end

namespace lynx {
namespace embedder {

std::unique_ptr<LynxUIRenderer> LynxUIRenderer::CreateWithBuilder(lynx_view_builder_t* builder) {
  return std::make_unique<LynxUIRendererImpl>(builder);
}

LynxUIRendererImpl::LynxUIRendererImpl(lynx_view_builder_t* builder)
    : LynxUIRenderer(builder), frame_timing_listener_(std::make_shared<FrameTimingListenerImpl>()) {
  // init ui thread native loop
  if (NSThread.isMainThread) {
    base::UIThread::Init();
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      base::UIThread::Init();
    });
  }

  auto lynx_ui_renderer = [[LynxUIRendererMac alloc] init];
  auto* view_context =
      reinterpret_cast<clay::ViewContext*>(lynx_ui_renderer.clayViewProvider.clayViewContext);
  auto module_factory = std::unique_ptr<lynx::runtime::NativeModuleFactory>(
      lynx::LynxModuleFactory::CreateModuleFactory(view_context));
  ui_delegate_ =
      std::make_unique<lynx::tasm::UIDelegateClay>(view_context, std::move(module_factory));
  lynx_ui_renderer_ = (__bridge_retained void*)lynx_ui_renderer;
  CGRect frame = CGRectMake(0, 0, builder->frame.width, builder->frame.height);
  [lynx_ui_renderer setFrame:frame];
  [lynx_ui_renderer setParent:builder->parent];

  clay::NativeViewServiceDesktop::SetViewFactories(view_context,
                                                   std::move(builder->native_view_creators));

  if (builder->generic_fetcher) {
    auto fetcher_holder = std::make_shared<LynxResourceFetcherHolder>(builder->generic_fetcher);
    [lynx_ui_renderer.clayViewProvider
        setInterceptUrlCallback:^NSString*(NSString* originUrl, BOOL shouldDecode,
                                           NSInteger maxPathLength) {
          char* redirected = lynx_generic_resource_fetcher_intercept(
              fetcher_holder->GenericFetcher(), [originUrl UTF8String], shouldDecode);
          if (!redirected) {
            return nil;
          }
          NSString* redirectedUrl = [NSString stringWithUTF8String:redirected];
          lynx_free(redirected);
          if (!redirectedUrl) {
            return nil;
          }
          if (maxPathLength > 0 && redirectedUrl.length >= maxPathLength) {
            return [redirectedUrl substringToIndex:maxPathLength - 1];
          }
          return redirectedUrl;
        }];
    view_context->GetServiceManager()->RegisterService<clay::ResourceLoaderCreatorService>(
        std::make_shared<clay::ResourceLoaderCreatorService>(
            [fetcher_holder](fml::RefPtr<fml::TaskRunner> task_runner,
                             std::shared_ptr<clay::ResourceLoaderIntercept> intercept) {
              return std::make_shared<clay::ResourceLoaderEmbedder>(task_runner, intercept,
                                                                    fetcher_holder);
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
  ui_delegate_.reset();
  if (lynx_ui_renderer_) {
    (void)(__bridge_transfer id)lynx_ui_renderer_;
  }
  frame_timing_listener_->RemoveAllClients();
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

void LynxUIRendererImpl::InjectBubbleEvent(const char* params) {
#if ENABLE_TESTBENCH_REPLAY
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
#endif
}

void LynxUIRendererImpl::RegisterNativeView(const char* name, lynx_native_view_creator creator,
                                            void* opaque) {
  if (!lynx_ui_renderer_) {
    return;
  }

  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  auto* view_context =
      reinterpret_cast<clay::ViewContext*>(lynx_ui_renderer.clayViewProvider.clayViewContext);
  clay::NativeViewServiceDesktop::AddViewFactory(view_context, name, creator, opaque);
}

lynx::tasm::UIDelegate* LynxUIRendererImpl::GetUIDelegate() { return ui_delegate_.get(); }

void LynxUIRendererImpl::RegisterIMEHandler(void* handler, void* opaque) {
  if (!lynx_ui_renderer_) {
    return;
  }
  LynxUIRendererMac* lynx_ui_renderer = (__bridge LynxUIRendererMac*)lynx_ui_renderer_;
  [lynx_ui_renderer RegisterIMEHandler:handler arg:opaque];
}

void LynxUIRendererImpl::AddClient(LynxViewClients* client) {
  frame_timing_listener_->AddClient(client);
}

}  // namespace embedder
}  // namespace lynx
