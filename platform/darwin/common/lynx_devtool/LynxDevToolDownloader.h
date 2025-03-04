// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

typedef void (^downloadCallback)(NSData* _Nullable data, NSError* _Nullable error);

@interface LynxDevToolDownloader : NSObject

+ (void)download:(NSString* _Nonnull)url withCallback:(downloadCallback _Nonnull)callback;

@end
