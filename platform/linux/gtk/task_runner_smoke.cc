// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#include <cstdio>
#include <memory>
#include <thread>

#include "clay/shell/platform/embedder/embedder_task_runner.h"
int main() {
  int posted = 0, released = 0, ran = 0;
  const auto owner = std::this_thread::get_id();
  clay::EmbedderTaskRunner::DispatchTable table;
  table.post_task_callback = [&](auto*, uint64_t, auto) { ++posted; };
  table.runs_task_on_current_thread_callback = [owner] {
    return std::this_thread::get_id() == owner;
  };
  auto runner = fml::MakeRefCounted<clay::EmbedderTaskRunner>(table, 1);
  struct Resource {
    fml::RefPtr<fml::TaskRunner> runner;
    int* released;
    std::thread::id owner;
    ~Resource() {
      auto* output = released;
      const auto expected = owner;
      fml::TaskRunner::RunNowOrPostTask(runner, [output, expected] {
        *output = std::this_thread::get_id() == expected ? 1 : -1;
      });
    }
  };
  auto resource = std::make_shared<Resource>();
  resource->runner = runner;
  resource->released = &released;
  resource->owner = owner;
  fml::RefPtr<fml::TaskRunner> base = runner;
  base->PostDelayedTask([keep = resource, &ran] { ++ran; },
                        fml::TimeDelta::FromSeconds(3600));
  resource.reset();
  runner->OnThreadHostDestroyed();
  if (released != 1 || ran != 0 || posted != 1) {
    std::fprintf(stderr, "shutdown cleanup FAIL released=%d ran=%d posted=%d\n",
                 released, ran, posted);
    return 1;
  }
  std::puts(
      "shutdown cleanup: dropped delayed closure releases resource inline on "
      "owner thread PASS");
  return 0;
}
