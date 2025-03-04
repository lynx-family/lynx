// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXTEMPLATEPROVIDER_H_
#define DARWIN_COMMON_LYNX_LYNXTEMPLATEPROVIDER_H_

#import <Foundation/Foundation.h>

/**
 * LynxTemplateLoadBlock.
 * the type of data parameter can be NSData* or LynxTemplateBundle*.
 */
typedef void (^LynxTemplateLoadBlock)(id data, NSError* error);

/**
 * A helper class for load template
 */
@protocol LynxTemplateProvider <NSObject>

- (void)loadTemplateWithUrl:(NSString*)url onComplete:(LynxTemplateLoadBlock)callback;

@end

#endif  // DARWIN_COMMON_LYNX_LYNXTEMPLATEPROVIDER_H_
