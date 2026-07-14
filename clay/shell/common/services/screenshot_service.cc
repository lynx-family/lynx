// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/screenshot_service.h"

#include <atomic>
#include <utility>

#include "clay/shell/common/services/screenshot_encoder.h"
#include "clay/shell/common/shell.h"
#include "clay/ui/common/isolate.h"

namespace clay {

std::shared_ptr<ScreenshotService> ScreenshotService::Create() {
  return std::make_shared<ScreenshotService>();
}

void ScreenshotService::OnInit(clay::ServiceManager& service_manager,
                               const clay::PlatformServiceContext& ctx) {
  shell_ = ctx.shell;
}

void ScreenshotService::SetExternalScreenshotCallback(
    clay::ExternalScreenshotCallback callback) {
  external_screenshot_callback_ = std::move(callback);
}

GrDataPtr ScreenshotService::TakeScreenshotHardware(
    const clay::ScreenshotRequest& request) {
  if (external_screenshot_callback_) {
    return TakeExternalScreenshot(request);
  }

  if (request.is_sync_) {
    auto screenshot = shell_->ScreenshotSync(
        ScreenshotData::ScreenshotType::UncompressedImage,
        request.background_color_);
    if (request.type_ == clay::ScreenshotType::BITMAP) {
      return screenshot.data;
    } else {
      auto result = ScreenshotEncoder::ScaleAndEncode(screenshot, request);
      return result.data;
    }
  }

  if (!request.callback_.has_value()) {
    FML_DLOG(ERROR) << "Has requested an asynchronous screenshot, but there "
                       "is no callback.";
    return nullptr;
  }
  auto callback_posted = std::make_shared<std::atomic_bool>(false);
  shell_->ScreenshotAsync(
      ScreenshotData::ScreenshotType::UncompressedImage,
      request.background_color_,
      [request, callback_posted](ScreenshotData screenshot) {
        if (callback_posted->exchange(true)) {
          return;
        }
        auto task_runner = request.task_runner_
                               ? request.task_runner_
                               : clay::Isolate::Instance().GetIOTaskRunner();
        task_runner->PostTask([screenshot, request]() {
          ScreenshotEncodeResult result;
          if (screenshot.data) {
            result = ScreenshotEncoder::ScaleAndEncode(screenshot, request);
          }
          if (result.data) {
            result.metadata.timestamp_ =
                std::chrono::steady_clock::now().time_since_epoch().count();
          }
          request.callback_.value()(result.data, result.metadata);
        });
      });
  return nullptr;
}

GrDataPtr ScreenshotService::TakeExternalScreenshot(
    const clay::ScreenshotRequest& request) {
  if (!external_screenshot_callback_) {
    return nullptr;
  }

  if (!request.is_sync_ && !request.callback_.has_value()) {
    FML_DLOG(ERROR) << "Has requested an asynchronous screenshot, but there "
                       "is no callback.";
    return nullptr;
  }

  clay::GrImagePtr screenshot = external_screenshot_callback_();

#ifndef ENABLE_SKITY
  SkPixmap pixmap;
  const bool has_pixels = screenshot && screenshot->peekPixels(&pixmap);
#else
  std::shared_ptr<skity::Pixmap> pixmap = screenshot && screenshot->GetPixmap()
                                              ? *(screenshot->GetPixmap())
                                              : nullptr;
  const bool has_pixels = pixmap != nullptr;
#endif  // ENABLE_SKITY

  if (request.is_sync_) {
    if (!has_pixels) {
      return nullptr;
    }
    auto result = ScreenshotEncoder::Encode(pixmap, request);
    return result.data;
  }

  auto task_runner = request.task_runner_
                         ? request.task_runner_
                         : clay::Isolate::Instance().GetIOTaskRunner();
  task_runner->PostTask([screenshot, pixmap, request, has_pixels]() {
    ScreenshotEncodeResult result;
    if (has_pixels) {
      result = ScreenshotEncoder::Encode(pixmap, request);
    }
    if (result.data) {
      result.metadata.timestamp_ =
          std::chrono::steady_clock::now().time_since_epoch().count();
    }
    request.callback_.value()(result.data, result.metadata);
  });
  return nullptr;
}

}  // namespace clay
