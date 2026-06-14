// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHARED_DATA_SHARED_DATA_TRACE_EVENT_DEF_H_
#define CORE_SHARED_DATA_SHARED_DATA_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

static constexpr const char* const
    WHITE_BOARD_DELEGATE_SET_SESSION_STORAGE_ITEM = "SetSessionStorageItem";
static constexpr const char* const
    WHITE_BOARD_DELEGATE_GET_SESSION_STORAGE_ITEM = "GetSessionStorageItem";
static constexpr const char* const
    WHITE_BOARD_DELEGATE_SUBSCRIBE_LEPUS_SESSION_STORAGE =
        "SubscribeLepusSessionStorage";
static constexpr const char* const
    WHITE_BOARD_DELEGATE_UNSUBSCRIBE_LEPUS_SESSION_STORAGE =
        "UnsubscribeLepusSessionStorage";
static constexpr const char* const
    WHITE_BOARD_DELEGATE_SUBSCRIBE_JS_SESSION_STORAGE =
        "SubscribeJSSessionStorage";
static constexpr const char* const
    WHITE_BOARD_DELEGATE_UNSUBSCRIBE_JS_SESSION_STORAGE =
        "UnsubscribeJSSessionStorage";
static constexpr const char* const
    WHITE_BOARD_DELEGATE_SUBSCRIBE_CLIENT_SESSION_STORAGE =
        "SubScribeClientSessionStorage";
static constexpr const char* const
    WHITE_BOARD_DELEGATE_UNSUBSCRIBE_CLIENT_SESSION_STORAGE =
        "UnsubscribeClientSessionStorage";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_SHARED_DATA_SHARED_DATA_TRACE_EVENT_DEF_H_
