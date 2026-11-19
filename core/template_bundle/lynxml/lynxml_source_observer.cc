// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynxml/lynxml_source_observer.h"

#include <utility>

namespace lynx {
namespace lynxml {
namespace {

constexpr char kEngineVersionAttributeName[] = "engine-version";
constexpr char kThreadAttributeName[] = "thread";
constexpr char kMainThreadValue[] = "main";
constexpr char kBackgroundThreadValue[] = "background";
constexpr char kStyleBlockDescription[] = "<style>";
constexpr char kMainThreadScriptBlockDescription[] = "<script thread=\"main\">";
constexpr char kBackgroundThreadScriptBlockDescription[] =
    "<script thread=\"background\">";

}  // namespace

void LynxMLSourceObserver::OnDocumentStart(std::vector<Attribute> attributes) {
  // TODO: Consume the remaining <lynx> attributes as page config. Currently,
  // only engine-version is handled.
  for (const auto& attribute : attributes) {
    if (attribute.name == kEngineVersionAttributeName) {
      result_.sources.engine_version = attribute.value;
    }
  }
}

void LynxMLSourceObserver::OnSourceBlock(SourceBlock source_block) {
  if (!result_.error.empty()) {
    return;
  }

  switch (source_block.type) {
    case SourceBlockType::kStyle:
      HandleStyleSourceBlock(std::move(source_block));
      break;
    case SourceBlockType::kScript:
      HandleScriptSourceBlock(std::move(source_block));
      break;
  }
}

void LynxMLSourceObserver::HandleStyleSourceBlock(SourceBlock source_block) {
  if (!source_block.attributes.empty()) {
    result_.error =
        "unsupported LynxML feature: '<style>' attributes are not supported";
    return;
  }

  StoreSourceBlock(std::move(source_block.source), result_.sources.style,
                   has_style_, kStyleBlockDescription);
}

void LynxMLSourceObserver::HandleScriptSourceBlock(SourceBlock source_block) {
  const Attribute* thread = nullptr;
  for (const auto& attribute : source_block.attributes) {
    if (attribute.name == kThreadAttributeName) {
      thread = &attribute;
    } else {
      result_.error =
          "unsupported LynxML feature: unsupported '<script>' attribute '" +
          attribute.name + "'";
      return;
    }
  }

  if (thread == nullptr || thread->value.empty()) {
    result_.error =
        "invalid LynxML: '<script>' requires a 'thread' attribute with value "
        "'main' or 'background'";
    return;
  }

  if (thread->value == kMainThreadValue) {
    StoreSourceBlock(
        std::move(source_block.source), result_.sources.main_thread_script,
        has_main_thread_script_, kMainThreadScriptBlockDescription);
  } else if (thread->value == kBackgroundThreadValue) {
    StoreSourceBlock(std::move(source_block.source),
                     result_.sources.background_thread_script,
                     has_background_thread_script_,
                     kBackgroundThreadScriptBlockDescription);
  } else {
    result_.error =
        "invalid LynxML: '<script>' attribute 'thread' must be 'main' or "
        "'background'";
  }
}

void LynxMLSourceObserver::StoreSourceBlock(std::string source,
                                            std::string& destination,
                                            bool& has_block,
                                            const char* block_description) {
  if (has_block) {
    result_.error = "unsupported LynxML feature: multiple '" +
                    std::string(block_description) +
                    "' blocks are not supported";
    return;
  }
  destination = std::move(source);
  has_block = true;
}

void LynxMLSourceObserver::OnDocumentEnd() { completed_ = true; }

void LynxMLSourceObserver::OnError(ParseError error) {
  if (result_.error.empty()) {
    result_.error = error.ToString();
  }
}

LynxMLSourceParseResult LynxMLSourceObserver::TakeResult() {
  if (result_.error.empty() && !completed_) {
    result_.error = "invalid LynxML: parser did not complete";
  }
  return std::move(result_);
}

LynxMLSourceParseResult ParseLynxMLSources(std::string_view source) {
  LynxMLSourceObserver observer;
  LynxMLParser parser(observer);
  parser.Parse(source);
  return observer.TakeResult();
}

}  // namespace lynxml
}  // namespace lynx
