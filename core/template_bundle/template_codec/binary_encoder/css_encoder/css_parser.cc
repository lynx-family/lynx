// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_keyframes_token.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parse_token_group.h"

#define APP_TTSS "/app.ttss"
#define TTSS_SUFFIX ".ttss"
#define TYPE "type"
#define IMPORT_RULE "ImportRule"
#define HREF "href"
#define DEFAULT_CSS_IMPORT "_default_lynx.ttss"

namespace lynx {
namespace tasm {

static int32_t sCSSIDGenerator = 0;

CSSParser::CSSParser(const CompileOptions &compile_options)
    : compile_options_(compile_options) {}

bool CSSParser::Parse(const rapidjson::Value &value) {
  DCHECK(value.IsObject());
  sCSSIDGenerator = 0;
  ParseAppTTSS(value);
  ParseOtherTTSS(value);
  return true;
}

bool CSSParser::ParseCSSForFiber(const rapidjson::Value &css_map,
                                 const rapidjson::Value &css_source) {
  for (auto it = css_map.GetObject().begin(); it != css_map.GetObject().end();
       ++it) {
    const rapidjson::Value &id = it->name;
    ParseCSS(css_map, id, css_source);
  }
  return true;
}

bool CSSParser::ParseOtherTTSS(const rapidjson::Value &value) {
  for (auto it = value.GetObject().begin(); it != value.GetObject().end();
       ++it) {
    const char *path = it->name.GetString();
    if (base::EndsWith(path, TTSS_SUFFIX) &&
        !base::EndsWith(path, DEFAULT_CSS_IMPORT)) {
      ParseCSS(value, path);
    } else if (!base::EndsWith(path, TTSS_SUFFIX)) {
      LOGE("Warning: not support ttss path : %s" << path << "\n");
    }
  }
  return true;
}

void CSSParser::ParseCSS(const rapidjson::Value &value,
                         const std::string &path) {
  if (value.HasMember(path) && fragments_.find(path) == fragments_.end()) {
    std::vector<int32_t> dependent_css_list;
    // find import css
    for (rapidjson::Value::ConstValueIterator itr = value[path].Begin();
         itr != value[path].End(); ++itr) {
      if (itr->HasMember(TYPE) &&
          strcmp(itr->GetObject()[TYPE].GetString(), IMPORT_RULE) == 0 &&
          itr->GetObject().HasMember(HREF)) {
        std::string import_path = itr->GetObject()[HREF].GetString();
        // Ignore import if the imported css is default css that lynx
        // support for IDE
        if (base::EndsWith(import_path, DEFAULT_CSS_IMPORT)) {
          continue;
        }

        if (!base::EndsWith(import_path, TTSS_SUFFIX)) {
          LOGE("Warning: not support ttss path : %s" << import_path << "\n");
          continue;
        }

        if (fragments_.find(import_path) == fragments_.end() &&
            value.HasMember(import_path.c_str())) {
          ParseCSS(value, import_path);
        }
        dependent_css_list.emplace_back(fragments_[import_path]->id());
      }
    }
    // page css
    ParseCSS(value[path], path, dependent_css_list, sCSSIDGenerator++);
  }
}

void CSSParser::ParseAppTTSS(const rapidjson::Value &value) {
  if (value.HasMember(APP_TTSS)) {
    // app css
    ParseCSS(value, APP_TTSS);
  }
}

void CSSParser::MergeCSSParseToken(std::shared_ptr<CSSParseToken> &originToken,
                                   std::shared_ptr<CSSParseToken> &newToken) {
  StyleMap originStyle = originToken->GetAttributes();
  StyleMap newStyle = newToken->GetAttributes();
  for (auto iter : newStyle) {
    originStyle[iter.first] = std::move(iter.second);
  }
  originToken->SetAttributes(std::move(originStyle));
}

void CSSParser::ParseCSS(const rapidjson::Value &ttss, const std::string &path,
                         const std::vector<int32_t> &dependent_css_list,
                         int32_t fragment_id) {
  CSSParserTokenMap css;
  encoder::CSSKeyframesTokenMapForEncode keyframes;
  encoder::CSSFontFaceTokenMapForEncode fontfaces;
  std::vector<encoder::LynxCSSSelectorTuple> selector_tuple_list;
  for (int i = 0; i < ttss.Size(); i++) {
    if (CSSParseTokenGroup::IsCSSParseToken(ttss[i])) {
      if (compile_options_.enable_css_selector_) {
        ParseCSSTokensNew(selector_tuple_list, css, ttss[i], path);
      } else {
        ParseCSSTokens(css, ttss[i], path);
      }
    } else if (encoder::CSSKeyframesToken::IsCSSKeyframesToken(ttss[i])) {
      ParseCSSKeyframes(keyframes, ttss[i], path);
    } else if (CSSFontFaceToken::IsCSSFontFaceToken(ttss[i])) {
      ParseCSSFontFace(fontfaces, ttss[i], path);
    }
  }
  auto parse_result = new encoder::SharedCSSFragment(
      fragment_id, dependent_css_list, std::move(css), std::move(keyframes),
      std::move(fontfaces));
  parse_result->SetSelectorTuple(std::move(selector_tuple_list));
  shared_css_fragments_.push_back(
      std::unique_ptr<encoder::SharedCSSFragment>(parse_result));
  fragments_.insert({path, parse_result});
}

void HandleCascadeSelector(std::shared_ptr<CSSParseToken> &token,
                           std::string &key) {
  const auto &sheets = token->sheets();
  if (sheets.size() != 2) {
    return;
  }

  int child_type = sheets[0]->GetType();
  int parent_type = sheets[1]->GetType();
  if (((child_type & CSSSheet::CLASS_SELECT) == CSSSheet::CLASS_SELECT ||
       (child_type & CSSSheet::ID_SELECT) == CSSSheet::ID_SELECT) &&
      ((parent_type & CSSSheet::CLASS_SELECT) == CSSSheet::CLASS_SELECT ||
       (parent_type & CSSSheet::ID_SELECT) == CSSSheet::ID_SELECT)) {
    key += sheets[0]->GetSelector().str();
  } else {
    LOGE("Warning: Descendant Selector must be class or id selector: \"" +
         sheets[0]->GetSelector().str() + " " + sheets[1]->GetSelector().str() +
         "\"");
  }
}

void CSSParser::ParseCSSTokens(CSSParserTokenMap &css,
                               const rapidjson::Value &value,
                               const std::string &path) {
  std::shared_ptr<CSSParseTokenGroup> tokengroup(
      new CSSParseTokenGroup(value, path, compile_options_));
  std::vector<std::shared_ptr<CSSParseToken>> tokens =
      tokengroup.get()->getCssTokens();
  for (auto iter = tokens.cbegin(); iter != tokens.cend(); iter++) {
    std::shared_ptr<CSSParseToken> token = std::move(*iter);
    int token_size = token.get()->sheets().size();
    if (token_size == 0 ||
        (token_size >= 3 && compile_options_.disable_multiple_cascade_css_)) {
      continue;
    }
    int index = token_size - 1;
    std::string key = token.get()->sheets()[index].get()->GetSelector().str();
    // add some tricky logic for .a .b
    if (token.get()->IsCascadeSelectorStyleToken()) {
      HandleCascadeSelector(token, key);
    }

    int type = token.get()->sheets()[index].get()->GetType();
    auto it = css.find(key);
    if (it != css.end()) {
      int sheet_index_ = it->second.get()->sheets().size() - 1;
      int sheet_type = it->second.get()->sheets()[sheet_index_]->GetType();
      if (sheet_type == type) {
        if (compile_options_.enable_css_class_merge_) {
          MergeCSSParseToken(it->second, token);
        } else {
          css.erase(it);
          css.insert({key, std::move(token)});
        }
      }
    } else {
      css.insert({key, std::move(token)});
    }
  }
}

void CSSParser::ParseCSSTokensNew(
    std::vector<encoder::LynxCSSSelectorTuple> &selector_tuple_lists,
    CSSParserTokenMap &css, const rapidjson::Value &value,
    const std::string &path) {
  auto token_group =
      std::make_unique<CSSParseTokenGroup>(value, path, compile_options_);
  std::string key = token_group.get()->selector_key_;
  for (auto &selector_tuple : selector_tuple_lists) {
    if (selector_tuple.selector_key == key) {
      // the new css ng will support class merge by default
      MergeCSSParseToken(selector_tuple.parse_token,
                         token_group->selector_tuple_.parse_token);
      break;
    }
  }
  selector_tuple_lists.emplace_back(std::move(token_group->selector_tuple_));
}

void CSSParser::ParseCSSKeyframes(
    encoder::CSSKeyframesTokenMapForEncode &keyframes,
    const rapidjson::Value &value, const std::string &path) {
  std::string key = encoder::CSSKeyframesToken::GetCSSKeyframesTokenName(value);
  if (key.empty()) {
    return;
  }
  std::shared_ptr<encoder::CSSKeyframesToken> token(
      new encoder::CSSKeyframesToken(value, path, compile_options_));

  auto it = keyframes.find(key);
  if (it != keyframes.end()) {
    keyframes.erase(it);
  }
  keyframes.insert({key, token});
}

void CSSParser::ParseCSSFontFace(
    encoder::CSSFontFaceTokenMapForEncode &fontfaces,
    const rapidjson::Value &value, const std::string &path) {
  std::string key = CSSFontFaceToken::GetCSSFontFaceTokenKey(value);
  if (key.empty()) {
    return;
  }
  std::shared_ptr<CSSFontFaceToken> token(new CSSFontFaceToken(value, path));

  auto it = fontfaces.find(key);
  if (it != fontfaces.end()) {
    fontfaces[key].emplace_back(token);
  } else {
    std::vector<std::shared_ptr<CSSFontFaceToken>> token_list{token};
    fontfaces[key] = std::move(token_list);
  }
}

// For fiber
void CSSParser::ParseCSS(const rapidjson::Value &map,
                         const rapidjson::Value &id,
                         const rapidjson::Value &source) {
  std::vector<int32_t> dependent_css_list;
  const rapidjson::Value &fragment = map[id.GetString()];
  // find import css
  for (rapidjson::Value::ConstValueIterator itr = fragment.Begin();
       itr != fragment.End(); ++itr) {
    if (itr->HasMember(TYPE) &&
        strcmp(itr->GetObject()[TYPE].GetString(), IMPORT_RULE) == 0 &&
        itr->GetObject().HasMember(HREF)) {
      const rapidjson::Value &import_id = itr->GetObject()[HREF];
      if (fragments_.find(import_id.GetString()) == fragments_.end() &&
          map.HasMember(import_id)) {
        ParseCSS(map, import_id, source);
      }
      dependent_css_list.emplace_back(atoi(import_id.GetString()));
    }
  }
  // page css
  ParseCSS(fragment, source[id.GetString()].GetString(), dependent_css_list,
           atoi(id.GetString()));
}

}  // namespace tasm
}  // namespace lynx
