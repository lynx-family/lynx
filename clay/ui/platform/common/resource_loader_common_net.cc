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

#include "base/include/string/string_utils.h"
#include "clay/fml/mapping.h"
#include "clay/fml/paths.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/net/net_loader_callback.h"
#include "clay/net/url/url_helper.h"
#include "clay/ui/common/isolate.h"

#if defined(OS_WIN)
#include "clay/ui/resource/zip_resource_helper.h"
#endif

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
            task_runner->PostTask([callback]() { callback(nullptr, 0); });
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
      [weak_ref, callback, ui_task_runner, src](size_t request_seq,
                                                RawResource&& raw_resource) {
        auto self = weak_ref.lock();
        if (!self) {
          FML_LOG(ERROR) << "load resource fail: ResourceLoaderCommon is null";
          if (callback) {
            ui_task_runner->PostTask([callback]() { callback(nullptr, 0); });
          }
          return;
        }
        self->OnLoadFinished(request_seq);
        RawResource resource = self->GetResource(src, raw_resource);

        ui_task_runner->PostTask(
            [callback, resource = std::move(resource)]() mutable {
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
            ui_task_runner->PostTask([callback]() { callback(nullptr, 0); });
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

RawResource ResourceLoaderCommon::GetResource(std::string src,
                                              const RawResource& raw_resource) {
  RawResource resource;
#if defined(OS_WIN)
  static constexpr const char* kClayResourceDir0 = "clay";
  static constexpr const char* kClayResourceDir1 = "resource";
  static const std::string temp_path = fml::CreateTemporaryDirectory();
  std::string output_filename =
      std::to_string(std::hash<std::string>{}(src)) + ".zip";
  if (temp_path.empty()) {
    return resource;
  }
  std::string output_dir_path =
      fml::paths::JoinPaths({temp_path, kClayResourceDir0, kClayResourceDir1});
  if (lynx::base::EndsWith(src, ".zip")) {
    auto output_path =
        fml::paths::JoinPaths({output_dir_path, output_filename});
    fml::NonOwnedMapping mapping(raw_resource.data.get(), raw_resource.length);

    auto temp_dir = fml::OpenDirectory(temp_path.c_str(), true,
                                       fml::FilePermission::kReadWrite);
    if (!temp_dir.is_valid()) {
      return resource;
    }

    auto output_dir =
        fml::CreateDirectory(temp_dir, {kClayResourceDir0, kClayResourceDir1},
                             fml::FilePermission::kReadWrite);
    if (!fml::WriteAtomically(output_dir, output_filename.c_str(), mapping)) {
      output_dir.reset();
      FML_LOG(ERROR) << "Failed to write file: " << output_path;
      resource = {0, nullptr};
    } else {
      output_dir.reset();
      if (ZipResourceHelper::UnzipToPath(output_path, output_dir_path)) {
        std::string file_path = "file:///" + output_dir_path;
        resource = RawResource::MakeWithCopy(
            reinterpret_cast<const uint8_t*>(file_path.c_str()),
            file_path.length());
      } else {
        resource = {0, nullptr};
      }
    }

  } else {
    resource = raw_resource;
  }
#else
  resource = raw_resource;
#endif
  return resource;
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
