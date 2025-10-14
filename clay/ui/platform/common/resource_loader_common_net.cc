// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/common/resource_loader_common_net.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <utility>
#if defined(OS_WIN)
#include <Shlwapi.h>
#endif

#include "clay/fml/mapping.h"
#include "clay/fml/paths.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/net/net_loader_callback.h"
#include "clay/net/url/url_helper.h"
#include "clay/ui/common/isolate.h"

namespace clay {

ResourceLoaderCommon::ResourceLoaderCommon(
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    fml::RefPtr<fml::TaskRunner> task_runner)
    : resource_loader_intercept_(intercept),
      ui_task_runner_(std::move(task_runner)) {}

ResourceLoaderCommon::~ResourceLoaderCommon() = default;

void ResourceLoaderCommon::Load(
    const std::string& src,
    const std::function<void(const uint8_t*, size_t)>& callback,
    const ResourceType resource_type, bool need_redirect) {
  std::weak_ptr<ResourceLoaderCommon> weak_ref = shared_from_this();
  Isolate::Instance().GetIOTaskRunner()->PostTask(
      [weak_ref, src, callback, need_redirect, task_runner = ui_task_runner_,
       intercept = resource_loader_intercept_]() {
        auto self = weak_ref.lock();
        if (!self) {
          FML_LOG(ERROR) << "load resource fail: ResourceLoaderCommon is null";
          if (callback) {
            callback(nullptr, 0);
          }
          return;
        }
        std::string intercept_url = src;
        if (need_redirect && intercept) {
          intercept_url = intercept->ShouldInterceptUrl(src, false);
        }
        self->RealLoad(weak_ref, task_runner, intercept_url, callback);
      });
}

void ResourceLoaderCommon::RealLoad(
    std::weak_ptr<ResourceLoaderCommon> weak_ref,
    fml::RefPtr<fml::TaskRunner> ui_task_runner, const std::string& src,
    const std::function<void(const uint8_t*, size_t)>& callback) {
  url::UriSchemeType scheme_type = url::ParseUriScheme(src);

  // Since base64(data) is handled in data_image_loader, we will not handle it.
  if (scheme_type == url::UriSchemeType::kNet) {
    LoadOnNet(weak_ref, ui_task_runner, src, callback);
  } else if (scheme_type == url::UriSchemeType::kLocalFile) {
    LoadByFile(ui_task_runner, src, callback);
  } else {
    ui_task_runner->PostTask([callback]() { callback(nullptr, 0); });
  }
}

void ResourceLoaderCommon::LoadOnNet(
    std::weak_ptr<ResourceLoaderCommon> weak_ref,
    fml::RefPtr<fml::TaskRunner> ui_task_runner, const std::string& src,
    const std::function<void(const uint8_t*, size_t)>& callback) {
  NetLoaderCallback loader_callback;
  loader_callback.set_succeeded_func(
      [weak_ref, callback, ui_task_runner](size_t request_seq,
                                           RawResource&& raw_resource) {
        auto self = weak_ref.lock();
        if (!self) {
          FML_LOG(ERROR) << "load resource fail: ResourceLoaderCommon is null";
          if (callback) {
            callback(nullptr, 0);
          }
          return;
        }
        self->OnLoadFinished(request_seq);
        ui_task_runner->PostTask(
            [callback, resource = std::move(raw_resource)]() mutable {
              if (callback) {
                callback(resource.data.get(), resource.length);
              }
            });
      });
  loader_callback.set_failed_func(
      [weak_ref, src, callback, ui_task_runner](size_t request_seq,
                                                const std::string& reason) {
        auto self = weak_ref.lock();
        if (!self) {
          FML_LOG(ERROR) << "load resource fail: ResourceLoaderCommon is null";
          if (callback) {
            callback(nullptr, 0);
          }
          return;
        }
        self->OnLoadFinished(request_seq);
        FML_LOG(WARNING) << "fail to load " << src << " with reason " << reason;
        ui_task_runner->PostTask([callback] {
          if (callback) {
            callback(nullptr, 0);
          }
        });
      });
  size_t pending_seq =
      NetLoaderManager::Instance().Request(src, std::move(loader_callback));
  if (pending_seq != NetLoaderManager::kInvalidRequestSeq) {
    std::scoped_lock lock(pending_requests_mutex_);
    pending_requests_.insert(pending_seq);
  }
}

void ResourceLoaderCommon::LoadByFile(
    fml::RefPtr<fml::TaskRunner> ui_task_runner, const std::string& src,
    const std::function<void(const uint8_t*, size_t)>& callback) {
  ui_task_runner->PostTask([src, callback]() {
    std::string file_path = fml::paths::AbsolutePath(fml::paths::FromURI(src));
    auto mapping = fml::FileMapping::CreateReadOnly(file_path);
    if (mapping && mapping->IsValid()) {
      callback(mapping->GetMapping(), mapping->GetSize());
    } else {
      callback(nullptr, 0);
    }
  });
}

void ResourceLoaderCommon::OnLoadFinished(size_t request_seq) {
  std::scoped_lock lock(pending_requests_mutex_);
  pending_requests_.erase(request_seq);
}

RawResource ResourceLoaderCommon::LoadSync(const std::string& src,
                                           const ResourceType resource_type,
                                           bool need_redirect) {
  std::string intercept_url = src;
  if (need_redirect && resource_loader_intercept_) {
    intercept_url = resource_loader_intercept_->ShouldInterceptUrl(src, false);
  }

  url::UriSchemeType scheme_type = url::ParseUriScheme(intercept_url);

  // Since base64(data) is handled in data_image_loader, resource loader common
  // will not handle it.
  if (scheme_type == url::UriSchemeType::kNet) {
    resource_ = NetLoaderManager::Instance().RequestSync(intercept_url);
    return resource_;
  } else if (scheme_type == url::UriSchemeType::kLocalFile) {
    std::string file_path = fml::paths::FromURI(intercept_url);
    auto mapping = fml::FileMapping::CreateReadOnly(file_path);

    if (mapping && mapping->IsValid()) {
      return RawResource::MakeWithCopy(mapping->GetMapping(),
                                       mapping->GetSize());
    }
  }
  return {0, nullptr};
}

void ResourceLoaderCommon::CancelAll() {
  std::set<size_t> pending_requests;
  {
    std::scoped_lock lock(pending_requests_mutex_);
    pending_requests.swap(pending_requests_);
  }
  for (auto pending_request : pending_requests) {
    NetLoaderManager::Instance().CancelBySeq(pending_request);
  }
}

}  // namespace clay
