// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/testbench_utils_embedder.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

#include "third_party/modp_b64/modp_b64.h"
#include "third_party/zlib/zlib.h"

namespace lynx {
namespace tasm {
namespace replay {

#define CHUNK 1024 * 10

int DecompressString(const unsigned char* compressed_data,
                     unsigned char** uncompressed_data,
                     size_t* uncompress_data_size, size_t compressed_size) {
  z_stream strm;
  unsigned char out[CHUNK];
  int ret;

  // Initialize zlib stream
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;

  ret = inflateInit(&strm);
  if (ret != Z_OK) {
    return ret;
  }
  strm.avail_in = static_cast<unsigned int>(compressed_size);
  strm.next_in = const_cast<unsigned char*>(compressed_data);
  do {
    strm.avail_out = CHUNK;
    strm.next_out = out;
    ret = inflate(&strm, Z_SYNC_FLUSH);
    switch (ret) {
      case Z_NEED_DICT:
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        inflateEnd(&strm);
        return ret;
    }

    int have = CHUNK - strm.avail_out;
    *uncompressed_data = static_cast<unsigned char*>(
        realloc(*uncompressed_data, *uncompress_data_size + have));
    memcpy(*uncompressed_data + *uncompress_data_size, out, have);
    memset(out, 0, CHUNK);
    *uncompress_data_size += have;
  } while (strm.avail_out == 0);

  inflateEnd(&strm);

  return Z_OK;
}

std::string TestBenchUtilsEmbedder::ParserTestBenchRecordData(
    const std::string& source) {
  size_t end_idx = source.length();
  // filter out trailing '\0'
  while (end_idx > 0 && source[end_idx - 1] == '\0') end_idx--;
  std::string input(source.begin(), source.begin() + end_idx);
  lynx_modp_b64_decode(input);
  unsigned char* uncompress_data = nullptr;
  size_t uncompress_data_size = 0;
  int ret =
      DecompressString(reinterpret_cast<const unsigned char*>(input.c_str()),
                       &uncompress_data, &uncompress_data_size, input.size());
  if (ret == Z_OK) {
    return std::string(reinterpret_cast<char*>(uncompress_data),
                       uncompress_data_size);
  }
  return "parse error!";
}
}  // namespace replay
}  // namespace tasm
}  // namespace lynx
