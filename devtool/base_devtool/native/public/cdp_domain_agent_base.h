// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_DOMAIN_AGENT_BASE_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_DOMAIN_AGENT_BASE_H_
#include <cstdint>
#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/base_devtool_export.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"

namespace lynx {
namespace devtool {

/**
 *  When you want to handle CDP messages, you can inherit from this interface
 * and add it to DevToolAgent. You can implement it specifically referring to
 *  CDPDomainAgentExample.
 */
class BASE_DEVTOOL_EXPORT CDPDomainAgentBase {
 public:
  virtual ~CDPDomainAgentBase() = default;
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& msg) = 0;

  // Responder-based entry point. New agents should override this and fill the
  // response through |responder| (Result() / SendErrorResponse()) instead of
  // assembling the CDP envelope by hand. The default implementation bridges to
  // the legacy sender-based CallMethod so existing agents keep working during
  // the migration: it retrieves the raw sender from the responder and forwards
  // to CallMethod(sender, msg).
  virtual void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                          const Json::Value& msg);

  int CompressData(const std::string& tag, const std::string& data,
                   Json::Value& value, const std::string& key);

  int GetCompressionThreshold() const;
  void SetCompressionThreshold(uint32_t threshold);

  bool UseCompression() const;

 protected:
  void SendNotImplementedResponse(const std::shared_ptr<MessageSender>& sender,
                                  int64_t id, const std::string& method);

  bool use_compression_ = false;
  uint32_t compression_threshold_ = 10240;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_DOMAIN_AGENT_BASE_H_
