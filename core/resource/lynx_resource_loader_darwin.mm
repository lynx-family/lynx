// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lynx_resource_loader_darwin.h"
#import "LynxEnv.h"
#import "LynxError.h"
#import "LynxLog.h"
#import "LynxService.h"
#import "LynxSubErrorCode.h"
#import "LynxTemplateBundle+Converter.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/resource/lynx_resource_setting.h"
#include "core/shell/ios/data_utils.h"
#import "darwin/common/lynx/TemplateRenderCallbackProtocol.h"

namespace {
static NSString* const kFileScheme = @"file://";
static NSString* const kCoreDebugJS = @"lynx_core_dev";
static NSString* const kAssetsScheme = @"assets://";
static NSString* const kLynxAssetsScheme = @"lynx_assets://";
static NSString* const kAssetsCoreScheme = @"assets://lynx_core.js";

static NSString* const kLoadScriptMethodName = @"LoadScript";
static NSString* const kLoadLocalScriptMethodName = @"LoadLocalScript";
static NSString* const kNullDataMsg = @"response with null data";

void ReportError(__weak id<LynxErrorReceiverProtocol> weakErrorReceiver, NSString* methodName,
                 NSString* url, NSInteger code, NSString* errorMsg, NSString* rootCause) {
  dispatch_async(dispatch_get_main_queue(), ^() {
    id<LynxErrorReceiverProtocol> errorReceiver = weakErrorReceiver;
    if (!errorReceiver) {
      return;
    }
    LynxError* lynxError = [LynxError
        lynxErrorWithCode:code
                  message:[[NSString alloc] initWithFormat:@"ResourceLoaderDarwin::%@ "
                                                           @"failed, the error message is: %@",
                                                           methodName, errorMsg]
            fixSuggestion:LynxErrorSuggestionRefOfficialSite
                    level:LynxErrorLevelError];
    [lynxError setRootCause:rootCause];
    [lynxError addCustomInfo:url forKey:LynxErrorKeyResourceUrl];
    [errorReceiver onErrorOccurred:lynxError];
  });
}

}  // namespace

namespace lynx {
namespace shell {

void LynxResourceLoaderDarwin::FetchExternalResourceComplete(
    NSData* data, NSError* error, NSString* nsUrl,
    __weak id<LynxErrorReceiverProtocol> weakErrorReceiver, CopyableClosure callback) {
  NSInteger errCode = 0;
  NSString* errMsg = @"";
  NSString* rootCause = nil;
  if (error != nil) {
    errMsg = @"Error when fetch external resource";
    rootCause = error.localizedDescription;
    errCode = ECLynxResourceExternalResourceRequestFailed;
  } else if (data == nil || [data length] == 0) {
    errMsg = kNullDataMsg;
    errCode = ECLynxResourceExternalResourceRequestFailed;
  }

  if (errCode != 0) {
    // Report only when loading script, the lazy bundle error will be reported in C++
    // TODO(zhoupeng.z): Also Report script error in C++
    ReportError(weakErrorReceiver, kLoadScriptMethodName, nsUrl, errCode, errMsg, rootCause);
  }

  pub::LynxResourceResponse resp{.data = ConvertNSBinary(data),
                                 .err_code = static_cast<int32_t>(errCode),
                                 .err_msg = [errMsg UTF8String]};

  callback(resp);
}

LynxResourceLoaderDarwin::LynxResourceLoaderDarwin(
    LynxProviderRegistry* providerRegistry, id<LynxDynamicComponentFetcher> dynamicComponentFetcher,
    id<LynxErrorReceiverProtocol> errorReceiver,
    id<LynxTemplateResourceFetcher> genericTemplateFetcher,
    id<LynxGenericResourceFetcher> genericResourceFetcher)
    : _providerRegistry(providerRegistry),
      _fetcher_wrapper([[LynxExternalResourceFetcherWrapper alloc]
          initWithDynamicComponentFetcher:dynamicComponentFetcher]),
      _templateResourceFetcher(genericTemplateFetcher),
      _genericResourceFetcher(genericResourceFetcher),
      _errorReceiver(errorReceiver) {}

void LynxResourceLoaderDarwin::FetchScriptByProvider(const std::string& url,
                                                     CopyableClosure callback) {
  id<LynxResourceProvider> provider =
      [_providerRegistry getResourceProviderByKey:LYNX_PROVIDER_TYPE_EXTERNAL_JS];
  if (provider == nil) {
    LOGE("lynx resource provider is null, url: " << url);
    pub::LynxResourceResponse res;
    callback(res);
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FetchScriptByProvider", "url", url);
  NSString* nsUrl = [NSString stringWithUTF8String:url.c_str()];
  LynxResourceRequest* req = [[LynxResourceRequest alloc] initWithUrl:nsUrl];
  __block __weak id<LynxErrorReceiverProtocol> weakErrorReceiver = _errorReceiver;
  [provider request:req
         onComplete:^(LynxResourceResponse* response) {
           FetchExternalResourceComplete(response.data, response.error, nsUrl, weakErrorReceiver,
                                         std::move(callback));
         }];
}

bool LynxResourceLoaderDarwin::FetchTemplateByGenericFetcher(const std::string& url,
                                                             CopyableClosure callback) {
  if (_templateResourceFetcher != nil) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FetchTemplateByGenericFetcher", "url", url);
    NSString* nsUrl = [NSString stringWithUTF8String:url.c_str()];
    LynxResourceRequest* request =
        [[LynxResourceRequest alloc] initWithUrl:nsUrl type:LynxResourceTypeDynamicComponent];
    [_templateResourceFetcher
        fetchTemplate:request
           onComplete:^(LynxTemplateResource* _Nullable data, NSError* _Nullable error) {
             NSString* errMsg = error == nil ? @"" : [error localizedDescription];
             pub::LynxResourceResponse resp{.err_code = error == nil ? 0 : -1,
                                            .err_msg = [errMsg UTF8String]};
             if (data.bundle != nil) {
               resp.bundle = LynxGetRawTemplateBundle(data.bundle).get();
             } else if (data.data != nil) {
               resp.data = ConvertNSBinary(data.data);
             }
             callback(resp);
           }];
    return true;
  }
  return false;
}

bool LynxResourceLoaderDarwin::FetchResourceByGenericFetcher(const std::string& url,
                                                             CopyableClosure callback) {
  if (_genericResourceFetcher != nil) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FetchResourceByGenericFetcher", "url", url);
    NSString* nsUrl = [NSString stringWithUTF8String:url.c_str()];
    LynxResourceRequest* request =
        [[LynxResourceRequest alloc] initWithUrl:nsUrl type:LynxResourceTypeDynamicComponent];
    __block __weak id<LynxErrorReceiverProtocol> weakErrorReceiver = _errorReceiver;
    [_genericResourceFetcher fetchResource:request
                                onComplete:^(NSData* _Nullable data, NSError* _Nullable error) {
                                  FetchExternalResourceComplete(
                                      data, error, nsUrl, weakErrorReceiver, std::move(callback));
                                }];
    return true;
  }
  return false;
}

bool LynxResourceLoaderDarwin::FetchTemplateByProvider(const std::string& url,
                                                       CopyableClosure callback) {
  id<LynxResourceProvider> provider =
      [_providerRegistry getResourceProviderByKey:LYNX_PROVIDER_TYPE_DYNAMIC_COMPONENT];
  if (provider == nil) {
    // TYPE: LYNX_PROVIDER_TYPE_LAZY_BUNDLE is not registered, should fallback;
    return false;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FetchTemplateByProvider", "url", url);
  NSString* nsUrl = [NSString stringWithUTF8String:url.c_str()];
  LynxResourceRequest* req = [[LynxResourceRequest alloc] initWithUrl:nsUrl];
  [provider request:req
         onComplete:^(LynxResourceResponse* response) {
           NSInteger errCode = 0;
           NSString* errMsg = @"";
           if (response.error != nil) {
             errMsg = response.error.localizedDescription;
             errCode = ECLynxLazyBundleLoadBadResponse;
           } else if (response.data == nil || [response.data length] == 0) {
             errMsg = kNullDataMsg;
             errCode = ECLynxLazyBundleLoadEmptyFile;
           }
           pub::LynxResourceResponse resp{.data = ConvertNSBinary(response.data),
                                          .err_code = static_cast<int32_t>(errCode),
                                          .err_msg = [errMsg UTF8String]};
           callback(resp);
         }];
  return true;
}

bool LynxResourceLoaderDarwin::FetchTemplateByFetcherWrapper(const std::string& url,
                                                             CopyableClosure callback,
                                                             bool request_in_current_thread) {
  // use fetcher_wrapper to fetch template.
  __block volatile BOOL invoked = NO;
  __block NSObject* object = [[NSObject alloc] init];
  NSString* nsUrl = [NSString stringWithUTF8String:url.c_str()];
  LoadedBlock fetcherBlock = ^(NSData* _Nullable data, NSError* _Nullable error) {
    @synchronized(object) {
      if (invoked) {
        _LogE(@"DynamicComponent. Illegal callback invocation from native. The loaded "
              @"callback can only be invoked once! The url is %@",
              nsUrl);
        return;
      }
      invoked = YES;
    }

    if (!error && data.length > 0) {
      [LynxService(LynxServiceMonitorProtocol)
          reportErrorGlobalContextTag:LynxContextTagLastLynxAsyncComponentURL
                                 data:[NSString
                                          stringWithCString:[nsUrl UTF8String]
                                                   encoding:[NSString defaultCStringEncoding]]];
    }
    NSString* errMsg = error == nil ? @"" : [error localizedDescription];
    pub::LynxResourceResponse resp{.data = ConvertNSBinary(data),
                                   .err_code = error == nil ? 0 : -1,
                                   .err_msg = [errMsg UTF8String]};
    callback(resp);
  };
  if (request_in_current_thread) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FetchTemplateByFetcherWrapper", "url", url);
    [_fetcher_wrapper fetchResource:nsUrl withLoadedBlock:fetcherBlock];
  } else {
    // TODO(nihao.royal): it's only used in preloadTemplate by now, and need to be deleted later.
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^() {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "FetchTemplateByFetcherWrapper");
      [_fetcher_wrapper fetchResource:nsUrl withLoadedBlock:fetcherBlock];
    });
  }
  return true;
}

void LynxResourceLoaderDarwin::LoadResource(
    const pub::LynxResourceRequest& request, bool request_in_current_thread,
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LoadResource", [&request](lynx::perfetto::EventContext ctx) {
    ctx.event()->add_debug_annotations("url", request.url);
  });
  // fetch Assets
  if (request.type == pub::LynxResourceType::kAssets) {
    NSData* data = LoadJSSource(request.url);
    pub::LynxResourceResponse resp{.data = ConvertNSBinary(data)};
    callback(resp);
    return;
  }

  // fetch ExternalJS
  if (request.type == pub::LynxResourceType::kExternalJs) {
    auto copyable_callback = fml::MakeCopyable(std::move(callback));
    // 1. try to use LynxGenericResourceFetcher
    if (FetchResourceByGenericFetcher(request.url, copyable_callback)) {
      return;
    }
    FetchScriptByProvider(request.url, std::move(copyable_callback));
    return;
  }

  if (request.type == pub::LynxResourceType::kLazyBundle) {
    auto copyable_callback = fml::MakeCopyable(std::move(callback));
    std::string url_copy = request.url;
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback_wrapper =
        ^(pub::LynxResourceResponse& response) {
          VerifyLynxTemplateResource(url_copy, response);
          copyable_callback(response);
        };
    auto copyable_wrapper_callback = fml::MakeCopyable(std::move(callback_wrapper));
    // 1. try to use LynxTemplateResourceFetcher
    if (FetchTemplateByGenericFetcher(request.url, copyable_wrapper_callback)) {
      return;
    }
    // 2. try to use LynxResourceProvider
    if (FetchTemplateByProvider(request.url, copyable_wrapper_callback)) {
      return;
    }
    // 3. try to use LynxExternalResourceFetcherWrapper
    if (FetchTemplateByFetcherWrapper(request.url, copyable_wrapper_callback,
                                      request_in_current_thread)) {
      return;
    }
    return;
  }

  if (request.type == pub::LynxResourceType::kTemplateLazyBundle) {
    auto copyable_callback = fml::MakeCopyable(std::move(callback));
    std::string url_copy = request.url;
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback_wrapper =
        ^(pub::LynxResourceResponse& response) {
          VerifyLynxTemplateResource(url_copy, response);
          copyable_callback(response);
        };
    auto copyable_wrapper_callback = fml::MakeCopyable(std::move(callback_wrapper));
    // 1. try to use LynxTemplateResourceFetcher
    if (FetchTemplateByGenericFetcher(request.url, copyable_wrapper_callback)) {
      return;
    }

    // 2. try to use LynxExternalResourceFetcherWrapper;
    if (FetchTemplateByFetcherWrapper(request.url, copyable_wrapper_callback,
                                      request_in_current_thread)) {
      return;
    }
    return;
  }

  pub::LynxResourceResponse resp{.err_code = -1, .err_msg = "Unsupported type."};
  callback(resp);
}

void LynxResourceLoaderDarwin::SetEnableLynxResourceService(bool enable) {
  _fetcher_wrapper.enableLynxService = enable;
}

// private
/**
 * 1. the name is "lynx_core.js"
 *  if  devtool is enable, find the file in "LynxDebugResources"
 * bundle first, then in "LynxResources" bundle. (Android will check debugSource first) iii. else
 * devtool is disable, find the file in "LynxResources" bundle
 * 2. the name starts with "file://", try to use file system
 * 3. the name starts with "assets://" but doesn't equal to "assets://lynx_core.js", load file from
 * "Resource" bundle.
 * 4. the name starts with "lynx_assets://", the process is similar with "lynx_core.js" (the first
 * step)
 */
NSData* LynxResourceLoaderDarwin::LoadJSSource(const std::string& name) {
  NSString* str = [NSString stringWithUTF8String:name.c_str()];

  NSString* path = nil;
  NSBundle* frameworkBundle = [NSBundle mainBundle];
  if ([kAssetsCoreScheme isEqualToString:str]) {
    str = [str componentsSeparatedByString:@"."][0];
    NSURL* debugBundleUrl = [frameworkBundle URLForResource:@"LynxDebugResources"
                                              withExtension:@"bundle"];
    if (path == nil && debugBundleUrl && LynxEnv.sharedInstance.devtoolEnabled) {
      NSBundle* bundle = [NSBundle bundleWithURL:debugBundleUrl];
      path = [bundle pathForResource:kCoreDebugJS ofType:@"js"];
      if (path != nil) {
        piper::LynxResourceSetting::getInstance()->is_debug_resource_ = true;
      }
    }

    if (path == nil) {
      NSURL* bundleUrl = [frameworkBundle URLForResource:@"LynxResources" withExtension:@"bundle"];
      if (bundleUrl) {
        NSBundle* bundle = [NSBundle bundleWithURL:bundleUrl];
        path = [bundle pathForResource:[str substringFromIndex:[kAssetsScheme length]]
                                ofType:@"js"];
      }
    }
  } else if ([str length] > [kFileScheme length] && [str hasPrefix:kFileScheme]) {
    NSString* filePath = [str substringFromIndex:[kFileScheme length]];
    if ([filePath hasPrefix:@"/"]) {
      path = filePath;
    } else {
      NSString* cachePath = [NSSearchPathForDirectoriesInDomains(
          NSCachesDirectory, NSUserDomainMask, YES) firstObject];
      path = [cachePath stringByAppendingPathComponent:filePath];
    }
  } else if ([str length] > [kAssetsScheme length] && [str hasPrefix:kAssetsScheme]) {
    NSRange range = [str rangeOfString:@"." options:NSBackwardsSearch];
    str = [str substringToIndex:range.location];
    path = [[NSBundle mainBundle]
        pathForResource:[@"Resource/"
                            stringByAppendingString:[str substringFromIndex:[kAssetsScheme length]]]
                 ofType:@"js"];
  } else if ([str hasPrefix:kLynxAssetsScheme]) {
    NSURL* bundleUrl = [frameworkBundle URLForResource:@"LynxResources" withExtension:@"bundle"];
    NSURL* debugBundleUrl = [frameworkBundle URLForResource:@"LynxDebugResources"
                                              withExtension:@"bundle"];
    return LoadLynxJSAsset(name, *bundleUrl, *debugBundleUrl);
  }
  NSError* error = nil;
  if (path) {
    _LogI(@"LoadJSSource real path: %@", path);
    NSData* data = [NSData dataWithContentsOfFile:path options:0 error:&error];
    if (!error) {
      return data;
    }
  }
  ReportError(_errorReceiver, kLoadLocalScriptMethodName, str,
              ECLynxResourceExternalResourceLocalResourceLoadFail,
              @"Error occurred when load js source", [error localizedFailureReason]);
  _LogE(@"LoadJSSource no corejs find with %@, resource path: %@, error: %@", str, path,
        [error localizedDescription]);
  return nil;
}

/**
 * 1. If devtool is enabled, try to get the source path from LynxDebugResources.
 *   i. If [filename]_dev.js path can be found from "LynxDebugResources" bundle, use it, else try to
 * find [filename].js is available.
 * 2. If cannot find path from "LynxDebugResources", try to find whether [filename].js is available
 * in "LynxResources" bundle.
 */
NSData* LynxResourceLoaderDarwin::LoadLynxJSAsset(const std::string& name, NSURL& bundleUrl,
                                                  NSURL& debugBundleUrl) {
  NSString* str = [NSString stringWithUTF8String:name.c_str()];

  NSString* path = nil;
  NSRange range = [str rangeOfString:@"." options:NSBackwardsSearch];
  str = [str substringToIndex:range.location];
  // Under dev mode, try to load [filename]_dev.js first.
  // If the file is not available, try to load [filename].js.
  if (&debugBundleUrl && LynxEnv.sharedInstance.devtoolEnabled) {
    NSBundle* bundle = [NSBundle bundleWithURL:&debugBundleUrl];
    NSString* debugAssetName =
        [[str substringFromIndex:[kLynxAssetsScheme length]] stringByAppendingString:@"_dev"];
    path = [bundle pathForResource:debugAssetName ofType:@"js"];
    if (path == nil) {
      path = [bundle pathForResource:[str substringFromIndex:[kLynxAssetsScheme length]]
                              ofType:@"js"];
    }
  }

  if (path == nil) {
    NSBundle* bundle = [NSBundle bundleWithURL:&bundleUrl];
    NSString* filename = [str substringFromIndex:[kLynxAssetsScheme length]];
    path = [bundle pathForResource:filename ofType:@"js"];
  }

  if (path) {
    _LogI(@"LoadLynxJSAsset real path: %@", path);
    NSData* data = [NSData dataWithContentsOfFile:path];
    return data;
  }

  _LogE(@"LoadLynxJSAsset no js file find with %@", str);
  return nil;
}

void LynxResourceLoaderDarwin::VerifyLynxTemplateResource(const std::string& url,
                                                          pub::LynxResourceResponse& response) {
  // verify only when data valid;
  if (response.Success()) {
    if (response.bundle != nullptr) {
      return;
    }
    if (!response.data.empty()) {
      auto securityService = LynxService(LynxServiceSecurityProtocol);
      if (securityService) {
        NSData* nsData = [NSData dataWithBytesNoCopy:response.data.data()
                                              length:response.data.size()
                                        freeWhenDone:NO];
        LynxVerificationResult* result =
            [securityService verifyTASM:nsData
                                   view:nil
                                    url:[NSString stringWithUTF8String:url.c_str()]
                                   type:LynxTASMType::LynxTASMTypeDynamicComponent];
        if (!result.verified) {
          response.err_code = -1;
          response.err_msg = "tasm verify failed, url: " + url;
        }
      }
    }
  }
}

}  // namespace shell
}  // namespace lynx
