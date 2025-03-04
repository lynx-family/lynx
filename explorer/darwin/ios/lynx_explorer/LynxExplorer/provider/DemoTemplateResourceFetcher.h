// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTemplateResourceFetcher.h"

@interface DemoTemplateResourceFetcher : NSObject <LynxTemplateResourceFetcher>

typedef struct {
  BOOL isLocalScheme;
  NSData* data;
  NSString* url;
  NSString* query;
} LocalBundleResult;

+ (LocalBundleResult)readLocalBundleFromResource:(NSString*)url;

@end
