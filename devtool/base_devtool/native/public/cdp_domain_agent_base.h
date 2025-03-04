// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_DOMAIN_AGENT_BASE_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_DOMAIN_AGENT_BASE_H_
#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/message_sender.h"

namespace lynx {
namespace devtool {

/**
 *  When you want to handle CDP messages, you can inherit from this interface
 * and add it to DevToolAgent. You can implement it specifically referring to
 *  CDPDomainAgentExample.
 */
class CDPDomainAgentBase {
 public:
  virtual ~CDPDomainAgentBase() = default;
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& msg) = 0;
  int CompressData(const std::string& tag, const std::string& data,
                   Json::Value& value, const std::string& key);

  int GetCompressionThreshold() const;
  void SetCompressionThreshold(uint32_t threshold);

  bool UseCompression() const;

 protected:
  bool use_compression_ = false;
  uint32_t compression_threshold_ = 10240;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_DOMAIN_AGENT_BASE_H_
