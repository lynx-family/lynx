// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_ELEMENT_BINARY_READER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_ELEMENT_BINARY_READER_H_

#include <memory>
#include <string>
#include <utility>

#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_base_css_reader.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;

class ElementBinaryReader : public LynxBinaryBaseCSSReader {
 public:
  explicit ElementBinaryReader(std::unique_ptr<lepus::InputStream> stream)
      : LynxBinaryBaseCSSReader(std::move(stream)){};

  ~ElementBinaryReader() override = default;

  ElementBinaryReader(const ElementBinaryReader&) = delete;
  ElementBinaryReader& operator=(const ElementBinaryReader&) = delete;

  ElementBinaryReader(ElementBinaryReader&&) = default;
  ElementBinaryReader& operator=(ElementBinaryReader&&) = default;

  // Single template decoding API:
  // 1. This API is used to decode element binary and return an element tree.
  fml::RefPtr<FiberElement> DecodeSingleTemplate(ElementManager* manager,
                                                 TemplateAssembler* tasm);

  // Multiple templates decoding API:
  // 1. This API is used to decode the router for multiple templates.
  bool DecodeElementTemplatesRouter();

  // 2. This API is used to decode the router for parsed styles.
  bool DecodeParsedStylesRouter();

  // 2. This API is used to decode the binary of a specific template with key
  // and return the element info.
  std::shared_ptr<ElementTemplateInfo> DecodeTemplatesInfoWithKey(
      const std::string& key);

 protected:
  // These are the APIs used for decoding data into fiber elements.
  virtual bool DecodeElementChildrenSection(fml::RefPtr<FiberElement>& element,
                                            ElementManager* manager,
                                            TemplateAssembler* tasm,
                                            int64_t parent_component_id);
  bool DecodeBuiltinAttributesSection(fml::RefPtr<FiberElement>& element);
  bool DecodeIDSelectorSection(fml::RefPtr<FiberElement>& element);
  bool DecodeInlineStylesSection(fml::RefPtr<FiberElement>& element);
  bool DecodeClassesSection(fml::RefPtr<FiberElement>& element);
  bool DecodeEventsSection(fml::RefPtr<FiberElement>& element);
  bool DecodePiperEventsSection(fml::RefPtr<FiberElement>& element);
  bool DecodeAttributesSection(fml::RefPtr<FiberElement>& element);
  bool DecodeDatasetSection(fml::RefPtr<FiberElement>& element);
  bool DecodeParsedStylesSection(fml::RefPtr<FiberElement>& element);
  bool DecodeElementRecursively(fml::RefPtr<FiberElement>& element,
                                ElementManager* manager,
                                TemplateAssembler* tasm,
                                int64_t parent_component_id);
  bool ConstructElement(ElementSectionEnum section_type,
                        fml::RefPtr<FiberElement>& element,
                        ElementManager* manager, TemplateAssembler* tasm,
                        int64_t parent_component_id);
  // These are the APIs used for decoding data into element infos.
  bool DecodeTemplates(ElementTemplateInfo& info);
  bool DecodeElementRecursively(ElementInfo& info);
  bool DecodeBuiltinAttributesSection(ElementInfo& info);
  bool DecodeIDSelectorSection(ElementInfo& info);
  bool DecodeInlineStylesSection(ElementInfo& info);
  bool DecodeClassesSection(ElementInfo& info);
  bool DecodeEventsSection(ElementInfo& info);
  bool DecodeAttributesSection(ElementInfo& info);
  bool DecodeDatasetSection(ElementInfo& info);
  bool DecodeParsedStyleStringKeySection(ElementInfo& info);
  bool DecodeParsedStylesSection(ElementInfo& info);

  bool DecodeElementChildrenSection(ElementInfo& info);
  bool DecodeEnumTagSection(ElementInfo& info);
  bool DecodeStrTagSection(ElementInfo& info);

  // Common method
  bool DecodeStringKeyRouter(StringKeyRouter& router);
  bool DecodeOrderedStringKeyRouter(OrderedStringKeyRouter& router);
  bool DecodeConstructionInfoSection();
  bool DecodeParsedStylesSectionInternal(StyleMap& style_map,
                                         CSSVariableMap& css_var_map);
  const std::shared_ptr<ParsedStyles>& GetParsedStyles(const std::string& key);

 protected:
  // The router for element templates
  OrderedStringKeyRouter element_templates_router_;

  // parsed styles router
  StringKeyRouter string_key_parsed_styles_router_;

  // the cache of parsed styles
  ParsedStylesMap parsed_styles_cache_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_ELEMENT_BINARY_READER_H_
