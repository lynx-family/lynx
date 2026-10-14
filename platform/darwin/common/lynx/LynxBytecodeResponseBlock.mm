// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBytecodeResponseBlock.h>
#import "LynxBytecodeResponseBlock+Converter.h"

std::unique_ptr<lynx::runtime::js::cache::BytecodeGenerateCallback> CreateBytecodeGenerateCallback(
    LynxBytecodeResponseBlock _Nullable callback,
    LynxBytecodeContinueBlock _Nullable continueGeneration) {
  std::unique_ptr<lynx::runtime::js::cache::BytecodeGenerateCallback> bytecode_callback = nullptr;
  if (callback) {
    lynx::runtime::js::cache::BytecodeGenerateCallback::ContinueGenerationFunc continue_func =
        nullptr;
    if (continueGeneration) {
      continue_func = [continueGeneration](const std::string& js_name,
                                           const std::string& md5) -> bool {
        return continueGeneration([NSString stringWithUTF8String:js_name.c_str()],
                                  [NSString stringWithUTF8String:md5.c_str()]);
      };
    }
    bytecode_callback = std::make_unique<lynx::runtime::js::cache::BytecodeGenerateCallback>(
        [callback](
            std::string error_msg,
            std::unordered_map<std::string, std::shared_ptr<lynx::runtime::js::Buffer>> buffers) {
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
        },
        std::move(continue_func));
  }
  return bytecode_callback;
}
