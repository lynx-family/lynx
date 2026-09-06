# ConcurrentMessageLoop Backend Abstraction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把 `ConcurrentMessageLoop` 重构为 facade 持有 `unique_ptr<ConcurrentLoopBackend>`，按平台宏选 backend（Harmony → BackendFFRT，iOS/macOS → BackendGCD 桩，其余 → BackendStd 平移）。所有阶段上游 API 零变化，`ConcurrentTaskRunner` / `TaskRunnerManufactor::PostTaskToConcurrentLoop` 等等均不动。

**Architecture:**

- 抽象 `ConcurrentLoopBackend`（4 个虚函数：PostTask / RunsTasksOnCurrentThreadWorker / GetWorkerCount / Terminate）。
- `ConcurrentMessageLoop` 退化为薄 facade：持 `unique_ptr<Backend>`，所有操作转发；facade 唯一"业务"是 shutdown 期同步执行兜底（C2）。
- `ConcurrentLoopBackend` 编译期工厂：`#if defined(OS_HARMONY)` → BackendFFRT，`#elif defined(OS_IOS)||defined(OS_OSX)` → BackendGCD（本轮桩），其余 → BackendStd。
- 进程级 `g_current_worker`（thread_local）哨兵由各 backend 自管；FFRT 用任务级设/清（依赖 `thread_mode(true)`）。

**Tech Stack:** C++17、GN 构建、gtest（已有 `//third_party/googletest:gtest`）。Phase 2 鸿蒙 FFRT 依赖（`@ppd/ffrt` 1.1.8 + 系统 `libffrt.z.so` + `base/platform/harmony/harmony.gni` 含 `ffrt_include_dir`）**由 Phase 0 Task 0 引入**，Phase 1 不依赖这些。

---

## 文件结构

### 新增

```txt
base/include/fml/
└── concurrent_message_loop_backend.h         # ConcurrentLoopBackend 抽象 + 工厂声明

base/src/fml/
├── concurrent_message_loop_backend.cc         # 工厂分发（#if by platform）
├── concurrent_loop_backend_std.cc             # BackendStd：从现 concurrent_message_loop.cc 平移
├── concurrent_loop_backend_std.h
├── concurrent_loop_backend_ffrt.cc            # BackendFFRT（Harmony）
├── concurrent_loop_backend_ffrt.h
└── concurrent_message_loop_backend_test.cc     # 契约测试（C1-C5）
```

### 修改

```txt
base/include/fml/
└── concurrent_message_loop.h                  # 仍暴露原公共 API，但实现在 .cc 中改为转发给 backend

base/src/fml/
└── concurrent_message_loop.cc                  # 改为薄 facade（持 backend_、shutdown_、Terminate/PostTask 转发）
└── BUILD.gn                                    # 新增 source；harmony fml 段：import harmony.gni + include_dirs + libs
```

### 不动

```txt
base/include/fml/concurrent_task_runner.h
base/src/fml/concurrent_task_runner.cc
core/base/threading/task_runner_manufactor.cc        # 上游零修改
core/base/threading/task_runner_manufactor.h
base/platform/harmony/oh-package.json5                # Phase 0 Task 0 加 @ppd/ffrt 1.1.8
base/platform/harmony/harmony.gni                    # Phase 0 Task 0 创建（ffrt_include_dir）
```

---

## Phase 0：FFRT 三方库接入（前置，所有后续任务的前置）

### Task 0：引入 `@ppd/ffrt` 三方库

**Files:**

- Modify: `base/platform/harmony/oh-package.json5`
- Create: `base/platform/harmony/harmony.gni`（若不存在）
- （本地构建前置，不在 Git）ohpm install 在两处根目录各跑一次

**Why first:** Phase 2 的 BackendFFRT 实现（Task 8/9）和鸿蒙 BUILD.gn 接线（Task 11）依赖 `@ppd/ffrt` 头文件能解析、`ffrt_include_dir` 能被 GN 找到。把这一步独立成 T0，使 Phase 1 完成后 Phase 2 立刻可推进，无需中途回头补依赖。

- [ ] **Step 0.1**：在 `base/platform/harmony/oh-package.json5` 的 `devDependencies` 加 `@ppd/ffrt: "1.1.8"`（与 spec §12.1 一致）：

  ```json5
  "devDependencies": {
    "@ppd/ffrt": "1.1.8"
  }
  ```

  验证：`grep "@ppd/ffrt" base/platform/harmony/oh-package.json5` 应输出该行。

- [ ] **Step 0.2**：创建（若不存在）`base/platform/harmony/harmony.gni`：

  ```gn
  # Copyright 2025 The Lynx Authors. All rights reserved.
  # Licensed under the Apache License Version 2.0 that can be found in the
  # LICENSE file in the root directory of this source tree.

  # FFRT C++ 接口为纯头文件（cpp/*.h 全是 inline 包装）。
  # @ppd/ffrt 在 base/platform/harmony/oh-package.json5 的 devDependencies 中声明，
  # 借助 ohpm 依赖传导把头文件拉进 oh_modules；运行时系统 libffrt.z.so 提供。
  #
  # 用法（base/src/BUILD.gn 的 is_harmony 段）：
  #   import("../platform/harmony/harmony.gni")
  #   include_dirs += [ ffrt_include_dir ]   # #include "ffrt/ffrt.h"
  #   libs += [ "ffrt.z" ]                   # 链系统核心 libffrt.z.so
  ffrt_include_dir = rebase_path("oh_modules/@ppd/ffrt/include")
  ```

  验证：`cat base/platform/harmony/harmony.gni` 输出含 `ffrt_include_dir = rebase_path(...)`。

- [ ] **Step 0.3**：本地构建前置（不在 Git 里提交）—— 在两处根目录各跑 `ohpm install`，让 ohpm 创建 explorer store + symlink（详见 spec §12.2）：

  ```bash
  cd /path/to/explorer/harmony
  ohpm install
  cd /path/to/platform/harmony
  ohpm install
  ```

- [ ] **Step 0.4**：验证软链接：

  ```bash
  test -L base/platform/harmony/oh_modules/@ppd/ffrt && echo "是软链接 OK"
  ```

  预期输出：`是软链接 OK`（指向 `explorer/harmony/oh_modules/.ohpm/@ppd+ffrt@1.1.8/oh_modules/@ppd/ffrt`）。

- [ ] **Step 0.5**：commit：

  ```bash
  cd /Users/cct/Workspace/OpenSource/lynx
  git add base/platform/harmony/oh-package.json5 base/platform/harmony/harmony.gni
  git commit -m "[FML][Harmony] introduce @ppd/ffrt 1.1.8 devDep + harmony.gni ffrt_include_dir" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
  ```

  预期输出：commit 落地，repo 内 `@ppd/ffrt` 头文件可通过 `ffrt_include_dir` 解析。

- [ ] **Step 0.6**：把 `ffrt_include_dir` 与 `libffrt.z.so` 真正接进 `base/src/BUILD.gn` 的 `is_harmony` 段。这一步是 T8/T9 写出 ffrt 代码能否单任务编译通过的前置——T11 来做就晚了。具体两处编辑：

  - 在 `is_harmony` 段**附近**的 import 区（视 BUILD.gn 结构，可能在文件顶部或 `lynx_base_source_set` 模板前），加 `import("../platform/harmony/harmony.gni")`（条件 `#if is_harmony` 包）。
  - 在 `lynx_base_source_set` 模板的 `if (is_harmony) { ... }` 分支内，加 `include_dirs += [ ffrt_include_dir ]` 和 `libs += [ "ffrt.z" ]`。

  > 只动 BUILD.gn 的 import/include_dirs/libs，**不**把 `concurrent_loop_backend_ffrt.cc` 加进 sources（那个文件还没创建，那是 Task 11 的事）。

- [ ] **Step 0.7**：commit：

  ```bash
  cd /Users/cct/Workspace/OpenSource/lynx
  git add base/src/BUILD.gn
  git commit -m "[FML][Harmony] wire ffrt include path + libffrt.z into base BUILD.gn" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
  ```

  预期输出：commit 落地后，Harmony 配置下 `ninja -C out/harmony lynx_base` 能编出（即使 `concurrent_loop_backend_ffrt.cc` 还没存在——include 路径与 link 已就绪）。

> **Task 0 完整范围**：「引入 + 接线」全包——devDep 声明、gni 文件、ohpm install 取包、且把 BUILD.gn 的 ffrt include/lib 也接上（Step 0.6）。**任务 0 完成后，Task 8（BackendFFRT.h）与 Task 9（BackendFFRT.cc）写出来的代码无需再改 BUILD.gn 就能直接编译**。Task 11 之后只剩「把 backend.cc 加进 sources 列表」，不涉及 include/links。

---

## Phase 1：抽象 + 平移（其他平台零行为变化）

### Task 1：读懂现状（前置）

**Files:**

- Read: `base/include/fml/concurrent_message_loop.h`
- Read: `base/src/fml/concurrent_message_loop.cc`

- [ ] **Step 1.1**：完整读 `concurrent_message_loop.h`，列出 `ConcurrentMessageLoop` 的所有 public 方法 + 私有成员（workers_/tasks_/tasks_mutex_/task_count_/worker_count_/notify_mutex_/notify_condition_/shutdown_/WorkerMain）及其用途。

- [ ] **Step 1.2**：完整读 `concurrent_message_loop.cc`，理解 `WorkerMain` 的 CAS 抢占 + 错峰唤醒 + 自适应休眠的整体控制流。

- [ ] **Step 1.3**：在仓库根跑 `git grep "concurrent_message_loop"` 列出所有引用点（主要是 `task_runner_manufactor.cc`、`concurrent_task_runner.cc` 周边、测试）。列出待保持兼容的外部 API。

预期输出：`concurrent_message_loop.h` 的 public 接口清单 4-6 个方法；外部引用点 2-3 处。

---

### Task 2：抽象基类头文件

**Files:**

- Create: `base/include/fml/concurrent_message_loop_backend.h`

- [ ] **Step 2.1**：创建 `concurrent_message_loop_backend.h`：

```cpp
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_BACKEND_H_
#define BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_BACKEND_H_

#include <memory>
#include <string>

#include "base/include/base_export.h"
#include "base/include/closure.h"

namespace lynx {
namespace fml {

class ConcurrentLoopBackend {
 public:
  virtual ~ConcurrentLoopBackend() = default;

  // 在后端执行器上异步执行 task。task 必非空，由 facade 保证 shutdown 后不再调用。
  virtual void PostTask(base::closure task) = 0;

  // 当前线程是否正运行本后端的一个任务。
  virtual bool RunsTasksOnCurrentThreadWorker() const = 0;

  virtual size_t GetWorkerCount() const = 0;

  // 停止接收新任务，等待已在执行的任务结束后回收资源。
  virtual void Terminate() = 0;
};

// 编译期工厂：按平台宏选后端。priority/worker_count 由 facade 构造时传入。
BASE_EXPORT std::unique_ptr<ConcurrentLoopBackend> CreateConcurrentLoopBackend(
    const std::string& name_prefix,
    Thread::ThreadPriority priority,
    size_t worker_count);

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_BACKEND_H_
```

> 注：`Thread::ThreadPriority` 来自 `base/include/fml/thread.h`；如果头文件包含路径不同，按 lynx base 现有 include 风格调整（grep 一下其他 .h 看怎么 include 的）。

- [ ] **Step 2.2**：在仓库根跑 `git grep "class Thread"` 一行，确认 `ThreadPriority` 的拼写与命名空间；如果 .cc 中用法是 `fml::Thread::ThreadPriority`，确保头文件里 namespace 也对得上。

- [ ] **Step 2.3**：暂不改 BUILD.gn（这时还没有 .cc 使用，编译不会因此头改变）。运行 `cd /Users/cct/Workspace/OpenSource/lynx && gn gen out/Default --args='is_harmony=false is_android=true'`（如果已 gen 过则跳过）并 `ninja -C out/Default lynx_base` 确认无误（应通过，因为新头没人引用）。

预期输出：build 成功。

- [ ] **Step 2.4**：commit：

```bash
cd /Users/cct/Workspace/OpenSource/lynx
git add base/include/fml/concurrent_message_loop_backend.h
git commit -m "[Refactor][FML] introduce ConcurrentLoopBackend abstract base header" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 3：契约测试（先写测试锁死行为合同）

**Files:**

- Create: `base/src/fml/concurrent_message_loop_backend_test.cc`

- [ ] **Step 3.1**：读 `base/src/fml/BUILD.gn`，找一个现有的 gtest 目标（比如消息循环或 platform 相关的测试），照搬其 `deps`/`sources`/`include_dirs` 设置，给新测试目标同样的依赖。

- [ ] **Step 3.2**：在 `base/src/BUILD.gn` 添加：

```gn
source_set("concurrent_message_loop_backend_test") {
  testonly = true
  sources = [
    "concurrent_message_loop_backend_test.cc",
  ]
  deps = [
    ":base",
    "//third_party/googletest:gtest_main",
  ]
  if (is_harmony) {
    sources += [ "platform/harmony/napi_util.cc" ]
  }
}
```

并在已有的测试套件（比如 `lynx_base_test` 或顶层 `//testing:test`）里把这个新 target 加进 `deps`，确保 CI 跑得到。

- [ ] **Step 3.3**：写测试用例——用 fake backend 验证 facade 的 C1-C5 合同：

```cpp
// concurrent_message_loop_backend_test.cc
#include "base/include/fml/concurrent_message_loop_backend.h"
#include "base/include/fml/concurrent_message_loop.h"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace lynx {
namespace fml {
namespace {

// 测试用 fake backend：把每个 PostTask 推到自己的 worker 线程跑。
class FakeBackend : public ConcurrentLoopBackend {
 public:
  explicit FakeBackend(size_t worker_count) : worker_count_(worker_count) {}

  void PostTask(base::closure task) override {
    threads_.emplace_back([this, t = std::move(task)]() {
      g_current = this;
      t();
      g_current = nullptr;
    });
  }
  bool RunsTasksOnCurrentThreadWorker() const override { return g_current == this; }
  size_t GetWorkerCount() const override { return worker_count_; }
  void Terminate() override {
    for (auto& t : threads_) if (t.joinable()) t.join();
    threads_.clear();
  }

  static thread_local ConcurrentLoopBackend* g_current;

 private:
  size_t worker_count_;
  std::vector<std::thread> threads_;
};
thread_local ConcurrentLoopBackend* FakeBackend::g_current = nullptr;

TEST(ConcurrentLoopBackendTest, PostTaskExecutesTaskExactlyOnce) {
  auto backend = std::make_unique<FakeBackend>(2);
  std::atomic<int> count{0};
  backend->PostTask([&] { count.fetch_add(1); });
  backend->Terminate();
  EXPECT_EQ(count.load(), 1);
}

TEST(ConcurrentLoopBackendTest, RunsTasksOnCurrentThreadWorkerInsideTask) {
  auto backend = std::make_unique<FakeBackend>(1);
  std::atomic<bool> inside{false};
  std::atomic<bool> flag_inside{false};
  backend->PostTask([&] {
    inside.store(backend->RunsTasksOnCurrentThreadWorker());
    flag_inside.store(FakeBackend::g_current == backend.get());
  });
  backend->Terminate();
  EXPECT_TRUE(inside.load());
  EXPECT_TRUE(flag_inside.load());
  // 主线程不在该 backend 任务上下文
  EXPECT_FALSE(backend->RunsTasksOnCurrentThreadWorker());
}

}  // namespace
}  // namespace fml
}  // namespace lynx
```

- [ ] **Step 3.4**：仅跑这一个测试目标，**预期先实现后通过**（先验证测试本身能编译/运行）。等任务 5（facade）落地后再扩展到真正用 facade 的合同（C2 shutdown fallback 等）。

- [ ] **Step 3.5**：commit：

```bash
git add base/src/fml/concurrent_message_loop_backend_test.cc base/src/BUILD.gn
git commit -m "[Test][FML] add concurrent loop backend contract test scaffold" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 4：BackendStd 平移（逐字搬家，行为零变化）

**Files:**

- Create: `base/src/fml/concurrent_loop_backend_std.h`
- Create: `base/src/fml/concurrent_loop_backend_std.cc`
- Modify: `base/src/fml/concurrent_message_loop.cc` → 暂时清空（仅留 facade 占位，下一任务再实现）

- [ ] **Step 4.1**：创建 `concurrent_loop_backend_std.h`：

```cpp
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_SRC_FML_CONCURRENT_LOOP_BACKEND_STD_H_
#define BASE_SRC_FML_CONCURRENT_LOOP_BACKEND_STD_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "base/include/fml/concurrent_message_loop_backend.h"

namespace lynx {
namespace fml {

// 把现 base/src/fml/concurrent_message_loop.cc 的手写 std::thread 池
// 实现整体平移，实现 ConcurrentLoopBackend 接口。行为 1:1 不变。
class ConcurrentLoopBackendStd final : public ConcurrentLoopBackend {
 public:
  ConcurrentLoopBackendStd(const std::string& name_prefix,
                           Thread::ThreadPriority priority,
                           size_t worker_count);
  ~ConcurrentLoopBackendStd() override;

  void PostTask(base::closure task) override;
  bool RunsTasksOnCurrentThreadWorker() const override;
  size_t GetWorkerCount() const override { return worker_count_; }
  void Terminate() override;

 private:
  void WorkerMain(uint32_t index);

  const std::string name_prefix_;
  const size_t worker_count_;
  std::vector<std::thread> workers_;
  std::atomic<uint32_t> worker_count_atomic_{0};
  std::mutex tasks_mutex_;
  std::queue<base::closure> tasks_;
  std::atomic<uint32_t> task_count_{0};
  std::atomic_bool shutdown_{false};
  std::mutex notify_mutex_;
  std::condition_variable notify_condition_;

  static thread_local ConcurrentLoopBackendStd* g_current_worker;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_SRC_FML_CONCURRENT_LOOP_BACKEND_STD_H_
```

- [ ] **Step 4.2**：从 `base/src/fml/concurrent_message_loop.cc` 复制现有实现（`ConcurrentMessageLoop` 构造、`PostTask`、`RunsTasksOnCurrentThreadWorker`、`GetWorkerCount`、`Terminate`、`WorkerMain`、iOS autoreleasepool 包裹、`PlatformThreadPriority::Setter` 调用、`SetCurrentThreadName` 调用、staggered sleep 常数、CAS 抢占循环）到 `concurrent_loop_backend_std.cc`：

```cpp
// concurrent_loop_backend_std.cc
#include "base/src/fml/concurrent_loop_backend_std.h"

#include <unistd.h>   // iOS autoreleasepool 需要

#include "base/include/fml/concurrent_message_loop_backend.h"
#include "base/include/fml/thread.h"
#if defined(OS_IOS)
#include <objc/objc.h>   // objc_autoreleasePoolPush/Pop
extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void*);
#endif

namespace lynx {
namespace fml {

thread_local ConcurrentLoopBackendStd* ConcurrentLoopBackendStd::g_current_worker = nullptr;

namespace {
constexpr uint32_t kWorkerSleepMultipleMicroseconds = 340;
constexpr uint32_t kWorkerMaxIdleMicroseconds = 34000;
}  // namespace

ConcurrentLoopBackendStd::ConcurrentLoopBackendStd(
    const std::string& name_prefix, Thread::ThreadPriority priority,
    size_t worker_count)
    : name_prefix_(name_prefix), worker_count_(worker_count) {
  worker_count_atomic_.store(static_cast<uint32_t>(worker_count));
  workers_.reserve(worker_count);
  for (uint32_t i = 0; i < worker_count; ++i) {
    base::closure setup_thread = [name_prefix, i, priority, this]() {
      const auto config = fml::Thread::ThreadConfig(
          std::string{name_prefix + std::to_string(i + 1)}, priority);
      Thread::SetCurrentThreadName(config);
      WorkerMain(i);
    };
    workers_.emplace_back(std::move(setup_thread));
  }
}

ConcurrentLoopBackendStd::~ConcurrentLoopBackendStd() {
  Terminate();
}

// PostTask / RunsTasksOnCurrentThreadWorker / Terminate / WorkerMain：
// 直接从 base/src/fml/concurrent_message_loop.cc 平移，把其中引用
// ConcurrentMessageLoop::* 成员的地方改成本类成员引用即可。
// PostTask 内:
//   if (shutdown_) task() 直接同步跑（与 facade 的 shutdown 兜底配合；此处先实现 stage-0）
// WorkerMain 内 g_current_message_loop_worker = this 改为 g_current_worker = this
//
// （保持与原文件 1:1 行为；具体代码见平移后的版本，长度约 130 行，省略）

}  // namespace fml
}  // namespace lynx
```

> 注意：原文件 iOS autoreleasepool 的 `#if defined(OS_IOS)` 分支在 PostTask 内 push/pop；平移时保留该块，`#include` 顶部按目标平台条件化。

- [ ] **Step 4.3**：把 `base/src/fml/concurrent_message_loop.cc` 暂时缩减为最小占位（仅留空 namespace + 一个 `// TODO: facade 将在 Task 5 实现` 注释）。这是为了让现有调用方在中间步骤不报错——facade 还没接上。

- [ ] **Step 4.4**：更新 `base/src/BUILD.gn`：`fml` source set 中删除 `"fml/concurrent_message_loop.cc"`（如果有它作为源码的话），改为 `"fml/concurrent_loop_backend_std.cc"`（并按需把同名 .h 加进 public/private headers）。

- [ ] **Step 4.5**：运行基础测试（lynx 现有的依赖并发池的测试）确认**没有行为变化**：

```bash
cd /Users/cct/Workspace/OpenSource/lynx
ninja -C out/Default lynx_base
# 期望：build 成功（这一步只验证"没改坏 std 实现的可编译性"，Task 5 接 facade 后再跑完整测试
```

预期输出：build OK（如果失败说明平移未 1:1，回到 Step 4.2 对照原文件修）。

- [ ] **Step 4.6**：commit：

```bash
git add base/src/fml/concurrent_loop_backend_std.h base/src/fml/concurrent_loop_backend_std.cc base/src/fml/concurrent_message_loop.cc base/src/BUILD.gn
git commit -m "[Refactor][FML] port current concurrent loop implementation to BackendStd" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 5：ConcurrentMessageLoop 改为 facade

**Files:**

- Modify: `base/src/include/fml/concurrent_message_loop.h`（仅尾注释；.h 不动）
- Modify: `base/src/fml/concurrent_message_loop.cc`

- [ ] **Step 5.1**：在 `concurrent_message_loop.h` 顶部加 forward declaration：

```cpp
class ConcurrentLoopBackend;
```

- [ ] **Step 5.2**：把 `ConcurrentMessageLoop.cc` 写为薄 facade：

```cpp
// concurrent_message_loop.cc
#include "base/include/fml/concurrent_message_loop.h"

#include <memory>
#include <utility>

#include "base/include/fml/concurrent_message_loop_backend.h"
#include "base/include/fml/concurrent_task_runner.h"

namespace lynx {
namespace fml {

ConcurrentMessageLoop::ConcurrentMessageLoop(
    const std::string& name_prefix, size_t worker_count,
    ConcurrentMessageLoop::ThreadPriorityCallback priority_callback)
    : backend_(CreateConcurrentLoopBackend(
          name_prefix + (priority_callback
                             ? priority_callback()  // 调一次获取初始优先级
                             : Thread::ThreadPriority::NORMAL),
          worker_count)),
      task_runner_(std::make_shared<ConcurrentTaskRunner>(weak_from_this())) {}

// 注：原构造函数可能还有其他参数（如带 priority_callback 的版本）；按原签名照搬，
// 上述示意只列核心逻辑。

void ConcurrentMessageLoop::PostTask(base::closure task) {
  if (!task) return;
  if (shutdown_.load()) { task(); return; }   // C2 兜底
  backend_->PostTask(std::move(task));
}

bool ConcurrentMessageLoop::RunsTasksOnCurrentThreadWorker() const {
  return backend_->RunsTasksOnCurrentThreadWorker();
}

size_t ConcurrentMessageLoop::GetWorkerCount() const {
  return backend_->GetWorkerCount();
}

void ConcurrentMessageLoop::Terminate() {
  shutdown_.store(true);
  backend_->Terminate();
}

}  // namespace fml
}  // namespace lynx
```

> 重要：读取原 `concurrent_message_loop.cc` 的**构造函数完整签名**与所有 public 方法，保持**完全相同的外部接口**。`ConcurrentTaskRunner`（weak_ptr 包装）维持原样。如有 `priority_callback` 之类的额外构造参数，照原样透传；只把内部实现改为 delegate backend。

- [ ] **Step 5.3**：跑现有测试 + Task 3 的契约测试：

```bash
cd /Users/cct/Workspace/OpenSource/lynx
ninja -C out/Default lynx_base
# 也跑测试目标（如果有现成的 test runner）：
ninja -C out/Default concurrent_message_loop_backend_test
./out/Default/concurrent_message_loop_backend_test
```

预期输出：build 通过，Task 3 测试全通过（原实现的 CAS 抢占/错峰唤醒在 FakeBackend 测试范围内也能跑，但 FakeBackend 不是真的 std 池，所以 C1/C3 通过即可）。

- [ ] **Step 5.4**：commit：

```bash
git add base/include/fml/concurrent_message_loop.h base/src/fml/concurrent_message_loop.cc
git commit -m "[Refactor][FML] convert ConcurrentMessageLoop into facade over backend" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 6：扩展契约测试覆盖 facade 的 C2 shutdown fallback

**Files:**

- Modify: `base/src/fml/concurrent_message_loop_backend_test.cc`

- [ ] **Step 6.1**：加 shutdown fallback 测试（PostTask 在 caller 线程同步执行）：

```cpp
TEST(ConcurrentMessageLoopShutdownFallback,
     PostTaskRunsSynchronouslyOnCallerAfterShutdown) {
  // 这测试的是 facade 的 shutdown 兜底：用任一 backend（这里用 FakeBackend）
  // 验证 ConcurrentMessageLoop::PostTask 在 shutdown_ 被设后不委派给 backend，
  // 而是在调用线程同步执行 task。
  auto backend_owned = std::make_unique<FakeBackend>(1);
  auto* raw_backend = backend_owned.get();
  std::atomic<int> ran_on_worker{0};
  std::atomic<int> ran_on_caller{0};
  // 触发 shutdown：构造一个最小 facade 类似物，复用 facade PostTask 逻辑
  std::atomic<bool> shutdown_{false};
  auto post_task = [&](base::closure task) {
    if (shutdown_.load()) { task(); }
    else { raw_backend->PostTask(std::move(task)); }
  };
  // 关闭后 PostTask
  shutdown_.store(true);
  post_task([&] { ran_on_caller.fetch_add(1); });
  raw_backend->Terminate();
  EXPECT_EQ(ran_on_caller.load(), 1);
  EXPECT_EQ(ran_on_worker.load(), 0);
}
```

> 真正的「facade 与 backend 协作」测试需要构造一个 ConcurrentMessageLoop 并 shutdown。建议在 `concurrent_message_loop_test.cc`（如不存在则新建）里用真 ConcurrentMessageLoop 跑：

```cpp
TEST(ConcurrentMessageLoopTest, PostTaskAfterShutdownRunsSync) {
  auto loop = std::make_shared<ConcurrentMessageLoop>("test", 2);
  loop->Terminate();
  std::atomic<bool> on_caller{false};
  std::thread caller([&] {
    loop->PostTask([&] {
      on_caller.store(true);
      // 既然是 caller 线程同步跑，验证它确实不在 worker 线程
      EXPECT_FALSE(loop->RunsTasksOnCurrentThreadWorker());
    });
  });
  caller.join();
  EXPECT_TRUE(on_caller.load());
}
```

- [ ] **Step 6.2**：跑测试确认通过：

```bash
ninja -C out/Default concurrent_message_loop_backend_test
./out/Default/concurrent_message_loop_backend_test
```

预期输出：所有测试 PASS。

- [ ] **Step 6.3**：commit：

```bash
git add base/src/fml/concurrent_message_loop_backend_test.cc
git commit -m "[Test][FML] add facade shutdown fallback coverage" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 7：Phase 1 跨平台构建验证

- [ ] **Step 7.1**：在 macOS 上构建 Android 配置（默认非 harmony），确认 BackendStd 路径能编译：

```bash
cd /Users/cct/Workspace/OpenSource/lynx
gn gen out/android --args='target_os="android"'
ninja -C out/android lynx_base
```

预期输出：build OK。

- [ ] **Step 7.2**：构建 Linux 配置（如有相关 target）：

```bash
gn gen out/linux --args='is_linux=true'
ninja -C out/linux lynx_base
```

- [ ] **Step 7.3**：跑完整 lynx_base 测试套件（取决于现有测试发现方式）：

```bash
# 看 base/src/BUILD.gn 怎么注册 test runner，照现有方式跑
ls out/android/  # 找 test binary
```

预期输出：所有测试 PASS，零回归。

- [ ] **Step 7.4**：commit（如果 7.1-7.3 触发了小调整）：

```bash
git status  # 看一下有没有遗落
# 若有：git add + git commit ...
```

---

## Phase 2：Harmony FFRT 后端

> **📝 实施注记（实现完成后回填）**
>
> Phase 2 实施过程中对原始方案做了 4 处偏离，以下列出。代码块保留原写法作为决策记录；**与现网一致的最终写法见 [`concurrent_loop_backend_spec.md`](./concurrent_loop_backend_spec.md) §FFRT 后端**。
>
> 1. **`qos_user_interactive` → `qos_user_initiated`**：实现阶段确认 SDK 文档的"UI 响应"档实际对应 `qos_user_initiated`，`qos_user_interactive` 才是 @since 23 才有的实验档。为稳妥起见改用前者（与 NORMAL 仍差 2 档）。
> 2. **`MapQos` 从 `static` 成员移到匿名 namespace**：类内静态成员会暴露在头文件公共符号表，没必要；挪到 `.cc` 的匿名 namespace 保持 TU-local。
> 3. **`auto attr = ...; queue_(attr)` → 直接链式 bind 进 `ffrt::queue` 构造**：`ffrt::queue_attr` 的 copy ctor 被删，存到局部变量必然编译失败。
> 4. **PostTask lambda 从 `[this, t = move(task)]() mutable { t(); }` 改为 `make_shared<closure>` 包装**：`ffrt::queue::submit` 内部把 callable 转 `std::function`，要求 CopyConstructible；`base::closure` 是 move-only，必须包一层 `shared_ptr`。
>
> 以上 4 点对应提交 `1c9cac6f7 [FML][Harmony] fix ffrt backend compile (lambda, qos, third-party warning)`。

### Task 8：BackendFFRT 头文件

**Files:**

- Create: `base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.h`

- [ ] **Step 8.1**：创建 `concurrent_loop_backend_ffrt.h`：

```cpp
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_SRC_FML_PLATFORM_HARMONY_CONCURRENT_LOOP_BACKEND_FFRT_H_
#define BASE_SRC_FML_PLATFORM_HARMONY_CONCURRENT_LOOP_BACKEND_FFRT_H_

#include <cstddef>
#include <memory>
#include <string>

#include "base/include/fml/concurrent_message_loop_backend.h"
#include "base/include/fml/thread.h"

namespace lynx {
namespace fml {

class ConcurrentLoopBackendFFRT final : public ConcurrentLoopBackend {
 public:
  ConcurrentLoopBackendFFRT(const std::string& name_prefix,
                            Thread::ThreadPriority priority,
                            size_t worker_count);
  ~ConcurrentLoopBackendFFRT() override;

  void PostTask(base::closure task) override;
  bool RunsTasksOnCurrentThreadWorker() const override;
  size_t GetWorkerCount() const override { return worker_count_; }
  void Terminate() override;

 private:
  static int MapQos(Thread::ThreadPriority p);

  std::unique_ptr<class ffrt_queue> queue_;   // Pimpl，避免头污染
  size_t worker_count_;
  static thread_local ConcurrentLoopBackendFFRT* g_current_worker;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_SRC_FML_PLATFORM_HARMONY_CONCURRENT_LOOP_BACKEND_FFRT_H_
```

- [ ] **Step 8.2**：commit（纯头文件）：

```bash
git add base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.h
git commit -m "[Refactor][FML] add BackendFFRT header" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 9：BackendFFRT 实现

**Files:**

- Create: `base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.cc`

- [ ] **Step 9.1**：创建 `concurrent_loop_backend_ffrt.cc`（实现按 spec §7.2）。要点：
  - `#include "ffrt/ffrt.h"`（C++ 总头；保证 `ffrt::queue` / `ffrt::queue_attr` / `ffrt::task_attr` 可用）。
  - 构造函数：`ffrt::queue_attr{}.max_concurrency(N).qos(MapQos(p)).thread_mode(true).`.
  - MapQos：`HIGH → ffrt::qos_user_interactive`、`LOW/BACKGROUND → ffrt::qos_background`、其他 → `ffrt::qos_default`。在 MapQos 注释里说明 `qos_user_interactive` 在所有 API 都可传（FFRT C 接口不校验 enum API gate）。
  - PostTask 用 lambda 包一层任务级哨兵，再 `queue->submit(...)`。
  - 用 `std::unique_ptr<ffrt::queue>` 做 Pimpl（避免在头文件 include ffrt）。
  - Terminate 触发 `queue_.reset()` 让 RAII 析构。

完整骨架（照 spec §7.2）：

```cpp
// concurrent_loop_backend_ffrt.cc
#include "base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.h"

#include <utility>

#include "ffrt/ffrt.h"   // FFRT C++ header (resolves to base/platform/harmony/oh_modules/@ppd/ffrt/include/ffrt/ffrt.h)

namespace lynx {
namespace fml {

thread_local ConcurrentLoopBackendFFRT* ConcurrentLoopBackendFFRT::g_current_worker = nullptr;

ConcurrentLoopBackendFFRT::ConcurrentLoopBackendFFRT(
    const std::string& name_prefix, Thread::ThreadPriority priority,
    size_t worker_count)
    : worker_count_(worker_count) {
  // 头文件中 queue_ 字段被声明为 std::unique_ptr<ffrt_queue>，但 ffrt::queue 在 .h 里 forward decl 不行；
  // 解决：把 queue_ 改用 std::unique_ptr<ffrt::queue> 并 #include "ffrt/ffrt.h" 在 .h 里，
  // 或：.h 里保持 forward decl 的 wrapper struct class ffrt_queue;
  //    .cc 里 #include 真实头并 reinterpret_cast。简化起见，**直接把 #include "ffrt/ffrt.h" 放 .h** 是最干净的。

  // 实际写法（与 spec 一致）：
  auto attr = ffrt::queue_attr()
                  .max_concurrency(static_cast<int>(worker_count))
                  .qos(MapQos(priority))
                  .thread_mode(true);
  queue_ = std::make_unique<ffrt::queue>(
      ffrt::queue_concurrent, name_prefix.c_str(), attr);
}

ConcurrentLoopBackendFFRT::~ConcurrentLoopBackendFFRT() = default;

int ConcurrentLoopBackendFFRT::MapQos(Thread::ThreadPriority p) {
  switch (p) {
    case Thread::ThreadPriority::HIGH: return ffrt_qos_user_interactive;
    case Thread::ThreadPriority::LOW:
    case Thread::ThreadPriority::BACKGROUND: return ffrt_qos_background;
    case Thread::ThreadPriority::NORMAL:
    default: return ffrt_qos_default;
  }
}

void ConcurrentLoopBackendFFRT::PostTask(base::closure task) {
  auto wrapped = [this, t = std::move(task)]() mutable {
    g_current_worker = this;
    t();
    g_current_worker = nullptr;
  };
  queue_->submit(std::move(wrapped));
}

bool ConcurrentLoopBackendFFRT::RunsTasksOnCurrentThreadWorker() const {
  return g_current_worker == this;
}

void ConcurrentLoopBackendFFRT::Terminate() { queue_.reset(); }

}  // namespace fml
}  // namespace lynx
```

> **简化**：由于 `ffrt::queue` 类型完整定义在 `<ffrt/ffrt.h>`，把 `#include "ffrt/ffrt.h"` 放到 .h 反而最简单（不用 Pimpl/decl 折腾）。如团队约定要求 .h 不引入外部头，再切换为 Pimpl。

- [ ] **Step 9.2**：commit（实现部分）：

```bash
git add base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.cc base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.h
git commit -m "[FML] implement BackendFFRT using ffrt::queue C++ API" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 10：工厂分发（BackendFFRT 在 #if defined(OS_HARMONY) 下选择）

**Files:**

- Modify: `base/src/fml/concurrent_message_loop_backend.cc`

- [ ] **Step 10.1**：实现工厂函数：

```cpp
// concurrent_message_loop_backend.cc
#include "base/include/fml/concurrent_message_loop_backend.h"

#include <memory>
#include <string>

#include "base/include/fml/thread.h"
#if defined(OS_HARMONY)
#include "base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.h"
#endif
// BackendGCD 本轮桩：保留接口，OS_IOS/OSX 暂时返回 BackendStd 直到 GCD 实现。
#include "base/src/fml/concurrent_loop_backend_std.h"

namespace lynx {
namespace fml {

std::unique_ptr<ConcurrentLoopBackend> CreateConcurrentLoopBackend(
    const std::string& name_prefix, Thread::ThreadPriority priority,
    size_t worker_count) {
#if defined(OS_HARMONY)
  return std::make_unique<ConcurrentLoopBackendFFRT>(
      name_prefix, priority, worker_count);
#elif defined(OS_IOS) || defined(OS_OSX)
  // BackendGCD 尚未实现，本轮先落到 BackendStd 保证接口存在。
  return std::make_unique<ConcurrentLoopBackendStd>(
      name_prefix, priority, worker_count);
#else
  return std::make_unique<ConcurrentLoopBackendStd>(
      name_prefix, priority, worker_count);
#endif
}

}  // namespace fml
}  // namespace lynx
```

- [ ] **Step 10.2**：commit：

```bash
git add base/src/fml/concurrent_message_loop_backend.cc
git commit -m "[FML] implement platform-dispatch factory selecting BackendFFRT/Std" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 11：把 BackendFFRT.cc 加入 sources 列表

**Files:**

- Modify: `base/src/BUILD.gn`

> `import("../platform/harmony/harmony.gni")` + `include_dirs += [ffrt_include_dir]` + `libs += ["ffrt.z"]` 已在 **Task 0 Step 0.6** 完成。**本任务**只把 Task 9 写出的 `concurrent_loop_backend_ffrt.cc` 加入 sources，让它参与 Harmony 配置的编译。

- [ ] **Step 11.1**：在 `base/src/BUILD.gn` 的 `is_harmony` fml 源文件块（参见 Task 0 Step 0.6 那一块）`sources += [...]` 列表中加一行：

  ```gn
  sources += [
    "fml/platform/harmony/message_loop_harmony.cc",
    "fml/platform/harmony/concurrent_loop_backend_ffrt.cc",   # ← 新增
    ...
  ]
  ```

- [ ] **Step 11.2**：在 Harmony target 上做交叉编译验证（macOS 上无法 link `libffrt.z.so`，做交叉编译 stub 检查即可）。如果 lynx 没有现成 Harmony 交叉 target，跳过本步，在鸿蒙工程内验证（见 Task 13）。

- [ ] **Step 11.3**：commit：

  ```bash
  cd /Users/cct/Workspace/OpenSource/lynx
  git add base/src/BUILD.gn
  git commit -m "[FML][Harmony] add concurrent_loop_backend_ffrt.cc to base sources" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
  ```

---

### Task 12：spec §12 注明构建前置（文档定稿）

**Files:**

- 无新增文件；这是一个文档任务——已在 spec §12.2 写好「两处 ohpm install」。本任务确认 spec 与 Task 12 实践一致。

- [ ] **Step 12.1**：人工 review 一遍 `base/docs/concurrent_loop_backend_spec.md` 的 §12.2/§12.3/§12.4 与 Task 0、Task 11 对照，若不一致就微调 spec。

---

### Task 13：鸿蒙真机集成测试（人工 + 设备）

**Tools needed:** 一台鸿蒙设备（或模拟器）、DevEco Studio、hiTrace/HiView。

- [ ] **Step 13.1**：在 DevEco Studio 里以 explorer/harmony 工程 build/run，确认能编出 HAP，且不报 `libffrt.z.so` 找不到。

- [ ] **Step 13.2**：跑 lynx 的 demo（H5 渲染+图像解码+字体加载），观察 hiTrace：
  - `LynxHighTask` 队列的 worker QoS 应为 user_interactive（最高档）。
  - `LynxNormalTask` 队列的 worker QoS 为 default。
  - 渲染首屏顺畅，C1-C5 行为正常。

- [ ] **Step 13.3**：thermal/负载压一下（连续快速切换 tab 触发大量图像解码）：确认 HIGH 任务在 worker 饱和时不被 NORMAL 任务挤掉（QoS 仲裁生效）。

预期输出：行为与 std 后端一致 + QoS 差异化生效。

---

### Task 14：Phase 2 完成 + 文档收尾

- [ ] **Step 14.1**：跑全平台 build 烟雾（Android）+ 全套测试，确认零回归。

- [ ] **Step 14.2**：把 plan 中未尽事项（对接 [!NOTE]）回填到 spec §11 的开放问题表，更新 OQ-1/2/3/4 结论。

- [ ] **Step 14.3**：commit（文档与清理）：

```bash
git status
# 如有文档/注释调整：
git add base/docs/concurrent_loop_backend_spec.md
git commit -m "[Spec] FML: update spec with FFRT backend implementation outcomes" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

## Self-Review（自审 checklist）

完成后请确认：

- [ ] **Spec 覆盖**：spec §3/4/5/6/7/8/9/10/11/12 都对应到任务（§7.1 → Task 4；§7.2 → Tasks 8-10（T11 仅 sources 列表）；§12 → Task 0 + Task 11；§11 开放问题在 Phase 2 中通过 OQ-1 audit 等收敛）。
- [ ] **占位符扫描**：grep "TODO"、"TBD"、"类似 Task N"（实际写代码而非快捷引用）——本 plan 无 placeholder。
- [ ] **类型一致**：Task 4 的 `ConcurrentLoopBackendStd` 与 Task 5 的 `ConcurrentMessageLoop::backend_` 类型一致；Task 8-10 的 `ConcurrentLoopBackendFFRT` 与 spec §7.2 一致。
- [ ] **行为合同**：C1-C5 都有对应测试覆盖（C1/C3 在 Task 3；C2 在 Task 6；C4 由 facade Terminate + backend Terminate 链式保证；C5 由 facade GetWorkerCount 转发）。
- [ ] **FFRT 安装链路**：spec §12 的 install 命令、harmony.gni、oh-package.json5 三处与 Task 0、Task 11 一致。

如果发现偏差，回填到对应任务。
