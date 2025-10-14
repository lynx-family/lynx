// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_TTTEXT_HEADERS_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_TTTEXT_HEADERS_H_

#if defined(OS_IOS)
#import <textra/font_info.h>
#import <textra/fontmgr_collection.h>
#import <textra/i_canvas_helper.h>
#import <textra/i_font_manager.h>
#import <textra/i_typeface_helper.h>
#import <textra/macro.h>
#import <textra/painter.h>
#ifdef ENABLE_SKITY
#import <textra/platform/skity/skity_canvas_helper.h>
#import <textra/platform/skity/skity_font_manager.h>
#import <textra/platform/skity/skity_font_manager_coretext.h>
#endif
#import <textra/run_delegate.h>
#import <textra/style.h>
#import <textra/text_layout.h>
#else
#include <textra/font_info.h>
#include <textra/fontmgr_collection.h>
#include <textra/i_canvas_helper.h>
#include <textra/i_font_manager.h>
#include <textra/i_typeface_helper.h>
#if !defined(OS_WIN)
#include <textra/icu_wrapper.h>
#endif
#include <textra/macro.h>
#include <textra/painter.h>
#ifdef ENABLE_SKITY
#include <textra/platform/skity/skity_canvas_helper.h>
#include <textra/platform/skity/skity_font_manager.h>
#if defined(OS_IOS) || defined(OS_OSX)
#include <textra/platform/skity/skity_font_manager_coretext.h>
#endif
#endif
#include <textra/run_delegate.h>
#include <textra/style.h>
#include <textra/text_layout.h>
#endif
#endif  // CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_TTTEXT_HEADERS_H_
