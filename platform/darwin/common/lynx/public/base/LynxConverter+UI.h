// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxClassAliasDefines.h"
#import "LynxConverter.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxConverter (UI)

+ (COLOR_CLASS *)toUIColor:(id)value;

// TODO (xiamengfei.moonface): move to iOS folder
#if TARGET_OS_IOS
+ (UIAccessibilityTraits)toAccessibilityTraits:(id)value;
#endif

@end

NS_ASSUME_NONNULL_END
