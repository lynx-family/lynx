// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCEREQUESTPARAMETERS_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCEREQUESTPARAMETERS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * The scene of the request.
 */
typedef NS_ENUM(NSInteger, LynxServiceResourceScene) {
  ResourceSceneOther,
  ResourceSceneLynxTemplate,
  ResourceSceneLynxChildResource,
  ResourceSceneLynxComponent,
  ResourceSceneLynxFont,
  ResourceSceneLynxI18n,
  ResourceSceneLynxImage,
  ResourceSceneLynxLottie,
  ResourceSceneLynxVideo,
  ResourceSceneLynxSVG,
  ResourceSceneLynxExternalJS,
  ResourceSceneWebMainResource,
  ResourceSceneWebChildResource,
};

@interface LynxServiceResourceRequestParameters : NSObject <NSCopying>

// enable reuse request
@property(nonatomic, strong) NSNumber *enableRequestReuse;

// resource scene
@property(nonatomic, assign) LynxServiceResourceScene resourceScene;

// Using templateUrl to fetch resource by the registered resourceLoader.
@property(nonatomic, copy) NSString *templateUrl;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCEREQUESTPARAMETERS_H_
