// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_util.h"

#include "base/include/log/logging.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/zlib/zlib.h"

namespace lynx {
namespace devtool {

int InspectorUtil::CompressData(const std::string& tag, const std::string& data,
                                Json::Value& value, const std::string& key) {
  uLong compressed_size = compressBound(data.size());
  std::unique_ptr<Byte[]> compressed_data =
      std::make_unique<Byte[]>(compressed_size);
  int z_result =
      compress(compressed_data.get(), &compressed_size,
               reinterpret_cast<const Cr_z_Bytef*>(data.c_str()), data.size());
  if (z_result == Z_OK) {
    unsigned long base64_size = modp_b64_encode_len(compressed_size);
    std::unique_ptr<char[]> base64_data = std::make_unique<char[]>(base64_size);
    modp_b64_encode(base64_data.get(),
                    reinterpret_cast<const char*>(compressed_data.get()),
                    compressed_size);

    LOGI("[" << tag << "] original size " << data.size() << ", compressed size "
             << compressed_size << ", base64 size " << base64_size);

    value["compress"] = true;
    value[key] = std::string(base64_data.get());
  }

  return z_result;
}

}  // namespace devtool
}  // namespace lynx
