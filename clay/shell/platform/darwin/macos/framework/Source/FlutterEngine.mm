// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterEngine.h"

#include <algorithm>
#include <iostream>
#include <vector>

#import "clay/shell/platform/darwin/macos/framework/Source/ClayViewProvider_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterEngine_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterPlatformViewController.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterRenderer.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterViewController_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterViewEngineProvider.h"

#include "clay/common/service/service_manager.h"
#include "clay/fml/logging.h"
#include "clay/shell/common/services/drag_drop_service.h"
#include "clay/shell/common/services/gesture_mediate_service.h"
#include "clay/shell/common/switches.h"
#include "clay/shell/platform/common/engine_switches.h"
#include "clay/shell/platform/darwin/common/clay_service_manager_service_darwin.h"
#include "clay/shell/platform/embedder/embedder_engine.h"
#include "clay/shell/platform/embedder/platform_view_embedder.h"
#include "clay/shell/platform/embedder/platform_view_embedder_delegate.h"

/// The private notification for voice over.
static NSString* const kEnhancedUserInterfaceNotification =
    @"NSApplicationDidChangeAccessibilityEnhancedUserInterfaceNotification";
static NSString* const kEnhancedUserInterfaceKey = @"AXEnhancedUserInterface";

/// Clipboard plain text format.
constexpr char kTextPlainFormat[] = "text/plain";

NSString* const kDragTextType = @"drag_text";
NSString* const kDragFileType = @"drag_file";
NSString* const kDragDropPathKey = @"path";
NSString* const kDragDropNameKey = @"name";
NSString* const kDragDropTypeKey = @"type";
NSString* const kDragDropSizeKey = @"size";
NSString* const kDragDropLastModifiedKey = @"lastModified";

#pragma mark -

/**
 * Private interface declaration for FlutterEngine.
 */
@interface FlutterEngine ()

/**
 * A mutable array that holds one bool value that determines if responses to platform messages are
 * clear to execute. This value should be read or written only inside of a synchronized block and
 * will return `NO` after the FlutterEngine has been dealloc'd.
 */
@property(nonatomic, strong) NSMutableArray<NSNumber*>* isResponseValid;

/**
 * Sends the list of user-preferred locales to the Flutter engine.
 */
- (void)sendUserLocales;

/**
 * Invoked right before the engine is restarted.
 *
 * This should reset states to as if the application has just started.  It
 * usually indicates a hot restart (Shift-R in Flutter CLI.)
 */
- (void)engineCallbackOnPreEngineRestart;

/**
 * Requests that the task be posted back the to the Flutter engine at the target time. The target
 * time is in the clock used by the Flutter engine.
 */
- (void)postMainThreadTask:(ClayTask)task targetTimeInNanoseconds:(uint64_t)targetTime;

@end

#pragma mark -

/**
 * `FlutterPluginRegistrar` implementation handling a single plugin.
 */
@interface FlutterEngineRegistrar : NSObject <FlutterPluginRegistrar>
- (instancetype)initWithPlugin:(nonnull NSString*)pluginKey
                 flutterEngine:(nonnull FlutterEngine*)flutterEngine;
@end

@implementation FlutterEngineRegistrar {
  NSString* _pluginKey;
  FlutterEngine* _flutterEngine;
}

- (instancetype)initWithPlugin:(NSString*)pluginKey flutterEngine:(FlutterEngine*)flutterEngine {
  self = [super init];
  if (self) {
    _pluginKey = [pluginKey copy];
    _flutterEngine = flutterEngine;
  }
  return self;
}

- (NSView*)view {
  if (!_flutterEngine.viewController.viewLoaded) {
    [_flutterEngine.viewController loadView];
  }
  return _flutterEngine.viewController.flutterView;
}

- (void)registerViewFactory:(nonnull NSObject<FlutterPlatformViewFactory>*)factory
                     withId:(nonnull NSString*)factoryId {
  [[_flutterEngine platformViewController] registerViewFactory:factory withId:factoryId];
}

@end

#pragma mark - Static methods provided to engine configuration

class ClayPlatformViewMacDelegate : public clay::PlatformViewEmbedderDelegate {
 public:
  ClayPlatformViewMacDelegate() = default;
  virtual ~ClayPlatformViewMacDelegate() = default;
  void SetInterceptUrlCallback(FlutterViewShouldInterceptUrlCallback callback) {
    auto intercept_callback = [callback](const char* origin_url, bool should_decode,
                                         char* intercept_url, int max_path_length) {
      if (callback) {
        NSString* origin_url_str = [NSString stringWithUTF8String:origin_url];
        NSString* intercept_url_str = nil;
        intercept_url_str = callback(origin_url_str, should_decode, max_path_length);
        if (intercept_url_str) {
          std::string temp_url = [intercept_url_str UTF8String];
          strcpy(intercept_url, temp_url.c_str());
        }
      }
    };
    this->BindShouldInterceptUrlCallback(intercept_callback);
  }
};

@implementation FlutterEngine {
  // The embedder engine object.
  std::unique_ptr<clay::EmbedderEngine> _engine;

  std::shared_ptr<clay::ServiceManager> _service_manager;

  // Whether the engine can continue running after the view controller is removed.
  BOOL _allowHeadlessExecution;

  FlutterViewEngineProvider* _viewProvider;

  // Used to support creation and deletion of platform views and registering platform view
  // factories. Lifecycle is tied to the engine.
  FlutterPlatformViewController* _platformViewController;

  ClayPlatformViewMacDelegate _platformViewDelegate;

  ClayPointerEvent _lastScrollEvent;
}

- (instancetype)initWithName:(NSString*)labelPrefix {
  return [self initWithName:labelPrefix allowHeadlessExecution:YES];
}

- (instancetype)initWithName:(NSString*)labelPrefix
      allowHeadlessExecution:(BOOL)allowHeadlessExecution {
  self = [super init];
  NSAssert(self, @"Super init cannot be nil");

  _lastScrollEvent.signal_kind = kClayPointerSignalKindNone;
  _allowHeadlessExecution = allowHeadlessExecution;
  _viewProvider = [[FlutterViewEngineProvider alloc] initWithEngine:self];
  _isResponseValid = [[NSMutableArray alloc] initWithCapacity:1];
  [_isResponseValid addObject:@YES];

  _renderer = [[FlutterRenderer alloc] initWithFlutterEngine:self];

  NSNotificationCenter* notificationCenter = [NSNotificationCenter defaultCenter];
  [notificationCenter addObserver:self
                         selector:@selector(sendUserLocales)
                             name:NSCurrentLocaleDidChangeNotification
                           object:nil];

  _platformViewController = [[FlutterPlatformViewController alloc] init];
  [self setUpNotificationCenterListeners];

  return self;
}

- (void)dealloc {
  @synchronized(_isResponseValid) {
    [_isResponseValid removeAllObjects];
    [_isResponseValid addObject:@NO];
  }
  [self shutDownEngine];
}

- (void)setViewProvider:(ClayViewProvider*)viewProvider {
  _clayViewProvider = viewProvider;
}

- (BOOL)runWithEntrypoint:(NSString*)entrypoint {
  if (self.running) {
    return NO;
  }

  if (!_allowHeadlessExecution && !_view) {
    FML_LOG(ERROR)
        << "Attempted to run an engine with no view controller without headless mode enabled.";
    return NO;
  }

  // The first argument of argv is required to be the executable name.
  std::vector<const char*> argv = {[self.executableName UTF8String]};

  fml::CommandLine command_line;
  if (!argv.empty()) {
    command_line = fml::CommandLineFromArgcArgv(argv.size(), argv.data());
  }
  clay::Settings settings = clay::SettingsFromCommandLine(command_line);
  const char* icu_data_path =
      [[NSBundle mainBundle] pathForResource:@"icudtl.dat" ofType:nil].UTF8String;
  if (icu_data_path) {
    settings.icu_data_path = icu_data_path;
  }
  clay::PlatformViewEmbedder::PlatformDispatchTable platform_dispatch_table = {};
  platform_dispatch_table.clipboard.set_clipboard_data_callback =
      [self](const std::u16string& data) {
        std::string u8string = lynx::base::U16StringToU8(data);
        NSString* dataString = [NSString stringWithUTF8String:u8string.c_str()];
        NSDictionary* dataDict = @{@"text" : dataString};
        [self setClipboardData:dataDict];
      };
  platform_dispatch_table.clipboard.get_clipboard_data_callback = [self]() {
    NSDictionary* clipboardData = [self getClipboardData:@(kTextPlainFormat)];
    NSString* text = clipboardData[@"text"];
    if (text) {
      return lynx::base::U8StringToU16([text UTF8String]);
    }
    return std::u16string();
  };

  platform_dispatch_table.textinput.set_text_input_client_callback =
      [self](int client_id, const char* input_action, const char* input_type) {
        if (self.clayViewProvider && self.clayViewProvider.textInputPlugin) {
          [self.clayViewProvider.textInputPlugin setTextInputClient:client_id
                                                        inputAction:@(input_action)
                                                          inputType:@(input_type)];
        }
      };
  platform_dispatch_table.textinput.clear_text_input_client_callback = [self]() {
    if (self.clayViewProvider && self.clayViewProvider.textInputPlugin) {
      [self.clayViewProvider.textInputPlugin clearTextInputClient];
    }
  };
  platform_dispatch_table.textinput.set_editable_transform_callback =
      [self](const float transform[16]) {
        if (self.clayViewProvider && self.clayViewProvider.textInputPlugin) {
          NSMutableArray* transformArray = [NSMutableArray arrayWithCapacity:16];
          for (int i = 0; i < 16; i++) {
            [transformArray addObject:@(transform[i])];
          }
          [self.clayViewProvider.textInputPlugin setEditableTransform:transformArray];
        }
      };
  platform_dispatch_table.textinput.set_editing_state_callback =
      [self](uint64_t selection_base, uint64_t composing_extent,
             const std::string& selection_affinity, const std::string& text,
             bool selection_directional, uint64_t selection_extent, uint64_t composing_base) {
        if (self.clayViewProvider && self.clayViewProvider.textInputPlugin) {
          NSMutableDictionary* state = [NSMutableDictionary dictionary];
          state[@"text"] = @(text.c_str());
          state[@"selectionBase"] = @(selection_base);
          state[@"selectionExtent"] = @(selection_extent);
          state[@"selectionAffinity"] = @(selection_affinity.c_str());
          state[@"selectionIsDirectional"] = @(selection_directional);
          state[@"composingBase"] = @(composing_base);
          state[@"composingExtent"] = @(composing_extent);
          [self.clayViewProvider.textInputPlugin setEditingState:state];
        }
      };
  platform_dispatch_table.textinput.set_caret_rect_callback = [self](float x, float y, float width,
                                                                     float height) {
    if (self.clayViewProvider && self.clayViewProvider.textInputPlugin) {
      NSMutableDictionary* rect = [NSMutableDictionary dictionary];
      rect[@"x"] = @(x);
      rect[@"y"] = @(y);
      rect[@"width"] = @(width);
      rect[@"height"] = @(height);
      [self.clayViewProvider.textInputPlugin updateCaretRect:rect];
    }
  };
  platform_dispatch_table.textinput.set_marked_text_rect_callback = [](float x, float y,
                                                                       float width, float height) {
    // INFO: this method is not implemented in textInputPlugin yet.
  };
  platform_dispatch_table.textinput.show_text_input_callback = [self]() {
    if (self.clayViewProvider && self.clayViewProvider.textInputPlugin) {
      [self.clayViewProvider.textInputPlugin showTextInput];
    }
  };
  platform_dispatch_table.textinput.hide_text_input_callback = [self]() {
    if (self.clayViewProvider && self.clayViewProvider.textInputPlugin) {
      [self.clayViewProvider.textInputPlugin hideTextInput];
    }
  };
  platform_dispatch_table.textinput.input_filter_callback =
      [self](const std::string& input, const std::string pattern) -> std::string {
    if (self.clayViewProvider && self.clayViewProvider.textInputPlugin) {
      NSString* inputString = [NSString stringWithUTF8String:input.c_str()];
      NSString* patternString = [NSString stringWithUTF8String:pattern.c_str()];
      NSString* filteredString = [self.clayViewProvider.textInputPlugin filterInput:inputString
                                                                        withPattern:patternString];
      if (filteredString != nullptr) {
        return std::string([filteredString UTF8String]);
      }
    }
    return input;
  };

  platform_dispatch_table.window_move_callback = []() {
    NSWindow* window = [[NSApplication sharedApplication] mainWindow];
    dispatch_async(dispatch_get_main_queue(), ^{
      [window performWindowDragWithEvent:[window currentEvent]];
    });
  };
  platform_dispatch_table.activate_system_cursor_callback = [self](int type,
                                                                   const std::string& path) {
    if (self.clayViewProvider && self.clayViewProvider.mouseCursorPlugin) {
      [self.clayViewProvider.mouseCursorPlugin activateSystemCursor:type path:path.c_str()];
    }
  };
  platform_dispatch_table.on_pre_engine_restart_callback = [self]() {
    [self engineCallbackOnPreEngineRestart];
  };

  static size_t sTaskRunnerIdentifiers = 0;
  const ClayTaskRunnerDescription cocoa_task_runner_description = {
      .struct_size = sizeof(ClayTaskRunnerDescription),
      .user_data = (void*)CFBridgingRetain(self),
      .runs_task_on_current_thread_callback = [](void* user_data) -> bool {
        return [[NSThread currentThread] isMainThread];
      },
      .post_task_callback = [](ClayTask task, uint64_t target_time_nanos, void* user_data) -> void {
        [((__bridge FlutterEngine*)(user_data)) postMainThreadTask:task
                                           targetTimeInNanoseconds:target_time_nanos];
      },
      .identifier = ++sTaskRunnerIdentifiers,
  };
  const clay::ClayCustomTaskRunners custom_task_runners = {
      .struct_size = sizeof(clay::ClayCustomTaskRunners),
      .platform_task_runner = &cocoa_task_runner_description,
  };

  fml::RefPtr<clay::EmbedderSurfaceMetal> embedderSurfaceMetal =
      [_renderer createEmbedderSurfaceMetal];
  _engine = clay::EmbedderEngine::CreateEngine(settings, &custom_task_runners, embedderSurfaceMetal,
                                               platform_dispatch_table, &_platformViewDelegate,
                                               (__bridge void*)(self));
  if (!_engine) {
    FML_LOG(ERROR) << "Failed to initialize Flutter engine";
    return NO;
  }

  _service_manager = _engine->GetServiceManager();
  if (!_service_manager) {
    FML_LOG(ERROR) << "Failed to get clay service manager";
    return NO;
  }
  _service_manager->RegisterService<clay::GestureMediateService>(
      std::make_shared<clay::GestureMediateService>());

  // The engine must not already be running. Initialize may only be called
  // once on an engine instance.
  if (_engine->IsValid()) {
    FML_LOG(ERROR) << "Engine handle was invalid.";
    return NO;
  }

  // Step 1: Launch the shell.
  if (!_engine->LaunchShell()) {
    FML_LOG(ERROR) << "Could not launch the engine using supplied initialization arguments.";
    return NO;
  }

  // Step 2: Tell the platform view to initialize itself.
  if (!_engine->NotifyCreated()) {
    FML_LOG(ERROR) << "Could not create platform view components.";
    return NO;
  }
  _service_manager->RegisterService<clay::ClayServiceManagerServiceDarwin>(
      std::make_shared<clay::ClayServiceManagerServiceDarwin>(
          [_clayViewProvider GetClayServiceManager]));

  [self sendUserLocales];
  [self updateWindowMetrics];
  [self updateDisplayConfig];
  // Send the initial user settings such as brightness and text scale factor
  // to the engine.
  [self sendInitialSettings];
  return YES;
}

- (void)setViewController:(FlutterViewController*)controller {
  if (_viewController != controller) {
    _viewController = controller;
    _view = controller.flutterView;

    if (!controller && !_allowHeadlessExecution) {
      [self shutDownEngine];
    }
  }
}

- (void)setView:(FlutterView*)view {
  if (_view) {
    [_view shutdown];
  }
  _viewController = nil;
  _view = view;

  if (!view && !_allowHeadlessExecution) {
    [self shutDownEngine];
  }
}

#pragma mark - Framework-internal methods

- (BOOL)running {
  return _engine != nullptr;
}

- (void)updateDisplayConfig {
  if (!_engine) {
    return;
  }

  CVDisplayLinkRef displayLinkRef;
  CGDirectDisplayID mainDisplayID = CGMainDisplayID();
  CVDisplayLinkCreateWithCGDisplay(mainDisplayID, &displayLinkRef);
  CVTime nominal = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(displayLinkRef);
  if (!(nominal.flags & kCVTimeIsIndefinite)) {
    double refreshRate = static_cast<double>(nominal.timeScale) / nominal.timeValue;

    std::vector<std::unique_ptr<clay::Display>> displays;
    displays.push_back(std::make_unique<clay::Display>(mainDisplayID, round(refreshRate)));
    _engine->GetShell().OnDisplayUpdates(clay::DisplayUpdateType::kStartup, std::move(displays));
  }

  CVDisplayLinkRelease(displayLinkRef);
}

- (void)onSettingsChanged:(NSNotification*)notification {
  // TODO(jonahwilliams): https://github.com/flutter/flutter/issues/32015.
  // NSString* brightness =
  //  [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
}

- (void)sendInitialSettings {
  // TODO(jonahwilliams): https://github.com/flutter/flutter/issues/32015.
  [[NSDistributedNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(onSettingsChanged:)
             name:@"AppleInterfaceThemeChangedNotification"
           object:nil];
  [self onSettingsChanged:nil];
}

- (nonnull NSString*)executableName {
  return [[[NSProcessInfo processInfo] arguments] firstObject] ?: @"Flutter";
}

- (void)updateWindowMetrics {
  if (!_engine) {
    return;
  }
  NSView* view = _view;
  CGRect scaledBounds = [view convertRectToBacking:view.bounds];
  CGSize scaledSize = scaledBounds.size;
  double pixelRatio = view.bounds.size.width == 0 ? 1 : scaledSize.width / view.bounds.size.width;

  clay::ViewportMetrics metrics;

  metrics.physical_width = static_cast<size_t>(scaledSize.width);
  metrics.physical_height = static_cast<size_t>(scaledSize.height);
  metrics.device_pixel_ratio = pixelRatio;
  // Default logical pixel is 96
  metrics.device_density_dpi = pixelRatio * 96.f;
  metrics.physical_view_inset_top = 0.0;
  metrics.physical_view_inset_right = 0.0;
  metrics.physical_view_inset_bottom = 0.0;
  metrics.physical_view_inset_left = 0.0;
  _engine->SetViewportMetrics(metrics);
}

- (void)sendPointerEvent:(const ClayPointerEvent&)event {
  if (event.signal_kind == kClayPointerSignalKindScroll) {
    [self sendScrollEvent:event];
    return;
  }
  _engine->SendPointerEvent(&event, 1);
}

- (void)sendScrollEvent:(const ClayPointerEvent&)event {
  if (_lastScrollEvent.signal_kind == kClayPointerSignalKindScroll) {
    _lastScrollEvent.scroll_delta_x += event.scroll_delta_x;
    _lastScrollEvent.scroll_delta_y += event.scroll_delta_y;
    return;
  }
  _lastScrollEvent = event;

  const int kVSyncInterval = 16 * 1000 * 1000;  // 16ms
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, kVSyncInterval), dispatch_get_main_queue(), ^{
    if (_lastScrollEvent.signal_kind == kClayPointerSignalKindNone) {
      return;
    }
    _engine->SendPointerEvent(&_lastScrollEvent, 1);
    _lastScrollEvent.signal_kind = kClayPointerSignalKindNone;
  });
}

- (void)sendKeyEvent:(const ClayKeyEvent&)event
            callback:(ClayKeyEventCallback)callback
            userData:(void*)userData {
  _engine->SendKeyEvent(&event, callback, userData);
}

- (FlutterPlatformViewController*)platformViewController {
  return _platformViewController;
}

#pragma mark - Private methods

- (void)sendUserLocales {
}

- (void)engineCallbackOnPreEngineRestart {
  if (_viewController) {
    [_viewController onPreEngineRestart];
  }
}

/**
 * Note: Called from dealloc. Should not use accessors or other methods.
 */
- (void)shutDownEngine {
  if (_engine == nullptr) {
    return;
  }

  if (_view) {
    [_view shutdown];
  }
  _engine->NotifyDestroyed();
  _engine->CollectShell();

  // Balancing release for the retain in the task runner dispatch table.
  CFRelease((CFTypeRef)self);
  _engine = nullptr;
}

- (void)setUpNotificationCenterListeners {
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  // macOS fires this private message when VoiceOver turns on or off.
  [center addObserver:self
             selector:@selector(applicationWillTerminate:)
                 name:NSApplicationWillTerminateNotification
               object:nil];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
  [self shutDownEngine];
}

- (void)playSystemSound:(NSString*)soundType {
  if ([soundType isEqualToString:@"SystemSoundType.alert"]) {
    NSBeep();
  }
}

- (NSDictionary*)getClipboardData:(NSString*)format {
  NSPasteboard* pasteboard = self.pasteboard;
  if ([format isEqualToString:@(kTextPlainFormat)]) {
    NSString* stringInPasteboard = [pasteboard stringForType:NSPasteboardTypeString];
    return stringInPasteboard == nil ? nil : @{@"text" : stringInPasteboard};
  }
  return nil;
}

- (void)setClipboardData:(NSDictionary*)data {
  NSPasteboard* pasteboard = self.pasteboard;
  NSString* text = data[@"text"];
  [pasteboard clearContents];
  if (text && ![text isEqual:[NSNull null]]) {
    [pasteboard setString:text forType:NSPasteboardTypeString];
  }
}

- (BOOL)clipboardHasStrings {
  return [self.pasteboard stringForType:NSPasteboardTypeString].length > 0;
}

- (NSPasteboard*)pasteboard {
  return [NSPasteboard generalPasteboard];
}

- (nullable void*)clayViewContext {
  if (_engine == nullptr) {
    return nullptr;
  }

  return _engine->GetShell().GetEngine()->GetViewContext();
}

- (void)onEnterForeground {
  if (_engine == nullptr) {
    return;
  }
  _engine->GetShell().GetEngine()->OnEnterForeground();
}

- (void)onEnterBackground {
  if (_engine == nullptr) {
    return;
  }
  _engine->GetShell().GetEngine()->OnEnterBackground();
}

- (void)setVisible:(BOOL)visible {
  if (!_engine) {
    return;
  }
  _engine->GetShell().GetEngine()->SetVisible(visible);
}

- (void)setInterceptUrlCallback:(FlutterViewShouldInterceptUrlCallback _Nullable)callback {
  _platformViewDelegate.SetInterceptUrlCallback(callback);
}

- (void)updateEditState:(int)client_id
          selectionBase:(uint64_t)selection_base
        composingExtent:(uint64_t)composing_extent
      selectionAffinity:(const char*)selection_affinity
                   text:(const char*)text
        selectionExtent:(uint64_t)selection_extent
          composingBase:(uint64_t)composing_base {
  _engine->GetShell().GetEngine()->GetPageView()->OnPlatformUpdateEditState(
      client_id, selection_base, composing_extent, selection_affinity, text, selection_extent,
      composing_base);
}

- (void)performInputAction:(int)client_id {
  _engine->GetShell().GetEngine()->GetPageView()->OnPlatformPerformInputAction(client_id);
}

- (void)performMouseDragLeave {
  clay::Puppet<clay::Owner::kPlatform, clay::DragDropService> drag_drop_service =
      _service_manager->GetService<clay::DragDropService>();
  drag_drop_service.Act([](auto& impl) { impl.OnPlatformDragLeave(); });
}

- (void)performMouseDragEnterAndOverAtPoint:(NSPoint)point {
  clay::Puppet<clay::Owner::kPlatform, clay::DragDropService> drag_drop_service =
      _service_manager->GetService<clay::DragDropService>();
  drag_drop_service.Act(
      [point](auto& impl) { impl.OnPlatformDragEnterAndOver(clay::FloatPoint(point.x, point.y)); });
}

- (void)performMouseDragDropAtPoint:(NSPoint)point type:(NSString*)type dropContent:(id)content {
  std::string typeStr = "";
  // content maybe a string or an array.
  std::string contentStr = "";
  std::list<std::unordered_map<std::string, std::string>> contentList;
  if ([content isKindOfClass:[NSString class]]) {
    typeStr = clay::kDragTextType;
    contentStr = [(NSString*)content UTF8String];
  } else if ([content isKindOfClass:[NSArray class]]) {
    typeStr = clay::kDragFileType;
    NSArray* contentArrayObj = (NSArray*)content;
    for (id item in contentArrayObj) {
      if ([item isKindOfClass:[NSDictionary class]]) {
        NSDictionary* dict = (NSDictionary*)item;
        std::unordered_map<std::string, std::string> mapItem;
        mapItem[clay::kDragDropPathKey] =
            [(NSString*)[dict objectForKey:kDragDropPathKey] UTF8String];
        mapItem[clay::kDragDropNameKey] =
            [(NSString*)[dict objectForKey:kDragDropNameKey] UTF8String];
        mapItem[clay::kDragDropTypeKey] =
            [(NSString*)[dict objectForKey:kDragDropTypeKey] UTF8String];
        mapItem[clay::kDragDropSizeKey] =
            [[(NSNumber*)[dict objectForKey:kDragDropSizeKey] stringValue] UTF8String];
        mapItem[clay::kDragDropLastModifiedKey] =
            [(NSString*)[dict objectForKey:kDragDropLastModifiedKey] UTF8String];
        contentList.push_back(mapItem);
      }
    }
  }
  clay::Puppet<clay::Owner::kPlatform, clay::DragDropService> drag_drop_service =
      _service_manager->GetService<clay::DragDropService>();
  drag_drop_service.Act([&](auto& impl) {
    impl.OnPlatformDragDrop(clay::FloatPoint(point.x, point.y), typeStr, contentStr, contentList);
  });
}

#pragma mark - FlutterPluginRegistry

- (id<FlutterPluginRegistrar>)registrarForPlugin:(NSString*)pluginName {
  return [[FlutterEngineRegistrar alloc] initWithPlugin:pluginName flutterEngine:self];
}

#pragma mark - Task runner integration

- (void)runTaskOnEmbedder:(ClayTask)task {
  if (!_engine) {
    return;
  }
  if (!_engine->RunTask(&task)) {
    FML_LOG(ERROR) << "Could not post a task to the Flutter engine.";
  }
}

- (void)postMainThreadTask:(ClayTask)task targetTimeInNanoseconds:(uint64_t)targetTime {
  __weak FlutterEngine* weakSelf = self;
  auto worker = ^{
    [weakSelf runTaskOnEmbedder:task];
  };

  const auto engine_time = fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
  if (targetTime <= engine_time) {
    dispatch_async(dispatch_get_main_queue(), worker);

  } else {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, targetTime - engine_time),
                   dispatch_get_main_queue(), worker);
  }
}

- (BOOL)postUIThreadTask:(const fml::closure&)callback {
  struct Captures {
    fml::closure callback;
  };
  auto captures = new Captures();
  captures->callback = std::move(callback);
  FlutterEngine* strong_self = self;
  if (strong_self && strong_self->_engine) {
    auto task = [captures]() {
      captures->callback();
      delete captures;
    };
    if (_engine->PostUIThreadTask(std::move(task))) {
      return true;
    }
  }
  delete captures;
  return false;
}

@end
