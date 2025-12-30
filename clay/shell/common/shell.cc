// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/shell.h"

#include <future>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "base/include/fml/message_loop.h"
#include "base/include/fml/unique_fd.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/common/graphics/shared_image_external_texture.h"
#include "clay/common/sys_info.h"
#include "clay/common/task_runners.h"
#include "clay/common/thread_host.h"
#include "clay/flow/frame_timings.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/shell/common/services/animation_event_service_impl.h"
#include "clay/shell/common/services/sync_compositor_service.h"
#include "clay/ui/common/frame_timing_collector.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/common/render_settings.h"
#ifdef ENABLE_NET_LOADER
#include "clay/net/net_loader_manager.h"
#endif
#include "base/trace/native/trace_event.h"
#include "clay/fml/base32.h"
#include "clay/fml/file.h"
#include "clay/fml/log_settings.h"
#include "clay/fml/logging.h"
#include "clay/fml/paths.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/shell/common/output_surface.h"
#include "clay/shell/common/pointer_data_to_event.h"
#include "clay/shell/common/services/platform_const_service.h"
#include "clay/shell/common/shell_common_rendering_backend.h"
#ifndef ENABLE_SKITY
#include "clay/shell/common/skia_event_tracer_impl.h"
#endif  // ENABLE_SKITY
#include "clay/shell/common/switches.h"
#include "clay/shell/common/vsync_waiter.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"
#if defined(OS_WIN) || defined(OS_MAC)
#include "clay/memory/memory_pressure_monitor.h"
#endif
#include "base/include/fml/task_runner.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/ui/platform/keyboard_types.h"

namespace clay {

namespace {

std::unique_ptr<Engine> CreateEngine(
    std::shared_ptr<clay::ServiceManager> service_manager,
    TaskRunners task_runners, Settings settings,
    fml::RefPtr<GPUUnrefQueue> unref_queue, Engine::Delegate* delegate) {
  return std::make_unique<Engine>(service_manager, task_runners, settings,
                                  unref_queue, delegate);
}

// Though there can be multiple shells, some settings apply to all components in
// the process. These have to be set up before the shell or any of its
// sub-components can be initialized. In a perfect world, this would be empty.
// TODO(chinmaygarde): The unfortunate side effect of this call is that settings
// that cause shell initialization failures will still lead to some of their
// settings being applied.
void PerformInitializationTasks(Settings& settings) {  // NOLINT
  {
    fml::LogSettings log_settings;
#if defined(OS_WIN) || defined(OS_MAC)
    log_settings.min_log_level = fml::LOG_INFO;
#else
    log_settings.min_log_level =
        settings.verbose_logging ? fml::LOG_INFO : fml::LOG_WARNING;
#endif
    fml::SetLogSettings(log_settings);
  }

  static std::once_flag gShellSettingsInitialization = {};
  std::call_once(gShellSettingsInitialization, [&settings] {
#ifndef ENABLE_SKITY
    if (settings.trace_skia) {
      InitSkiaEventTracer(settings.trace_skia, settings.trace_skia_allowlist);
    }

    if (!settings.skia_deterministic_rendering_on_cpu) {
      SkGraphics::Init();
    } else {
      FML_DLOG(INFO) << "Skia deterministic rendering is enabled.";
    }

#if !defined(USE_SYSTEM_ICU)
    if (settings.icu_initialization_required) {
      if (!settings.icu_data_path.empty()) {
        fml::icu::InitializeICU(settings.icu_data_path);
      } else if (settings.icu_mapper) {
        fml::icu::InitializeICUFromMapping(settings.icu_mapper());
      } else {
        FML_DLOG(WARNING) << "Skipping ICU initialization in the shell.";
      }
    }
#endif  // USE_SYSTEM_ICU
#endif  // ENABLE_SKITY
  });

  PersistentCache::SetCacheSkSL(settings.cache_sksl);
}

}  // namespace

std::unique_ptr<Shell> Shell::Create(
    std::shared_ptr<clay::ServiceManager> service_manager,
    const TaskRunners& task_runners, Settings settings,
    const Shell::CreateCallback<PlatformView>& on_create_platform_view,
    const Shell::CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer,
    bool is_gpu_disabled) {
  clay::Isolate::Instance().SetIOTaskRunner(task_runners.GetIOTaskRunner());
  clay::Isolate::Instance().SetPlatformTaskRunner(
      task_runners.GetPlatformTaskRunner());

  // Lynx would init tracing func early before here.
  TRACE_EVENT("clay", "Shell::Create");

  PerformInitializationTasks(settings);
  auto resource_cache_limit_calculator =
      std::make_shared<ResourceCacheLimitCalculator>(
          settings.resource_cache_max_bytes_threshold);
  return CreateWithSnapshot(service_manager,
                            task_runners,                     //
                            resource_cache_limit_calculator,  //
                            settings,                         //
                            on_create_platform_view,          //
                            on_create_rasterizer,             //
                            CreateEngine, is_gpu_disabled);
}

std::unique_ptr<Shell> Shell::CreateShellOnPlatformThread(
    std::shared_ptr<clay::ServiceManager> service_manager,
    const std::shared_ptr<ResourceCacheLimitCalculator>&
        resource_cache_limit_calculator,
    const TaskRunners& task_runners, const Settings& settings,
    const Shell::CreateCallback<PlatformView>& on_create_platform_view,
    const Shell::CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer,
    const Shell::EngineCreateCallback& on_create_engine, bool is_gpu_disabled) {
  if (!task_runners.IsValid()) {
    FML_LOG(ERROR) << "Task runners to run the shell were invalid.";
    return nullptr;
  }

  auto shell = std::unique_ptr<Shell>(new Shell(service_manager, task_runners,
                                                resource_cache_limit_calculator,
                                                settings, is_gpu_disabled));

  // Create the platform view on the platform thread (this thread).
  auto platform_view = on_create_platform_view(service_manager, *shell.get());
  if (!platform_view || !platform_view->GetWeakPtr()) {
    return nullptr;
  }

  // Send dispatcher_maker to the engine constructor because shell won't have
  // platform_view set until Shell::Setup is called later.
  auto dispatcher_maker = platform_view->GetDispatcherMaker();

  auto unref_queue = clay::GraphicsIsolate::Instance().GetOrCreateUnrefQueue(
      task_runners.GetRasterTaskRunner());
  // create clay engine
  std::promise<std::unique_ptr<Engine>> engine_promise;
  auto engine_future = engine_promise.get_future();
  fml::TaskRunner::RunNowOrPostTask(
      shell->GetTaskRunners().GetUITaskRunner(),
      fml::MakeCopyable([shell = shell.get(), service_manager, &engine_promise,
                         unref_queue, &on_create_engine]() mutable {
        TRACE_EVENT("clay", "ShellSetupUISubsystem");
        const auto& task_runners = shell->GetTaskRunners();

        std::unique_ptr<Engine> engine =
            on_create_engine(service_manager, task_runners,
                             shell->GetSettings(), unref_queue, shell);

        engine_promise.set_value(std::move(engine));
      }));

  fml::TaskRunner::RunNowOrPostTask(shell->GetTaskRunners().GetIOTaskRunner(),
                                    []() {
                                      // Calling IsLowEndDevice() for the first
                                      // time may be time-consuming.
                                      clay::SysInfo::IsLowEndDevice();
                                    });
  fml::RefPtr<clay::RenderSettings> render_settings =
      fml::MakeRefCounted<clay::RenderSettings>();
  std::unique_ptr<Engine> engine = engine_future.get();
  engine->SetRenderSettings(render_settings);

  if (!shell->Setup(std::move(platform_view), std::move(engine))) {
    return nullptr;
  }

  // The detach order must be reversed from the attach order.
  // Attach: Platform -> Raster -> UI
  // Detach: UI -> Raster -> Platform
  service_manager->Attach<clay::Owner::kPlatform>(clay::PlatformServiceContext{
      .platform_view = shell->GetPlatformView().get(), .shell = shell.get()});

  shell->GetTaskRunners().GetIOTaskRunner()->PostTask([service_manager] {
    service_manager->Attach<clay::Owner::kIO>(clay::IOServiceContext{});
  });

  // Create the rasterizer on the raster thread.
  fml::TaskRunner::RunNowOrPostTask(
      task_runners.GetRasterTaskRunner(),
      [service_manager, on_create_rasterizer, render_settings, unref_queue,
       page_id = static_cast<uint32_t>(
           shell->GetEngine()->GetPageView()->PageUniqueId())]() {
        TRACE_EVENT("clay", "ShellSetupGPUSubsystem");
        std::unique_ptr<Rasterizer> rasterizer(
            on_create_rasterizer(service_manager));
        rasterizer->set_unref_queue(unref_queue);
        rasterizer->SetRenderSettings(render_settings);
        service_manager->Attach<clay::Owner::kRaster>(
            clay::RasterServiceContext{.rasterizer = rasterizer.release(),
                                       .page_unique_id = page_id});
      });

  fml::TaskRunner::RunNowOrPostTask(
      shell->GetTaskRunners().GetUITaskRunner(),
      fml::MakeCopyable([service_manager, engine = shell->GetEngine(),
                         unref_queue]() mutable {
        if (engine) {
          service_manager->Attach<clay::Owner::kUI>(
              clay::UIServiceContext{.engine = engine.get()});
        }
      }));

  return shell;
}

std::unique_ptr<Shell> Shell::CreateWithSnapshot(
    std::shared_ptr<clay::ServiceManager> service_manager,
    const TaskRunners& task_runners,
    const std::shared_ptr<ResourceCacheLimitCalculator>&
        resource_cache_limit_calculator,
    Settings settings,
    const Shell::CreateCallback<PlatformView>& on_create_platform_view,
    const Shell::CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer,
    const Shell::EngineCreateCallback& on_create_engine, bool is_gpu_disabled) {
  // This must come first as it initializes tracing.
  PerformInitializationTasks(settings);

  TRACE_EVENT("clay", "Shell::CreateWithSnapshot");

  const bool callbacks_valid =
      on_create_platform_view && on_create_rasterizer && on_create_engine;
  if (!task_runners.IsValid() || !callbacks_valid) {
    return nullptr;
  }

  fml::AutoResetWaitableEvent latch;
  std::unique_ptr<Shell> shell;
  auto platform_task_runner = task_runners.GetPlatformTaskRunner();
  fml::TaskRunner::RunNowOrPostTask(
      platform_task_runner,
      fml::MakeCopyable([service_manager, &latch,                            //
                         &shell,                                             //
                         resource_cache_limit_calculator,                    //
                         task_runners = task_runners,                        //
                         settings = settings,                                //
                         on_create_platform_view = on_create_platform_view,  //
                         on_create_rasterizer = on_create_rasterizer,        //
                         on_create_engine = on_create_engine,
                         is_gpu_disabled]() mutable {
        shell = CreateShellOnPlatformThread(service_manager,
                                            resource_cache_limit_calculator,  //
                                            task_runners,                     //
                                            settings,                         //
                                            on_create_platform_view,          //
                                            on_create_rasterizer,             //
                                            on_create_engine, is_gpu_disabled);
        latch.Signal();
      }));
  latch.Wait();
#if defined(OS_ANDROID)
  shell->SetUpMemoryPressureListener();
#endif
#if defined(OS_WIN)
  clay::MemoryPressureMonitor::GetInstance().SetTaskRunner(
      task_runners.GetUITaskRunner());
  shell->SetUpMemoryPressureListener();
  clay::MemoryPressureMonitor::GetInstance().StartMonitoring();
#endif
#if defined(OS_MAC)
  shell->SetUpMemoryPressureListener();
  clay::MemoryPressureMonitor::GetInstance().StartMonitoring();
#endif
  return shell;
}

#if defined(OS_ANDROID) || defined(OS_WIN) || defined(OS_MAC)
void Shell::SetUpMemoryPressureListener() {
  FML_DCHECK(memory_pressure_listener_ == nullptr);
  auto low_memory_callback =
      [self = weak_factory_.GetWeakPtr()](
          clay::MemoryPressureListener::MemoryPressureLevel) {
        if (self) {
          self->NotifyLowMemoryWarning();
        }
      };
  memory_pressure_listener_ = std::make_unique<clay::MemoryPressureListener>(
      low_memory_callback, task_runners_.GetUITaskRunner());
}
#endif

Shell::Shell(std::shared_ptr<clay::ServiceManager> service_manager,
             const TaskRunners& task_runners,
             const std::shared_ptr<ResourceCacheLimitCalculator>&
                 resource_cache_limit_calculator,
             const Settings& settings, bool is_gpu_disabled)
    : service_manager_(service_manager),
      task_runners_(task_runners),
      animator_info_service_(
          service_manager->GetService<AnimatorInfoService>()),
      rasterizer_service_(service_manager->GetService<RasterizerService>()),
      resource_cache_limit_calculator_(resource_cache_limit_calculator),
      settings_(settings),
      weak_factory_(this) {
  FML_DCHECK(task_runners_.IsValid());
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  display_manager_ = std::make_shared<DisplayManager>();
  auto platform_const_service = std::make_shared<PlatformConstService>(
      settings, is_gpu_disabled, display_manager_);
  is_gpu_disabled_sync_switch_ =
      platform_const_service->GetIsGPUDisabledSyncSwitch();
  service_manager->RegisterService<PlatformConstService>(
      platform_const_service);
  service_manager_->RegisterService<AnimationEventService>(
      std::make_shared<AnimationEventServiceImpl>());

  resource_cache_limit_calculator->AddResourceCacheLimitItem(
      weak_factory_.GetWeakPtr());

  frame_timing_collector_ = std::make_shared<clay::FrameTimingCollector>(
      task_runners_.GetPlatformTaskRunner());
}

Shell::~Shell() {
  // If GPUUnrefQueue is not shared, we need to remove it from GraphicsIsolate.
  if (engine_ && engine_->GetPageView()) {
    auto unref_queue = engine_->GetPageView()->GetUnrefQueue();
    if (unref_queue && !unref_queue->IsShared()) {
      clay::GraphicsIsolate::Instance().RemoveUnrefQueue(
          unref_queue->GetTaskRunner());
    }
  }

  task_runners_.GetIOTaskRunner()->PostTask(
      [service_manager = service_manager_] {
        service_manager->Detach<clay::Owner::kIO>();
      });

#if defined(OS_ANDROID) || defined(OS_MAC) || defined(OS_WIN)
  memory_pressure_listener_.reset();
#endif

  PersistentCache::GetCacheForProcess()->RemoveWorkerTaskRunner(
      task_runners_.GetIOTaskRunner());

  fml::AutoResetWaitableEvent ui_latch, platform_latch, io_latch;

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetPlatformTaskRunner(),
      fml::MakeCopyable(
          [platform_view = platform_view_.get(), &platform_latch]() mutable {
            if (platform_view) {
              platform_view->ReleasePlatformInstanceManager();
            }
            platform_latch.Signal();
          }));
  platform_latch.Wait();

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      fml::MakeCopyable([service_manager = service_manager_,
                         engine = std::move(engine_), &ui_latch]() mutable {
        service_manager->Detach<clay::Owner::kUI>();
        engine.reset();
        ui_latch.Signal();
      }));
  ui_latch.Wait();

  std::optional<std::promise<void>> raster_thread_promise;
  std::optional<std::future<void>> raster_thread_future;

  if (platform_view_ && !platform_view_->IsOutputSurfaceThreadSafe()) {
    raster_thread_promise.emplace();
    raster_thread_future.emplace(raster_thread_promise->get_future());
  }

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetRasterTaskRunner(),
      [p = std::move(raster_thread_promise),
       // OutputSurface MUST live longer than Rasterizer, since GPU surface
       // holds the raw pointer to it as a delegate
       output_surface = platform_view_->GetOutputSurface(),
       service_manager = service_manager_]() mutable {
        clay::Puppet<clay::Owner::kRaster, RasterizerService> raster_service =
            service_manager->GetService<RasterizerService>();
        Rasterizer* rasterizer = raster_service->GetRasterizer();
        service_manager->Detach<clay::Owner::kRaster>();
        delete rasterizer;
        if (p.has_value()) {
          p->set_value();
        }
      });

  if (raster_thread_future.has_value()) {
    raster_thread_future->wait();
  }

  // The platform view must go last because it may be holding onto platform side
  // counterparts to resources owned by subsystems running on other threads. For
  // example, the NSOpenGLContext on the Mac.
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetPlatformTaskRunner(),
      fml::MakeCopyable([service_manager = service_manager_,
                         platform_view = std::move(platform_view_),
                         &platform_latch]() mutable {
        service_manager->Detach<clay::Owner::kPlatform>();
        platform_view.reset();
        platform_latch.Signal();
      }));
  platform_latch.Wait();
}

std::unique_ptr<Shell> Shell::Spawn(
    std::shared_ptr<clay::ServiceManager> service_manager,
    const std::string& initial_route,
    const CreateCallback<PlatformView>& on_create_platform_view,
    const CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer) const {
  FML_DCHECK(task_runners_.IsValid());
  // It's safe to store this value since it is set on the platform thread.
  bool is_gpu_disabled = false;
  GetIsGpuDisabledSyncSwitch()->Execute(
      fml::SyncSwitch::Handlers()
          .SetIfFalse([&is_gpu_disabled] { is_gpu_disabled = false; })
          .SetIfTrue([&is_gpu_disabled] { is_gpu_disabled = true; }));

  std::unique_ptr<Shell> result = CreateWithSnapshot(
      service_manager, task_runners_, resource_cache_limit_calculator_,
      GetSettings(), on_create_platform_view, on_create_rasterizer,
      [engine = this->engine_.get()](
          std::shared_ptr<clay::ServiceManager> service_manager,
          TaskRunners task_runners, Settings settings,
          fml::RefPtr<GPUUnrefQueue> unref_queue, Engine::Delegate* delegate) {
        return engine->Spawn(service_manager, /*settings=*/settings,
                             unref_queue, delegate);
      },
      is_gpu_disabled);
  return result;
}

std::shared_ptr<clay::ServiceManager> Shell::SpawnServiceManager() const {
  return clay::ServiceManager::Create(
      {task_runners_.GetPlatformTaskRunner(), task_runners_.GetUITaskRunner(),
       task_runners_.GetRasterTaskRunner(), task_runners_.GetIOTaskRunner()});
}

void Shell::NotifyLowMemoryWarning() const {
  uint64_t flow_id = TRACE_FLOW_ID();
  TRACE_EVENT("clay", "Shell::NotifyLowMemoryWarning",
              [flow_id = flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(flow_id);
              });
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(), [engine = weak_engine_] {
        if (engine) {
          // Try to clear image cache when memory is low
          clay::Isolate::Instance().GetResourceCache()->ClearCache();
          auto context = engine->GetViewContext();
          if (context) {
            context->NotifyLowMemory();
          }
        }
      });
  rasterizer_service_.Act([flow_id = flow_id](auto& impl) {
    impl.GetRasterizer()->NotifyLowMemoryWarning();
    TRACE_EVENT("clay", "Rasterizer::NotifyLowMemoryWarning",
                [flow_id = flow_id](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_terminating_flow_ids(flow_id);
                });
  });
  task_runners_.GetIOTaskRunner()->PostTask([]() {
#ifdef ENABLE_NET_LOADER
    clay::NetLoaderManager::Instance().NotifyLowMemory();
#endif  // ENABLE_NET_LOADER
  });
}

bool Shell::IsSetup() const { return is_setup_; }

bool Shell::Setup(std::unique_ptr<PlatformView> platform_view,
                  std::unique_ptr<Engine> engine) {
  if (is_setup_) {
    return false;
  }

  if (!platform_view) {
    return false;
  }
  platform_view_ = std::move(platform_view);
  engine_ = std::move(engine);

  weak_engine_ = engine_->GetWeakPtr();
  weak_platform_view_ = platform_view_->GetWeakPtr();

  // Setup the time-consuming default font manager right after engine created.
  if (!settings_.prefetched_default_font_manager) {
    fml::TaskRunner::RunNowOrPostTask(task_runners_.GetUITaskRunner(),
                                      [engine = weak_engine_] {
                                        if (engine) {
                                          engine->SetupDefaultFontManager();
                                        }
                                      });
  }

  // Create FrameTimingCollector and set to engine
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      [frame_timing_collector = frame_timing_collector_, engine = weak_engine_,
       ui_task_runner = task_runners_.GetUITaskRunner(),
       raster_task_runner = task_runners_.GetRasterTaskRunner()] {
        if (engine) {
          engine->SetFrameTimingCollector(frame_timing_collector);
          frame_timing_collector->SetPageView(engine->GetPageView());
        }
      });

  // async init main skia context in rasterThread
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetRasterTaskRunner(),
      [output_surface = platform_view_->GetOutputSurface(),
       unref_queue = engine_->GetPageView()->GetUnrefQueue(),
       task_runner = task_runners_.GetUITaskRunner()]() {
        if (output_surface) {
          output_surface->CreateMainGrContext();
          clay::GrContextPtr main_context = output_surface->GetMainGrContext();
          if (main_context && unref_queue) {
            unref_queue->SetContext(main_context);
          }
        }
      });

  is_setup_ = true;

  PersistentCache::GetCacheForProcess()->AddWorkerTaskRunner(
      task_runners_.GetIOTaskRunner());

  PersistentCache::GetCacheForProcess()->SetIsDumpingSkp(
      settings_.dump_skp_on_shader_compilation);

#ifdef ENABLE_NET_LOADER
  clay::NetLoaderManager::Instance().EnsureSetup(
      clay::Isolate::Instance().GetIOTaskRunner(), 10 * 1024 * 1024 /*10mB*/);
#endif  // ENABLE_NET_LOADER

  if (settings_.purge_persistent_cache) {
    PersistentCache::GetCacheForProcess()->Purge();
  }

  return true;
}

const Settings& Shell::GetSettings() const { return settings_; }

const TaskRunners& Shell::GetTaskRunners() const { return task_runners_; }

fml::WeakPtr<Engine> Shell::GetEngine() {
  FML_DCHECK(is_setup_);
  return weak_engine_;
}

fml::WeakPtr<PlatformView> Shell::GetPlatformView() {
  FML_DCHECK(is_setup_);
  return weak_platform_view_;
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewCreated() {
  TRACE_EVENT("clay", "Shell::OnPlatformViewCreated");

  fml::TaskRunner::RunNowOrPostTask(task_runners_.GetUITaskRunner(),
                                    [engine = engine_->GetWeakPtr()] {
                                      if (engine) {
                                        engine->OnPlatformViewCreated();
                                      }
                                    });

  fml::RefPtr<OutputSurface> output_surface =
      platform_view_->GetOutputSurface();
  if (!output_surface) {
    fml::TaskRunner::RunNowOrPostTask(task_runners_.GetUITaskRunner(),
                                      [engine = engine_->GetWeakPtr()] {
                                        if (engine) {
                                          engine->OnOutputSurfaceCreateFailed();
                                        }
                                      });
    return;
  }
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  rasterizer_service_.Act([waiting_for_first_frame = waiting_for_first_frame_,
                           output_surface,
                           ui_task_runner = task_runners_.GetUITaskRunner(),
                           engine = engine_->GetWeakPtr()](auto& impl) mutable {
    bool success = false;
    if (auto surface = output_surface->CreateGPUSurface();
        surface && surface->IsValid()) {
      impl.GetRasterizer()->Setup(std::move(surface));
      waiting_for_first_frame->store(true);
      success = true;
    }
    fml::TaskRunner::RunNowOrPostTask(ui_task_runner, [engine, success] {
      if (engine) {
        if (success) {
          engine->OnOutputSurfaceCreated();
        } else {
          engine->OnOutputSurfaceCreateFailed();
        }
      }
    });
  });
  clay::Puppet<clay::Owner::kPlatform, RasterFrameService>
      raster_frame_service =
          GetServiceManager()->GetService<RasterFrameService>();
  raster_frame_service.Act(
      [](auto& impl) { impl.SetOutputSurfaceValid(true); });
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewDestroyed() {
  TRACE_EVENT("clay", "Shell::OnPlatformViewDestroyed");
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  clay::Puppet<clay::Owner::kPlatform, RasterFrameService>
      raster_frame_service =
          GetServiceManager()->GetService<RasterFrameService>();
  raster_frame_service.Act(
      [](auto& impl) { impl.SetOutputSurfaceValid(false); });

  task_runners_.GetUITaskRunner()->PostTask([engine = engine_->GetWeakPtr()]() {
    if (engine) {
      engine->OnOutputSurfaceDestroyed();
    }
  });

  rasterizer_service_.Act([](auto& impl) { impl.GetRasterizer()->Teardown(); });
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewScheduleFrame() {
  TRACE_EVENT("clay", "Shell::OnPlatformViewScheduleFrame");
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask([engine = engine_->GetWeakPtr()]() {
    if (engine) {
      engine->ScheduleFrame();
    }
  });
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewSetViewportMetrics(const ViewportMetrics& metrics) {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  if (metrics.device_pixel_ratio <= 0 || metrics.physical_width <= 0 ||
      metrics.physical_height <= 0) {
    // Ignore invalid view-port metrics.
    if (metrics.device_pixel_ratio > 0) {
      // NOTE(Xietong): although the physical size is still invalid, the device
      // pixel ratio can still be useful. Pass it to clay.
      fml::TaskRunner::RunNowOrPostTask(
          task_runners_.GetUITaskRunner(),
          [engine = engine_->GetWeakPtr(), metrics]() {
            if (engine) {
              engine->SetViewportMetrics(metrics);
            }
          });
    }
    return;
  }

  // This is the formula Android uses.
  // https://android.googlesource.com/platform/frameworks/base/+/39ae5bac216757bc201490f4c7b8c0f63006c6cd/libs/hwui/renderthread/CacheManager.cpp#45
  resource_cache_limit_ = metrics.physical_width * metrics.physical_height *
                          settings_.gpu_resource_cache_multiplier * 4;
  size_t resource_cache_max_bytes =
      resource_cache_limit_calculator_->GetResourceCacheMaxBytes();
  rasterizer_service_.Act([resource_cache_max_bytes](auto& impl) {
    impl.GetRasterizer()->SetResourceCacheMaxBytes(resource_cache_max_bytes,
                                                   false);
  });

  {
    std::scoped_lock<std::mutex> lock(*resize_mutex_);
    *expected_frame_size_ =
        skity::Vec2(metrics.physical_width, metrics.physical_height);
    device_pixel_ratio_ = metrics.device_pixel_ratio;
  }
  if (task_runners_.GetPlatformTaskRunner() ==
      task_runners_.GetUITaskRunner()) {
    // That mean the platform thread is the ui thread
    // and this method will called in platform thread.
    engine_->SetViewportMetrics(metrics);
  } else {
    task_runners_.GetUITaskRunner()->PostTask(
        [engine = engine_->GetWeakPtr(), metrics]() {
          if (engine) {
            engine->SetViewportMetrics(metrics);
          }
        });
  }
}

// |PlatformView::Delegate|
bool Shell::OnPlatformViewDispatchPointerDataPacket(
    std::unique_ptr<PointerDataPacket> packet) {
  TRACE_EVENT("clay", "Shell::OnPlatformViewDispatchPointerDataPacket");
  std::vector<clay::PointerEvent> events =
      GetEventsFromPointerDataPacket(packet.get());

  // On some platform(ex: harmony), this action is already in the UI Looper,
  // so we have no need to post task to next loop, just run immediatly.
  // (On harmony, post to next loop may lose context for TouchEvent when using
  // BuilderNode.postTouchEvent.)
  // In Android, we need to know whether the event is consumed by the engine.
  // i.e. whether we need to let other platform views consume it.
  if (task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread()) {
    auto engine = engine_->GetWeakPtr();
    return engine->DispatchPointerEvent(std::move(events));
  }
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      [engine = engine_->GetWeakPtr(), e = std::move(events)]() {
        if (engine) {
          engine->DispatchPointerEvent(std::move(e));
        }
      });
  return true;
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewDispatchKeyEvent(
    std::unique_ptr<clay::KeyEvent> key_event,
    std::function<void(bool /* handled */)> callback) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask(fml::MakeCopyable(
      [engine = engine_->GetWeakPtr(), event = std::move(key_event),
       callback = std::move(callback)]() mutable {
        if (engine) {
          engine->DispatchKeyEvent(std::move(event), std::move(callback));
        }
      }));
}

void Shell::OnPlatformViewSendKeyboardEvent(
    std::unique_ptr<clay::KeyEvent> key_event) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask(fml::MakeCopyable(
      [engine = engine_->GetWeakPtr(), event = std::move(key_event)]() mutable {
        if (engine) {
          engine->SendKeyboardEvent(std::move(event));
        }
      }));
}

void Shell::OnPlatformViewPerformEditorAction(
    clay::KeyboardAction action_code) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), action_code = action_code]() {
        if (engine) {
          engine->PerformEditorAction(action_code);
        }
      });
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewDeleteSurroundingText(int before_length,
                                                int after_length) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), before_length, after_length]() {
        if (engine) {
          engine->DeleteSurroundingText(before_length, after_length);
        }
      });
}

void Shell::OnPlatformViewOnEnterForeground() {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask([engine = engine_->GetWeakPtr()]() {
    if (engine) {
      engine->OnEnterForeground();
    }
  });
}

void Shell::OnPlatformViewOnEnterBackground() {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask([engine = engine_->GetWeakPtr()]() {
    if (engine) {
      engine->OnEnterBackground();
    }
  });
}

void Shell::OnPlatformViewSetDefaultFocusRingEnabled(bool enable) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), enable]() {
        if (engine) {
          engine->SetDefaultFocusRingEnabled(enable);
        }
      });
}

void Shell::OnPlatformViewSetPerformanceOverlayEnabled(bool enable) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      [engine = engine_->GetWeakPtr(), enable]() {
        if (engine) {
          engine->SetPerformanceOverlayEnabled(enable);
        }
      });
}

void Shell::OnDefaultOverflowVisibleChanged(bool visible) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      [engine = engine_->GetWeakPtr(), visible]() {
        if (engine) {
          engine->SetDefaultOverflowVisible(visible);
        }
      });
}

void Shell::OnExposurePropsChanged(int freq, bool enable_exposure_ui_margin) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      [engine = engine_->GetWeakPtr(), freq, enable_exposure_ui_margin]() {
        if (engine) {
          engine->SetExposureProps(freq, enable_exposure_ui_margin);
        }
      });
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewSetNextFrameCallback(const fml::closure& closure) {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  rasterizer_service_.Act([closure](auto& impl) {
    impl.GetRasterizer()->SetNextFrameCallback(closure);
  });
}

// |PlatformView::Delegate|
const Settings& Shell::OnPlatformViewGetSettings() const { return settings_; }

void Shell::ReportTimings() {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetRasterTaskRunner()->RunsTasksOnCurrentThread());
}

void Shell::UpdateRootSize(int32_t width, int32_t height) {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  fml::TaskRunner::RunNowOrPostTask(
      GetTaskRunners().GetPlatformTaskRunner(),
      [weak = weak_platform_view_, width, height]() {
        if (weak) {
          weak->UpdateRootSize(width, height);
        }
      });
}

void Shell::RegisterDrawableImage(
    std::shared_ptr<DrawableImage> drawable_image) {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  drawable_image->SetFrameAvailableCallback(
      [ui_runner = task_runners_.GetUITaskRunner(),
       raster_runner = task_runners_.GetRasterTaskRunner(),
       engine = engine_->GetWeakPtr(),
       service_manager = engine_->GetServiceManager(),
       image_id = drawable_image->Id(),
       weak_drawable_image =
           std::weak_ptr<clay::DrawableImage>(drawable_image)] {
        // Tell the rasterizer that one of its textures has a new frame
        // available.
        raster_runner->PostTask([weak_drawable_image, service_manager]() {
          if (auto image = weak_drawable_image.lock()) {
            image->MarkNewFrameAvailable();
            // Request a new frame from Rasterizer directly.
            clay::Puppet<clay::Owner::kRaster, RasterFrameService>
                raster_frame_service =
                    service_manager->GetService<RasterFrameService>();
            raster_frame_service.Act(
                [](auto& impl) { impl.RequestRasterFrame(); });
          }
        });

        fml::TaskRunner::RunNowOrPostTask(ui_runner, [engine, image_id] {
          if (engine) {
            engine->MarkDrawableImageFrameAvailable(image_id);
          }
        });
      });

  rasterizer_service_.Act([drawable_image](auto& impl) {
    impl.GetRasterizer()->GetDrawableImageRegistry()->RegisterDrawableImage(
        drawable_image);
  });
}

void Shell::UnregisterDrawableImage(int64_t image_id) {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  rasterizer_service_.Act([image_id](auto& impl) {
    impl.GetRasterizer()->GetDrawableImageRegistry()->UnregisterDrawableImage(
        image_id);
  });
}

size_t Shell::UnreportedFramesCount() const {
  // Check that this is running on the raster thread to avoid race conditions.
  FML_DCHECK(task_runners_.GetRasterTaskRunner()->RunsTasksOnCurrentThread());
  FML_DCHECK(unreported_timings_.size() % (FrameTiming::kStatisticsCount) == 0);
  return unreported_timings_.size() / (FrameTiming::kStatisticsCount);
}

void Shell::OnDrawHardware(bool force_draw) {
  TRACE_EVENT("clay", "Shell::OnDrawHardware");
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  clay::Puppet<clay::Owner::kPlatform, SyncCompositorService>
      sync_compositor_service =
          GetServiceManager()->GetService<SyncCompositorService>();
  sync_compositor_service.Act(
      [force_draw](auto& impl) { impl.DemandDrawHw(force_draw); });
}

double Shell::GetMainDisplayRefreshRate() {
  return display_manager_->GetMainDisplayRefreshRate();
}

void Shell::ShowSoftInput(int type, int action) {
  fml::TaskRunner::RunNowOrPostTask(
      GetTaskRunners().GetPlatformTaskRunner(),
      [type, action, weak = weak_platform_view_]() {
        if (weak) {
          weak->ShowSoftInput(type, action);
        }
      });
}

void Shell::HideSoftInput() {
  fml::TaskRunner::RunNowOrPostTask(GetTaskRunners().GetPlatformTaskRunner(),
                                    [weak = weak_platform_view_]() {
                                      if (weak) {
                                        weak->HideSoftInput();
                                      }
                                    });
}

void Shell::FilterInputAsync(const std::string& input,
                             const std::string& pattern,
                             std::function<void(const std::string&)> callback) {
  // TODO(wangyanyi) maybe we can post this task to io runner,
  // this will improve performance.
  fml::TaskRunner::RunNowOrPostTask(GetTaskRunners().GetUITaskRunner(),
                                    [weak = weak_platform_view_, input, pattern,
                                     callback = std::move(callback)]() {
                                      std::string result = input;  // 默认值
                                      if (weak) {
                                        result =
                                            weak->InputFilter(input, pattern);
                                      }
                                      callback(result);
                                    });
}

std::string Shell::ShouldInterceptUrl(const std::string& origin_url,
                                      bool should_decode) {
  if (weak_platform_view_) {
    return weak_platform_view_->ShouldInterceptUrl(origin_url, should_decode);
  }
  return origin_url;
}

std::shared_ptr<clay::ResourceLoaderIntercept>
Shell::GetResourceLoaderIntercept() {
  if (weak_platform_view_) {
    return weak_platform_view_->GetResourceLoaderIntercept();
  }
  return nullptr;
}

ScreenshotData Shell::ScreenshotSync(
    ScreenshotData::ScreenshotType screenshot_type, uint32_t background_color) {
  TRACE_EVENT("clay", "Shell::ScreenshotSync");
  std::future<std::optional<ScreenshotData>> screenshot_future =
      rasterizer_service_.ActWithPromise(
          [screenshot_type, background_color](auto& impl) {
            return impl.GetRasterizer()->ScreenshotLastLayerTree(
                screenshot_type, false, background_color);
          });
  return screenshot_future.get().value_or(ScreenshotData());
}

void Shell::ScreenshotAsync(ScreenshotData::ScreenshotType screenshot_type,
                            uint32_t background_color,
                            std::function<void(ScreenshotData)> callback) {
  rasterizer_service_.Act(
      [screenshot = screenshot, screenshot_type, background_color](auto& impl) {
        if (screenshot->load()) {
          auto result = impl.GetRasterizer()->ScreenshotLastLayerTree(
              screenshot_type, false, background_color);
          screenshot->store(false);
          return std::make_optional<ScreenshotData>(result);
        } else {
          return std::make_optional<ScreenshotData>();
        }
      },
      [callback, screenshot = screenshot](
          std::optional<ScreenshotData> screenshot_result) {
        if (screenshot_result.has_value()) {
          callback(screenshot_result.value());
          screenshot->store(true);
        }
      });
}

void Shell::MakeRasterSnapshot(
    std::unique_ptr<clay::LayerTree> layer_tree,
    std::function<void(fml::RefPtr<clay::PaintImage>)> callback) {
  rasterizer_service_.Act(
      [layer_tree = std::move(layer_tree), callback,
       io_runner = GetTaskRunners().GetIOTaskRunner()](auto& impl) mutable {
        auto image =
            impl.GetRasterizer()->MakeRasterSnapshot(std::move(layer_tree));
        // Callback to IO thread for post-processing like jpeg/png compression,
        // do not block raster thread.
        io_runner->PostTask(
            [image = std::move(image), callback] { callback(image); });
      });
}

fml::RefPtr<PaintImage> Shell::MakeRasterSnapshot(GrPicturePtr picture,
                                                  skity::Vec2 picture_size) {
  TRACE_EVENT("clay", "Shell::MakeRasterSnapshot");
  std::future<std::optional<fml::RefPtr<clay::PaintImage>>> future =
      rasterizer_service_.ActWithPromise(
          [picture = std::move(picture), picture_size](auto& impl) {
            return impl.GetRasterizer()->MakeRasterSnapshot(std::move(picture),
                                                            picture_size);
          });
  return future.get().value_or(nullptr);
}

fml::Status Shell::WaitForFirstFrame(fml::TimeDelta timeout) {
  FML_DCHECK(is_setup_);
  if (!settings_.enable_sync_compositor &&
      (task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread() ||
       task_runners_.GetRasterTaskRunner()->RunsTasksOnCurrentThread())) {
    return fml::Status(fml::StatusCode::kFailedPrecondition,
                       "WaitForFirstFrame called from thread that can't wait "
                       "because it is responsible for generating the frame.");
  }

  // Check for overflow.
  auto now = std::chrono::steady_clock::now();
  auto max_duration = std::chrono::steady_clock::time_point::max() - now;
  auto desired_duration = std::chrono::milliseconds(timeout.ToMilliseconds());
  auto duration =
      now + (desired_duration > max_duration ? max_duration : desired_duration);

  std::unique_lock<std::mutex> lock(waiting_for_first_frame_mutex_);
  bool success = waiting_for_first_frame_condition_.wait_until(
      lock, duration, [&waiting_for_first_frame = waiting_for_first_frame_] {
        return !waiting_for_first_frame->load();
      });
  if (success) {
    return fml::Status();
  } else {
    return fml::Status(fml::StatusCode::kDeadlineExceeded, "timeout");
  }
}

bool Shell::ReloadSystemFonts() {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  return true;
}

std::shared_ptr<const fml::SyncSwitch> Shell::GetIsGpuDisabledSyncSwitch()
    const {
  return is_gpu_disabled_sync_switch_;
}

void Shell::SetGpuAvailability(GpuAvailability availability) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  switch (availability) {
    case GpuAvailability::kAvailable:
      is_gpu_disabled_sync_switch_->SetSwitch(false);
      return;
    case GpuAvailability::kFlushAndMakeUnavailable: {
    }
      // FALLTHROUGH
    case GpuAvailability::kUnavailable:
      is_gpu_disabled_sync_switch_->SetSwitch(true);
      return;
    default:
      FML_DCHECK(false);
  }
}

void Shell::OnDisplayUpdates(DisplayUpdateType update_type,
                             std::vector<std::unique_ptr<Display>> displays) {
  display_manager_->HandleDisplayUpdates(update_type, std::move(displays));
}

void Shell::OnPostInvalidate(bool is_raster_frame) {
  fml::TaskRunner::RunNowOrPostTask(
      GetTaskRunners().GetPlatformTaskRunner(),
      fml::MakeCopyable([weak = weak_platform_view_]() mutable {
        if (weak) {
          weak->PostInvalidate();
        }
      }));
}

const std::weak_ptr<VsyncWaiter> Shell::GetVsyncWaiter() const {
  return engine_->GetVsyncWaiter();
}

void Shell::DumpInfoToDevtoolEnabled(bool enabled) {
  devtools_instrumentation_enabled_ = enabled;
}

void Shell::UpdateRasterInfo(
    const std::vector<RasterCacheInfo>& raster_cache_info) {
  if (devtools_instrumentation_enabled_ && devtool_instrumentation_) {
    devtool_instrumentation_->ResetRasterDocument();
    for (auto info : raster_cache_info) {
      devtool_instrumentation_->UpdateRasterCacheInfo(
          info.single_cache_size, info.single_cache_height,
          info.single_cache_width, info.single_cache_color_type,
          info.layer_address, info.cache_address, info.image);
    }
    devtool_instrumentation_->UpdateRasterInfoToDevtool();
  }
}

void Shell::ReportTiming(const std::unordered_map<std::string, int64_t>& timing,
                         const std::string& flag) {
  // Make sure on Platform Thread.
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  if (weak_platform_view_) {
    if (flag == kSetupFlag) {
      weak_platform_view_->OnTimingSetup(timing);
    } else if (flag == kUpdateFlag || flag == kForceUpdateFlag) {
      weak_platform_view_->OnTimingUpdate(timing, flag);
    }
  }
}

#ifdef ENABLE_ACCESSIBILITY
void Shell::UpdateSemantics(const clay::SemanticsUpdateNodes& update_nodes) {
  FML_DCHECK(task_runners_.GetPlatformTaskRunner());
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetPlatformTaskRunner(), [this, update_nodes]() {
        if (platform_view_) {
          platform_view_->UpdateSemantics(update_nodes);
        }
      });
}

void Shell::SetSemanticsEnabled(bool enabled) {
  if (engine_) {
    engine_->SetSemanticsEnabled(enabled);
  }
}

void Shell::SetPageEnableAccessibilityElement(bool enabled) {
  if (engine_) {
    engine_->SetPageEnableAccessibilityElement(enabled);
  }
}

void Shell::OnPlatformViewDispatchSemanticsAction(int virtual_view_id,
                                                  int action) {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), virtual_view_id, action]() {
        if (engine) {
          engine->DispatchSemanticsAction(virtual_view_id, action);
        }
      });
}
#endif

void Shell::CleanForRecycle() {
  if (engine_) {
    engine_->CleanForRecycle();
  }

  rasterizer_service_.Act(
      [](auto& impl) mutable { impl.GetRasterizer()->CleanForRecycle(); });

  clay::Puppet<clay::Owner::kPlatform, RasterFrameService>
      raster_frame_service =
          GetServiceManager()->GetService<RasterFrameService>();
  raster_frame_service.Act([](auto& impl) { impl.CleanForRecycle(); });

  waiting_for_first_frame_->store(true);
  devtools_instrumentation_enabled_ = false;
  devtool_instrumentation_.reset();
}

void Shell::PrepareForRecycle() {
  if (engine_) {
    engine_->PrepareForRecycle();
  }

  frame_timing_collector_ = std::make_shared<clay::FrameTimingCollector>(
      task_runners_.GetPlatformTaskRunner());
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      [this, engine = weak_engine_,
       ui_task_runner = task_runners_.GetUITaskRunner(),
       raster_task_runner = task_runners_.GetRasterTaskRunner()] {
        if (engine) {
          engine->SetFrameTimingCollector(frame_timing_collector_);
          frame_timing_collector_->SetPageView(engine->GetPageView());
        }
      });

  clay::Puppet<clay::Owner::kPlatform, RasterFrameService>
      raster_frame_service =
          GetServiceManager()->GetService<RasterFrameService>();
  raster_frame_service.Act([](auto& impl) { impl.PrepareForRecycle(); });
}

}  // namespace clay
