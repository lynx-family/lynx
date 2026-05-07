// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/public/tasm_codec.h"

#include <cassert>
#include <iostream>

#include "core/renderer/template_entry.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "core/template_bundle/lynx_template_bundle_converter.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_lazy_reader_delegate.h"

namespace {

void TestEncodeSuccess() {
  std::cout << "TestEncodeSuccess..." << std::endl;
  std::string valid_json = R"({"source":"<div>test</div>","config":{}})";
  auto res = lynx::tasm::codec::Encode(valid_json);
  assert(res.status == 0);
  assert(res.buffer.size() > 0);
  std::cout << "  PASSED" << std::endl;
}

void TestEncodeInvalidJson() {
  std::cout << "TestEncodeInvalidJson..." << std::endl;
  std::string invalid_json = "{invalid json}";
  auto res = lynx::tasm::codec::Encode(invalid_json);
  assert(res.status != 0);
  assert(!res.error_msg.empty());
  std::cout << "  PASSED" << std::endl;
}

void TestDecodeInvalidPointer() {
  std::cout << "TestDecodeInvalidPointer..." << std::endl;
  auto res = lynx::tasm::codec::Decode(nullptr, 0);
  assert(res.status != 0);
  assert(res.error_msg == "Invalid Buffer!");
  std::cout << "  PASSED" << std::endl;
}

void TestDecodeInvalidData() {
  std::cout << "TestDecodeInvalidData..." << std::endl;
  std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02};
  auto res =
      lynx::tasm::codec::Decode(invalid_data.data(), invalid_data.size());
  assert(res.status != 0);
  assert(!res.error_msg.empty());
  std::cout << "  PASSED" << std::endl;
}

void TestEncodeDecodeRoundTrip() {
  std::cout << "TestEncodeDecodeRoundTrip..." << std::endl;
  std::string valid_json =
      R"({"source":"<div>roundtrip test</div>","config":{}})";
  auto encode_res = lynx::tasm::codec::Encode(valid_json);
  assert(encode_res.status == 0);
  assert(encode_res.buffer.size() > 0);

  auto decode_res = lynx::tasm::codec::Decode(encode_res.buffer.data(),
                                              encode_res.buffer.size());
  assert(decode_res.status == 0);
  assert(!decode_res.result.empty());
  std::cout << "  PASSED (encoded=" << encode_res.buffer.size()
            << " bytes, decoded result length=" << decode_res.result.size()
            << ")" << std::endl;
}

void TestConvertWithNullPageConfig() {
  std::cout << "TestConvertWithNullPageConfig..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  // Default-constructed bundle has page_configs_ == nullptr
  assert(bundle.GetPageConfig() == nullptr);
  auto result = lynx::tasm::LynxTemplateBundleConverter::
      ConvertTemplateBundleToSerializedString(bundle);
  assert(!result.empty());
  assert(result.find("\"page-config\":null") != std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

void TestFromBinary() {
  std::cout << "TestFromBinary..." << std::endl;
  std::string valid_json =
      R"({"source":"<div>lazy reader test</div>","config":{}})";
  auto encode_res = lynx::tasm::codec::Encode(valid_json);
  assert(encode_res.status == 0);

  lynx::tasm::LynxTemplateBundle bundle;
  std::string error =
      bundle.FromBinary(std::move(encode_res.buffer), true, "test://bundle");
  assert(error.empty());
  assert(bundle.Size() > 0);
  std::cout << "  PASSED" << std::endl;
}

}  // namespace

int main() {
  std::cout << "=== TASM Codec Facade Unit Tests ===" << std::endl;

  TestEncodeSuccess();
  TestEncodeInvalidJson();
  TestDecodeInvalidPointer();
  TestDecodeInvalidData();
  TestEncodeDecodeRoundTrip();
  TestConvertWithNullPageConfig();
  TestFromBinary();

  std::cout << "=== All tests passed ===" << std::endl;
  return 0;
}
