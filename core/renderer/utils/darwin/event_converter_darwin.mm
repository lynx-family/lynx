// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/darwin/event_converter_darwin.h"

#import "LynxTemplateData+Converter.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {
namespace darwin {

// Convert MessageEvent to NSDictionary
NSDictionary* EventConverterDarwin::ConverMessageEventToNSDictionary(
    const runtime::MessageEvent& event) {
  NSMutableDictionary* dict = [NSMutableDictionary new];
  [dict setObject:[NSString stringWithUTF8String:event.type().c_str()]
           forKey:[NSString stringWithUTF8String:kType]];
  [dict setObject:[NSNumber numberWithLongLong:event.time_stamp()]
           forKey:[NSString stringWithUTF8String:kTimestamp]];
  [dict setObject:[NSString stringWithUTF8String:event.GetTargetString().c_str()]
           forKey:[NSString stringWithUTF8String:kTarget]];
  [dict setObject:[NSString stringWithUTF8String:event.GetOriginString().c_str()]
           forKey:[NSString stringWithUTF8String:kOrigin]];

  [dict setObject:convertLepusValueToNSObject(event.message()) ?: [NSNull null]
           forKey:[NSString stringWithUTF8String:kData]];
  return dict;
}

// Convert NSDictionary to MessageEvent
runtime::MessageEvent EventConverterDarwin::ConvertNSDictionaryToMessageEvent(NSDictionary* dict) {
  auto event = LynxConvertToLepusValue(dict);
  const auto& type = event.GetProperty(BASE_STATIC_STRING(kType)).StdString();
  auto time_stamp =
      static_cast<int64_t>(event.GetProperty(BASE_STATIC_STRING(kTimestamp)).Number());
  auto target = runtime::ContextProxy::ConvertStringToContextType(
      event.GetProperty(BASE_STATIC_STRING(kTarget)).StdString());
  auto origin = runtime::ContextProxy::ConvertStringToContextType(
      event.GetProperty(BASE_STATIC_STRING(kOrigin)).StdString());
  auto data = event.GetProperty(BASE_STATIC_STRING(kData));
  return runtime::MessageEvent(type, time_stamp, origin, target, data);
}

}  // namespace darwin
}  // namespace tasm
}  // namespace lynx
