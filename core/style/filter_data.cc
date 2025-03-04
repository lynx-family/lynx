// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/filter_data.h"
namespace lynx {
namespace starlight {

FilterData::FilterData()
    : type(FilterType::kNone), amount(NLength::MakeUnitNLength(0.0f)) {}

void FilterData::Reset() {
  type = FilterType::kNone;
  amount = NLength::MakeUnitNLength(0.0f);
}

}  // namespace starlight
}  // namespace lynx
