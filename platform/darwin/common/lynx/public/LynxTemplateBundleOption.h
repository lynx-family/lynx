// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Class to control the behavior when constructing TemplateBundle from template
 */

@interface LynxTemplateBundleOption : NSObject

/**
 * Pre-create a certain number of lepus contexts, which could speed up loadTemplateBundle.
 * Notice:
 * 1. one context will be pre-created in TemplateBundle without actively calling if FE marks
 * `enableUseContextPool` true, in which case there is no need to actively call this method.
 * 2. not all TemplateBundles are able to pre-create context, it is only applicable to templates
 * using lepusNG.
 * 3. this API will override the FE settings
 * Default value: 0
 */
@property(nonatomic, assign) int contextPoolSize;

/**
 * Whether to automatically replenish the context after it is consumed
 * Default value: NO
 */
@property(nonatomic, assign) BOOL enableContextAutoRefill;

@end
