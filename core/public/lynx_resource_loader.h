// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LYNX_RESOURCE_LOADER_H_
#define CORE_PUBLIC_LYNX_RESOURCE_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/closure.h"

namespace lynx {
namespace pub {

enum class LynxResourceType : int32_t {
  kGeneric = 0,
  kImage = 1,
  kFont = 2,
  kLottie = 3,
  kVideo = 4,
  kSvg = 5,
  kTemplate = 6,
  kLazyBundle = 7,  // LazyBundle from js
  kLynxCoreJs = 8,
  kExternalJs = 9,
  // There are some differences between JSLazyBundle and
  // TemplateLazyBundlein the old logic, so here is a new type to be
  // compatible with the old logic.
  kTemplateLazyBundle = 10,  // LazyBundle from template
  kAssets = 11,
  kI18nText = 12,
  kGraphics = 13,
  kTheme = 14,
};

struct ResourceLoadTiming {
  // all microseconds
  uint64_t request_start;                    // receive the request from client
  uint64_t request_internal_prepare_finish;  // internal prepare done, like
                                             // check url, fallback logic etc.
  uint64_t
      request_prepare_to_call_fetcher;      // start to prepare to call fetcher,
                                            // mostly it is the same as
                                            // request_internal_prepare_finish
  uint64_t request_send_to_fetcher;         // actually send request to fetcher
  uint64_t response_received_from_fetcher;  // actually receive response from
                                            // fetcher
  uint64_t response_trigger_callback;       // trigger callback from client
};

struct LynxResourceRequest {
  std::string url;
  LynxResourceType type;
};

struct LynxResourceResponse {
  std::vector<uint8_t> data;

  // used to represent template_bundle type that can be returned by platform.
  // TODO(nihao.royal): make LynxTemplateBundle a public class.
  void* bundle = nullptr;
  int32_t err_code = 0;
  std::string err_msg;
  ResourceLoadTiming timing;

  bool Success() const { return err_code == 0; }
};

class LynxStreamDelegate {
 public:
  virtual ~LynxStreamDelegate() = default;
  virtual void OnStart(size_t size) = 0;
  virtual void OnData(std::vector<uint8_t> data) = 0;
  virtual void OnEnd() = 0;
  virtual void OnError(std::string error_msg) = 0;
};

class LynxResourceLoader
    : public std::enable_shared_from_this<LynxResourceLoader> {
 public:
  virtual ~LynxResourceLoader() = default;

  virtual void LoadResource(
      const LynxResourceRequest& request, bool request_in_current_thread,
      base::MoveOnlyClosure<void, LynxResourceResponse&> callback) = 0;

  virtual void LoadResourcePath(
      const LynxResourceRequest& request,
      base::MoveOnlyClosure<void, LynxResourceResponse&> callback){};

  virtual void LoadStream(
      const LynxResourceRequest& request,
      const std::shared_ptr<LynxStreamDelegate>& stream_delegate){};

  virtual void SetEnableLynxResourceService(bool enable) {}
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_PUBLIC_LYNX_RESOURCE_LOADER_H_
