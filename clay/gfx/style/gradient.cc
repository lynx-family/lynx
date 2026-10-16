// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/gradient.h"

#include <utility>

#include "clay/gfx/style/gradient_factory.h"

namespace clay {

Gradient::Gradient() : type_(GradientType::kNotSet) {}

// static
std::optional<Gradient> Gradient::Create(std::string raw_data) {
  return GradientFactory::Create(std::move(raw_data));
}

// static
std::optional<Gradient> Gradient::CreateLinear(
    const ClayLinearGradient& gradient_data) {
  return GradientFactory::CreateLinear(gradient_data);
}

// static
std::optional<Gradient> Gradient::CreateRadial(
    const ClayRadialGradient& gradient_data) {
  return GradientFactory::CreateRadial(gradient_data);
}

// static
std::optional<Gradient> Gradient::CreateConic(
    const ClayConicGradient& gradient_data) {
  return GradientFactory::CreateConic(gradient_data);
}

bool Gradient::operator==(const Gradient& oth) const {
  return raw_data_ == oth.raw_data_;
}

bool Gradient::operator!=(const Gradient& oth) const {
  return raw_data_ != oth.raw_data_;
}

}  // namespace clay
