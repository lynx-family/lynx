// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_OVERLAY_SERVICE_H_
#define CLAY_UI_PLATFORM_OVERLAY_SERVICE_H_

#include <memory>
#include <string>
#include <utility>

#include "clay/common/service/service.h"

namespace clay {

class ExternalViewPlugin;

class OverlayPlatformPlugin : public ActorObject<Owner::kPlatform> {
 public:
  class OverlayListener : public ActorObject<Owner::kUI> {
   public:
    virtual ~OverlayListener() = default;
    virtual void OnDialogBackPressed() = 0;
    virtual void OnViewOffsetUpdated(int offset_x, int offset_y) = 0;
    virtual void OnViewSizeUpdated(int width, int height) {}
  };

  virtual ~OverlayPlatformPlugin() = default;

  virtual void ChangeVisibility(bool visible) = 0;
  virtual void SetLevel(int level) = 0;
  virtual void SetCutOutMode(bool is_cut_out) = 0;
  virtual void SetAndroidSoftInputMode(std::string mode) = 0;
  virtual void SetAndroidNativeEventPass(bool is_pass) = 0;
  virtual void SetStatusBarTranslucent(bool is_translucent) = 0;
  virtual void SetStatusBarTranslucentStyle(std::string style) = 0;
  virtual void SetAndroidFullScreen(bool is_full_screen) = 0;
  virtual void SetEventThrough(bool event_through) {}
  virtual void SetPreferredSize(int width, int height) {}

  virtual bool ShouldHandleTreeLifecycle() const { return false; }
  virtual bool RequiresExternalViewPlugin() const { return true; }
  virtual void OnAttachToTree() = 0;
  virtual void OnDetachFromTree() = 0;
  virtual void OnViewDestroy() = 0;

  virtual void InitPlatformOverlay(
      std::shared_ptr<Actor<fml::WeakPtr<OverlayListener>>> overlay_listener,
      int id, std::string tag, ExternalViewPlugin* recording_plugin) {
    overlay_listener_ = std::move(overlay_listener);
  }

 protected:
  Puppet<Owner::kPlatform, fml::WeakPtr<OverlayListener>> overlay_listener_;
};

class OverlayService : public Service<OverlayService, Owner::kPlatform,
                                      ServiceFlags::kManualRegister> {
 public:
  virtual std::unique_ptr<OverlayPlatformPlugin>
  CreateOverlayPlatformPlugin() = 0;
};

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_OVERLAY_SERVICE_H_
