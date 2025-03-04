// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXTEMPLATEDATA_CONVERTER_H_
#define DARWIN_COMMON_LYNX_LYNXTEMPLATEDATA_CONVERTER_H_

#import <Lynx/LynxTemplateData.h>

#include <memory>

#include "core/renderer/data/template_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"

/**
 * Convert ObjC values to lepus Value
 * @param data input NS data
 * @param useBoolLiterals if useBoolLiterals, convert @YES/@NO to lepus bool true/false, else to
 * lepus number 1/0, default value: FALSE
 * @return output lepus value
 */
lynx::lepus::Value LynxConvertToLepusValue(id data, BOOL useBoolLiterals = NO);

@class LynxTemplateData;
lynx::lepus::Value *LynxGetLepusValueFromTemplateData(LynxTemplateData *data);

std::shared_ptr<lynx::tasm::TemplateData> ConvertLynxTemplateDataToTemplateData(
    LynxTemplateData *data);

@interface LynxTemplateData ()

@property(readonly) NSString *processorName;

- (NSArray *)obtainUpdateActions;
- (NSArray *)copyUpdateActions;

- (lynx::lepus::Value)getDataForJSThread;

@end

#endif  // DARWIN_COMMON_LYNX_LYNXTEMPLATEDATA_CONVERTER_H_
