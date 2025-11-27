// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/public/capi/lynx_resource_request_capi.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxResourceRequest, Create) {
  lynx_resource_request_t* request = lynx_resource_request_create(
      "assets://main.lynx.bundle", kLynxResourceTypeGeneric);
  EXPECT_TRUE(request != nullptr);
  EXPECT_EQ(lynx_resource_request_get_type(request), kLynxResourceTypeGeneric);
  EXPECT_STREQ(lynx_resource_request_get_url(request),
               "assets://main.lynx.bundle");
  lynx_resource_request_release(request);
}
