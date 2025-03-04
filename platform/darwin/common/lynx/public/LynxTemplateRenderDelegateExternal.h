// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXTEMPLATERENDERDELEGATEEXTERNAL_H_
#define DARWIN_COMMON_LYNX_LYNXTEMPLATERENDERDELEGATEEXTERNAL_H_

#import "LynxTemplateRenderDelegate.h"

/**
 * Used only by the LynxTemplateRender, to support LynxTemplateRender can  receive lifecycle’s
 * callbacks.
 */
@interface LynxTemplateRenderDelegateExternal : NSObject <LynxTemplateRenderDelegate>
@end
#endif /* DARWIN_COMMON_LYNX_LYNXTEMPLATERENDERDELEGATEEXTERNAL_H_ */
