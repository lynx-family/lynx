// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxResourceHandle.h>

#include <memory>
#include <string>
#include <utility>

#include "core/resource/lynx_resource_handle.h"

namespace {

lynx::pub::LynxResourceHandle::CreateResult CreateFileResourceHandle(NSString* filePath) {
  const char* utf8Path = filePath.UTF8String;
  return lynx::pub::LynxResourceHandle::CreateFile(utf8Path == nullptr ? "" : utf8Path);
}

}  // namespace

@interface LynxResourceHandle ()

- (std::shared_ptr<lynx::pub::LynxResourceHandle>)rawResourceHandle;

@end

@implementation LynxResourceHandle {
  std::shared_ptr<lynx::pub::LynxResourceHandle> _resourceHandle;
}

- (nullable instancetype)initWithFilePath:(NSString*)filePath {
  auto result = CreateFileResourceHandle(filePath);
  if (!result.has_value()) {
    return nil;
  }

  if (self = [super init]) {
    _filePath = [filePath copy];
    _resourceHandle = std::move(result.value());
  }
  return self;
}

- (nullable NSData*)readAllBytes {
  auto handle = [self rawResourceHandle];
  if (handle == nullptr) {
    return nil;
  }

  auto result = handle->GetData();
  if (!result.has_value() || result.value() == nullptr) {
    return nil;
  }
  const auto& bytes = *result.value();
  if (bytes.empty()) {
    return [NSData data];
  }
  return [NSData dataWithBytes:bytes.data() length:bytes.size()];
}

- (NSInteger)size {
  auto handle = [self rawResourceHandle];
  return handle == nullptr ? -1 : static_cast<NSInteger>(handle->Size());
}

- (BOOL)isValid {
  @synchronized(self) {
    return _resourceHandle != nullptr;
  }
}

- (void)invalidate {
  @synchronized(self) {
    _resourceHandle.reset();
  }
}

- (std::shared_ptr<lynx::pub::LynxResourceHandle>)rawResourceHandle {
  @synchronized(self) {
    return _resourceHandle;
  }
}

@end
