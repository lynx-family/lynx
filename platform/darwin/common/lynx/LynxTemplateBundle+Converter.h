// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_
#define DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_

#import <Lynx/LynxError.h>
#import <Lynx/LynxTemplateBundle.h>

#include "core/template_bundle/lynx_template_bundle.h"

@interface LynxTemplateBundle ()

// A nil target uses this bundle as the target of synchronous verification.
- (nullable instancetype)initWithResourceHandle:(nullable LynxResourceHandle*)handle
                                         option:(nullable LynxTemplateBundleOption*)option
                                 securityTarget:(nullable id<LynxSecurityTarget>)target;

@end

// A nil target uses the new bundle itself. Returns nil and a LynxError on failure.
LynxTemplateBundle* _Nullable LynxCreateTemplateBundleFromResourceHandle(
    LynxResourceHandle* _Nonnull handle, NSString* _Nullable url,
    id<LynxSecurityTarget> _Nullable target, LynxError* _Nullable* _Nullable error);

std::shared_ptr<lynx::tasm::LynxTemplateBundle> LynxGetRawTemplateBundle(
    LynxTemplateBundle* _Null_unspecified bundle);

LynxTemplateBundle* _Null_unspecified ConstructTemplateBundleFromNative(
    lynx::tasm::LynxTemplateBundle bundle);

void LynxSetRawTemplateBundle(LynxTemplateBundle* _Null_unspecified bundle,
                              lynx::tasm::LynxTemplateBundle* _Null_unspecified raw_bundle);

#endif  // DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_
