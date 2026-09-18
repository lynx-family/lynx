// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <future>
#include <memory>
#include <vector>

#define private public
#define protected public

#include "core/runtime/lepus/context_binary_writer.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "core/template_bundle/template_codec/binary_decoder/element_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
namespace {

void WriteStyle(lepus::ContextBinaryWriter& writer, bool parsed, bool variables,
                bool multiple_defaults) {
  if (parsed) {
    writer.WriteCompactU32(static_cast<uint32_t>(CSSValuePattern::STRING));
  }
  writer.WriteU8(lepus::Value_String);
  writer.EncodeUtf8Str("12px");
  if (variables) {
    writer.WriteCompactU32(0);
    writer.EncodeUtf8Str("fallback");
    if (multiple_defaults) {
      writer.WriteU8(lepus::Value_Nil);
    }
  }
}

std::shared_ptr<TemplateBinaryReader> MakeReader(LynxTemplateBundle& bundle,
                                                 bool parsed, bool variables,
                                                 bool multiple_defaults) {
  lepus::ContextBinaryWriter writer(nullptr);
  writer.WriteCompactU32(1);
  writer.WriteU32(0);
  writer.WriteU8(static_cast<uint8_t>(ElementSectionEnum::ELEMENT_TAG_ENUM));
  writer.WriteU8(ELEMENT_VIEW);
  writer.WriteU8(static_cast<uint8_t>(ElementSectionEnum::ELEMENT_ID_SELECTOR));
  writer.EncodeUtf8Str("leaf");
  writer.WriteU8(
      static_cast<uint8_t>(ElementSectionEnum::ELEMENT_PARSED_STYLES));
  writer.WriteCompactU32(1);
  writer.WriteCompactU32(kPropertyIDWidth);
  WriteStyle(writer, parsed, variables, multiple_defaults);
  writer.WriteCompactU32(0);
  writer.WriteU8(static_cast<uint8_t>(ElementSectionEnum::ELEMENT_CHILDREN));
  writer.WriteCompactU32(0);
  auto reader = std::make_shared<TemplateBinaryReader>(
      std::make_unique<lepus::ByteArrayInputStream>(writer.byte_array()),
      &bundle);
  reader->SetStringList(std::make_shared<std::vector<base::String>>(
      writer.string_table_.string_list));
  reader->element_templates_router_.descriptor_offset_ = 0;
  reader->element_templates_router_.start_offsets_.insert_or_assign("root", 0);
  reader->enable_css_parser_ = parsed;
  reader->enable_css_variable_ = variables;
  reader->enable_css_variable_multi_default_value_ = multiple_defaults;
  reader->enable_css_font_face_extension_ = true;
  reader->compile_options_.target_sdk_version_ = "2.14";
  bundle.lazy_reader_ = reader;
  return reader;
}

}  // namespace

TEST(ElementTemplateInfoStoreTest, MatchesLazyRecyclerDecodeOptions) {
  for (bool parsed : {false, true}) {
    for (bool variables : {false, true}) {
      for (bool multiple_defaults : {false, true}) {
        SCOPED_TRACE(parsed);
        SCOPED_TRACE(variables);
        SCOPED_TRACE(multiple_defaults);
        LynxTemplateBundle bundle;
        auto source = MakeReader(bundle, parsed, variables, multiple_defaults);
        auto recycled = source->CreateRecycler();
        auto* baseline = static_cast<TemplateBinaryReader*>(recycled.get());
        auto expected = baseline->DecodeElementTemplateInRender("root");
        ASSERT_TRUE(expected->exist_);
        ASSERT_EQ(expected->elements_.size(), 1u);

        auto store = bundle.GetElementTemplateInfoStore();
        EXPECT_TRUE(store->infos_.empty());
        EXPECT_EQ(source->Offset(), 0u);
        EXPECT_EQ(store->reader_->Offset(), 0u);
        EXPECT_EQ(store->reader_->enable_css_font_face_extension_,
                  baseline->enable_css_font_face_extension_);
        EXPECT_EQ(store->reader_->compile_options_.target_sdk_version_,
                  baseline->compile_options_.target_sdk_version_);
        auto actual = store->Get("root");
        ASSERT_TRUE(actual->exist_);
        ASSERT_EQ(actual->elements_.size(), 1u);
        EXPECT_EQ(actual->elements_[0].id_selector_,
                  expected->elements_[0].id_selector_);
        ASSERT_NE(actual->elements_[0].parsed_styles_, nullptr);
        EXPECT_EQ(actual->elements_[0].parsed_styles_->first,
                  expected->elements_[0].parsed_styles_->first);
        EXPECT_EQ(actual->elements_[0].parsed_styles_->second,
                  expected->elements_[0].parsed_styles_->second);

        bundle.GreedyConstructElements();
        auto elements = bundle.TryGetElements("root");
        ASSERT_TRUE(elements.has_value());
        EXPECT_EQ(elements->size(), 1u);
      }
    }
  }
}

TEST(ElementTemplateInfoStoreTest, PreservesStaticAttributePreprocessing) {
  lepus::ContextBinaryWriter writer(nullptr);
  writer.WriteCompactU32(1);
  writer.WriteCompactU32(kPropertyIDWidth);
  WriteStyle(writer, false, false, false);
  TemplateBinaryReader source(
      std::make_unique<lepus::ByteArrayInputStream>(writer.byte_array()));
  source.SetStringList(std::make_shared<std::vector<base::String>>(
      writer.string_table_.string_list));
  source.compile_options_.target_sdk_version_ = "2.14";
  auto recycled = source.CreateRecycler();
  auto* baseline = static_cast<TemplateBinaryReader*>(recycled.get());
  auto candidate = source.CreateElementTemplateReader();
  StyleMap expected;
  StyleMap actual;
  RawStyleMap expected_raw;
  RawStyleMap actual_raw;
  ASSERT_TRUE(baseline->DecodeCSSAttributes(expected, expected_raw,
                                            CSSParserConfigs()));
  ASSERT_TRUE(
      candidate->DecodeCSSAttributes(actual, actual_raw, CSSParserConfigs()));
  EXPECT_FALSE(expected.empty());
  EXPECT_TRUE(actual_raw.empty());
  EXPECT_EQ(actual, expected);
  EXPECT_EQ(actual_raw, expected_raw);
}

TEST(ElementTemplateInfoStoreTest,
     IndependentInputAndSharedCacheOutliveBundle) {
  std::shared_ptr<ElementTemplateInfoStore> store;
  {
    LynxTemplateBundle bundle;
    auto source = MakeReader(bundle, true, true, true);
    store = bundle.GetElementTemplateInfoStore();
    auto copied = bundle;
    EXPECT_EQ(copied.GetElementTemplateInfoStore(), store);
    EXPECT_TRUE(store->infos_.empty());
  }
  auto first =
      std::async(std::launch::async, [store]() { return store->Get("root"); });
  auto second =
      std::async(std::launch::async, [store]() { return store->Get("root"); });
  auto info = first.get();
  ASSERT_TRUE(info->exist_);
  EXPECT_EQ(info, second.get());
  EXPECT_EQ(info, store->Get("root"));
  EXPECT_EQ(store->infos_.size(), 1u);
  auto missing = store->Get("missing");
  EXPECT_FALSE(missing->exist_);
  EXPECT_EQ(missing, store->Get("missing"));
}

TEST(ElementTemplateInfoStoreTest,
     RecycledBundleCapturesInputBeforeFirstDescriptorAccess) {
  LynxTemplateBundle recycled_bundle;
  std::shared_ptr<const ElementTemplateInfo> published;
  {
    LynxTemplateBundle bundle;
    auto source = MakeReader(bundle, true, true, true);
    EXPECT_EQ(bundle.element_template_info_store_->reader_, nullptr);
    EXPECT_TRUE(bundle.element_template_info_store_->infos_.empty());

    auto recycler = bundle.CreateRecycler();
    auto* decoder = static_cast<TemplateBinaryReader*>(recycler.get());
    ASSERT_TRUE(decoder->GreedyDecodeElementTemplateSection());
    recycled_bundle = recycler->GetCompleteTemplateBundle();

    // Fail before destroying the source if recycling still depends on its
    // bundle. Accessing that dangling pointer is not a safe regression test.
    ASSERT_NE(recycled_bundle.element_template_info_store_->reader_, nullptr);
    EXPECT_EQ(recycled_bundle.element_template_info_store_,
              bundle.element_template_info_store_);
    published = recycled_bundle.element_template_info_store_->Get("root");
    ASSERT_TRUE(published->exist_);
  }

  auto copied_bundle = recycled_bundle;
  EXPECT_EQ(&copied_bundle.GetElementTemplateInfo("root"), published.get());
  EXPECT_EQ(copied_bundle.GetElementTemplateInfoStore(),
            recycled_bundle.GetElementTemplateInfoStore());
  EXPECT_FALSE(copied_bundle.GetElementTemplateInfo("missing").exist_);
}

TEST(ElementTemplateInfoStoreTest,
     GreedyRecyclerPreservesPublishedDescriptors) {
  LynxTemplateBundle bundle;
  auto source = MakeReader(bundle, true, true, true);
  for (int index = 0; index < 64; ++index) {
    source->element_templates_router_.start_offsets_.insert_or_assign(
        "template-" + std::to_string(index), 0);
  }
  auto store = bundle.GetElementTemplateInfoStore();
  auto published = store->Get("root");
  auto recycled = source->CreateRecycler();
  auto* greedy = static_cast<TemplateBinaryReader*>(recycled.get());
  auto decode = std::async(std::launch::async, [greedy]() {
    return greedy->GreedyDecodeElementTemplateSection();
  });
  for (int index = 0; index < 64; ++index) {
    auto key = "template-" + std::to_string(index);
    EXPECT_TRUE(store->Get(key)->exist_);
    EXPECT_EQ(published, store->Get("root"));
  }
  EXPECT_TRUE(decode.get());
  EXPECT_EQ(store->infos_.size(), 65u);
  EXPECT_EQ(published, store->Get("root"));
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
