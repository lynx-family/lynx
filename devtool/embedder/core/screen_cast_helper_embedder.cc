// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/screen_cast_helper_embedder.h"

#include <string>
#include <utility>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/public/devtool/lynx_devtool_proxy.h"
#include "devtool/embedder/core/devtool_platform_embedder.h"
#include "devtool/embedder/core/frame_capturer_embedder.h"
#include "devtool/embedder/core/screenshot_thread_manager.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"

namespace lynx {
namespace devtool {

class ScreenCastHelper : public FrameCapturerEmbedder,
                         public FrameCapturerEmbedderDelegate,
                         public std::enable_shared_from_this<ScreenCastHelper> {
 public:
  ScreenCastHelper(
      devtool::LynxDevToolProxy* proxy,
      const std::shared_ptr<DevtoolPlatformEmbedder>& platform_embedder);
  ~ScreenCastHelper() override;

  // FrameCapturerEmbedderDelegate
  void OnNewSnapshot(const std::string& data,
                     const devtool::ScreenMetadata& metadata) override;
  void OnFrameChanged() override;
  void TakeSnapshotAsync(tasm::TakeSnapshotCompletedCallback callback) override;

  void StartCapture(int32_t quality, int32_t max_width, int32_t max_height);
  void PauseCapture();
  void ContinueCapture();
  void StopCapture();

  void GetLynxScreenShot();

  void ClearSnapshotCache();

  void OnEngineDestroy();

  double GetScreenScaleFactor();

 private:
  void TriggerNextCapture();

  int32_t max_height_;
  int32_t max_width_;
  int32_t quality_;
  devtool::LynxDevToolProxy* embedder_proxy_;
  std::weak_ptr<DevtoolPlatformEmbedder> weak_platform_embedder_;
};

ScreenCastHelper::ScreenCastHelper(
    devtool::LynxDevToolProxy* proxy,
    const std::shared_ptr<DevtoolPlatformEmbedder>& platform_embedder)
    : max_height_(0),
      max_width_(0),
      quality_(0),
      embedder_proxy_(proxy),
      weak_platform_embedder_(platform_embedder) {}

ScreenCastHelper::~ScreenCastHelper(){};

void ScreenCastHelper::StartCapture(int32_t quality, int32_t max_width,
                                    int32_t max_height) {
  quality_ = quality;
  max_width_ = max_width;
  max_height_ = max_height;
  StartFrameViewTrace();
}

void ScreenCastHelper::PauseCapture() { PauseFrameViewTrace(); }

void ScreenCastHelper::ContinueCapture() { ContinueFrameViewTrace(); }

void ScreenCastHelper::StopCapture() { StopFrameViewTrace(); }

// be used when screen capture trigger by screen content change
void ScreenCastHelper::TriggerNextCapture() {}

void ScreenCastHelper::ClearSnapshotCache() { snapshot_cache_ = ""; }

double ScreenCastHelper::GetScreenScaleFactor() {
  if (!embedder_proxy_) {
    return 1.0f;
  } else {
    return embedder_proxy_->GetScreenScaleFactor();
  }
}

void ScreenCastHelper::GetLynxScreenShot() {
  TakeSnapshotAsync([wp_self = weak_from_this()](
                        const std::string& snapshot, float timestamp,
                        float device_width, float device_height,
                        float page_scale_factor) {
    auto sp_self = wp_self.lock();
    if (!sp_self) {
      return;
    }
    auto platform_delegate = sp_self->weak_platform_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(platform_delegate, "platform_delegate is null");
    platform_delegate->SendScreenCapture(snapshot);
  });
}

void ScreenCastHelper::TakeSnapshotAsync(
    tasm::TakeSnapshotCompletedCallback callback) {
  lynx::base::UIThread::GetRunner()->PostTask(
      [wp_self = weak_from_this(), callback = std::move(callback)]() {
        auto sp_self = wp_self.lock();
        if (!sp_self) {
          return;
        }

        auto embedder_proxy = sp_self->embedder_proxy_;
        if (!embedder_proxy) {
          return;
        }

        embedder_proxy->TakeSnapshot(
            sp_self->max_width_, sp_self->max_height_, sp_self->quality_,
            sp_self->GetScreenScaleFactor(),
            ScreenshotThreadManager::GetScreenshotRunner(),
            std::move(callback));
      });
}

void ScreenCastHelper::OnNewSnapshot(const std::string& data,
                                     const devtool::ScreenMetadata& metadata) {
  auto platform_delegate = weak_platform_embedder_.lock();
  CHECK_NULL_AND_LOG_RETURN(platform_delegate, "platform_delegate is null");
  platform_delegate->SendScreenCast(data, metadata);
}

// be used when screen capture trigger by screen content change
void ScreenCastHelper::OnFrameChanged() { TriggerNextCapture(); }

void ScreenCastHelper::OnEngineDestroy() { embedder_proxy_ = nullptr; }

ScreenCastHelperEmbedder::ScreenCastHelperEmbedder(
    devtool::LynxDevToolProxy* proxy,
    const std::shared_ptr<DevtoolPlatformEmbedder>& platform_embedder)
    : paused_(false), weak_platform_embedder_(platform_embedder) {
  screen_cast_helper_ =
      std::make_shared<ScreenCastHelper>(proxy, platform_embedder);
  screen_cast_helper_->SetDelegate(screen_cast_helper_);
  screen_cast_helper_->Init(screen_cast_helper_);
}

ScreenCastHelperEmbedder::~ScreenCastHelperEmbedder() {
  StopCasting();
  screen_cast_helper_->OnEngineDestroy();
}

void ScreenCastHelperEmbedder::StartCasting(int32_t quality, int32_t max_width,
                                            int32_t max_height) {
  auto platform_embedder = weak_platform_embedder_.lock();
  if (platform_embedder) {
    platform_embedder->DispatchScreencastVisibilityChanged(true);
  }
  screen_cast_helper_->StartCapture(quality, max_width, max_height);
}

void ScreenCastHelperEmbedder::ContinueCasting() {
  if (paused_) {
    paused_ = false;
    auto platform_embedder = weak_platform_embedder_.lock();
    if (platform_embedder) {
      platform_embedder->DispatchScreencastVisibilityChanged(true);
    }
    screen_cast_helper_->ContinueCapture();
  }
}

void ScreenCastHelperEmbedder::PauseCasting() {
  if (!paused_) {
    paused_ = true;
    auto platform_embedder = weak_platform_embedder_.lock();
    if (platform_embedder) {
      platform_embedder->DispatchScreencastVisibilityChanged(false);
    }
    screen_cast_helper_->PauseCapture();
  }
}

void ScreenCastHelperEmbedder::StopCasting() {
  screen_cast_helper_->StopCapture();
  auto platform_embedder = weak_platform_embedder_.lock();
  if (platform_embedder) {
    platform_embedder->DispatchScreencastVisibilityChanged(false);
  }
}

void ScreenCastHelperEmbedder::GetLynxScreenShot() {
  screen_cast_helper_->GetLynxScreenShot();
}

}  // namespace devtool
}  // namespace lynx
