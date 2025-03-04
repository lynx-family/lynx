// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXLOADMETA_H_
#define DARWIN_COMMON_LYNX_LYNXLOADMETA_H_

#import "LynxTemplateBundle.h"
#import "LynxTemplateData.h"

// LynxLoadModeNormal:Load Template as usual
// LynxLoadModePrePainting:Pending JS Events when load template, events will be send when update
typedef NS_ENUM(NSInteger, LynxLoadMode) {
  LynxLoadModeNormal = 0,
  LynxLoadModePrePainting = 1 << 0,
};

// Switches for each atomic abaility
typedef NS_OPTIONS(NSInteger, LynxLoadOption) {
  // enable element tree copy when first loadTemplate
  // element tree struct will be stored in TemplateBundle
  // developers will get noticed by onTemplateBunldReady in LynxViewClient
  LynxLoadOptionDumpElement = 1 << 1,
  // enable to provide a reusable TemplateBundle after template is decoding through
  // onTemplateBunldReady in LynxViewClient
  LynxLoadOptionRecycleTemplateBundle = 1 << 2,
  // PROCESS_LAYOUT_WITHOUT_UI_FLUSH:For calculating layout results without UI operations, used for
  // height calculation/pre-layout scenarios.
  LynxLoadOptionProcessLayoutWithoutUIFlush = 1 << 3,
};

// if both templateBundle and binaryData exist, use templateBundle first
@interface LynxLoadMeta : NSObject
@property(nonatomic, copy, nonnull) NSString* url;
@property(nonatomic, assign) LynxLoadMode loadMode;
@property(nonatomic, assign) LynxLoadOption loadOption;
@property(nonatomic, strong, nullable) NSData* binaryData;
@property(nonatomic, strong, nullable) LynxTemplateData* initialData;
@property(nonatomic, strong, nullable) LynxTemplateData* globalProps;
@property(nonatomic, strong, nullable) LynxTemplateBundle* templateBundle;
@property(nonatomic, nullable, strong) NSMutableDictionary<NSString*, id>* lynxViewConfig;
@end

#endif  // DARWIN_COMMON_LYNX_LYNXLOADMETA_H_
