// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "oliver/node-lynx/native/windowed_lynx_view_mac.h"

#import <AppKit/AppKit.h>
#import <IOSurface/IOSurface.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#import "clay/shell/platform/darwin/macos/framework/Source/KeyCodeMap_Internal.h"
#include "clay/ui/event/keyboard_key.h"

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace lynx {
namespace node {
class NodeLynxWindowHostMac;
}  // namespace node
}  // namespace lynx

@interface NodeLynxWindowDelegate : NSObject <NSWindowDelegate>
- (instancetype)initWithHost:(lynx::node::NodeLynxWindowHostMac*)host;
@end

@interface NodeLynxMetalView : NSView <NSTextInputClient>
- (instancetype)initWithHost:(lynx::node::NodeLynxWindowHostMac*)host device:(id<MTLDevice>)device;
- (CAMetalLayer*)metalLayer;
- (void)notifyResize;
- (void)setTextInputActive:(BOOL)active;
- (void)setTextInputRect:(NSRect)rect;
@end

namespace {

std::mutex g_appkit_pump_mutex;
uv_timer_t* g_appkit_pump_timer = nullptr;
int g_appkit_pump_ref_count = 0;

uint64_t NowMicros() {
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count());
}

double EventTimestampMicros(NSEvent* event) {
  return event ? static_cast<double>(event.timestamp * 1000000.0)
               : static_cast<double>(NowMicros());
}

uint64_t KeyOfPlane(uint64_t base_key, uint64_t plane) {
  return plane | (base_key & clay::kValueMask);
}

uint64_t ToLower(uint64_t value) {
  constexpr uint64_t lower_a = 0x61;
  constexpr uint64_t upper_a = 0x41;
  constexpr uint64_t upper_z = 0x5a;
  if (value >= upper_a && value <= upper_z) {
    return value - upper_a + lower_a;
  }
  return value;
}

uint32_t FirstCodePoint(NSString* text) {
  if (!text || text.length == 0) {
    return 0;
  }
  unichar first = [text characterAtIndex:0];
  if (first >= 0xD800 && first <= 0xDBFF && text.length > 1) {
    unichar second = [text characterAtIndex:1];
    if (second >= 0xDC00 && second <= 0xDFFF) {
      return ((static_cast<uint32_t>(first) - 0xD800) << 10) +
             (static_cast<uint32_t>(second) - 0xDC00) + 0x10000;
    }
  }
  return first;
}

bool IsControlCharacter(uint32_t character) {
  return (character <= 0x1f) || (character >= 0x7f && character <= 0x9f);
}

bool IsFunctionKeyCharacter(uint32_t character) {
  return character >= 0xF700 && character <= 0xF7FF;
}

uint64_t PhysicalKeyForKeyCode(unsigned short key_code) {
  NSNumber* physical_key = [clay::keyCodeToPhysicalKey objectForKey:@(key_code)];
  if (physical_key) {
    return physical_key.unsignedLongLongValue;
  }
  return KeyOfPlane(key_code, clay::kMacosPlane);
}

uint64_t LogicalKeyForModifier(unsigned short key_code, uint64_t physical_key) {
  NSNumber* logical_key = [clay::keyCodeToLogicalKey objectForKey:@(key_code)];
  if (logical_key) {
    return logical_key.unsignedLongLongValue;
  }
  return KeyOfPlane(physical_key, clay::kMacosPlane);
}

uint64_t LogicalKeyForEvent(NSEvent* event, uint64_t physical_key) {
  NSNumber* logical_key = [clay::keyCodeToLogicalKey objectForKey:@(event.keyCode)];
  if (logical_key) {
    return logical_key.unsignedLongLongValue;
  }
  uint32_t character = FirstCodePoint(event.charactersIgnoringModifiers);
  if (character != 0 && !IsControlCharacter(character) && !IsFunctionKeyCharacter(character)) {
    return KeyOfPlane(ToLower(character), clay::kUnicodePlane);
  }
  return KeyOfPlane(physical_key, clay::kMacosPlane);
}

std::string Utf8StringFromInput(id input) {
  NSString* text = nil;
  if ([input isKindOfClass:[NSAttributedString class]]) {
    text = [input string];
  } else if ([input isKindOfClass:[NSString class]]) {
    text = input;
  }
  if (!text || text.length == 0) {
    return "";
  }
  return std::string(text.UTF8String ?: "");
}

std::string PrintableCharactersForEvent(NSEvent* event) {
  NSString* characters = event.characters;
  uint32_t character = FirstCodePoint(characters);
  if (character == 0 || IsFunctionKeyCharacter(character)) {
    return "";
  }
  return std::string(characters.UTF8String ?: "");
}

bool ShouldInterpretTextInput(NSEvent* event) {
  if (!event || event.type != NSEventTypeKeyDown || event.characters.length == 0) {
    return false;
  }
  if ((event.modifierFlags & NSEventModifierFlagCommand) ||
      (event.modifierFlags & NSEventModifierFlagControl)) {
    return false;
  }
  uint32_t character = FirstCodePoint(event.characters);
  return character != 0 && !IsFunctionKeyCharacter(character);
}

void FillKeyEvent(lynx_key_event_t* key_event, lynx_key_event_type_e type, double timestamp,
                  uint64_t physical, uint64_t logical, const char* character, bool synthesized) {
  *key_event = {};
  key_event->struct_size = sizeof(*key_event);
  key_event->timestamp = timestamp;
  key_event->type = type;
  key_event->physical = physical;
  key_event->logical = logical;
  key_event->character = character;
  key_event->synthesized = synthesized;
}

struct PresentationState {
  std::atomic_bool closed{false};
  __strong NodeLynxMetalView* content_view = nil;
  __strong id<MTLDevice> device = nil;
  __strong id<MTLCommandQueue> command_queue = nil;
};

bool PresentSurfaceOnMain(const std::shared_ptr<PresentationState>& state, IOSurfaceRef surface,
                          uint32_t width, uint32_t height) {
  if (!state || state->closed.load() || !state->content_view || !state->device ||
      !state->command_queue || !surface || width == 0 || height == 0) {
    return false;
  }
  CAMetalLayer* layer = [state->content_view metalLayer];
  if (!layer) {
    return false;
  }
  layer.device = state->device;
  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  layer.framebufferOnly = NO;
  layer.drawableSize = CGSizeMake(width, height);

  id<CAMetalDrawable> drawable = [layer nextDrawable];
  if (!drawable) {
    return false;
  }
  MTLTextureDescriptor* descriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                         width:width
                                                        height:height
                                                     mipmapped:NO];
  descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
  id<MTLTexture> source = [state->device newTextureWithDescriptor:descriptor
                                                        iosurface:surface
                                                            plane:0];
  if (!source) {
    return false;
  }

  id<MTLCommandBuffer> command_buffer = [state->command_queue commandBuffer];
  id<MTLBlitCommandEncoder> blit = [command_buffer blitCommandEncoder];
  MTLSize size = MTLSizeMake(std::min<uint32_t>(width, drawable.texture.width),
                             std::min<uint32_t>(height, drawable.texture.height), 1);
  [blit copyFromTexture:source
            sourceSlice:0
            sourceLevel:0
           sourceOrigin:MTLOriginMake(0, 0, 0)
             sourceSize:size
              toTexture:drawable.texture
       destinationSlice:0
       destinationLevel:0
      destinationOrigin:MTLOriginMake(0, 0, 0)];
  [blit endEncoding];
  [command_buffer presentDrawable:drawable];
  [command_buffer commit];
  return true;
}

bool PresentPixelsOnMain(const std::shared_ptr<PresentationState>& state, const uint8_t* bgra,
                         int width, int height) {
  if (!state || state->closed.load() || !state->content_view || !state->device ||
      !state->command_queue || !bgra || width <= 0 || height <= 0) {
    return false;
  }
  CAMetalLayer* layer = [state->content_view metalLayer];
  if (!layer) {
    return false;
  }
  layer.device = state->device;
  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  layer.framebufferOnly = NO;
  layer.drawableSize = CGSizeMake(width, height);

  id<CAMetalDrawable> drawable = [layer nextDrawable];
  if (!drawable) {
    return false;
  }
  MTLTextureDescriptor* descriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                         width:width
                                                        height:height
                                                     mipmapped:NO];
  descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
  id<MTLTexture> source = [state->device newTextureWithDescriptor:descriptor];
  if (!source) {
    return false;
  }
  MTLRegion region =
      MTLRegionMake2D(0, 0, static_cast<NSUInteger>(width), static_cast<NSUInteger>(height));
  [source replaceRegion:region
            mipmapLevel:0
              withBytes:bgra
            bytesPerRow:static_cast<NSUInteger>(width) * 4];

  id<MTLCommandBuffer> command_buffer = [state->command_queue commandBuffer];
  id<MTLBlitCommandEncoder> blit = [command_buffer blitCommandEncoder];
  MTLSize size = MTLSizeMake(std::min<NSUInteger>(source.width, drawable.texture.width),
                             std::min<NSUInteger>(source.height, drawable.texture.height), 1);
  [blit copyFromTexture:source
            sourceSlice:0
            sourceLevel:0
           sourceOrigin:MTLOriginMake(0, 0, 0)
             sourceSize:size
              toTexture:drawable.texture
       destinationSlice:0
       destinationLevel:0
      destinationOrigin:MTLOriginMake(0, 0, 0)];
  [blit endEncoding];
  [command_buffer presentDrawable:drawable];
  [command_buffer commit];
  return true;
}

void PumpAppKitEvents(uv_timer_t*) {
  @autoreleasepool {
    NSApplication* app = [NSApplication sharedApplication];
    while (true) {
      NSEvent* event = [app nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
      if (!event) {
        break;
      }
      [app sendEvent:event];
    }
    [app updateWindows];
  }
}

bool RetainAppKitPump(uv_loop_t* loop) {
  if (!loop) {
    return false;
  }
  std::lock_guard<std::mutex> lock(g_appkit_pump_mutex);
  ++g_appkit_pump_ref_count;
  if (g_appkit_pump_timer) {
    return true;
  }
  g_appkit_pump_timer = new uv_timer_t;
  if (uv_timer_init(loop, g_appkit_pump_timer) != 0) {
    delete g_appkit_pump_timer;
    g_appkit_pump_timer = nullptr;
    --g_appkit_pump_ref_count;
    return false;
  }
  uv_timer_start(g_appkit_pump_timer, PumpAppKitEvents, 0, 8);
  return true;
}

void ReleaseAppKitPump() {
  std::lock_guard<std::mutex> lock(g_appkit_pump_mutex);
  if (g_appkit_pump_ref_count > 0) {
    --g_appkit_pump_ref_count;
  }
  if (g_appkit_pump_ref_count != 0 || !g_appkit_pump_timer) {
    return;
  }
  uv_timer_stop(g_appkit_pump_timer);
  uv_close(reinterpret_cast<uv_handle_t*>(g_appkit_pump_timer),
           [](uv_handle_t* handle) { delete reinterpret_cast<uv_timer_t*>(handle); });
  g_appkit_pump_timer = nullptr;
}

void EnsureApplicationReady(bool activate) {
  NSApplication* app = [NSApplication sharedApplication];
  if (app.activationPolicy == NSApplicationActivationPolicyProhibited) {
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
  }
  [app finishLaunching];
  if (activate) {
    [app activateIgnoringOtherApps:YES];
  }
}

}  // namespace

namespace lynx {
namespace node {

class NodeLynxWindowHostMac final : public NodeLynxWindowHost {
 public:
  NodeLynxWindowHostMac(uv_loop_t* loop, WindowedLynxViewOptions options,
                        PointerCallback pointer_callback, KeyCallback key_callback,
                        ResizeCallback resize_callback, ClosedCallback closed_callback)
      : loop_(loop),
        options_(std::move(options)),
        pointer_callback_(std::move(pointer_callback)),
        key_callback_(std::move(key_callback)),
        resize_callback_(std::move(resize_callback)),
        closed_callback_(std::move(closed_callback)),
        presentation_state_(std::make_shared<PresentationState>()) {
    device_ = MTLCreateSystemDefaultDevice();
    if (device_) {
      command_queue_ = [device_ newCommandQueue];
    }
    presentation_state_->device = device_;
    presentation_state_->command_queue = command_queue_;
  }

  ~NodeLynxWindowHostMac() override {
    Close();
    ReleasePumpIfNeeded();
  }

  bool Show() override {
    __block bool result = false;
    if ([NSThread isMainThread]) {
      result = EnsureWindowOnMain(true);
    } else {
      dispatch_sync(dispatch_get_main_queue(), ^{
        result = EnsureWindowOnMain(true);
      });
    }
    return result;
  }

  void Close() override {
    if ([NSThread isMainThread]) {
      CloseOnMain();
    } else {
      dispatch_sync(dispatch_get_main_queue(), ^{
        CloseOnMain();
      });
    }
  }

  bool IsClosed() const override { return closed_.load(); }

  bool PresentFrame(const lynx_accelerated_paint_info_t& info) override {
    if (!info.shared_texture_handle || !device_ || !command_queue_) {
      return false;
    }
    IOSurfaceRef surface = static_cast<IOSurfaceRef>(info.shared_texture_handle);
    CFRetain(surface);
    uint32_t width = info.width;
    uint32_t height = info.height;
    auto state = presentation_state_;
    void (^present)() = ^{
      PresentSurfaceOnMain(state, surface, width, height);
      CFRelease(surface);
    };
    if ([NSThread isMainThread]) {
      present();
    } else {
      dispatch_async(dispatch_get_main_queue(), present);
    }
    return true;
  }

  bool PresentPixels(const uint8_t* rgba, int width, int height) override {
    if (!rgba || width <= 0 || height <= 0 || !device_ || !command_queue_) {
      return false;
    }
    auto bgra = std::make_shared<std::vector<uint8_t>>(static_cast<size_t>(width) * height * 4);
    for (int i = 0; i < width * height; ++i) {
      const uint8_t* src = rgba + static_cast<size_t>(i) * 4;
      uint8_t* dst = bgra->data() + static_cast<size_t>(i) * 4;
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = src[3];
    }
    auto state = presentation_state_;
    void (^present)() = ^{
      PresentPixelsOnMain(state, bgra->data(), width, height);
    };
    if ([NSThread isMainThread]) {
      present();
    } else {
      dispatch_async(dispatch_get_main_queue(), present);
    }
    return true;
  }

  void Click(double x, double y) override {
    double scale = CurrentScale();
    SendPointer(kLynxPointerPhaseAdd, x * scale, y * scale, 0, kLynxPointerSignalKindNone, 0, 0,
                false);
    SendPointer(kLynxPointerPhaseDown, x * scale, y * scale, kLynxPointerMouseButtonsMousePrimary,
                kLynxPointerSignalKindNone, 0, 0, false);
    SendPointer(kLynxPointerPhaseUp, x * scale, y * scale, 0, kLynxPointerSignalKindNone, 0, 0,
                false);
  }

  void TypeText(const std::string& text) override {
    if (text.empty()) {
      return;
    }
    lynx_key_event_t key_event{};
    FillKeyEvent(&key_event, kLynxKeyEventTypeDown, static_cast<double>(NowMicros()), 0, 0,
                 text.c_str(), true);
    SendKey(key_event, text);
  }

  void PressKey(const std::string& key) override {
    uint64_t physical = 0;
    uint64_t logical = 0;
    std::string character;
    if (!ResolveAutomationKey(key, &physical, &logical, &character)) {
      return;
    }
    lynx_key_event_t key_down{};
    FillKeyEvent(&key_down, kLynxKeyEventTypeDown, static_cast<double>(NowMicros()), physical,
                 logical, character.empty() ? nullptr : character.c_str(), true);
    SendKey(key_down, character);

    lynx_key_event_t key_up{};
    FillKeyEvent(&key_up, kLynxKeyEventTypeUp, static_cast<double>(NowMicros()), physical, logical,
                 nullptr, true);
    SendKey(key_up, "");
  }

  void SetTextInputActive(bool active) override {
    void (^update)() = ^{
      if (content_view_) {
        [content_view_ setTextInputActive:active ? YES : NO];
        if (active && window_) {
          [window_ makeFirstResponder:content_view_];
        }
      }
    };
    if ([NSThread isMainThread]) {
      update();
    } else {
      dispatch_async(dispatch_get_main_queue(), update);
    }
  }

  void UpdateCaretPosition(float x, float y, float width, float height) override {
    SetTextInputRect(x, y, width, height);
  }

  void SetMarkedTextRect(float x, float y, float width, float height) override {
    SetTextInputRect(x, y, width, height);
  }

  void HandleResize(double width, double height, double scale) {
    if (width <= 0 || height <= 0 || scale <= 0) {
      return;
    }
    if (resize_callback_) {
      resize_callback_(width, height, scale);
    }
  }

  void HandleWindowClosed() {
    if (closed_.load()) {
      return;
    }
    closed_.store(true);
    window_ = nil;
    window_delegate_ = nil;
    content_view_ = nil;
    if (presentation_state_) {
      presentation_state_->closed.store(true);
      presentation_state_->content_view = nil;
    }
    ReleasePumpIfNeeded();
    if (closed_callback_) {
      closed_callback_();
    }
  }

  void HandleMouseEvent(NSEvent* event, lynx_pointer_phase_e phase, int64_t buttons,
                        lynx_pointer_signal_kind_e signal_kind = kLynxPointerSignalKindNone,
                        double scroll_delta_x = 0, double scroll_delta_y = 0,
                        bool precise_scroll = false) {
    NSPoint point = [content_view_ convertPoint:event.locationInWindow fromView:nil];
    double scale = CurrentScale();
    double x = point.x * scale;
    double y = (content_view_.bounds.size.height - point.y) * scale;
    SendPointer(phase, x, y, buttons, signal_kind, scroll_delta_x * scale, scroll_delta_y * scale,
                precise_scroll);
  }

  void HandleKeyDown(NSEvent* event) { SendNativeKeyEvent(event, true); }

  void HandleKeyUp(NSEvent* event) { SendNativeKeyEvent(event, false); }

  void HandleFlagsChanged(NSEvent* event) {
    NSNumber* modifier_flag = [clay::keyCodeToModifierFlag objectForKey:@(event.keyCode)];
    if (!modifier_flag) {
      return;
    }
    uint64_t physical = PhysicalKeyForKeyCode(event.keyCode);
    uint64_t logical = LogicalKeyForModifier(event.keyCode, physical);
    bool is_down = (event.modifierFlags & modifier_flag.unsignedLongValue) != 0;
    auto pressed = pressed_keys_.find(physical);
    if (is_down && pressed != pressed_keys_.end()) {
      return;
    }
    if (!is_down && pressed == pressed_keys_.end()) {
      return;
    }
    lynx_key_event_t key_event{};
    FillKeyEvent(&key_event, is_down ? kLynxKeyEventTypeDown : kLynxKeyEventTypeUp,
                 EventTimestampMicros(event), physical, logical, nullptr, false);
    if (is_down) {
      pressed_keys_[physical] = logical;
    } else {
      pressed_keys_.erase(physical);
    }
    SendKey(key_event, "");
  }

 private:
  bool EnsureWindowOnMain(bool make_visible) {
    if (!device_ || !command_queue_) {
      return false;
    }
    if (closed_.load()) {
      return false;
    }
    EnsureApplicationReady(make_visible);
    if (!pump_retained_) {
      pump_retained_ = RetainAppKitPump(loop_);
      if (!pump_retained_) {
        return false;
      }
    }
    if (!window_) {
      NSUInteger style =
          NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
      if (options_.resizable) {
        style |= NSWindowStyleMaskResizable;
      }
      NSRect rect = NSMakeRect(0, 0, options_.width, options_.height);
      window_ = [[NSWindow alloc] initWithContentRect:rect
                                            styleMask:style
                                              backing:NSBackingStoreBuffered
                                                defer:NO];
      NSString* title = options_.title.empty()
                            ? @"Node Lynx"
                            : [NSString stringWithUTF8String:options_.title.c_str()];
      window_.title = title ?: @"Node Lynx";
      window_delegate_ = [[NodeLynxWindowDelegate alloc] initWithHost:this];
      window_.delegate = window_delegate_;
      content_view_ = [[NodeLynxMetalView alloc] initWithHost:this device:device_];
      window_.contentView = content_view_;
      if (presentation_state_) {
        presentation_state_->closed.store(false);
        presentation_state_->content_view = content_view_;
        presentation_state_->device = device_;
        presentation_state_->command_queue = command_queue_;
      }
      [window_ center];
      [content_view_ notifyResize];
    }
    if (make_visible) {
      [window_ makeKeyAndOrderFront:nil];
    }
    return true;
  }

  void CloseOnMain() {
    if (!window_) {
      HandleWindowClosed();
      return;
    }
    [window_ close];
    if (!closed_.load()) {
      HandleWindowClosed();
    }
  }

  void ReleasePumpIfNeeded() {
    if (!pump_retained_) {
      return;
    }
    pump_retained_ = false;
    ReleaseAppKitPump();
  }

  double CurrentScale() const {
    if (window_) {
      return std::max(1.0, static_cast<double>(window_.backingScaleFactor));
    }
    return std::max(1.0, options_.device_pixel_ratio);
  }

  void SendPointer(lynx_pointer_phase_e phase, double x, double y, int64_t buttons,
                   lynx_pointer_signal_kind_e signal_kind, double scroll_delta_x,
                   double scroll_delta_y, bool precise_scroll) {
    lynx_pointer_event_t pointer_event{};
    pointer_event.struct_size = sizeof(pointer_event);
    pointer_event.phase = phase;
    pointer_event.timestamp = NowMicros();
    pointer_event.x = x;
    pointer_event.y = y;
    pointer_event.device = 0;
    pointer_event.signal_kind = signal_kind;
    pointer_event.scroll_delta_x = scroll_delta_x;
    pointer_event.scroll_delta_y = scroll_delta_y;
    pointer_event.device_kind =
        precise_scroll ? kLynxPointerDeviceKindTrackpad : kLynxPointerDeviceKindMouse;
    pointer_event.buttons = buttons;
    pointer_event.scale = 1.0;
    pointer_event.is_precise_scroll = precise_scroll ? 1 : 0;
    if (pointer_callback_) {
      pointer_callback_(pointer_event);
    }
  }

  bool ResolveAutomationKey(const std::string& key, uint64_t* physical, uint64_t* logical,
                            std::string* character) const {
    if (key == "Backspace") {
      *physical = clay::keycodes::kPhysicalBackspace;
      *logical = static_cast<uint64_t>(clay::LogicalKeyboardKey::kBackspace);
      return true;
    }
    if (key == "Delete") {
      *physical = clay::keycodes::kPhysicalDelete;
      *logical = static_cast<uint64_t>(clay::LogicalKeyboardKey::kDelete);
      return true;
    }
    if (key == "Enter") {
      *physical = clay::keycodes::kPhysicalEnter;
      *logical = static_cast<uint64_t>(clay::LogicalKeyboardKey::kEnter);
      *character = "\r";
      return true;
    }
    if (key == "ArrowLeft") {
      *physical = clay::keycodes::kPhysicalArrowLeft;
      *logical = static_cast<uint64_t>(clay::LogicalKeyboardKey::kArrowLeft);
      return true;
    }
    if (key == "ArrowRight") {
      *physical = clay::keycodes::kPhysicalArrowRight;
      *logical = static_cast<uint64_t>(clay::LogicalKeyboardKey::kArrowRight);
      return true;
    }
    if (key == "ArrowUp") {
      *physical = clay::keycodes::kPhysicalArrowUp;
      *logical = static_cast<uint64_t>(clay::LogicalKeyboardKey::kArrowUp);
      return true;
    }
    if (key == "ArrowDown") {
      *physical = clay::keycodes::kPhysicalArrowDown;
      *logical = static_cast<uint64_t>(clay::LogicalKeyboardKey::kArrowDown);
      return true;
    }
    return false;
  }

  void SendKey(const lynx_key_event_t& event, std::string character) {
    if (!key_callback_) {
      return;
    }
    WindowedKeyEvent key_event{event, std::move(character)};
    key_event.event.character = key_event.character.empty() ? nullptr : key_event.character.c_str();
    key_callback_(key_event);
  }

  void SendNativeKeyEvent(NSEvent* event, bool is_down_event) {
    if (!event) {
      return;
    }
    uint64_t physical = PhysicalKeyForKeyCode(event.keyCode);
    uint64_t logical = LogicalKeyForEvent(event, physical);
    std::string character = is_down_event ? PrintableCharactersForEvent(event) : "";
    bool is_repeat =
        is_down_event && (event.isARepeat || pressed_keys_.find(physical) != pressed_keys_.end());
    lynx_key_event_t key_event{};
    FillKeyEvent(&key_event,
                 is_down_event ? (is_repeat ? kLynxKeyEventTypeRepeat : kLynxKeyEventTypeDown)
                               : kLynxKeyEventTypeUp,
                 EventTimestampMicros(event), physical, logical,
                 character.empty() ? nullptr : character.c_str(), false);
    if (is_down_event) {
      pressed_keys_[physical] = logical;
    } else {
      pressed_keys_.erase(physical);
    }
    SendKey(key_event, std::move(character));
  }

  void SetTextInputRect(float x, float y, float width, float height) {
    void (^update)() = ^{
      if (!content_view_) {
        return;
      }
      CGFloat view_height = content_view_.bounds.size.height;
      CGFloat rect_height = std::max(1.0f, height);
      NSRect rect =
          NSMakeRect(x, view_height - y - rect_height, std::max(1.0f, width), rect_height);
      [content_view_ setTextInputRect:rect];
    };
    if ([NSThread isMainThread]) {
      update();
    } else {
      dispatch_async(dispatch_get_main_queue(), update);
    }
  }

  uv_loop_t* loop_ = nullptr;
  WindowedLynxViewOptions options_;
  PointerCallback pointer_callback_;
  KeyCallback key_callback_;
  ResizeCallback resize_callback_;
  ClosedCallback closed_callback_;
  std::shared_ptr<PresentationState> presentation_state_;
  std::unordered_map<uint64_t, uint64_t> pressed_keys_;
  std::atomic_bool closed_{false};
  bool pump_retained_ = false;
  __strong NSWindow* window_ = nil;
  __strong NodeLynxWindowDelegate* window_delegate_ = nil;
  __strong NodeLynxMetalView* content_view_ = nil;
  __strong id<MTLDevice> device_ = nil;
  __strong id<MTLCommandQueue> command_queue_ = nil;
};

}  // namespace node
}  // namespace lynx

@implementation NodeLynxWindowDelegate {
  lynx::node::NodeLynxWindowHostMac* _host;
}

- (instancetype)initWithHost:(lynx::node::NodeLynxWindowHostMac*)host {
  self = [super init];
  if (self) {
    _host = host;
  }
  return self;
}

- (void)windowWillClose:(NSNotification*)notification {
  if (_host) {
    _host->HandleWindowClosed();
  }
}

@end

@implementation NodeLynxMetalView {
  lynx::node::NodeLynxWindowHostMac* _host;
  NSTrackingArea* _trackingArea;
  NSMutableAttributedString* _markedText;
  NSRange _selectedRange;
  NSRect _textInputRect;
  BOOL _textInputActive;
}

- (instancetype)initWithHost:(lynx::node::NodeLynxWindowHostMac*)host device:(id<MTLDevice>)device {
  self = [super initWithFrame:NSMakeRect(0, 0, 1, 1)];
  if (self) {
    _host = host;
    self.wantsLayer = YES;
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = NO;
    self.layer = layer;
    _markedText = [[NSMutableAttributedString alloc] init];
    _selectedRange = NSMakeRange(0, 0);
    _textInputRect = NSMakeRect(0, 0, 1, 1);
    _textInputActive = NO;
  }
  return self;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (CAMetalLayer*)metalLayer {
  return (CAMetalLayer*)self.layer;
}

- (void)notifyResize {
  if (!_host || !self.window) {
    return;
  }
  _host->HandleResize(self.bounds.size.width, self.bounds.size.height,
                      self.window.backingScaleFactor);
}

- (void)setTextInputActive:(BOOL)active {
  _textInputActive = active;
}

- (void)setTextInputRect:(NSRect)rect {
  _textInputRect = rect;
}

- (void)setFrameSize:(NSSize)newSize {
  [super setFrameSize:newSize];
  [self notifyResize];
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  [self.window makeFirstResponder:self];
  [self notifyResize];
}

- (void)viewDidChangeBackingProperties {
  [super viewDidChangeBackingProperties];
  [self notifyResize];
}

- (void)updateTrackingAreas {
  if (_trackingArea) {
    [self removeTrackingArea:_trackingArea];
  }
  _trackingArea =
      [[NSTrackingArea alloc] initWithRect:self.bounds
                                   options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                                           NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect
                                     owner:self
                                  userInfo:nil];
  [self addTrackingArea:_trackingArea];
  [super updateTrackingAreas];
}

- (void)mouseEntered:(NSEvent*)event {
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseAdd, 0);
  }
}

- (void)mouseExited:(NSEvent*)event {
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseRemove, 0);
  }
}

- (void)mouseMoved:(NSEvent*)event {
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseHover, 0);
  }
}

- (void)mouseDragged:(NSEvent*)event {
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseMove, kLynxPointerMouseButtonsMousePrimary);
  }
}

- (void)mouseDown:(NSEvent*)event {
  [self.window makeFirstResponder:self];
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseDown, kLynxPointerMouseButtonsMousePrimary);
  }
}

- (void)mouseUp:(NSEvent*)event {
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseUp, 0);
  }
}

- (void)rightMouseDown:(NSEvent*)event {
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseDown, kLynxPointerMouseButtonsMouseSecondary);
  }
}

- (void)rightMouseUp:(NSEvent*)event {
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseUp, 0);
  }
}

- (void)scrollWheel:(NSEvent*)event {
  if (_host) {
    _host->HandleMouseEvent(event, kLynxPointerPhaseHover, 0, kLynxPointerSignalKindScroll,
                            event.scrollingDeltaX, event.scrollingDeltaY,
                            event.hasPreciseScrollingDeltas);
  }
}

- (void)keyDown:(NSEvent*)event {
  if (_host) {
    _host->HandleKeyDown(event);
  }
  if (ShouldInterpretTextInput(event)) {
    [self interpretKeyEvents:@[ event ]];
  }
}

- (void)keyUp:(NSEvent*)event {
  if (_host) {
    _host->HandleKeyUp(event);
  }
}

- (void)flagsChanged:(NSEvent*)event {
  if (_host) {
    _host->HandleFlagsChanged(event);
  }
}

- (void)insertText:(id)string {
  [self insertText:string replacementRange:NSMakeRange(NSNotFound, 0)];
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
  std::string text = Utf8StringFromInput(string);
  if (!text.empty() && _host) {
    _host->TypeText(text);
  }
  [_markedText setAttributedString:[[NSAttributedString alloc] initWithString:@""]];
  _selectedRange = NSMakeRange(0, 0);
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
  NSAttributedString* attributed = nil;
  if ([string isKindOfClass:[NSAttributedString class]]) {
    attributed = string;
  } else if ([string isKindOfClass:[NSString class]]) {
    attributed = [[NSAttributedString alloc] initWithString:string];
  }
  [_markedText setAttributedString:attributed ?: [[NSAttributedString alloc] initWithString:@""]];
  _selectedRange = selectedRange;
}

- (void)unmarkText {
  [_markedText setAttributedString:[[NSAttributedString alloc] initWithString:@""]];
  _selectedRange = NSMakeRange(0, 0);
}

- (BOOL)hasMarkedText {
  return _markedText.length > 0;
}

- (NSRange)markedRange {
  if (![self hasMarkedText]) {
    return NSMakeRange(NSNotFound, 0);
  }
  return NSMakeRange(0, _markedText.length);
}

- (NSRange)selectedRange {
  return _selectedRange;
}

- (NSArray*)validAttributesForMarkedText {
  return @[];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
  if (actualRange) {
    *actualRange = NSMakeRange(NSNotFound, 0);
  }
  return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
  return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
  if (actualRange) {
    *actualRange = NSMakeRange(0, 0);
  }
  NSRect rect = NSIsEmptyRect(_textInputRect) ? NSMakeRect(0, 0, 1, 1) : _textInputRect;
  NSRect window_rect = [self convertRect:rect toView:nil];
  return [self.window convertRectToScreen:window_rect];
}

- (void)doCommandBySelector:(SEL)selector {
  if (!_host) {
    return;
  }
  if (selector == @selector(deleteBackward:)) {
    _host->PressKey("Backspace");
  } else if (selector == @selector(deleteForward:)) {
    _host->PressKey("Delete");
  } else if (selector == @selector(insertNewline:)) {
    _host->PressKey("Enter");
  } else if (selector == @selector(moveLeft:)) {
    _host->PressKey("ArrowLeft");
  } else if (selector == @selector(moveRight:)) {
    _host->PressKey("ArrowRight");
  } else if (selector == @selector(moveUp:)) {
    _host->PressKey("ArrowUp");
  } else if (selector == @selector(moveDown:)) {
    _host->PressKey("ArrowDown");
  }
}

@end

namespace lynx {
namespace node {

bool NodeLynxWindowHost::SupportsAcceleratedRenderer() {
  return MTLCreateSystemDefaultDevice() != nil;
}

std::unique_ptr<NodeLynxWindowHost> NodeLynxWindowHost::Create(
    uv_loop_t* loop, WindowedLynxViewOptions options, PointerCallback pointer_callback,
    KeyCallback key_callback, ResizeCallback resize_callback, ClosedCallback closed_callback) {
  return std::make_unique<NodeLynxWindowHostMac>(
      loop, std::move(options), std::move(pointer_callback), std::move(key_callback),
      std::move(resize_callback), std::move(closed_callback));
}

}  // namespace node
}  // namespace lynx
