// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LYNX_RESOURCE_LOADER_DARWIN_H_
#define CORE_RESOURCE_LYNX_RESOURCE_LOADER_DARWIN_H_

#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "core/public/lynx_resource_loader.h"

#import <Foundation/Foundation.h>
#import <Lynx/LynxDynamicComponentFetcher.h>
#import <Lynx/LynxExternalResourceFetcherWrapper.h>
#import <Lynx/LynxGenericResourceFetcher.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxTemplateResourceFetcher.h>
#import "LynxErrorReceiverProtocol.h"

@protocol TemplateRenderCallbackProtocol;

namespace lynx {
namespace shell {

class LynxResourceLoaderDarwin : public pub::LynxResourceLoader {
 public:
  LynxResourceLoaderDarwin(LynxProviderRegistry* providerRegistry,
                           id<LynxDynamicComponentFetcher> dynamicComponentFetcher,
                           id<LynxErrorReceiverProtocol> errorReceiver,
                           id<LynxTemplateResourceFetcher> genericTemplateFetcher,
                           id<LynxGenericResourceFetcher> genericResourceFetcher);
  ~LynxResourceLoaderDarwin() override = default;

  void LoadResource(const pub::LynxResourceRequest& request, bool request_in_current_thread,
                    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback) override;

 private:
  using CopyableClosure =
      fml::internal::CopyableLambda<base::MoveOnlyClosure<void, pub::LynxResourceResponse&>>;

  NSData* LoadJSSource(const std::string& name);
  NSData* LoadLynxJSAsset(const std::string& name, NSURL& bundleUrl, NSURL& debugBundleUrl);

  /**
   * Try to fetch script by ExternalJS Provider, if provider not set, return false;
   */
  bool FetchScriptByProvider(const std::string& url, CopyableClosure callback);

  /**
   * Try to fetch template by Generic Fetcher. if generic fetcher not set, return false;
   */
  bool FetchTemplateByGenericFetcher(const std::string& url, CopyableClosure callback);

  /**
   * Try to fetch resource by Generic Fetcher. if generic fetcher not set, return false;
   */
  bool FetchResourceByGenericFetcher(const std::string& url, CopyableClosure callback);

  /**
   * Try to fetch template by LynxResourceProvider registered with string type:
   * LYNX_PROVIDER_TYPE_LAZY_BUNDLE it's only used by js load lazy bundle by now.
   * TODO(@nihao.royal): it may be removed later.
   *
   *  @return false if provider not registered; true if request consumed by provider;
   */
  bool FetchTemplateByProvider(const std::string& url, CopyableClosure callback);

  /**
   * Try to fetch template by FetcherWrapper, it will use builtin resourceService or registered
   * dynamicComponentFetcher
   */
  bool FetchTemplateByFetcherWrapper(const std::string& url, CopyableClosure callback,
                                     bool request_in_current_thread);

  static void FetchExternalResourceComplete(NSData* data, NSError* error, NSString* nsUrl,
                                            id<LynxErrorReceiverProtocol> weakErrorReceiver,
                                            CopyableClosure callback);

  LynxProviderRegistry* _providerRegistry;
  LynxExternalResourceFetcherWrapper* _fetcher_wrapper;
  id<LynxTemplateResourceFetcher> _templateResourceFetcher;
  id<LynxGenericResourceFetcher> _genericResourceFetcher;

  __weak id<LynxErrorReceiverProtocol> _errorReceiver;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_RESOURCE_LYNX_RESOURCE_LOADER_DARWIN_H_
