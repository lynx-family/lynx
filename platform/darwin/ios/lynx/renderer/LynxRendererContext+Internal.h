// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxRendererContext.h"

@interface LynxRendererContext (DrawingResources)

// Zero denotes a missing page. Every non-null page update advances the version,
// including updates that modify an existing page in place.
- (uint64_t)textBundleVersion:(int32_t)textID;

@end
