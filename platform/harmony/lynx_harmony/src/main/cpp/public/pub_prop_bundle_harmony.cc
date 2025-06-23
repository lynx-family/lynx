// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_prop_bundle_harmony.h"

#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {

PubPropBundleHarmony::PubPropBundleHarmony() {
  impl_ = new PropBundleHarmony();
}

PubPropBundleHarmony::~PubPropBundleHarmony() {
  delete impl_;
  impl_ = nullptr;
}

void PubPropBundleHarmony::SetNullProps(const char* key) const {
  impl_->SetNullProps(key);
}

void PubPropBundleHarmony::SetProps(const char* key, unsigned int value) const {
  impl_->SetProps(key, value);
}
void PubPropBundleHarmony::SetProps(const char* key, int value) const {
  impl_->SetProps(key, value);
}
void PubPropBundleHarmony::SetProps(const char* key, const char* value) const {
  impl_->SetProps(key, value);
}
void PubPropBundleHarmony::SetProps(const char* key, bool value) const {
  impl_->SetProps(key, value);
}
void PubPropBundleHarmony::SetProps(const char* key, double value) const {
  impl_->SetProps(key, value);
}
void PubPropBundleHarmony::SetProps(const char* key,
                                    const pub::Value& value) const {
  impl_->SetProps(key, value);
}
void PubPropBundleHarmony::SetProps(const pub::Value& value) const {
  impl_->SetProps(value);
}
void PubPropBundleHarmony::SetEventHandler(const pub::Value& event) const {
  impl_->SetEventHandler(event);
}
bool PubPropBundleHarmony::Contains(const char* key) const {
  return impl_->Contains(key);
}
void PubPropBundleHarmony::SetNullPropsByID(int id) const {
  impl_->SetNullPropsByID(static_cast<CSSPropertyID>(id));
}
void PubPropBundleHarmony::SetPropsByID(int id, unsigned int value) const {
  impl_->SetPropsByID(static_cast<CSSPropertyID>(id), value);
}
void PubPropBundleHarmony::SetPropsByID(int id, int value) const {
  impl_->SetPropsByID(static_cast<CSSPropertyID>(id), value);
}
void PubPropBundleHarmony::SetPropsByID(int id, const char* value) const {
  impl_->SetPropsByID(static_cast<CSSPropertyID>(id), value);
}
void PubPropBundleHarmony::SetPropsByID(int id, bool value) const {
  impl_->SetPropsByID(static_cast<CSSPropertyID>(id), value);
}
void PubPropBundleHarmony::SetPropsByID(int id, double value) const {
  impl_->SetPropsByID(static_cast<CSSPropertyID>(id), value);
}
void PubPropBundleHarmony::SetPropsByID(int id, const pub::Value& value) const {
  impl_->SetPropsByID(static_cast<CSSPropertyID>(id), value);
}

PropBundleHarmony* PubPropBundleHarmony::PropBundle() const { return impl_; }

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
