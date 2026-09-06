# ConcurrentMessageLoop Backend (Harmony / FFRT) — Device Test Plan

> Manual integration test plan for **Task 13** of `concurrent_loop_backend_plan.md`.
>
> This plan requires a **real HarmonyOS device** (or DevEco Studio Emulator
> running an image with API ≥ 20) plus DevEco Studio. The CI/sandbox subagent
> cannot run it. The sections below describe what the user should execute on
> their workstation + device.

---

## 0. Pre-conditions

- [ ] **Hardware / OS**
  - HarmonyOS device with **API ≥ 20** enabled.
    - Rationale: `ffrt/ffrt.h` (from `@ppd/ffrt@1.1.9`) uses
      `#if OH_CURRENT_API_VERSION >= 20` for the C-side `ffrt/fiber.h`. The
      Lynx backend does not include `fiber.h`, but the spec states API ≥ 20
      as the baseline required for `thread_mode(true)` semantics.
    - The `thread_mode(true)` flag is supported by `@ppd/ffrt@1.1.9` (added
      in CHANGELOG entry "v1.1.1: 队列任务支持以线程模式运行").
    - If your device is API < 20, the BackendFFRT code will still compile
      (the API 20 gate only affects `fiber.h`), but `thread_mode(true)` is
      not formally API-gated in the ohpm headers, so behavior may differ.
- [ ] **DevEco Studio**
  - Install DevEco Studio (latest stable that supports API 20 device images).
  - Make sure the HarmonyOS SDK is installed and the API 20+ platform is
    selected in `File → Settings → SDK`.
- [ ] **Source tree**
  - This repo checked out at branch `develop` (already has T0–T12 changes).
- [ ] **ohpm install** — populate the two install roots (run from the repo
      root; `base/platform/harmony` is a module, not an install root — see
      spec §12.2):

  ```bash
  (cd explorer/harmony && ohpm install)   # hvigor project root — covers @ppd/ffrt, imageknife, all lynx modules
  (cd platform/harmony   && ohpm install) # pulls @lynx/primjs (harmony.gni needs it)
  ls base/platform/harmony/oh_modules/@ppd/ffrt        # @ppd/ffrt symlink (auto-created by explorer install) should exist
  readlink base/platform/harmony/oh_modules/@ppd/ffrt  # should resolve to
    # ../../../../../explorer/harmony/oh_modules/.ohpm/@ppd+ffrt@1.1.9/oh_modules/@ppd/ffrt
  ```

- [ ] **HiTrace / HiView** — install HiTrace (or use the `hitrace` shell
  command bundled in the SDK) on the host. On-device DevEco "Profiler" or
  "App Analyzer" can also visualize threads.

---

## 1. Build the demo HAP

1. Open DevEco Studio → **Open Project** → select `explorer/harmony/`.
2. Confirm the project SDK is API 20+ (check `build-profile.json5` /
   `lynx_explorer/build-profile.json5` → `compatibleSdkVersion` and
   `compileSdkVersion`).
3. **Build → Build Hap(s) / APP(s) → Build Hap(s)** (or run on device/emulator).
4. **Expected**: build succeeds. If `libffrt.z.so` is reported missing, the
   HarmonyOS SDK is missing the FunctionFlowRuntime subsystem — re-install
   the matching SDK platform.

> What this confirms: `base/src/BUILD.gn` line 270 lists
> `fml/platform/harmony/concurrent_loop_backend_ffrt.cc`; `harmony.gni` exposes
> `ffrt_include_dir`; the final `liblynxbase.so` links `-lffrt.z`. (CI verified
> the first two; the third is verified here.)

---

## 2. Run the demo and exercise both task pools

1. Run the HAP on the device/emulator. The demo loads a Lynx page that
   exercises both task pools:
   - `TaskRunnerManufactor::GetConcurrentTaskRunner("LynxHighTask")` →
     `ThreadPriority::HIGH` → `BackendFFRT` → `qos_user_initiated`.
   - `TaskRunnerManufactor::GetConcurrentTaskRunner("LynxNormalTask")` →
     `ThreadPriority::NORMAL` → `BackendFFRT` → `qos_default`.
2. Trigger the demo's image-decode / first-paint path so both queues get
   real work.

---

## 3. Verification checklist (contracts C1–C5 + QoS)

### 3.1 C1 — PostTask executes on a worker

- [ ] In **hiTrace** (filter `tag=ConcurrentMessageLoop` or instrumented
      logcat), confirm the posted task body runs on a thread **other than**
      the caller's thread.
- [ ] `RunsTasksOnCurrentThreadWorker()` returns `true` from inside the task
      (see C3 below for instrumentation). This proves the task reached a
      worker.

### 3.2 C2 — Each FFRT task runs on its own OS thread (`thread_mode(true)`)

- [ ] `adb shell ps -T -p <pid>` (or `/proc/<pid>/task`) while the demo is
      running. You should see the thread count rise as tasks are posted
      (each task gets its own OS thread, then exits). With
      `thread_mode(true)`, the FFRT queue does not multiplex tasks onto a
      fixed worker pool — it spawns a fresh thread per task.
- [ ] Sanity counter-check: temporarily flip
      `concurrent_loop_backend_ffrt.cc:50` `.thread_mode(true)` → `false`,
      rebuild, re-run. The thread count should plateau at ~`worker_count_`
      instead of spiking. Revert before continuing.

### 3.3 C3 — `RunsTasksOnCurrentThreadWorker()` inside the task

- [ ] Add a one-shot debug log in the demo (or a small test entry) that
      calls `RunsTasksOnCurrentThreadWorker()` from inside a posted task
      and prints the result + the current thread id (e.g.
      `std::this_thread::get_id()`).
- [ ] Expected: `true` and a thread id that is NOT the caller's.
- [ ] Cross-check with the implementation:
      ```cpp
      // base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.cc:53
      void ConcurrentLoopBackendFFRT::PostTask(base::closure task) {
        auto wrapped = [this, t = std::move(task)]() mutable {
          g_current_worker = this;   // sentinel set per task
          t();
          g_current_worker = nullptr;
        };
        queue_->submit(std::move(wrapped));
      }
      ```
      The `thread_mode(true)` is what guarantees `g_current_worker` is
      observable from inside `t()` (it would be lost on a co-routine).

### 3.4 QoS differentiation (the reason for picking BackendFFRT)

- [ ] **Inspect nice / priority** of the OS threads spawned by each pool.
      While the demo runs:
      ```bash
      adb shell ps -eLo pid,tid,pri,ni,cmd | grep <process-name>
      ```
      Threads serving `LynxHighTask` should report a higher `pri` / lower
      `ni` (lower nice value = higher priority) than threads serving
      `LynxNormalTask`. **Caveat**: the OS does not always honor
      `nice`-style renice on app-spawned threads — prefer DevEco's
      **App Analyzer → Thread** view which shows QoS class.
- [ ] **Visual check in DevEco Studio Profiler**: open the CPU profiler,
      record a 5–10s window during the demo. The threads for `LynxHighTask`
      should show **higher QoS class** (UI / user-interactive) and
      `LynxNormalTask` should show **default**.
- [ ] **Functional stress test (Step 13.3 in the plan)**: in the demo,
      rapidly switch tabs / fire image-decode storms. Confirm:
      - UI frame rate stays smooth (HIGH tasks win scheduling).
      - NORMAL tasks (image decode) get temporarily preempted but make
        progress after the burst.
      - **No NORMAL task starves**: over a 30s window, NORMAL threads
        still get CPU share.

### 3.5 C4 / C5 — facade passthroughs

- [ ] `GetWorkerCount()` returns the configured value
      (typically 4 or 8). Confirms C5 (facade forwards).
- [ ] **C2 shutdown fallback** (optional, only if you wired a test
      entry; the contract test in
      `base/src/fml/concurrent_message_loop_backend_test.cc` already
      covers this on non-Harmony via `BackendStd`):
      ```cpp
      loop->Terminate();
      loop->PostTask([] { /* … */ });  // must run synchronously on caller
      ```
      On Harmony, `BackendFFRT::Terminate()` calls `queue_.reset()`, which
      invokes `ffrt_queue_destroy` and joins in-flight tasks; subsequent
      `PostTask` should then run synchronously via the facade's
      `shutdown_` fallback path.

---

## 4. Logging instrumentation (one-shot, recommended)

If the demo does not already log enough, add temporary trace points in
`concurrent_loop_backend_ffrt.cc`:

```cpp
void ConcurrentLoopBackendFFRT::PostTask(base::closure task) {
  auto wrapped = [this, t = std::move(task), name = name_prefix_]() mutable {
    LOGI("FFRT task start name=%s tid=%zu qos=%d",
         name.c_str(), gettid(), (int)MapQos(priority_));
    g_current_worker = this;
    t();
    g_current_worker = nullptr;
    LOGI("FFRT task end   name=%s tid=%zu",
         name.c_str(), gettid());
  };
  queue_->submit(std::move(wrapped));
}
```

Then `hilog | grep FFRT` will show per-task thread ids and QoS values.
Remove before commit.

---

## 5. Pass / fail criteria

- [ ] Demo HAP builds and runs without `libffrt.z.so` link errors.
- [ ] C1: Posted tasks execute off the caller's thread.
- [ ] C2: Each task spawns its own OS thread (`thread_mode(true)`) — verify
      via `ps -T`.
- [ ] C3: `RunsTasksOnCurrentThreadWorker()` returns `true` inside the task.
- [ ] QoS: `LynxHighTask` threads show higher QoS than `LynxNormalTask`
      threads; under load HIGH preempts or shares more fairly with NORMAL.
- [ ] No regressions: first-paint is no slower than the prior `BackendStd`
      baseline (perceptual check); demo is responsive under tab-switch
      stress.

---

## 6. Reporting back

Please attach:

1. **hilog / hiTrace output** (C1, C3, QoS).
2. **`ps -T` snapshot** during steady state (C2).
3. **DevEco Profiler screenshot** of the two pools under stress (C4 / QoS).
4. Any unexpected warnings from `libffrt.z.so`.

If any item fails, capture the failing log line and the API level of the
device (`getprop ro.build.version.sdk` or `hdc shell getprop
ro.build.version.sdk`).
