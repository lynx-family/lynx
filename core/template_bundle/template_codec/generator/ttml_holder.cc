// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/generator/ttml_holder.h"

namespace lynx {
namespace tasm {

const char *kTTMLResourceSuffix = ".ttml";
const char *kTTSSResourceSuffix = ".ttss";
const char *defaultSlotName = "#default#";
const char *kFallbackName = "#fallback#";
// TODO use a better way
int sPageIdGenerator = 0;
int sComponentIdGenerator = 0;
int sComponentInstanceIdGenerator = 0;
int sFragmentIdGenerator = 0;
int sTemplateIdGenerator = 0;
int sElementIdGenerator = 0;
int sDynamicIdGenerator = 0;

}  // namespace tasm
}  // namespace lynx
