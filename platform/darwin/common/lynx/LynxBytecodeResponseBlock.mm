// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBytecodeResponseBlock.h>
#import "LynxBytecodeResponseBlock+Converter.h"

std::unique_ptr<lynx::piper::cache::BytecodeGenerateCallback> CreateBytecodeGenerateCallback(
    LynxBytecodeResponseBlock _Nullable callback) {
  std::unique_ptr<lynx::piper::cache::BytecodeGenerateCallback> bytecode_callback = nullptr;
  if (callback) {
    bytecode_callback = std::make_unique<lynx::piper::cache::BytecodeGenerateCallback>(
        [callback](std::string error_msg,
                   std::unordered_map<std::string, std::shared_ptr<lynx::piper::Buffer>> buffers) {
          NSString* errorInfo =
              error_msg.empty() ? nil : [NSString stringWithUTF8String:error_msg.c_str()];
          NSMutableDictionary* dict = nil;
          if (buffers.size() > 0) {
            dict = [[NSMutableDictionary alloc] init];
            for (const auto& iter : buffers) {
              NSData* byteBuffer =
                  [NSData dataWithBytesNoCopy:const_cast<void*>(
                                                  static_cast<const void*>(iter.second->data()))
                                       length:iter.second->size()
                                 freeWhenDone:NO];
              [dict setObject:byteBuffer forKey:[NSString stringWithUTF8String:iter.first.c_str()]];
            }
          }
          callback(errorInfo, dict);
        });
  }
  return bytecode_callback;
}
