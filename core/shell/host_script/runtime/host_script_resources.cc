// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_resources.h"

#include <cerrno>
#include <cstring>
#include <utility>

#if !defined(_WIN32)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "base/include/string/string_utils.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/devtool_lifecycle.h"

namespace lynx::shell {
namespace {
constexpr size_t kMaxResourceBytes = 64 * 1024 * 1024;

bool ValidPath(const std::string& path) {
  if (path.empty() || path.find_first_of("\\:") != std::string::npos ||
      path.find('\0') != std::string::npos)
    return false;
  size_t start = 0;
  do {
    auto end = path.find('/', start);
    auto part = path.substr(start, end - start);
    if (part.empty() || part == "." || part == "..") return false;
    if (end == std::string::npos) return true;
    start = end + 1;
  } while (start <= path.size());
  return false;
}

// Open each component relative to an already-open directory. Reject symlinks
// rather than checking a canonical path and then reopening a different file.
std::string ReadFile(const std::string& root, const std::string& path,
                     std::string& bytes) {
#if defined(_WIN32)
  return "RESOURCE_PLATFORM_UNSUPPORTED";
#else
  if (root.empty()) return "RESOURCE_ROOT_UNAVAILABLE";
  int fd = open(root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (fd < 0) return "RESOURCE_ROOT_UNAVAILABLE";
  size_t start = 0;
  while (true) {
    auto end = path.find('/', start);
    auto part = path.substr(start, end - start);
    int flags = O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK;
    if (end != std::string::npos) flags |= O_DIRECTORY;
    int next = openat(fd, part.c_str(), flags);
    int error = errno;
    close(fd);
    if (next < 0)
      return error == ENOENT ? "RESOURCE_NOT_FOUND" : "RESOURCE_READ_FAILED";
    fd = next;
    if (end == std::string::npos) break;
    start = end + 1;
  }
  struct CloseFile {
    int fd;
    ~CloseFile() { close(fd); }
  } file{fd};
  struct stat status {};
  if (fstat(fd, &status) != 0 || !S_ISREG(status.st_mode) || status.st_size < 0)
    return "RESOURCE_READ_FAILED";
  if (static_cast<uint64_t>(status.st_size) > kMaxResourceBytes)
    return "RESOURCE_TOO_LARGE";
  bytes.resize(static_cast<size_t>(status.st_size));
  size_t offset = 0;
  while (offset < bytes.size()) {
    auto count = read(fd, bytes.data() + offset, bytes.size() - offset);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) return "RESOURCE_READ_FAILED";
    offset += static_cast<size_t>(count);
  }
  // Also reject files that grew while being read.
  char extra;
  ssize_t count;
  do {
    count = read(fd, &extra, 1);
  } while (count < 0 && errno == EINTR);
  return count == 0 ? "" : "RESOURCE_READ_FAILED";
#endif
}

void Reject(Napi::Promise::Deferred& deferred, const std::string& code) {
  auto error = Napi::Error::New(deferred.Env(), code.c_str());
  error.Set("code", Napi::String::New(deferred.Env(), code));
  deferred.Reject(error);
}
}  // namespace

HostScriptResources::HostScriptResources(fml::RefPtr<fml::TaskRunner> runner,
                                         std::string root)
    : runner_(std::move(runner)), root_(std::move(root)) {}

std::shared_ptr<HostScriptResources> HostScriptResources::Install(
    Napi::Env env, fml::RefPtr<fml::TaskRunner> runner, std::string root) {
  auto resources = std::shared_ptr<HostScriptResources>(
      new HostScriptResources(std::move(runner), std::move(root)));
  struct CallbackData {
    std::weak_ptr<HostScriptResources> resources;
    bool text;
  };
  auto object = Napi::Object::New(env);
  for (bool text : {true, false}) {
    const char* name = text ? "readText" : "readBytes";
    auto* data = new CallbackData{resources, text};
    auto function = Napi::Function::New(
        env,
        [](const Napi::CallbackInfo& info) -> Napi::Value {
          auto* data = static_cast<CallbackData*>(info.Data());
          auto self = data->resources.lock();
          if (self && self->attached_) return self->Read(info, data->text);
          auto deferred = Napi::Promise::Deferred::New(info.Env());
          Reject(deferred, "INVALID_STATE");
          return deferred.Promise();
        },
        name, data);
    function.AddFinalizer(data, [](napi_env, void* data, void*) {
      delete static_cast<CallbackData*>(data);
    });
    object.Set(name, function);
  }
  env.Global().Set("resources", object);
  return resources;
}

Napi::Value HostScriptResources::Read(const Napi::CallbackInfo& info,
                                      bool text) {
  auto deferred = Napi::Promise::Deferred::New(info.Env());
  auto promise = deferred.Promise();
  if (info.Length() != 1 || !info[0].IsString()) {
    Reject(deferred, "INVALID_ARGUMENT");
    return promise;
  }
  auto path = info[0].As<Napi::String>().Utf8Value();
  if (!ValidPath(path)) {
    Reject(deferred, "INVALID_ARGUMENT");
    return promise;
  }
  if (!tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    Reject(deferred, "INVALID_STATE");
    return promise;
  }
  const auto request = ++next_request_;
  pending_.emplace(request, std::move(deferred));
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [weak = weak_from_this(), runner = runner_, root = root_,
       path = std::move(path), request, text] {
        std::string bytes;
        auto error = ReadFile(root, path, bytes);
        if (error.empty() && text &&
            !base::IsValidUtf8(reinterpret_cast<const uint8_t*>(bytes.data()),
                               bytes.size()))
          error = "INVALID_UTF8";
        if (!error.empty()) bytes.clear();
        runner->PostTask([weak, request, text, bytes = std::move(bytes),
                          error = std::move(error)]() mutable {
          if (auto self = weak.lock(); self && self->attached_)
            self->Complete(request, text, std::move(bytes), error);
        });
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
  return promise;
}

void HostScriptResources::Complete(uint64_t request, bool text,
                                   std::string bytes,
                                   const std::string& error) {
  auto found = pending_.find(request);
  if (found == pending_.end()) return;
  auto deferred = std::move(found->second);
  pending_.erase(found);
  Napi::ContextScope context_scope(deferred.Env());
  Napi::HandleScope scope(deferred.Env());
  if (!tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    Reject(deferred, "INVALID_STATE");
  } else if (!error.empty()) {
    Reject(deferred, error);
  } else if (text) {
    deferred.Resolve(Napi::String::New(deferred.Env(), bytes));
  } else {
    auto buffer = Napi::ArrayBuffer::New(deferred.Env(), bytes.size());
    if (!bytes.empty()) memcpy(buffer.Data(), bytes.data(), bytes.size());
    deferred.Resolve(buffer);
  }
}

void HostScriptResources::Detach() {
  attached_ = false;
  auto pending = std::move(pending_);
  pending_.clear();
  for (auto& entry : pending) {
    Napi::ContextScope context_scope(entry.second.Env());
    Napi::HandleScope scope(entry.second.Env());
    Reject(entry.second, "INVALID_STATE");
  }
}
}  // namespace lynx::shell
