// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_TRACE_RESOURCE_TRACE_EVENT_DEF_H_
#define CORE_RESOURCE_TRACE_RESOURCE_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

inline constexpr const char* const LOAD_RESOURCE = "LoadResource";
inline constexpr const char* const LOAD_BYTE_CODE = "LoadExternalBytecode";
inline constexpr const char* const DYNAMIC_COMPONENT_DID_LOAD_COMPONENT =
    "DynamicComponent::DidLoadComponent";
inline constexpr const char* const DYNAMIC_COMPONENT_REQUIRE_TEMPLATE =
    "DynamicComponent::RequireTemplate";
inline constexpr const char* const LAZY_BUNDLE_DID_FETCH_BUNDLE =
    "LazyBundle::DidFetchBundle";
inline constexpr const char* const LAZY_BUNDLE_FETCH_BUNDLE =
    "LazyBundle::FetchBundle";

inline constexpr const char* const FETCH_SCRIPT_BY_PROVIDER =
    "FetchScriptByProvider";
inline constexpr const char* const FETCH_TEMPLATE_BY_GENERIC_FETCHER =
    "FetchTemplateByGenericFetcher";
inline constexpr const char* const FETCH_RESOURCE_BY_GENERIC_FETCHER =
    "FetchResourceByGenericFetcher";
inline constexpr const char* const FETCH_TEMPLATE_BY_PROVIDER =
    "FetchTemplateByProvider";
inline constexpr const char* const FETCH_TEMPLATE_BY_FETCHER_WRAPPER =
    "FetchTemplateByFetcherWrapper";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_RESOURCE_TRACE_RESOURCE_TRACE_EVENT_DEF_H_
