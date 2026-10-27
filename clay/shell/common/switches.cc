// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "clay/fml/native_library.h"
#include "clay/fml/paths.h"
#include "clay/fml/size.h"
#include "clay/version/version.h"

// Include once for the default enum definition.
#include "clay/shell/common/switches.h"

#undef CLAY_SHELL_COMMON_SWITCHES_H_

struct SwitchDesc {
  clay::Switch sw;
  const std::string_view flag;
  const char* help;
};

#undef DEF_SWITCHES_START
#undef DEF_SWITCH
#undef DEF_SWITCHES_END

// clang-format off
#define DEF_SWITCHES_START static const struct SwitchDesc gSwitchDescs[] = {
#define DEF_SWITCH(p_swtch, p_flag, p_help) \
  { clay::Switch::p_swtch, p_flag, ""/*p_help*/ },
#define DEF_SWITCHES_END };
// clang-format on

// Include again for struct definition.
#include "clay/shell/common/switches.h"

// Define symbols for the ICU data that is linked into the Clay library on
// Android.  This is a workaround for crashes seen when doing dynamic lookups
// of the engine's own symbols on some older versions of Android.
#ifndef USE_SYSTEM_ICU
#if defined(ENABLE_BINARY_ICUDTL)
extern uint8_t _binary_icudtl_dat_start[];
extern uint8_t _binary_icudtl_dat_end[];

static std::unique_ptr<fml::Mapping> GetICUStaticMapping() {
  return std::make_unique<fml::NonOwnedMapping>(
      _binary_icudtl_dat_start,
      _binary_icudtl_dat_end - _binary_icudtl_dat_start);
}
#endif
#endif

namespace clay {
const std::string_view FlagForSwitch(Switch swtch) {
  for (const auto& gSwitchDesc : gSwitchDescs) {
    if (gSwitchDesc.sw == swtch) {
      return gSwitchDesc.flag;
    }
  }
  return std::string_view();
}

static std::vector<std::string> ParseCommaDelimited(const std::string& input) {
  std::istringstream ss(input);
  std::vector<std::string> result;
  std::string token;
  while (std::getline(ss, token, ',')) {
    result.push_back(token);
  }
  return result;
}

template <typename T>
static bool GetSwitchValue(const fml::CommandLine& command_line, Switch sw,
                           T* result) {
  std::string switch_string;

  if (!command_line.GetOptionValue(FlagForSwitch(sw), &switch_string)) {
    return false;
  }

  std::stringstream stream(switch_string);
  T value = 0;
  if (stream >> value) {
    *result = value;
    return true;
  }

  return false;
}

#ifndef USE_SYSTEM_ICU
std::unique_ptr<fml::Mapping> GetSymbolMapping(
    const std::string& symbol_prefix, const std::string& native_lib_path) {
  const uint8_t* mapping = nullptr;
  intptr_t size;

  auto lookup_symbol = [&mapping, &size, symbol_prefix](
                           const fml::RefPtr<fml::NativeLibrary>& library) {
    mapping = library->ResolveSymbol((symbol_prefix + "_start").c_str());
    size = reinterpret_cast<intptr_t>(
        library->ResolveSymbol((symbol_prefix + "_size").c_str()));
  };

  fml::RefPtr<fml::NativeLibrary> library =
      fml::NativeLibrary::CreateForCurrentProcess();
  lookup_symbol(library);

  if (!(mapping && size)) {
    // Symbol lookup for the current process fails on some devices.  As a
    // fallback, try doing the lookup based on the path to the Flutter library.
    library = fml::NativeLibrary::Create(native_lib_path.c_str());
    lookup_symbol(library);
  }

  FML_CHECK(mapping && size) << "Unable to resolve symbols: " << symbol_prefix;
  return std::make_unique<fml::NonOwnedMapping>(mapping, size);
}
#endif

Settings SettingsFromCommandLine(const fml::CommandLine& command_line) {
  Settings settings = {};

  settings.enable_software_rendering =
      command_line.HasOption(FlagForSwitch(Switch::EnableSoftwareRendering));

#if !FLUTTER_RELEASE
  settings.trace_skia = true;

  if (command_line.HasOption(FlagForSwitch(Switch::TraceSkia))) {
    // If --trace-skia is specified, then log all Skia events.
    settings.trace_skia_allowlist.reset();
  } else {
    std::string trace_skia_allowlist;
    command_line.GetOptionValue(FlagForSwitch(Switch::TraceSkiaAllowlist),
                                &trace_skia_allowlist);
    if (trace_skia_allowlist.size()) {
      settings.trace_skia_allowlist = ParseCommaDelimited(trace_skia_allowlist);
    } else {
      settings.trace_skia_allowlist = {"skia.shaders"};
    }
  }
#endif  // !FLUTTER_RELEASE

  std::string trace_allowlist;
  command_line.GetOptionValue(FlagForSwitch(Switch::TraceAllowlist),
                              &trace_allowlist);
  settings.trace_allowlist = ParseCommaDelimited(trace_allowlist);

  settings.skia_deterministic_rendering_on_cpu =
      command_line.HasOption(FlagForSwitch(Switch::SkiaDeterministicRendering));

  settings.verbose_logging =
      command_line.HasOption(FlagForSwitch(Switch::VerboseLogging));

  command_line.GetOptionValue(FlagForSwitch(Switch::ClayAssetsDir),
                              &settings.assets_path);

  command_line.GetOptionValue(FlagForSwitch(Switch::CacheDirPath),
                              &settings.temp_directory_path);

#ifndef USE_SYSTEM_ICU
  if (settings.icu_initialization_required) {
    command_line.GetOptionValue(FlagForSwitch(Switch::ICUDataFilePath),
                                &settings.icu_data_path);
#if defined(ENABLE_BINARY_ICUDTL)
    settings.icu_mapper = GetICUStaticMapping;
#else
    if (command_line.HasOption(FlagForSwitch(Switch::ICUSymbolPrefix))) {
      std::string icu_symbol_prefix, native_lib_path;
      command_line.GetOptionValue(FlagForSwitch(Switch::ICUSymbolPrefix),
                                  &icu_symbol_prefix);
      command_line.GetOptionValue(FlagForSwitch(Switch::ICUNativeLibPath),
                                  &native_lib_path);
      settings.icu_mapper = [icu_symbol_prefix, native_lib_path] {
        return GetSymbolMapping(icu_symbol_prefix, native_lib_path);
      };
    }
#endif
  }
#endif  // USE_SYSTEM_ICU

  settings.use_test_fonts =
      command_line.HasOption(FlagForSwitch(Switch::UseTestFonts));
  settings.use_asset_fonts =
      !command_line.HasOption(FlagForSwitch(Switch::DisableAssetFonts));

  std::string enable_skparagraph = command_line.GetOptionValueWithDefault(
      FlagForSwitch(Switch::EnableSkParagraph), "");
  settings.enable_skparagraph = enable_skparagraph != "false";

  settings.prefetched_default_font_manager = command_line.HasOption(
      FlagForSwitch(Switch::PrefetchedDefaultFontManager));

  settings.enable_default_focus_ring =
      command_line.HasOption(FlagForSwitch(Switch::EnableDefaultFocusRing));

  settings.dump_skp_on_shader_compilation =
      command_line.HasOption(FlagForSwitch(Switch::DumpSkpOnShaderCompilation));

  settings.cache_sksl =
      command_line.HasOption(FlagForSwitch(Switch::CacheSkSL));

  settings.purge_persistent_cache =
      command_line.HasOption(FlagForSwitch(Switch::PurgePersistentCache));

  if (command_line.HasOption(
          FlagForSwitch(Switch::ResourceCacheMaxBytesThreshold))) {
    std::string resource_cache_max_bytes_threshold;
    command_line.GetOptionValue(
        FlagForSwitch(Switch::ResourceCacheMaxBytesThreshold),
        &resource_cache_max_bytes_threshold);
    settings.resource_cache_max_bytes_threshold =
        std::stoi(resource_cache_max_bytes_threshold);
  }

  if (command_line.HasOption(FlagForSwitch(Switch::MsaaSamples))) {
    std::string msaa_samples;
    command_line.GetOptionValue(FlagForSwitch(Switch::MsaaSamples),
                                &msaa_samples);
    settings.msaa_samples = std::stoi(msaa_samples);
  }
  settings.enable_sync_compositor =
      command_line.HasOption(FlagForSwitch(Switch::EnableRendermodeSync));
  Settings::SetStencilBuffer(
      command_line.HasOption(FlagForSwitch(Switch::EnableStencilBuffer)) &&
      !settings.enable_sync_compositor);
  if (command_line.HasOption(
          FlagForSwitch(Switch::GpuResourceCacheMultiplier))) {
    std::string mul = "1";
    command_line.GetOptionValue(
        FlagForSwitch(Switch::GpuResourceCacheMultiplier), &mul);
    settings.gpu_resource_cache_multiplier = std::stoi(mul);
  }
  return settings;
}

}  // namespace clay
