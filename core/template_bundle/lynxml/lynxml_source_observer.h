// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_SOURCE_OBSERVER_H_
#define CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_SOURCE_OBSERVER_H_

#include <string>
#include <string_view>
#include <vector>

#include "core/template_bundle/lynxml/lynxml_parser.h"

namespace lynx {
namespace lynxml {

struct LynxMLSources {
  std::string engine_version;
  std::string main_thread_script;
  std::string background_thread_script;
  std::string style;
};

struct LynxMLSourceParseResult {
  LynxMLSources sources;
  std::string error;
};

class LynxMLSourceObserver final : public LynxMLParser::Observer {
 public:
  void OnDocumentStart(std::vector<Attribute> attributes) override;
  void OnSourceBlock(SourceBlock source_block) override;
  void OnDocumentEnd() override;
  void OnError(ParseError error) override;

  LynxMLSourceParseResult TakeResult();

 private:
  void HandleStyleSourceBlock(SourceBlock source_block);
  void HandleScriptSourceBlock(SourceBlock source_block);
  void StoreSourceBlock(std::string source, std::string& destination,
                        bool& has_block, const char* block_description);

  LynxMLSourceParseResult result_;
  bool has_main_thread_script_{false};
  bool has_background_thread_script_{false};
  bool has_style_{false};
  bool completed_{false};
};

// Parses a LynxML document and collects its supported top-level source blocks.
// An error is returned if a supported source-block type occurs more than once.
LynxMLSourceParseResult ParseLynxMLSources(std::string_view source);

}  // namespace lynxml
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_SOURCE_OBSERVER_H_
