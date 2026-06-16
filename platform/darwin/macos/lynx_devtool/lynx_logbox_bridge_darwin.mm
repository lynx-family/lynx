// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "platform/darwin/macos/lynx_devtool/lynx_logbox_bridge_darwin.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <string>
#import "devtool/base_devtool/darwin/ios/logbox/DevToolLogBoxProxy.h"
#import "devtool/base_devtool/darwin/ios/logbox/DevToolLogBoxResProvider.h"
#include "platform/embedder/lynx_devtool/logbox/logbox_resource_provider.h"

static lynx::embedder::LogBoxResourceProvider* ProviderFromOpaque(void* provider) {
  return static_cast<lynx::embedder::LogBoxResourceProvider*>(provider);
}

static NSString* ToNSString(const std::string& value) {
  return [NSString stringWithUTF8String:value.c_str()] ?: @"";
}

static NSString* ToLevelString(int level) { return level == 2 ? @"warn" : @"error"; }

@interface DarwinLogBoxResProviderAdapter : NSObject <DevToolLogBoxResProvider>

- (instancetype)initWithProvider:(void*)provider;

@end

@implementation DarwinLogBoxResProviderAdapter {
  void* _provider;
}

- (instancetype)initWithProvider:(void*)provider {
  self = [super init];
  if (self) {
    _provider = provider;
  }
  return self;
}

- (NSString*)entryUrlForLogSrc {
  auto* provider = ProviderFromOpaque(_provider);
  return provider ? ToNSString(provider->GetEntryUrl()) : @"";
}

- (UIView*)getView {
  auto* provider = ProviderFromOpaque(_provider);
  return provider ? (__bridge UIView*)provider->GetHostView() : nil;
}

- (NSDictionary*)logSources {
  auto* provider = ProviderFromOpaque(_provider);
  if (provider == nullptr) {
    return @{};
  }
  NSMutableDictionary* sources = [NSMutableDictionary dictionary];
  for (const auto& [key, value] : provider->GetLogSources()) {
    [sources setObject:ToNSString(value) forKey:ToNSString(key)];
  }
  return sources;
}

- (NSString*)logSourceWithFileName:(NSString*)fileName {
  auto* provider = ProviderFromOpaque(_provider);
  if (provider == nullptr || fileName == nil) {
    return @"";
  }
  return ToNSString(provider->GetLogSourceByFileName([fileName UTF8String]));
}

@end

namespace lynx {
namespace embedder {

std::unique_ptr<LynxLogBoxBridge> LynxLogBoxBridge::Create(LogBoxResourceProvider* provider) {
  return std::make_unique<LynxLogBoxBridgeDarwin>(provider);
}

class LynxLogBoxBridgeDarwin::Impl {
 public:
  explicit Impl(LogBoxResourceProvider* provider) {
    provider_adapter_ = [[DarwinLogBoxResProviderAdapter alloc] initWithProvider:provider];
    proxy_ = [[DevToolLogBoxProxy alloc] initWithNamespace:@"lynx"
                                          resourceProvider:provider_adapter_];
    [proxy_ registerErrorParserWithBundle:@"LynxDebugResources" file:@"logbox/lynx-error-parser"];
  }

  void OnHostViewAttached() {
    [proxy_ onMovedToWindow];
    [proxy_ onResourceProviderReady];
  }

  void OnError(const LogBoxErrorInfo& error) {
    [proxy_ showLogMessage:ToNSString(error.message) withLevel:ToLevelString(error.level)];
  }

  void OnReload() { [proxy_ reset]; }

  void OnDestroy() { [proxy_ destroy]; }

 private:
  DarwinLogBoxResProviderAdapter* provider_adapter_ = nil;
  DevToolLogBoxProxy* proxy_ = nil;
};

LynxLogBoxBridgeDarwin::LynxLogBoxBridgeDarwin(LogBoxResourceProvider* provider)
    : impl_(std::make_unique<Impl>(provider)) {}

LynxLogBoxBridgeDarwin::~LynxLogBoxBridgeDarwin() = default;

void LynxLogBoxBridgeDarwin::OnHostViewAttached() { impl_->OnHostViewAttached(); }

void LynxLogBoxBridgeDarwin::OnError(const LogBoxErrorInfo& error) { impl_->OnError(error); }

void LynxLogBoxBridgeDarwin::OnReload() { impl_->OnReload(); }

void LynxLogBoxBridgeDarwin::OnDestroy() { impl_->OnDestroy(); }

}  // namespace embedder
}  // namespace lynx
