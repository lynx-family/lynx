// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/http_service/lynx_http_service_impl.h"

#import <Foundation/Foundation.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace lynx {
namespace embedder {

void LynxHttpServiceImpl::Request(std::shared_ptr<pub::LynxHttpRequest> http_request,
                                  std::shared_ptr<pub::LynxHttpResponse> http_response) {
  NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:http_request->GetUrl().c_str()]];
  NSMutableURLRequest *nsRequest = [NSMutableURLRequest requestWithURL:url];
  nsRequest.HTTPMethod = [NSString stringWithUTF8String:http_request->GetMethod().c_str()];
  for (const auto &header : http_request->GetHeaders()) {
    [nsRequest setValue:[NSString stringWithUTF8String:header.second.c_str()]
        forHTTPHeaderField:[NSString stringWithUTF8String:header.first.c_str()]];
  }
  const std::vector<uint8_t> &body = http_request->GetBody();
  if (!body.empty()) {
    nsRequest.HTTPBody = [NSData dataWithBytes:body.data() length:body.size()];
  }

  NSURLSession *session = [NSURLSession sharedSession];
  NSURLSessionDataTask *dataTask =
      [session dataTaskWithRequest:nsRequest
                 completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                                     NSError *_Nullable error) {
                   if (data && data.length > 0) {
                     http_response->SetBody(
                         (uint8_t *)data.bytes, data.length,
                         [](uint8_t *body, size_t length, void *opaque) { CFRelease(opaque); },
                         (__bridge_retained void *)data);
                   }
                   if (error) {
                     static const int kSdkErrorStatusCode = 499;
                     http_response->SetStatusCode(kSdkErrorStatusCode);
                     http_response->SetStatusText([error.localizedDescription UTF8String]);
                   } else {
                     NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
                     http_response->SetStatusCode(httpResponse.statusCode);
                     for (NSString *key in httpResponse.allHeaderFields) {
                       NSString *value = httpResponse.allHeaderFields[key];
                       if (key && value) {
                         http_response->AddHeader([key UTF8String], [value UTF8String]);
                       }
                     }
                   }
                 }];

  [dataTask resume];
}

}  // namespace embedder
}  // namespace lynx
