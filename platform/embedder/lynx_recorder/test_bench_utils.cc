// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_recorder/test_bench_utils.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include <cmath>
#include <cstring>

#include "third_party/zlib/zlib.h"

namespace lynx {
namespace embedder {

static const std::string BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string TestBenchDecode(const std::string& encoded) {
  std::string out_put;
  int in_len = encoded.size();
  int i = 0;
  int j = 0;
  int in = 0;
  unsigned char char_array_4[4], char_array_3[3];

  while (in_len-- && (((encoded[in] != '=') && isalnum(encoded[in])) ||
                      (encoded[in] == '+') || (encoded[in] == '/'))) {
    char_array_4[i++] = encoded[in];
    in++;
    if (i == 4) {
      for (i = 0; i < 4; i++) {
        char_array_4[i] = BASE64_CHARS.find(char_array_4[i]);
      }

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++) {
        out_put += char_array_3[i];
      }
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) {
      char_array_4[j] = 0;
    }

    for (j = 0; j < 4; j++) {
      char_array_4[j] = BASE64_CHARS.find(char_array_4[j]);
    }

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) {
      out_put += char_array_3[j];
    }
  }

  return out_put;
}

std::vector<uint8_t> TestBenchDecompress(const std::vector<uint8_t>& data) {
  if (data.empty()) {
    return std::vector<uint8_t>();
  }
  z_stream zs;
  memset(&zs, 0, sizeof(zs));
  if (inflateInit(&zs) != Z_OK) {
    return std::vector<uint8_t>();
  }
  zs.next_in = (Bytef*)data.data();
  zs.avail_in = data.size();
  int ret;
  char out_buffer[32768];
  std::vector<uint8_t> out_data;
  // get the decompressed bytes blockwise using repeated calls to inflate
  do {
    zs.next_out = reinterpret_cast<Bytef*>(out_buffer);
    zs.avail_out = sizeof(out_buffer);

    ret = inflate(&zs, 0);

    if (out_data.size() < zs.total_out) {
      out_data.insert(out_data.end(), out_buffer,
                      out_buffer + zs.total_out - out_data.size());
    }

  } while (ret == Z_OK);
  inflateEnd(&zs);
  if (ret != Z_STREAM_END) {  // an error occurred that was not EOF
    return std::vector<uint8_t>();
  }
  return out_data;
}

bool StringToInt(const std::string& input, int* output, uint8_t base) {
  int old_error = errno;
  errno = 0;
  char* endptr = nullptr;
  int64_t i = strtoll(input.c_str(), &endptr, base);
  bool valid = (errno == 0 && !input.empty() &&
                input.c_str() + input.length() == endptr && !isspace(input[0]));
  if (errno == 0) errno = old_error;
  if (valid) *output = static_cast<int>(i);
  return valid;
}

std::string ToJson(const rapidjson::Value& json) {
  rapidjson::Value msg(rapidjson::kObjectType);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json.Accept(writer);
  std::string str = buffer.GetString();

  return str;
}

}  // namespace embedder
}  // namespace lynx
