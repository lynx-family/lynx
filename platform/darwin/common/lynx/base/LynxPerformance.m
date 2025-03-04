// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxPerformance.h"

// The date of the change is used as kActualFMPIndex.
static int kActualFMPIndex = 20211216;
static NSString* kActualFMPDuration = @"actualFMPDuration";
static NSString* kActualFMPEndTimeStamp = @"actualFirstScreenEndTimeStamp";

static NSString* kIsSrrHydrate = @"is_srr_hydrate";

@implementation LynxPerformance {
  NSDictionary* _dic;
}

- (instancetype)initWithPerformance:(NSDictionary*)dic
                                url:(NSString*)url
                           pageType:(NSString*)pageType
                       reactVersion:(NSString*)reactVersion {
  if (self = [super init]) {
    _dic = dic;
    if (nil != url) {
      [_dic setValue:url forKey:@"url"];
    }
    [_dic setValue:pageType forKey:@"page_type"];
    [_dic setValue:reactVersion forKey:@"react_version"];
  }
  return self;
}

- (BOOL)hasActualFMP {
  return [[_dic objectForKey:kActualFMPDuration] boolValue];
}

- (double)actualFMPDuration {
  return [[_dic objectForKey:kActualFMPDuration] doubleValue];
}

- (double)actualFirstScreenEndTimeStamp {
  return [[[_dic objectForKey:@"timing"] objectForKey:kActualFMPEndTimeStamp] doubleValue];
}

- (NSDictionary*)toDictionary {
  return _dic;
}

+ (NSString*)toPerfKey:(int)index {
  return [self toPerfKey:index isSsrHydrate:NO];
}

+ (NSString*)toPerfKey:(int)index isSsrHydrate:(BOOL)isSsrHydrate {
  static NSArray* PERF_POINTS;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    PERF_POINTS = @[
      @"tasm_binary_decode",
      @"tasm_end_decode_finish_load_template",
      @"tasm_finish_load_template",
      @"diff_root_create",
      @"js_finish_load_core",
      @"js_finish_load_app",
      @"js_and_tasm_all_ready",
      @"tti",
      @"js_runtime_type",
      @"corejs_size",
      @"source_js_size",
      @"first_page_layout",
      @"FIRST_SEP",
      @"layout",
      @"BOTH_SEP",
      @"render_page",
      @"diff_same_root",
      @"UPDATE_SEP",
      @"SSR_START_SEP",
      @"ssr_fmp",
      @"ssr_dispatch",
      @"ssr_generate_dom",
      @"ssr_source_size",
      @"SSR_END_SEP"
    ];
  });
  if (index == kActualFMPIndex) {
    return kActualFMPDuration;
  }

  if (index == kLynxPerformanceIsSrrHydrateIndex) {
    return kIsSrrHydrate;
  }

  NSString* res = PERF_POINTS[index];
  if (isSsrHydrate) {
    res = [NSString stringWithFormat:@"%@_hydrate", res];
  }
  return res;
}

+ (NSString*)toPerfStampKey:(int)index {
  static NSArray* PERF_STAMP_POINTER;
  static dispatch_once_t onceToken;

  // clang-format off
  dispatch_once(&onceToken, ^{
    PERF_STAMP_POINTER = @[
      @"init_start",
      @"init_end",
      @"load_template_start",
      @"load_template_end",
      @"decode_binary_start",
      @"decode_binary_end",
      @"render_template_start",
      @"render_template_end",
      @"diff_root_start",
      @"diff_root_end",
      @"layout_start",
      @"layout_end",
      @"load_corejs_start",
      @"load_corejs_end",
      @"load_appjs_start",
      @"load_appjs_end",
      @"start_diff",
      @"end_diff",
      @"update_page_start",
      @"update_page_end"
    ];
  });
  if (index == kActualFMPIndex) {
    return kActualFMPEndTimeStamp;
  }
  // clang-format on
  return PERF_STAMP_POINTER[index];
}

@end
