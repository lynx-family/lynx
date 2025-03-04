// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXCONFIGINFO_H_
#define DARWIN_COMMON_LYNX_LYNXCONFIGINFO_H_

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, LynxThreadStrategyForRender);
// this class will be removed
@interface LynxConfigInfo : NSObject

@property(nonatomic, readonly, retain) NSString* pageVersion;
@property(nonatomic, readonly, retain) NSString* pageType;
@property(nonatomic, readonly, retain) NSString* cliVersion;
@property(nonatomic, readonly, retain) NSString* customData;  // json string
@property(nonatomic, readonly, retain) NSString* templateUrl;
@property(nonatomic, readonly, retain) NSString* targetSdkVersion;
@property(nonatomic, readonly, retain) NSString* lepusVersion;
@property(nonatomic, readonly, assign) LynxThreadStrategyForRender threadStrategyForRendering;
@property(nonatomic, readonly, assign) BOOL enableLepusNG;
@property(nonatomic, readonly, retain) NSString* radonMode;
@property(nonatomic, readonly, copy) NSString* reactVersion;
// all registered component
@property(nonatomic, readonly, retain) NSSet<NSString*>* registeredComponent;
// get LynxConfigInfo key value in json dictionary
@property(nonatomic, readonly, retain) NSData* json;
@property(nonatomic, readonly, assign) BOOL cssAlignWithLegacyW3c;
@property(nonatomic, readonly, assign) BOOL enableCSSParser;

@end

// this class will be removed
@interface LynxConfigInfoBuilder : NSObject

@property(nonatomic, readwrite, retain) NSString* pageVersion;
@property(nonatomic, readwrite, retain) NSString* pageType;
@property(nonatomic, readwrite, retain) NSString* cliVersion;
@property(nonatomic, readwrite, retain) NSString* customData;  // json string
@property(nonatomic, readwrite, retain) NSString* templateUrl;
@property(nonatomic, readwrite, retain) NSString* targetSdkVersion;
@property(nonatomic, readwrite, retain) NSString* lepusVersion;
@property(nonatomic, readwrite, assign) LynxThreadStrategyForRender threadStrategyForRendering;
@property(nonatomic, readwrite, assign) BOOL enableLepusNG;
@property(nonatomic, readwrite, retain) NSString* radonMode;
@property(nonatomic, readwrite, retain) NSSet<NSString*>* registeredComponent;
@property(nonatomic, readwrite, assign) BOOL cssAlignWithLegacyW3c;
@property(nonatomic, readwrite, assign) BOOL enableCSSParser;
@property(nonatomic, readwrite, copy) NSString* reactVersion;

- (instancetype)init;

- (LynxConfigInfo*)build;

@end

#endif  // DARWIN_COMMON_LYNX_LYNXCONFIGINFO_H_
