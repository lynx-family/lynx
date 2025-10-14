// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_NET_LOADER_CALLBACK_H_
#define CLAY_NET_NET_LOADER_CALLBACK_H_

#include <functional>
#include <string>
#include <utility>

namespace clay {

struct RawResource;

class NetLoaderCallback {
 public:
  using SucceededFunc = std::function<void(size_t, RawResource&&)>;
  using FailedFunc = std::function<void(size_t, const std::string&)>;

  NetLoaderCallback();
  ~NetLoaderCallback();

  void set_succeeded_func(SucceededFunc&& succeeded) {
    succeeded_func_ = std::move(succeeded);
  }
  void set_succeeded_func(const SucceededFunc& succeeded) {
    succeeded_func_ = succeeded;
  }

  void set_failed_func(FailedFunc&& failed) {
    failed_func_ = std::move(failed);
  }
  void set_failed_func(const FailedFunc& failed) { failed_func_ = failed; }

  void SetRequestSeq(size_t request_seq) { request_seq_ = request_seq; }

  void OnSucceeded(RawResource);
  void OnFailed(const std::string& error);

 private:
  size_t request_seq_ = 0;
  bool callback_issued_ = false;
  SucceededFunc succeeded_func_;
  FailedFunc failed_func_;
};

}  // namespace clay

#endif  // CLAY_NET_NET_LOADER_CALLBACK_H_
