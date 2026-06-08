// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxElement.h>
#import <Lynx/LynxTemplateRender.h>

#include <string>

#if defined(LynxElement)
#pragma push_macro("LynxElement")
#undef LynxElement
#define LYNX_RESTORE_LYNX_ELEMENT_EXTENSION 1
#endif

@interface LynxElement ()

- (instancetype)initWithTemplateRender:(LynxTemplateRender *)templateRender
                                  sign:(int32_t)sign NS_DESIGNATED_INITIALIZER;

@end

@interface LynxTemplateRender (LynxElement)

- (void)lynxElementToJSONStringWithSign:(int32_t)sign
                               callback:(void (^_Nonnull)(NSString *_Nullable json))callback;

@end

#if defined(LYNX_RESTORE_LYNX_ELEMENT_EXTENSION)
#pragma pop_macro("LynxElement")
#undef LYNX_RESTORE_LYNX_ELEMENT_EXTENSION
#endif

namespace {

void InvokeJSONStringCallback(void (^_Nonnull callback)(NSString *_Nullable), std::string json) {
  dispatch_async(dispatch_get_main_queue(), ^{
    NSString *result = nil;
    if (!json.empty()) {
      result = [[NSString alloc] initWithBytes:json.data()
                                        length:json.size()
                                      encoding:NSUTF8StringEncoding];
    }
    callback(result);
  });
}

}  // namespace

@implementation LynxElement {
  __weak LynxTemplateRender *_templateRender;
  int32_t _sign;
}

- (instancetype)initWithTemplateRender:(LynxTemplateRender *)templateRender sign:(int32_t)sign {
  self = [super init];
  if (self) {
    _templateRender = templateRender;
    _sign = sign;
  }
  return self;
}

- (void)toJSONString:(void (^_Nonnull)(NSString *_Nullable json))callback {
  if (callback == nil) {
    return;
  }
  void (^callbackCopy)(NSString *_Nullable) = [callback copy];
  LynxTemplateRender *templateRender = _templateRender;
  if (templateRender == nil || _sign == 0) {
    InvokeJSONStringCallback(callbackCopy, "");
    return;
  }
  [templateRender lynxElementToJSONStringWithSign:_sign callback:callbackCopy];
}

@end
