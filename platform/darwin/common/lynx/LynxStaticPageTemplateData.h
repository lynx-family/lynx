// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@class LynxTemplateData;

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT LynxTemplateData* LynxCreateStaticPageTemplateData(
    NSDictionary<NSString*, id>* dictionary);
FOUNDATION_EXPORT BOOL LynxTemplateDataIsForStaticPage(LynxTemplateData* _Nullable data);

NS_ASSUME_NONNULL_END
