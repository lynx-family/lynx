// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateBundleBuilder.h>
#import "LynxTemplateBundle+Converter.h"

#include "base/include/string/string_utils.h"

#include <string>
#include <utility>

@implementation LynxTemplateBundleBuilder {
  NSString* _mainThreadScript;
  NSString* _backgroundThreadScript;
}

+ (instancetype)create {
  return [[self alloc] init];
}

- (instancetype)main:(NSString*)script {
  _mainThreadScript = [script copy];
  return self;
}

- (instancetype)background:(NSString*)script {
  _backgroundThreadScript = [script copy];
  return self;
}

- (LynxTemplateBundle*)build {
  lynx::tasm::LynxTemplateBundle native_bundle;
  std::string error =
      native_bundle.Build(lynx::base::SafeStringConvert([_mainThreadScript UTF8String]),
                          lynx::base::SafeStringConvert([_backgroundThreadScript UTF8String]), "");
  if (!error.empty()) {
    return nil;
  }

  native_bundle.PrepareVMByConfigs();
  return ConstructTemplateBundleFromNative(std::move(native_bundle));
}

@end
