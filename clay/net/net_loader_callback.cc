// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/net_loader_callback.h"

#include "clay/fml/logging.h"
#include "clay/net/resource_type.h"

namespace clay {

NetLoaderCallback::NetLoaderCallback() = default;
NetLoaderCallback::~NetLoaderCallback() = default;

void NetLoaderCallback::OnSucceeded(RawResource result) {
  FML_DCHECK(!callback_issued_);
  callback_issued_ = true;
  succeeded_func_(request_seq_, std::move(result));
}

void NetLoaderCallback::OnFailed(const std::string& error) {
  FML_DCHECK(!callback_issued_);
  callback_issued_ = true;
  failed_func_(request_seq_, error);
}

}  // namespace clay
