// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_I18N_I18N_H_
#define CORE_RENDERER_TASM_I18N_I18N_H_

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class TemplateAssembler;

class I18nBinder {
 public:
  I18nBinder() = default;
  virtual ~I18nBinder() = default;
  virtual void Bind(intptr_t ptr) = 0;
};

class I18n {
  enum Status : uint8_t {
    I18n_STATUS_IDLE = 0,
    I18n_STATUS_WAIT = 1,
    I18n_STATUS_FINISH = 2,
  };

  struct I18nWrapper {
    std::string data;
    Status status = I18n_STATUS_IDLE;
    std::unordered_set<std::string> reserve_keys;
    lepus::Value lepus_data;
  };

 public:
  static void Bind(intptr_t ptr);
  bool UpdateData(const std::string &key, const std::string &new_data);

  lepus::Value GetData(TemplateAssembler *tasm, const std::string &channel,
                       const std::string &fallback_url);

  // add config for channel, the config will be use to filter i18n resource
  void SetChannelConfig(const std::string &channel,
                        const lepus::Value &reserve_keys);

 private:
  std::unordered_map<std::string, I18nWrapper> i18n_record_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_I18N_I18N_H_
