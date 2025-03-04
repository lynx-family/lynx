// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@interface CustomizedMessage : NSObject

@property(nonatomic, readwrite) NSString *type;
@property(nonatomic, readwrite) NSString *data;
@property(nonatomic, assign) int mark;

@end
