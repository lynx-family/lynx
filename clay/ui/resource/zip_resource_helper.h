// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_ZIP_RESOURCE_HELPER_H_
#define CLAY_UI_RESOURCE_ZIP_RESOURCE_HELPER_H_

#include <string>
#include <vector>

namespace clay {

class ZipResourceHelper {
 public:
  static bool UnzipToPath(const std::string& src_path,
                          const std::string& target_path);
  static int Decompress(const std::vector<char>& compressed_data,
                        std::vector<char>& decompressed_data);
};

}  // namespace clay

#endif  // CLAY_UI_RESOURCE_ZIP_RESOURCE_HELPER_H_
