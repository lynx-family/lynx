// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#if defined(__linux__)
#include <dlfcn.h>
#include <epoxy/egl.h>
#include <epoxy/gl.h>
#endif

#include "platform/embedder/public/lynx_load_meta.h"
#include "platform/embedder/public/lynx_view.h"
#include "platform/embedder/public/lynx_view_client.h"
#include "platform/embedder/public/lynx_windowless_renderer.h"
#if defined(__APPLE__)
// The standalone host does not collect Lynxtron presentation diagnostics.
#endif
namespace {
void Check(bool condition) {
  if (!condition) {
    std::fputs("WINDOWLESS_LIFECYCLE FAIL\n", stderr);
    std::abort();
  }
}
uint64_t Now() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
struct Task {
  uint64_t when;
  std::function<void()> run;
};
std::mutex task_mutex;
std::vector<Task> tasks;
std::atomic<unsigned> load_errors{0};
class Client final : public lynx::pub::LynxViewClient {
 public:
  void OnRuntimeReady() override { runtime_ready = true; }
  void OnReceivedError(int code, const char* message) override {
    std::fprintf(stderr, "WINDOWLESS_LOAD_ERROR %d %s\n", code,
                 message ? message : "");
    ++load_errors;
  }
  std::atomic<bool> runtime_ready{false};
};
void Post(uint64_t when, std::function<void()> run) {
  std::lock_guard<std::mutex> lock(task_mutex);
  tasks.push_back({when, std::move(run)});
}
void Pump(unsigned milliseconds) {
  const auto end = Now() + milliseconds * 1000000ull;
  while (Now() < end) {
    std::vector<Task> ready;
    {
      std::lock_guard<std::mutex> lock(task_mutex);
      for (auto it = tasks.begin(); it != tasks.end();) {
        if (it->when <= Now()) {
          ready.push_back(std::move(*it));
          it = tasks.erase(it);
        } else {
          ++it;
        }
      }
    }
    for (auto& task : ready) task.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
class Renderer : public lynx::pub::LynxWindowlessRenderer {
 public:
  explicit Renderer(
      lynx_windowless_renderer_type_e type = kRendererTypeSoftware)
      : LynxWindowlessRenderer(type) {}
  bool OnSoftwarePresent(const void* pixels, size_t stride,
                         size_t height) override {
    Check(pixels && stride && stride % 4 == 0 && height);
    if (!loaded_pixels) {
      const auto* data = static_cast<const uint8_t*>(pixels);
      for (size_t row = 0; row < height && !loaded_pixels; ++row)
        for (size_t col = 0; col < stride; col += 4) {
          const auto* p = data + row * stride + col;
          if (p[1] == 105 &&
              ((p[0] == 36 && p[2] == 222) || (p[0] == 222 && p[2] == 36))) {
            loaded_pixels = true;
            break;
          }
        }
    }
    ++frames;
    return true;
  }
  void OnPostTask(lynx_task_t task, uint64_t delay) override {
    Post(Now() + delay, [weak = weak_from_this(), task] {
      if (auto self = weak.lock()) self->RunTask(task);
    });
  }
  std::atomic<unsigned> frames{0};
  std::atomic<bool> loaded_pixels{false};
};
#if defined(__linux__)
#include "platform/embedder/tests/windowless_gl_renderer_linux.h"
#endif
}  // namespace
int main(int argc, char** argv) {
  if (argc != 3 && argc != 4) return 2;
#if defined(__linux__)
  std::shared_ptr<NativeEGL> egl;
  if (argc == 4) egl = std::make_shared<NativeEGL>();
#else
  if (argc == 4) return 2;
#endif
  std::ifstream input(argv[1], std::ios::binary);
  Check(input.good());
  const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
  auto ui = std::this_thread::get_id();
  lynx_windowless_ui_task_runner_config_t config{};
  config.struct_size = sizeof(config);
  config.user_data = &ui;
  config.runs_on_current_thread_callback = +[](void* data) {
    return std::this_thread::get_id() == *static_cast<std::thread::id*>(data);
  };
  config.post_task_callback = +[](lynx_task_t task, uint64_t target, void*) {
    Post(target, [task] { lynx_windowless_run_ui_task(task); });
  };
  Check(lynx_windowless_set_global_ui_task_runner(&config));
  for (unsigned cycle = 0; cycle < 20; ++cycle) {
    std::shared_ptr<Renderer> renderers[2];
    std::unique_ptr<lynx::pub::LynxView> views[2];
    std::shared_ptr<Client> clients[2];
    for (unsigned i = 0; i < 2; ++i) {
#if defined(__linux__)
      if (egl)
        renderers[i] = std::make_shared<GLRenderer>(egl);
      else
#endif
        renderers[i] = std::make_shared<Renderer>();
      lynx::pub::LynxView::Builder builder;
      builder.SetWindowlessRenderer(renderers[i])
          .SetScreenSize(640, 480, 1)
          .SetFrame(0, 0, 640, 480)
          .SetICUDataPath(argv[2])
          .SetEnableJSRuntime(true);
      views[i] = builder.Build();
      Check(views[i] && views[i]->Impl());
      clients[i] = std::make_shared<Client>();
      views[i]->AddClient(clients[i]);
      auto meta = std::make_shared<lynx::pub::LynxLoadMeta>();
      meta->SetUrl(argv[1]);
      meta->SetBinaryData(bytes);
      views[i]->LoadTemplate(meta);
    }
    if (cycle == 0) {
      for (unsigned tries = 0;
           tries < 100 &&
           (!renderers[0]->loaded_pixels || !renderers[1]->loaded_pixels ||
            !clients[0]->runtime_ready || !clients[1]->runtime_ready);
           ++tries)
        Pump(50);
      Check(renderers[0]->loaded_pixels && renderers[1]->loaded_pixels);
      Check(clients[0]->runtime_ready && clients[1]->runtime_ready);
    } else {
      Pump(1);  // Dispose during loading, with another live engine.
    }
    views[0].reset();
    renderers[0].reset();
    Pump(10);
    views[1].reset();
    renderers[1].reset();
    Pump(10);
  }
  Pump(100);
  Check(load_errors == 0);
  std::puts("WINDOWLESS_LIFECYCLE PASS cycles=20 views=40");
  return 0;
}
