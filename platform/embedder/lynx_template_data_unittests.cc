// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_template_data_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxTemplateData, Create) {
  lynx_template_data_t* template_data =
      lynx_template_data_create_from_json("{\"key\": \"value\"}");
  EXPECT_TRUE(template_data->template_data != nullptr);
  EXPECT_TRUE(template_data->template_data->value().IsObject());
  lynx_template_data_release(template_data);
}

TEST(LynxTemplateData, Null) {
  lynx_template_data_t* template_data =
      lynx_template_data_create_from_json(nullptr);
  EXPECT_TRUE(template_data->template_data->value().IsNil());
  lynx_template_data_release(template_data);
}

TEST(LynxTemplateData, ProcessorName) {
  lynx_template_data_t* template_data =
      lynx_template_data_create_from_json(nullptr);
  lynx_template_data_mark_state(template_data, "processor_name");
  EXPECT_STREQ(template_data->template_data->PreprocessorName().c_str(),
               "processor_name");
  lynx_template_data_release(template_data);
}

TEST(LynxTemplateData, ReadOnly) {
  lynx_template_data_t* template_data =
      lynx_template_data_create_from_json(nullptr);
  lynx_template_data_set_read_only(template_data, 1);
  EXPECT_TRUE(template_data->template_data->IsReadOnly());
  lynx_template_data_release(template_data);
}
