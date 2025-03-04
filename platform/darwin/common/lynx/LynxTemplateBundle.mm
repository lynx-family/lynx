// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTemplateBundle.h"
#import "LynxService.h"
#import "LynxTemplateBundle+Converter.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/runtime/jscache/js_cache_manager_facade.h"
#import "core/shell/ios/data_utils.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"

@implementation LynxTemplateBundle {
  std::shared_ptr<lynx::tasm::LynxTemplateBundle> template_bundle_;
  NSString* _error;
  NSDictionary* _extraInfo;
}

- (instancetype)initWithTemplate:(NSData*)tem {
  if (self = [super init]) {
    auto securityService = LynxService(LynxServiceSecurityProtocol);
    if (securityService != nil) {
      LynxVerificationResult* verification = [securityService verifyTASM:tem
                                                                    view:nil
                                                                     url:nil
                                                                    type:LynxTASMTypeTemplate];
      if (!verification.verified) {
        _error = verification.errorMsg;
        return self;
      }
    }
    auto source = ConvertNSBinary(tem);
    auto decoder = lynx::tasm::LynxBinaryReader::CreateLynxBinaryReader(std::move(source));
    if (decoder.Decode()) {
      // decode success.
      template_bundle_ =
          std::make_shared<lynx::tasm::LynxTemplateBundle>(decoder.GetTemplateBundle());
    } else {
      // decode failed.
      _error = [NSString stringWithUTF8String:decoder.error_message_.c_str()];
    }
  }
  return self;
}

- (instancetype _Nullable)initWithTemplate:(nonnull NSData*)tem
                                    option:(nullable LynxTemplateBundleOption*)option {
  if (self = [self initWithTemplate:tem]) {
    [self initWithOption:option];
  }
  return self;
}

- (void)initWithOption:(nullable LynxTemplateBundleOption*)option {
  if (!template_bundle_ || option == nil) {
    return;
  }
  [self constructContext:[option contextPoolSize]];
  template_bundle_->SetEnableVMAutoGenerate([option enableContextAutoRefill]);
}

- (instancetype)initWithNativeBundle:(std::shared_ptr<lynx::tasm::LynxTemplateBundle>)bundle {
  if (self = [super init]) {
    template_bundle_ = std::move(bundle);
  }
  return self;
}

- (NSString*)errorMsg {
  return _error;
}

- (NSDictionary*)extraInfo {
  if ([self errorMsg] || template_bundle_ == nullptr) {
    LOGI("cannot get extraInfo through a invalid TemplateBundle.");
    return @{};
  }
  if (!_extraInfo) {
    lynx::lepus::Value value = template_bundle_->GetExtraInfo();
    _extraInfo = lynx::tasm::convertLepusValueToNSObject(value);
  }
  return _extraInfo;
}

- (BOOL)constructContext:(int)count {
  return template_bundle_ && template_bundle_->PrepareLepusContext(count);
}

- (BOOL)isElementBundleValid {
  return template_bundle_ && template_bundle_->GetContainsElementTree();
}

- (void)postJsCacheGenerationTask:(NSString*)bytecodeSourceUrl {
  if (template_bundle_ && bytecodeSourceUrl && bytecodeSourceUrl.length > 0) {
    lynx::piper::cache::JsCacheManagerFacade::PostCacheGenerationTask(
        *template_bundle_, [bytecodeSourceUrl UTF8String], lynx::piper::JSRuntimeType::quickjs);
  }
}

std::shared_ptr<lynx::tasm::LynxTemplateBundle> LynxGetRawTemplateBundle(
    LynxTemplateBundle* bundle) {
  if ([bundle errorMsg]) {
    return nullptr;
  }
  return bundle->template_bundle_;
}

void LynxSetRawTemplateBundle(LynxTemplateBundle* bundle,
                              lynx::tasm::LynxTemplateBundle* raw_bundle) {
  bundle->template_bundle_ = std::shared_ptr<lynx::tasm::LynxTemplateBundle>(raw_bundle);
}

LynxTemplateBundle* ConstructTemplateBundleFromNative(lynx::tasm::LynxTemplateBundle bundle) {
  return [[LynxTemplateBundle alloc]
      initWithNativeBundle:std::make_shared<lynx::tasm::LynxTemplateBundle>(std::move(bundle))];
}

@end
