// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDevToolPool.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxTemplateBundle.h>
#import "LynxBytecodeResponseBlock+Converter.h"
#import "LynxTemplateBundle+Converter.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/runtime/js/bytecode/js_cache_manager_facade.h"
#import "core/shell/ios/data_utils.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"

@implementation LynxTemplateBundle {
  std::shared_ptr<lynx::tasm::LynxTemplateBundle> template_bundle_;
  LynxDevToolPool* _devtool_pool;
  NSString* _error;
  NSDictionary* _extraInfo;
}

- (BOOL)isValid {
  return template_bundle_ != nullptr;
}

- (void)decodeTemplate:(NSData*)tem
                   url:(NSString*)url
            debuggable:(BOOL)debuggable
               skipCSS:(BOOL)skipCSS {
  _url = url;
  if ([[LynxEnv sharedInstance] lynxDebugEnabled]) {
    _devtool_pool = [[LynxDevToolPool alloc] initWithURL:url debuggable:debuggable];
  }
  auto securityService = LynxService(LynxServiceSecurityProtocol);
  if (securityService != nil) {
    LynxVerificationResult* verification = [securityService verifyTASM:tem
                                                                target:self
                                                                   url:url
                                                                  type:LynxTASMTypeTemplate];
    if (!verification.verified) {
      _error = verification.errorMsg;
      return;
    }
  }
  auto source = ConvertNSBinary(tem);
  auto decoder = lynx::tasm::LynxBinaryReader::CreateLynxBinaryReader(std::move(source));
  decoder.SetSkipCSSDecode(skipCSS);
  if (decoder.Decode()) {
    // decode success.
    template_bundle_ =
        std::make_shared<lynx::tasm::LynxTemplateBundle>(decoder.GetTemplateBundle());
    [_devtool_pool onTemplateBundleCreated:reinterpret_cast<intptr_t>(template_bundle_.get())];
    template_bundle_->PrepareVMByConfigs();
  } else {
    // decode failed.
    _error = [NSString stringWithUTF8String:decoder.error_message_.c_str()];
  }
}

- (instancetype _Nullable)initWithTemplate:(NSData*)tem
                                       url:(NSString*)url
                                debuggable:(BOOL)debuggable {
  if (self = [super init]) {
    [self decodeTemplate:tem url:url debuggable:debuggable skipCSS:NO];
  }
  return self;
}

- (instancetype _Nullable)initWithTemplate:(NSData*)tem {
  return [self initWithTemplate:tem url:nil debuggable:NO];
}

+ (instancetype _Nullable)_swiftTemplateBundleWithTemplate:(NSData*)tem {
  return [[self alloc] initWithTemplate:tem];
}

+ (instancetype _Nullable)_swiftTemplateBundleWithTemplate:(NSData*)tem
                                                    option:
                                                        (nullable LynxTemplateBundleOption*)option {
  return [[self alloc] initWithTemplate:tem option:option];
}

- (instancetype _Nullable)initWithTemplate:(nonnull NSData*)tem
                                    option:(nullable LynxTemplateBundleOption*)option {
  BOOL debuggable = option ? [option debuggable] : NO;
  BOOL skipCSS = option ? [option skipCSS] : NO;
  if (self = [super init]) {
    [self decodeTemplate:tem url:[option url] debuggable:debuggable skipCSS:skipCSS];
    [self initWithOption:option];
  }
  return self;
}

- (instancetype _Nullable)_swiftInitWithData:(NSData*)data {
  return [self _swiftInitWithData:data option:nil];
}

- (instancetype _Nullable)_swiftInitWithData:(NSData*)data
                                      option:(nullable LynxTemplateBundleOption*)option {
  if ([self isValid] || _error != nil) {
    return self;
  }
  NSString* url = option ? [option url] : nil;
  BOOL debuggable = option ? [option debuggable] : NO;
  BOOL skipCSS = option ? [option skipCSS] : NO;
  [self decodeTemplate:data url:url debuggable:debuggable skipCSS:skipCSS];
  [self initWithOption:option];
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
  [self postJsCacheGenerationTask:bytecodeSourceUrl callback:nil];
}

- (void)postJsCacheGenerationTask:(nonnull NSString*)bytecodeSourceUrl
                         callback:(nullable LynxBytecodeResponseBlock)callback {
  if (template_bundle_ && bytecodeSourceUrl && bytecodeSourceUrl.length > 0) {
    std::unique_ptr<lynx::runtime::js::cache::BytecodeGenerateCallback> bytecode_callback =
        CreateBytecodeGenerateCallback(callback);
    lynx::runtime::js::cache::JsCacheManagerFacade::PostCacheGenerationTask(
        *template_bundle_, [bytecodeSourceUrl UTF8String],
        lynx::runtime::js::JSRuntimeType::quickjs, std::move(bytecode_callback));
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
