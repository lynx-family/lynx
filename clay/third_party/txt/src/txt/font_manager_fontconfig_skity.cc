// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "txt/font_manager_fontconfig_skity.h"
#include <fontconfig/fontconfig.h>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>
namespace txt {
namespace {
class FontconfigManager;
class StyleSet final : public skity::FontStyleSet {
 public:
  explicit StyleSet(std::vector<std::shared_ptr<skity::Typeface>> faces)
      : faces_(std::move(faces)) {}
  int Count() override { return faces_.size(); }
  void GetStyle(int i, skity::FontStyle* style, std::string* name) override {
    auto face = CreateTypeface(i);
    if (face && style)
      *style = face->GetFontStyle();
    if (name)
      name->clear();
  }
  std::shared_ptr<skity::Typeface> CreateTypeface(int i) override {
    return i >= 0 && i < Count() ? faces_[i] : nullptr;
  }
  std::shared_ptr<skity::Typeface> MatchStyle(
      const skity::FontStyle& style) override {
    return MatchStyleCSS3(style);
  }

 private:
  std::vector<std::shared_ptr<skity::Typeface>> faces_;
};
class FontconfigManager final : public skity::FontManager {
 public:
  FontconfigManager() : config_(FcInitLoadConfigAndFonts()) {
    if (!config_)
      return;
    auto pattern = FcPatternCreate();
    auto objects = FcObjectSetBuild(FC_FAMILY, nullptr);
    auto fonts = FcFontList(config_, pattern, objects);
    if (fonts) {
      for (int i = 0; i < fonts->nfont; ++i) {
        FcChar8* family = nullptr;
        if (FcPatternGetString(fonts->fonts[i], FC_FAMILY, 0, &family) ==
            FcResultMatch) {
          std::string name(reinterpret_cast<char*>(family));
          if (std::find(families_.begin(), families_.end(), name) ==
              families_.end())
            families_.push_back(std::move(name));
        }
      }
      FcFontSetDestroy(fonts);
    }
    FcObjectSetDestroy(objects);
    FcPatternDestroy(pattern);
  }
  ~FontconfigManager() override {
    if (config_)
      FcConfigDestroy(config_);
  }

 protected:
  int OnCountFamilies() const override { return families_.size(); }
  std::string OnGetFamilyName(int i) const override {
    return i >= 0 && i < OnCountFamilies() ? families_[i] : "";
  }
  std::shared_ptr<skity::FontStyleSet> OnCreateStyleSet(int i) const override {
    return OnMatchFamily(OnGetFamilyName(i).c_str());
  }
  std::shared_ptr<skity::FontStyleSet> OnMatchFamily(
      const char name[]) const override {
    std::vector<std::shared_ptr<skity::Typeface>> faces;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!config_)
      return skity::FontStyleSet::CreateEmpty();
    auto pattern = FcPatternCreate();
    if (name && *name)
      FcPatternAddString(pattern, FC_FAMILY,
                         reinterpret_cast<const FcChar8*>(name));
    auto objects = FcObjectSetBuild(FC_FILE, FC_INDEX, nullptr);
    auto fonts = FcFontList(config_, pattern, objects);
    if (fonts) {
      for (int i = 0; i < fonts->nfont; ++i) {
        auto face = Load(fonts->fonts[i]);
        if (face)
          faces.push_back(std::move(face));
      }
      FcFontSetDestroy(fonts);
    }
    FcObjectSetDestroy(objects);
    FcPatternDestroy(pattern);
    return std::make_shared<StyleSet>(std::move(faces));
  }
  std::shared_ptr<skity::Typeface> OnMatchFamilyStyle(
      const char name[],
      const skity::FontStyle& style) const override {
    return Match(name, style, nullptr, 0, 0);
  }
  std::shared_ptr<skity::Typeface> OnMatchFamilyStyleCharacter(
      const char name[],
      const skity::FontStyle& style,
      const char* langs[],
      int count,
      skity::Unichar character) const override {
    return Match(name, style, langs, count, character);
  }
  std::shared_ptr<skity::Typeface> OnMakeFromData(
      const std::shared_ptr<skity::Data>& data,
      int index) const override {
    return skity::FontManager::RefDefault()->MakeFromData(data, index);
  }
  std::shared_ptr<skity::Typeface> OnMakeFromFile(const char path[],
                                                  int index) const override {
    return skity::FontManager::RefDefault()->MakeFromFile(path, index);
  }
  std::shared_ptr<skity::Typeface> OnGetDefaultTypeface(
      const skity::FontStyle& style) const override {
    return Match("sans-serif", style, nullptr, 0, 0);
  }

 private:
  std::shared_ptr<skity::Typeface> Match(const char* family,
                                         const skity::FontStyle& style,
                                         const char* langs[],
                                         int count,
                                         skity::Unichar character) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!config_)
      return nullptr;
    auto pattern = FcPatternCreate();
    if (family && *family)
      FcPatternAddString(pattern, FC_FAMILY,
                         reinterpret_cast<const FcChar8*>(family));
    FcPatternAddInteger(pattern, FC_WEIGHT,
                        FcWeightFromOpenType(style.weight()));
    FcPatternAddInteger(
        pattern, FC_SLANT,
        style.slant() == skity::FontStyle::kUpright_Slant  ? FC_SLANT_ROMAN
        : style.slant() == skity::FontStyle::kItalic_Slant ? FC_SLANT_ITALIC
                                                           : FC_SLANT_OBLIQUE);
    for (int i = 0; langs && i < count; ++i)
      if (langs[i])
        FcPatternAddString(pattern, FC_LANG,
                           reinterpret_cast<const FcChar8*>(langs[i]));
    if (character) {
      auto charset = FcCharSetCreate();
      FcCharSetAddChar(charset, character);
      FcPatternAddCharSet(pattern, FC_CHARSET, charset);
      FcCharSetDestroy(charset);
    }
    FcConfigSubstitute(config_, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result;
    auto match = FcFontMatch(config_, pattern, &result);
    FcPatternDestroy(pattern);
    auto face = match ? Load(match) : nullptr;
    if (match)
      FcPatternDestroy(match);
    if (face && character && !face->UnicharToGlyph(character))
      return nullptr;
    return face;
  }
  // Caller holds mutex_ while looking up and caching font files.
  std::shared_ptr<skity::Typeface> Load(FcPattern* match) const {
    std::shared_ptr<skity::Typeface> face;
    FcChar8* file = nullptr;
    int index = 0;
    if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch) {
      FcPatternGetInteger(match, FC_INDEX, 0, &index);
      const std::string key = std::string(reinterpret_cast<char*>(file)) + ":" +
                              std::to_string(index);
      auto found = faces_.find(key);
      if (found != faces_.end())
        face = found->second;
      else {
        face = OnMakeFromFile(reinterpret_cast<char*>(file), index);
        if (face)
          faces_.emplace(key, face);
      }
    }
    return face;
  }
  FcConfig* config_;
  std::vector<std::string> families_;
  mutable std::mutex mutex_;
  mutable std::unordered_map<std::string, std::shared_ptr<skity::Typeface>>
      faces_;
};
}  // namespace
std::shared_ptr<skity::FontManager> MakeFontconfigSkityFontManager() {
  static auto manager = std::make_shared<FontconfigManager>();
  return manager;
}
}  // namespace txt
