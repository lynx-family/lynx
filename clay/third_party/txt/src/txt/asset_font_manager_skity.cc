// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "txt/asset_font_manager_skity.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <utility>

#include "build/build_config.h"
#include "clay/fml/logging.h"
#if defined(OS_IOS) || defined(OS_MAC) || defined(OS_OSX)
#include "skity/text/ports/typeface_ct.hpp"
#endif
#include "skity/text/typeface.hpp"

namespace txt {
namespace {

std::atomic<uint64_t> g_next_variation_font_id{0};

void AppendUint32(std::string& key, uint32_t value) {
  key.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

std::string VariationFontKey(
    skity::TypefaceID typeface_id,
    const std::map<std::string, float>& normalized_axes) {
  std::string key;
  AppendUint32(key, typeface_id);
  for (const auto& [axis, value] : normalized_axes) {
    key.append(axis);
    uint32_t value_bits = 0;
    static_assert(sizeof(value_bits) == sizeof(value));
    std::memcpy(&value_bits, &value, sizeof(value));
    AppendUint32(key, value_bits);
  }
  return key;
}

std::shared_ptr<skity::Typeface> MakeSkityVariation(
    const std::shared_ptr<skity::Typeface>& base_typeface,
    const std::map<std::string, float>& normalized_axes) {
  skity::VariationPosition position;
  for (const auto& [axis, value] : normalized_axes) {
    position.AddCoordinate(
        skity::SetFourByteTag(axis[0], axis[1], axis[2], axis[3]), value);
  }
  skity::FontArguments arguments;
  arguments
      .SetCollectionIndex(base_typeface->GetFontDescriptor().collection_index)
      .SetVariationDesignPosition(position);
  return base_typeface->MakeVariation(arguments);
}

#if defined(OS_IOS) || defined(OS_MAC) || defined(OS_OSX)
std::shared_ptr<skity::Typeface> MakeDarwinVariationWithoutCache(
    const std::shared_ptr<skity::Typeface>& base_typeface,
    const std::map<std::string, float>& normalized_axes) {
  CTFontRef base_font = skity::TypefaceCT::CTFontFromTypeface(base_typeface);
  if (!base_font) {
    return nullptr;
  }

  CFArrayRef variation_axes = CTFontCopyVariationAxes(base_font);
  if (!variation_axes) {
    return nullptr;
  }
  CFDictionaryRef old_variation = CTFontCopyVariation(base_font);
  const CFIndex axis_count = CFArrayGetCount(variation_axes);
  CFMutableDictionaryRef variation = CFDictionaryCreateMutable(
      kCFAllocatorDefault, axis_count, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);
  if (!variation) {
    if (old_variation) {
      CFRelease(old_variation);
    }
    CFRelease(variation_axes);
    return nullptr;
  }

  // CoreText expects a complete variation dictionary. Preserve every axis at
  // its current (or default) value, then override the requested coordinates.
  for (CFIndex i = 0; i < axis_count; ++i) {
    CFTypeRef axis_value = CFArrayGetValueAtIndex(variation_axes, i);
    if (CFGetTypeID(axis_value) != CFDictionaryGetTypeID()) {
      continue;
    }
    CFDictionaryRef axis = static_cast<CFDictionaryRef>(axis_value);
    CFNumberRef tag_ref = static_cast<CFNumberRef>(
        CFDictionaryGetValue(axis, kCTFontVariationAxisIdentifierKey));
    CFNumberRef default_ref = static_cast<CFNumberRef>(
        CFDictionaryGetValue(axis, kCTFontVariationAxisDefaultValueKey));
    if (!tag_ref || !default_ref) {
      continue;
    }

    int64_t tag = 0;
    float value = 0.f;
    if (!CFNumberGetValue(tag_ref, kCFNumberLongLongType, &tag) ||
        !CFNumberGetValue(default_ref, kCFNumberFloatType, &value)) {
      continue;
    }
    if (old_variation) {
      CFNumberRef old_value = static_cast<CFNumberRef>(
          CFDictionaryGetValue(old_variation, tag_ref));
      if (old_value) {
        CFNumberGetValue(old_value, kCFNumberFloatType, &value);
      }
    }
    for (const auto& [requested_axis, requested_value] : normalized_axes) {
      const auto requested_tag =
          skity::SetFourByteTag(requested_axis[0], requested_axis[1],
                                requested_axis[2], requested_axis[3]);
      if (static_cast<uint32_t>(tag) == requested_tag) {
        value = requested_value;
        break;
      }
    }

    CFNumberRef value_ref =
        CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &value);
    if (value_ref) {
      CFDictionarySetValue(variation, tag_ref, value_ref);
      CFRelease(value_ref);
    }
  }
  if (old_variation) {
    CFRelease(old_variation);
  }
  CFRelease(variation_axes);

  CFMutableDictionaryRef attributes = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);
  if (!attributes) {
    CFRelease(variation);
    return nullptr;
  }
  CFDictionarySetValue(attributes, kCTFontVariationAttribute, variation);
  CFRelease(variation);

  CTFontDescriptorRef descriptor =
      CTFontDescriptorCreateWithAttributes(attributes);
  CFRelease(attributes);
  if (!descriptor) {
    return nullptr;
  }
  CTFontRef variant_font =
      CTFontCreateCopyWithAttributes(base_font, 0, nullptr, descriptor);
  CFRelease(descriptor);
  if (!variant_font) {
    return nullptr;
  }

  auto varied_typeface =
      skity::TypefaceCT::TypefaceFromCTFontWithoutCache(variant_font);
  CFRelease(variant_font);
  return varied_typeface;
}
#endif

}  // namespace

AssetFontManager::AssetFontManager(
    std::unique_ptr<FontAssetProvider> font_provider)
    : font_provider_(std::move(font_provider)) {
  FML_DCHECK(font_provider_ != nullptr);
}

AssetFontManager::~AssetFontManager() = default;

int AssetFontManager::OnCountFamilies() const {
  std::lock_guard<std::mutex> lock(font_provider_mutex_);
  return font_provider_->GetFamilyCount();
}

std::string AssetFontManager::OnGetFamilyName(int index) const {
  std::lock_guard<std::mutex> lock(font_provider_mutex_);
  return font_provider_->GetFamilyName(index);
}

std::shared_ptr<skity::FontStyleSet> AssetFontManager::OnCreateStyleSet(
    int index) const {
  FML_DCHECK(false);
  return nullptr;
}

std::shared_ptr<skity::FontStyleSet> AssetFontManager::OnMatchFamily(
    const char familyName[]) const {
  std::lock_guard<std::mutex> lock(font_provider_mutex_);
  std::string family_name(familyName);
  return font_provider_->MatchFamily(family_name);
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnMatchFamilyStyle(
    const char familyName[],
    const skity::FontStyle& style) const {
  std::lock_guard<std::mutex> lock(font_provider_mutex_);
  auto font_style_set = font_provider_->MatchFamily(std::string(familyName));
  if (font_style_set == nullptr) {
    return nullptr;
  }
  return font_style_set->MatchStyle(style);
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnMatchFamilyStyleCharacter(
    const char familyName[],
    const skity::FontStyle&,
    const char* bcp47[],
    int bcp47Count,
    skity::Unichar character) const {
  return nullptr;
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnMakeFromData(
    std::shared_ptr<skity::Data> const&,
    int ttcIndex) const {
  FML_DCHECK(false);
  return nullptr;
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnMakeFromFile(
    const char path[],
    int ttcIndex) const {
  FML_DCHECK(false);
  return nullptr;
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnGetDefaultTypeface(
    skity::FontStyle const& font_style) const {
  return nullptr;
}

DynamicFontManager::DynamicFontManager()
    : AssetFontManager(std::make_unique<TypefaceFontAssetProvider>()) {}

std::optional<DynamicFontManager::VariationTypeface>
DynamicFontManager::GetOrCreateVariation(
    const std::shared_ptr<skity::Typeface>& base_typeface,
    const std::map<std::string, float>& axis_values) {
  if (!base_typeface) {
    return std::nullopt;
  }

  std::map<std::string, float> normalized_axes;
  const auto supported_axes = base_typeface->GetVariationDesignParameters();
  for (const auto& [axis, value] : axis_values) {
    if (axis.size() != 4 || !std::isfinite(value)) {
      continue;
    }
    const auto tag = skity::SetFourByteTag(axis[0], axis[1], axis[2], axis[3]);
    const auto supported =
        std::find_if(supported_axes.begin(), supported_axes.end(),
                     [tag](const skity::VariationAxis& candidate) {
                       return candidate.tag == tag;
                     });
    if (supported == supported_axes.end()) {
      continue;
    }
    normalized_axes[axis] = std::clamp(value, supported->min, supported->max);
  }
  if (normalized_axes.empty()) {
    return std::nullopt;
  }

  const std::string cache_key =
      VariationFontKey(base_typeface->TypefaceId(), normalized_axes);
  std::lock_guard<std::mutex> lock(font_provider_mutex_);
  auto cached = variation_typefaces_.find(cache_key);
  if (cached != variation_typefaces_.end()) {
    return cached->second;
  }

#if defined(OS_IOS) || defined(OS_MAC) || defined(OS_OSX)
  constexpr auto kCoreTextFactory = skity::SetFourByteTag('c', 't', 'x', 't');
  auto varied_typeface =
      base_typeface->GetFontDescriptor().factory_id == kCoreTextFactory
          ? MakeDarwinVariationWithoutCache(base_typeface, normalized_axes)
          : MakeSkityVariation(base_typeface, normalized_axes);
#else
  auto varied_typeface = MakeSkityVariation(base_typeface, normalized_axes);
#endif
  if (!varied_typeface) {
    return std::nullopt;
  }

  VariationTypeface result;
  result.family_name = "__clay_internal_variable_font_" +
                       std::to_string(g_next_variation_font_id.fetch_add(
                           1, std::memory_order_relaxed));
  result.font_style = varied_typeface->GetFontStyle();
  result.overrides_weight = normalized_axes.count("wght") != 0;
  result.overrides_width = normalized_axes.count("wdth") != 0;
  result.overrides_slant =
      normalized_axes.count("slnt") != 0 || normalized_axes.count("ital") != 0;
  static_cast<TypefaceFontAssetProvider&>(*font_provider_)
      .RegisterTypeface(std::move(varied_typeface), result.family_name);
  variation_typefaces_.emplace(cache_key, result);
  return result;
}

bool DynamicFontManager::HasVariations() const {
  std::lock_guard<std::mutex> lock(font_provider_mutex_);
  return !variation_typefaces_.empty();
}

}  // namespace txt
