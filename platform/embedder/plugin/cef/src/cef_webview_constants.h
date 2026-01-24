// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_CONSTANTS_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_CONSTANTS_H_

// Cookie property names
static const char kCookieDomain[] = "domain";
static const char kCookieExpirationDate[] = "expirationDate";
static const char kCookieHttpOnly[] = "httpOnly";
static const char kCookieName[] = "name";
static const char kCookiePath[] = "path";
static const char kCookieSameSite[] = "sameSite";
static const char kCookieSecure[] = "secure";
static const char kCookieValue[] = "value";

// SameSite values
static const char kSameSiteLax[] = "lax";
static const char kSameSiteNoRestriction[] = "no_restriction";
static const char kSameSiteStrict[] = "strict";
static const char kSameSiteUnspecified[] = "unspecified";

// Webview property names
static const char kPropertyCookies[] = "cookies";
static const char kPropertyEnableDebug[] = "enable-debug";
static const char kPropertyInitJs[] = "initjs";
static const char kPropertyMaxFps[] = "max-fps";
static const char kPropertySrc[] = "src";
static const char kPropertyUrl[] = "url";
static const char kPropertyUseOsr[] = "use-osr";

// Method names
static const char kMethodCookiesFlushStore[] = "cookies.flushStore";
static const char kMethodCookiesGet[] = "cookies.get";
static const char kMethodCookiesRemove[] = "cookies.remove";
static const char kMethodCookiesSet[] = "cookies.set";
static const char kMethodEval[] = "eval";
static const char kMethodEvalFunc[] = "func";
static const char kMethodReload[] = "reload";

// JavaScript execution context
static const char kJsExecutionContext[] = "<host>";

// Log
static constexpr const char* LOG_TAG = "cef-x-webview";
// Trace
static constexpr const char* CEF_TRACE_CATEGORY = "cef";

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_CONSTANTS_H_
