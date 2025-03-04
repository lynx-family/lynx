// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TEMPLATE_THEMED_H_
#define CORE_RENDERER_TEMPLATE_THEMED_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

typedef std::unordered_map<std::string, std::string> ThemedRes;
typedef std::unordered_map<std::string, std::shared_ptr<ThemedRes>> ThemeResMap;

struct Themed {
  typedef struct _TransMap {
    std::string name_, default_, fallback_;
    ThemeResMap resMap_;
    std::shared_ptr<ThemedRes> currentRes_, curFallbackRes_;
  } TransMap;
  using PageTransMaps =
      std::unordered_map<uint32_t, std::shared_ptr<std::vector<TransMap>>>;
  PageTransMaps pageTransMaps;
  std::shared_ptr<std::vector<TransMap>> currentTransMap;
  bool hasTransConfig = false, hasAnyCurRes = false, hasAnyFallback = false;

  void reset() {
    hasTransConfig = hasAnyCurRes = hasAnyFallback = false;
    pageTransMaps.clear();
    currentTransMap = nullptr;
  }

  void ResetWithPageTransMaps(const Themed::PageTransMaps& page_trans_maps) {
    if (!page_trans_maps.empty()) {
      reset();
      pageTransMaps = page_trans_maps;
      hasTransConfig = true;
    }
  }
};

struct ThemedTrans {
  ThemeResMap fileMap_;
  typedef struct _TransMap {
    ThemeResMap pathMap_;
    ThemedRes default_, fallback_;
    std::vector<std::string> priority_;
  } TransMap;
  std::unordered_map<uint32_t, std::shared_ptr<TransMap>> pageTransMap_;
  friend class TemplateBinaryWriter;
  friend class TemplateBinaryReader;
  friend class TemplateBinaryReaderSSR;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TEMPLATE_THEMED_H_
