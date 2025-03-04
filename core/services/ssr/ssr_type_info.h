// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_SSR_SSR_TYPE_INFO_H_
#define CORE_SERVICES_SSR_SSR_TYPE_INFO_H_

namespace lynx {
namespace ssr {

// ssr api version
// Server side ssr version is not already updated for the breaking change yet.
// So keep using the legacy Api version on the client side until we found no
// server is still using the legacy runtime api version.
static constexpr char kSSRCurrentApiVersion[] = "1.4";

static constexpr const char kLepusModuleFromPiper[] = "fromPiper";
static constexpr const char kLepusModuleCallbackId[] = "callbackId";
static constexpr const char kLepusModuleMethodDetail[] = "methodDetail";
static constexpr const char kLepusModuleParam[] = "param";
static constexpr const char kLepusModuleMethod[] = "method";
static constexpr const char kLepusModuleModule[] = "module";
static constexpr const char kLepusModuleTasmEntryName[] = "tasmEntryName";
static constexpr const char kLepusModuleCallMethod[] = "call";

static constexpr char kSSRPlaceHolderMagicPrefix[] = "$_ph{";
static constexpr char kSSRPlaceHolderMagicSuffix[] = "}hp_#";
static constexpr size_t kSSRPlaceHolderMagicPrefixLength = 5;
static constexpr size_t kSSRPlaceHolderMagicSuffixLength = 5;

}  // namespace ssr
}  // namespace lynx

#endif  // CORE_SERVICES_SSR_SSR_TYPE_INFO_H_
