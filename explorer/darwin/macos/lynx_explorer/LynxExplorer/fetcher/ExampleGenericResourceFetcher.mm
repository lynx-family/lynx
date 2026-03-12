// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxExplorer/fetcher/ExampleGenericResourceFetcher.h"
#import <Foundation/Foundation.h>

namespace lynx {
namespace example {

void ExampleGenericResourceFetcher::FetchResource(
    std::shared_ptr<pub::resource::LynxResourceRequest> fetcher_equest,
    std::shared_ptr<pub::resource::LynxResourceResponse> fetcher_response) {
  const char *url_str = fetcher_equest->GetUrl();
  NSString *ns_string = [NSString stringWithUTF8String:url_str];

  NSURL *url = [NSURL URLWithString:ns_string];
  NSMutableURLRequest *nsRequest = [NSMutableURLRequest requestWithURL:url];
  NSURLSession *session = [NSURLSession sharedSession];
  NSURLSessionDataTask *dataTask =
      [session dataTaskWithRequest:nsRequest
                 completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                                     NSError *_Nullable error) {
                   if (data && data.length > 0) {
                     fetcher_response->SetCode(0);
                     fetcher_response->SetData(
                         (uint8_t *)data.bytes, data.length,
                         [](uint8_t *body, size_t length, void *opaque) { CFRelease(opaque); },
                         (__bridge_retained void *)data);
                   } else {
                     fetcher_response->SetCode(-1);
                     fetcher_response->SetErrorMessage("error");
                   }
                   fetcher_response->Complete();
                 }];

  [dataTask resume];
}

void ExampleGenericResourceFetcher::FetchResourcePath(
    std::shared_ptr<pub::resource::LynxResourceRequest> request,
    std::shared_ptr<pub::resource::LynxResourceResponse> response) {}

}  // namespace example
}  // namespace lynx
