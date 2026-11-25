// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_interceptor.h"

#include <cmath>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/common/napi/napi_runtime_proxy.h"
#include "core/runtime/common/napi/napi_runtime_proxy_quickjs.h"
#include "quickjs/include/quickjs.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace {

using runtime::js::NapiEnvironment;
using runtime::js::NapiRuntimeProxy;
using runtime::js::NapiRuntimeProxyQuickjs;

class EnvironmentDelegate final : public NapiEnvironment::Delegate {
 public:
  void RegisterModule(const std::string&, std::unique_ptr<Module>) override {}
  Module* GetModule(const std::string&) override { return nullptr; }
};

class QueuedTaskRunner final : public fml::TaskRunner {
 public:
  QueuedTaskRunner() : fml::TaskRunner(nullptr) {}
  void PostTask(base::closure closure) override {
    tasks_.push_back(std::move(closure));
  }

 private:
  std::vector<base::closure> tasks_;
};

class HostScriptInterceptorEnvTest : public ::testing::Test {
 public:
  HostScriptInterceptorEnvTest()
      : runtime_(LEPUS_NewRuntime()),
        context_(LEPUS_NewContext(runtime_)),
        runtime_proxy_(static_cast<NapiRuntimeProxy*>(
            NapiRuntimeProxyQuickjs::Create(
                context_,
                std::make_shared<runtime::js::DelegateObserver>(task_runner_))
                .release())),
        environment_(std::make_unique<EnvironmentDelegate>()),
        env_(runtime_proxy_->Env()) {
    environment_.SetRuntimeProxy(std::move(runtime_proxy_));
    environment_.Attach();
  }

  ~HostScriptInterceptorEnvTest() override {
    environment_.Detach();
    LEPUS_FreeContext(context_);
    LEPUS_FreeRuntime(runtime_);
  }

 protected:
  std::string EvalString(const char* script) {
    return env_.RunScript(script).ToString().Utf8Value();
  }

  LEPUSRuntime* runtime_;
  LEPUSContext* context_;
  fml::RefPtr<QueuedTaskRunner> task_runner_ =
      fml::MakeRefCounted<QueuedTaskRunner>();
  std::unique_ptr<NapiRuntimeProxy> runtime_proxy_;
  NapiEnvironment environment_;
  Napi::Env env_;
};

class HostScriptInterceptorTest : public HostScriptInterceptorEnvTest {
 protected:
  ~HostScriptInterceptorTest() override {
    if (provider_) provider_->Uninstall();
  }
  void Install(HostScriptInterceptor::Domain thread =
                   HostScriptInterceptor::Domain::kUI) {
    provider_ = HostScriptInterceptor::Install(env_, thread);
    ASSERT_NE(provider_, nullptr);
  }
  void Script(const char* source) {
    env_.RunScript(source);
    ASSERT_FALSE(env_.IsExceptionPending());
  }
  void DrainJobs() {
    LEPUSContext* pending_context = nullptr;
    int status;
    while ((status = LEPUS_ExecutePendingJob(runtime_, &pending_context)) > 0) {
    }
    ASSERT_EQ(status, 0);
  }
  InterceptResult Dispatch(InterceptKind kind = InterceptKind::kLoadTemplate) {
    lepus::Value event(lepus::Dictionary::Create());
    event.SetProperty("viewId", lepus::Value("1"));
    if (kind == InterceptKind::kCreated) {
      event.SetProperty("url", lepus::Value(""));
    } else {
      lepus::Value request(lepus::Dictionary::Create());
      request.SetProperty("url", lepus::Value("original"));
      event.SetProperty("request", request);
    }
    return provider_->Dispatch(kind, event);
  }
  std::shared_ptr<HostScriptInterceptor> provider_;
};

TEST_F(HostScriptInterceptorTest, DomainsAcceptOnlyTheirEventKinds) {
  const std::pair<Interceptor::Domain, const char*> cases[] = {
      {Interceptor::Domain::kUI,
       "[['view.created','view.create','view.loadTemplate',"
       "'view.updateMetaData','view.updateViewport','resource.fetch'],"
       "['jsb.call','jsb.result','jsb.callback','jsb.resolveModule']]"},
      {Interceptor::Domain::kBTS,
       "[['view.created','jsb.call','jsb.result','jsb.callback',"
       "'jsb.resolveModule'],"
       "['view.create','view.loadTemplate',"
       "'view.updateMetaData','view.updateViewport','resource.fetch']]"},
      {Interceptor::Domain::kMTS,
       "[['view.created'],['view.create',"
       "'view.loadTemplate','view.updateMetaData','view.updateViewport',"
       "'resource.fetch','jsb.call','jsb.result','jsb.callback',"
       "'jsb.resolveModule']]"}};
  for (const auto& [domain, names] : cases) {
    Install(domain);
    std::string script =
        "const [accepted, rejected] = " + std::string(names) + R"(;
      for (const name of accepted) interceptor.on(name,(e,c)=>c.proceed());
      for (const name of rejected) {
        let message='';
        try { interceptor.on(name,()=>{}); } catch(e) { message=e.message; }
        if(!message.includes('execution domain')) throw Error(name);
      }
    )";
    Script(("(()=>{" + script + "})()").c_str());
    provider_->Uninstall();
    provider_.reset();
  }
}

TEST_F(HostScriptInterceptorTest, JsbRulesShareBridgeInRegistrationOrder) {
  Install(Interceptor::Domain::kBTS);
  Script(R"(
    globalThis.first = interceptor.on('jsb.call', (event, chain) => {
      if (event.module === 'bridge' && event.args[0] === 'report') {
        return chain.mock({returnValue: 'report'});
      }
      event.args[0] = 'changed';
      return chain.next();
    });
    interceptor.on('jsb.call', (event, chain) =>
      event.module === 'bridge' && event.args[0] === 'route'
        ? chain.mock({returnValue: 'route'})
        : chain.next());
  )");
  auto dispatch = [&](const char* api) {
    lepus::Value event(lepus::Dictionary::Create());
    event.SetProperty("module", lepus::Value("bridge"));
    auto args = lepus::CArray::Create();
    args->push_back(lepus::Value(api));
    event.SetProperty("args", lepus::Value(args));
    auto result = provider_->Dispatch(InterceptKind::kCall, event);
    EXPECT_FALSE(result.failed);
    return result.mock;
  };
  EXPECT_EQ(dispatch("report").GetProperty("returnValue").StdString(),
            "report");
  EXPECT_EQ(dispatch("route").GetProperty("returnValue").StdString(), "route");
  EXPECT_FALSE(dispatch("other").IsTable());
  Script("first.dispose(); first.dispose()");
  EXPECT_FALSE(dispatch("report").IsTable());
  EXPECT_EQ(dispatch("route").GetProperty("returnValue").StdString(), "route");
}

TEST_F(HostScriptInterceptorTest, ChainSkipsRemovedAndDefersNewHandlers) {
  Install(Interceptor::Domain::kBTS);
  Script(R"(
    const first = interceptor.on('jsb.result', (event, chain) => {
      first.dispose();
      second.dispose();
      interceptor.on('jsb.result',
        (_, next) => next.proceed({value: 'new'}));
      return chain.next();
    });
    const second = interceptor.on('jsb.result',
      (_, chain) => chain.proceed({value: 'removed'}));
  )");
  auto first = Dispatch(InterceptKind::kResult);
  EXPECT_FALSE(first.failed);
  EXPECT_FALSE(first.patch.IsTable());
  auto second = Dispatch(InterceptKind::kResult);
  EXPECT_FALSE(second.failed);
  EXPECT_EQ(second.patch.GetProperty("value").StdString(), "new");
}

TEST_F(HostScriptInterceptorTest, ResolverMockFollowsItsSubscription) {
  Install(Interceptor::Domain::kBTS);
  Script(R"(
    interceptor.on('jsb.resolveModule', (event, chain) =>
      event.module === 'invalid' ? chain.mock({methods: ['']}) : chain.next());
    globalThis.replay = interceptor.on('jsb.resolveModule', (event, chain) =>
      event.module === 'resourceloader' && event.viewId === '1'
        ? chain.mock({methods: ['read', 'write']}) : chain.next());
  )");
  auto resolve = [&](const char* module, const char* view) {
    lepus::Value event(lepus::Dictionary::Create());
    event.SetProperty("module", lepus::Value(module));
    event.SetProperty("viewId", lepus::Value(view));
    return provider_->Dispatch(InterceptKind::kResolveModule, event);
  };
  auto replay = resolve("resourceloader", "1");
  ASSERT_FALSE(replay.failed);
  const auto& methods = replay.mock.GetProperty("methods");
  ASSERT_TRUE(methods.IsArray());
  ASSERT_EQ(methods.GetLength(), 2);
  EXPECT_EQ(methods.GetProperty(0u).StdString(), "read");
  EXPECT_EQ(methods.GetProperty(1u).StdString(), "write");
  EXPECT_FALSE(replay.subscription.expired());
  EXPECT_FALSE(resolve("other", "1").mock.IsTable());
  EXPECT_FALSE(resolve("resourceloader", "2").mock.IsTable());
  EXPECT_TRUE(resolve("invalid", "1").failed);
  Script("replay.dispose()");
  EXPECT_TRUE(replay.subscription.expired());
  EXPECT_FALSE(resolve("resourceloader", "1").mock.IsTable());
}

TEST_F(HostScriptInterceptorTest, DomainLookupRequiresOwnerThread) {
  Install(HostScriptInterceptor::Domain::kBTS);
  Script("interceptor.on('jsb.call',(e,c)=>c.proceed())");
  const auto before = Interceptor::FailureCount();
  EXPECT_EQ(
      Interceptor::Current({InterceptKind::kCall}, Interceptor::Domain::kUI),
      nullptr);
  EXPECT_EQ(
      Interceptor::Current({InterceptKind::kCall}, Interceptor::Domain::kBTS),
      provider_);
  std::thread other([] {
    EXPECT_EQ(
        Interceptor::Current({InterceptKind::kCall}, Interceptor::Domain::kMTS),
        nullptr);
  });
  other.join();
  EXPECT_EQ(Interceptor::FailureCount(), before);
  std::thread unsupported([] {
    EXPECT_EQ(
        Interceptor::Current({InterceptKind::kCall}, Interceptor::Domain::kBTS),
        nullptr);
  });
  unsupported.join();
  EXPECT_EQ(Interceptor::FailureCount(), before + 1);
}

TEST_F(HostScriptInterceptorTest, GuardDisablesLookupAndDispatch) {
  auto ready = std::make_shared<bool>(true);
  provider_ =
      HostScriptInterceptor::Install(env_, HostScriptInterceptor::Domain::kUI,
                                     nullptr, [ready] { return *ready; });
  ASSERT_NE(provider_, nullptr);
  Script(
      "interceptor.on('view.loadTemplate',(e,c)=>c.proceed({url:'patched-2'})"
      ")");
  EXPECT_EQ(Dispatch().patch.GetProperty("url").StdString(), "patched-2");
  *ready = false;
  EXPECT_EQ(Interceptor::Current({InterceptKind::kLoadTemplate},
                                 Interceptor::Domain::kUI),
            nullptr);
  EXPECT_FALSE(Dispatch().patch.IsTable());
  *ready = true;
  EXPECT_EQ(Interceptor::Current({InterceptKind::kLoadTemplate},
                                 Interceptor::Domain::kUI),
            provider_);
  EXPECT_EQ(Dispatch().patch.GetProperty("url").StdString(), "patched-2");
}

TEST_F(HostScriptInterceptorTest, DomainsCanShareOneOwnerThread) {
  class Provider final : public Interceptor {
   public:
    bool HasHandlers(InterceptKind) const override { return true; }
    InterceptResult Dispatch(InterceptKind, const lepus::Value&) override {
      return {};
    }
    void ReportError(const std::string&) override {}
  };
  auto ui = std::make_shared<Provider>();
  auto bts = std::make_shared<Provider>();
  auto mts = std::make_shared<Provider>();
  ASSERT_TRUE(Interceptor::Attach(ui, Interceptor::Domain::kUI));
  ASSERT_TRUE(Interceptor::Attach(bts, Interceptor::Domain::kBTS));
  ASSERT_TRUE(Interceptor::Attach(mts, Interceptor::Domain::kMTS));
  EXPECT_EQ(Interceptor::Current({InterceptKind::kLoadTemplate},
                                 Interceptor::Domain::kUI),
            ui);
  EXPECT_EQ(
      Interceptor::Current({InterceptKind::kCall}, Interceptor::Domain::kBTS),
      bts);
  EXPECT_EQ(
      Interceptor::Current({InterceptKind::kCall}, Interceptor::Domain::kMTS),
      mts);
  ui->Uninstall();
  bts->Uninstall();
  mts->Uninstall();
}

TEST_F(HostScriptInterceptorTest, SupportsMultipleHandlersAcrossAllEventKinds) {
  for (auto domain : {Interceptor::Domain::kUI, Interceptor::Domain::kBTS,
                      Interceptor::Domain::kMTS}) {
    Install(domain);
    const char* names =
        domain == Interceptor::Domain::kUI
            ? "['view.created','view.loadTemplate','view.updateMetaData',"
              "'view.create','view.updateViewport','resource.fetch']"
            : (domain == Interceptor::Domain::kBTS
                   ? "['view.created','jsb.call','jsb.result',"
                     "'jsb.callback','jsb.resolveModule']"
                   : "['view.created']");
    std::string script = R"(
      globalThis.calls = [];
      globalThis.firstSubs = [];
      globalThis.names = )" +
                         std::string(names) + R"(;
      for (const name of names) {
        // Without View bindings every proxy is null and creation never fires.
        firstSubs.push(interceptor.on(name, (e,c,v) => {
          calls.push(v === null ? name + '#1' : 'proxy');
          return c.next();
        }));
        interceptor.on(name, (e,c,v) => {
          calls.push(v === null ? name + '#2' : 'proxy');
          return c.proceed();
        });
      }
    )";
    Script(script.c_str());
    for (size_t i = 0; i < static_cast<size_t>(InterceptKind::kCount); ++i) {
      auto kind = static_cast<InterceptKind>(i);
      if (provider_->HasHandlers(kind)) EXPECT_FALSE(Dispatch(kind).failed);
    }
    EXPECT_EQ(
        EvalString("calls.join(',')"),
        EvalString("names.slice(1).flatMap(n=>[n+'#1',n+'#2']).join(',')"));
    Script("calls = []; for (const sub of firstSubs) sub.dispose();");
    for (size_t i = 0; i < static_cast<size_t>(InterceptKind::kCount); ++i) {
      auto kind = static_cast<InterceptKind>(i);
      if (provider_->HasHandlers(kind)) EXPECT_FALSE(Dispatch(kind).failed);
    }
    EXPECT_EQ(EvalString("calls.join(',')"),
              EvalString("names.slice(1).map(n=>n+'#2').join(',')"));
    provider_->Uninstall();
    provider_.reset();
  }
}

TEST_F(HostScriptInterceptorTest, ProceedCommitsSingleHandlerPatch) {
  Install();
  Script(R"(
    interceptor.on('view.loadTemplate', (e,c) => {
      if (Object.keys(c).sort().join() !== 'mock,next,proceed')
        throw Error('unexpected chain decisions');
      return c.proceed({url:'patched-2'});
    });
  )");
  auto result = Dispatch();
  EXPECT_FALSE(result.failed);
  EXPECT_EQ(result.patch.GetProperty("url").StdString(), "patched-2");
}

TEST_F(HostScriptInterceptorTest, ExplicitUndefinedMeansOmission) {
  Install();
  Script(R"(
    globalThis.subscription = interceptor.on('view.loadTemplate',
        (e,c) => c.proceed(undefined));
  )");
  auto omitted = Dispatch();
  EXPECT_FALSE(omitted.failed);
  EXPECT_FALSE(omitted.patch.IsTable());
  Script(R"(
    subscription.dispose();
    interceptor.on('view.loadTemplate', (e,c) => c.proceed(
        {url:'patched-2', source:undefined, initialData:undefined}));
  )");
  auto partial = Dispatch();
  ASSERT_FALSE(partial.failed);
  EXPECT_EQ(partial.patch.GetLength(), 1);
  EXPECT_EQ(partial.patch.GetProperty("url").StdString(), "patched-2");
  provider_->Uninstall();

  Install(HostScriptInterceptor::Domain::kBTS);
  Script(R"(
    interceptor.on('jsb.result', (e,c) => c.proceed({value:undefined}));
  )");
  lepus::Value event(lepus::Dictionary::Create());
  event.SetProperty("value", lepus::Value(0));
  auto result = provider_->Dispatch(InterceptKind::kResult, event);
  ASSERT_FALSE(result.failed);
  ASSERT_TRUE(result.patch.IsTable());
  EXPECT_TRUE(result.patch.GetProperty("value").IsUndefined());
}

TEST_F(HostScriptInterceptorTest, NextPassesThroughWhenUnmodified) {
  Install();
  Script(R"(
    interceptor.on('view.loadTemplate', (_e, c) => c.next());
    interceptor.on('view.loadTemplate', (_e, c) => c.next(undefined));
  )");
  auto pass = Dispatch();
  EXPECT_FALSE(pass.failed);
  EXPECT_FALSE(pass.patch.IsTable());
  Script(R"(
    interceptor.on('view.loadTemplate', () => ({action:'next'}));
  )");
  EXPECT_TRUE(Dispatch().failed);
}

TEST_F(HostScriptInterceptorTest, RegistrationSnapshotAndDispose) {
  Install();
  Script(R"(
    globalThis.calls = [];
    globalThis.first = interceptor.on('view.loadTemplate', (e,c) => {
      calls.push('first'); first.dispose();
      interceptor.on('view.loadTemplate', (e,c) => {calls.push('late'); return c.proceed()});
      first.dispose();
      return c.proceed();
    });
  )");
  EXPECT_FALSE(Dispatch().failed);
  EXPECT_EQ(EvalString("calls.join(',')"), "first");
  Script("calls = []; first.dispose()");
  EXPECT_FALSE(Dispatch().failed);
  EXPECT_EQ(EvalString("calls.join(',')"), "late");
}

TEST_F(HostScriptInterceptorTest, InvalidDecisionsRollbackWholePhase) {
  const char* invalid[] = {
      "throw Error('failure')",
      "return Promise.resolve(c.proceed())",
      "return undefined",
      "return c.proceed({url:''})",
      "return c.proceed({url:1})",
      "return c.proceed({initialData:NaN})",
      "return c.proceed({url:'valid',globalProps:1})",
      "return c.proceed({unknown:true})",
      "return c.mock({})",
      "const x={}; x.self=x; return c.proceed({initialData:x})",
      "return c.proceed({source:{kind:'bytes',bytes:new ArrayBuffer(0)}})"};
  for (auto body : invalid) {
    Install();
    std::string source = "interceptor.on('view.loadTemplate', (e,c) => {" +
                         std::string(body) + "})";
    Script(source.c_str());
    auto before = Interceptor::FailureCount();
    auto result = Dispatch();
    EXPECT_TRUE(result.failed) << body;
    EXPECT_FALSE(result.patch.IsTable());
    EXPECT_FALSE(result.mock.IsTable());
    EXPECT_EQ(Interceptor::FailureCount(), before + 1);
    EXPECT_FALSE(env_.IsExceptionPending());
    provider_->Uninstall();
    provider_.reset();
  }
}

TEST_F(HostScriptInterceptorTest, ViewSnapshotsOnlyExposeCommittedPatches) {
  Install();
  lepus::Value request(lepus::Dictionary::Create());
  request.SetProperty("url", lepus::Value("original"));
  lepus::Value event(lepus::Dictionary::Create());
  event.SetProperty("request", request);
  Script(R"(
    globalThis.calls = 0;
    interceptor.on('view.loadTemplate', (e,c) => {
      if (++calls === 1) {
        globalThis.firstEvent = e;
        e.request.url = 'mutated';
        return c.proceed({url:'patched-2'});
      }
      if (e === firstEvent || e.request === firstEvent.request ||
          e.request.url !== 'original' || firstEvent.request.url !== 'mutated')
        throw Error('snapshots are not isolated');
      return c.proceed({url:'patched-3'});
    });
  )");
  auto first = provider_->Dispatch(InterceptKind::kLoadTemplate, event);
  ASSERT_FALSE(first.failed);
  EXPECT_EQ(first.patch.GetProperty("url").StdString(), "patched-2");
  auto second = provider_->Dispatch(InterceptKind::kLoadTemplate, event);
  ASSERT_FALSE(second.failed);
  EXPECT_EQ(second.patch.GetProperty("url").StdString(), "patched-3");
  EXPECT_EQ(first.patch.GetProperty("url").StdString(), "patched-2");
  EXPECT_EQ(request.GetProperty("url").StdString(), "original");
}

TEST_F(HostScriptInterceptorTest, DataRoundTripCopiesNestedRecordsAndBuffers) {
  Install(HostScriptInterceptor::Domain::kBTS);
  Script(R"(
    const record = Object.create(null);
    record.text = 'data';
    const shared = {n:7};
    globalThis.bytes = new Uint8Array([1,2,3]);
    globalThis.payload = [undefined, null, true, 1.5, record, bytes.buffer,
                          shared, shared];
    globalThis.subscription = interceptor.on('jsb.result', (e,c) => c.proceed({value:payload}));
  )");
  lepus::Value event(lepus::Dictionary::Create());
  event.SetProperty("value", lepus::Value(0));
  auto result = provider_->Dispatch(InterceptKind::kResult, event);
  ASSERT_FALSE(result.failed);
  auto value = result.patch.GetProperty("value");
  ASSERT_TRUE(value.IsArray());
  Script(R"(
    subscription.dispose();
    interceptor.on('jsb.result', (e,c) => {
      if (e.value[0] !== undefined || e.value[1] !== null ||
          e.value[2] !== true || e.value[3] !== 1.5 ||
          e.value[4].text !== 'data' || new Uint8Array(e.value[5])[1] !== 2 ||
          e.value[6].n !== 7 || e.value[7].n !== 7)
        throw Error('data did not round trip');
      e.value[4].text = 'changed';
      new Uint8Array(e.value[5])[0] = 99;
      bytes[2] = 99;
      return c.proceed();
    });
  )");
  event.SetProperty("value", value);
  auto next = provider_->Dispatch(InterceptKind::kResult, event);
  ASSERT_FALSE(next.failed);
  EXPECT_FALSE(next.patch.IsTable());
  EXPECT_TRUE(value.GetProperty(0).IsUndefined());
  EXPECT_TRUE(value.GetProperty(1).IsNil());
  EXPECT_EQ(value.GetProperty(4).GetProperty("text").StdString(), "data");
  auto bytes = value.GetProperty(5).ByteArray();
  ASSERT_EQ(bytes->GetLength(), 3);
  EXPECT_EQ(bytes->GetPtr()[0], 1);
  EXPECT_EQ(bytes->GetPtr()[2], 3);
  EXPECT_EQ(event.GetProperty("value"), value);
}

TEST_F(HostScriptInterceptorTest, SpecialNumbersRoundTrip) {
  Install(Interceptor::Domain::kBTS);
  Script(R"(
    interceptor.on('jsb.result', (e,c) => {
      if (e.value && !Object.is(e.value.nested[0], expected))
        throw Error('number changed during round trip');
      return c.proceed({value:{nested:[expected]}});
    });
  )");
  for (const char* source : {"NaN", "Infinity", "-Infinity", "-0", "0"}) {
    SCOPED_TRACE(source);
    Script((std::string("globalThis.expected = ") + source).c_str());
    const auto expected =
        env_.Global().Get("expected").As<Napi::Number>().DoubleValue();
    lepus::Value event(lepus::Dictionary::Create());
    auto result = provider_->Dispatch(InterceptKind::kResult, event);
    ASSERT_FALSE(result.failed);
    auto value = result.patch.GetProperty("value");
    auto number = value.GetProperty("nested").GetProperty(0);
    ASSERT_TRUE(number.IsNumber());
    if (std::isnan(expected)) {
      EXPECT_TRUE(std::isnan(number.Number()));
    } else {
      EXPECT_EQ(number.Number(), expected);
      EXPECT_EQ(std::signbit(number.Number()), std::signbit(expected));
    }
    event.SetProperty("value", value);
    EXPECT_FALSE(provider_->Dispatch(InterceptKind::kResult, event).failed);
  }
}

TEST_F(HostScriptInterceptorTest, SpecialNumbersInPromiseCallbacks) {
  Install(Interceptor::Domain::kBTS);
  Script(R"(
    globalThis.numbers = [NaN, Infinity, -Infinity, -0];
    interceptor.on('jsb.call', (e,c) => c.mock({
      returnValue: NaN,
      callbacks: [new Promise(resolve => {
        globalThis.resume = () => resolve({argumentIndex:0, args:numbers});
      })]
    }));
    interceptor.on('jsb.callback', async (e,c) => {
      await new Promise(resolve => { globalThis.resume = resolve; });
      return c.proceed({args:numbers});
    });
  )");
  lepus::Value event(lepus::Dictionary::Create());
  auto indices = lepus::CArray::Create();
  indices->push_back(lepus::Value(0));
  event.SetProperty("callbackIndices", lepus::Value(indices));
  for (auto kind : {InterceptKind::kCall, InterceptKind::kCallback}) {
    auto result = provider_->Dispatch(kind, event);
    ASSERT_FALSE(result.failed);
    if (kind == InterceptKind::kCall)
      EXPECT_TRUE(std::isnan(result.mock.GetProperty("returnValue").Number()));
    ASSERT_TRUE(result.pending);
    bool completed = false;
    result.pending([&](InterceptResult settled) {
      completed = true;
      ASSERT_FALSE(settled.failed);
      auto payload = kind == InterceptKind::kCall
                         ? settled.mock.GetProperty("callbacks").GetProperty(0)
                         : settled.patch;
      auto args = payload.GetProperty("args");
      EXPECT_TRUE(std::isnan(args.GetProperty(0).Number()));
      EXPECT_EQ(args.GetProperty(1).Number(), INFINITY);
      EXPECT_EQ(args.GetProperty(2).Number(), -INFINITY);
      EXPECT_EQ(args.GetProperty(3).Number(), 0);
      EXPECT_TRUE(std::signbit(args.GetProperty(3).Number()));
    });
    EXPECT_FALSE(completed);
    Script("resume()");
    DrainJobs();
    EXPECT_TRUE(completed);
  }
}

TEST_F(HostScriptInterceptorTest, PromiseNextResumesCallbackHandlersInOrder) {
  Install(Interceptor::Domain::kBTS);
  Script(R"(
    globalThis.order = [];
    interceptor.on('jsb.callback', async (event, chain) => {
      order.push('first');
      event.args[0] = 'edited';
      await new Promise(resolve => { globalThis.firstDone = resolve; });
      return chain.next();
    });
    interceptor.on('jsb.callback', async (event, chain) => {
      order.push(event.args[0] + '@' + event.viewURL);
      await new Promise(resolve => { globalThis.secondDone = resolve; });
      return chain.next();
    });
    interceptor.on('jsb.callback', (event, chain) => {
      order.push('finish@' + event.viewURL);
      return chain.proceed({args: ['patched']});
    });
    interceptor.on('jsb.callback', (_, chain) => {
      order.push('skipped');
      return chain.next();
    });
  )");
  lepus::Value event(lepus::Dictionary::Create());
  Interceptor::UpdateViewUrl(1, "before-reload");
  event.SetProperty("viewId", lepus::Value("1"));
  auto args = lepus::CArray::Create();
  args->push_back(lepus::Value("original"));
  event.SetProperty("args", lepus::Value(args));
  auto result = provider_->Dispatch(InterceptKind::kCallback, event);
  ASSERT_TRUE(result.pending);
  int completions = 0;
  result.pending([&](InterceptResult settled) {
    ++completions;
    EXPECT_FALSE(settled.failed);
    EXPECT_EQ(settled.patch.GetProperty("args").GetProperty(0u).StdString(),
              "patched");
  });
  EXPECT_EQ(EvalString("order.join(',')"), "first");
  Interceptor::UpdateViewUrl(1, "after-reload");
  Script("firstDone()");
  DrainJobs();
  EXPECT_EQ(completions, 0);
  EXPECT_EQ(EvalString("order.join(',')"), "first,original@before-reload");
  Script("secondDone()");
  DrainJobs();
  EXPECT_EQ(completions, 1);
  EXPECT_EQ(EvalString("order.join(',')"),
            "first,original@before-reload,finish@before-reload");
  Interceptor::ClearViewUrl(1);
}

TEST_F(HostScriptInterceptorTest, PromiseNextKeepsOriginalHandlerWindow) {
  Install(Interceptor::Domain::kBTS);
  Script(R"(
    interceptor.on('jsb.callback', async (_, chain) => {
      await new Promise(resolve => { globalThis.resume = resolve; });
      return chain.next();
    });
    globalThis.removed = interceptor.on('jsb.callback', (_, chain) =>
      chain.proceed({args: ['removed']}));
  )");
  auto result = Dispatch(InterceptKind::kCallback);
  ASSERT_TRUE(result.pending);
  int completions = 0;
  result.pending([&](InterceptResult settled) {
    ++completions;
    EXPECT_FALSE(settled.failed);
    EXPECT_FALSE(settled.patch.IsTable());
  });
  Script(R"(
    removed.dispose();
    interceptor.on('jsb.callback', (_, chain) =>
      chain.proceed({args: ['late']}));
    resume();
  )");
  DrainJobs();
  EXPECT_EQ(completions, 1);

  result = Dispatch(InterceptKind::kCallback);
  ASSERT_TRUE(result.pending);
  result.pending([&](InterceptResult settled) {
    ++completions;
    EXPECT_FALSE(settled.failed);
    EXPECT_EQ(settled.patch.GetProperty("args").GetProperty(0u).StdString(),
              "late");
  });
  Script("resume()");
  DrainJobs();
  EXPECT_EQ(completions, 2);
}

TEST_F(HostScriptInterceptorTest,
       InvalidNestedDataRollsBackAndClearsExceptions) {
  const char* invalid[] = {
      "() => {}",
      "Promise.resolve(1)",
      "new Date(0)",
      "new Uint8Array(1)",
      "Object.create({n:1})",
      "Symbol('value')",
      "{get value() { throw Error('getter'); }}",
      "new Proxy({}, {getPrototypeOf() { throw Error('prototype'); }})",
      "new Proxy({}, {ownKeys() { throw Error('keys'); }})",
      "(() => { const a=[]; a.push(a); return a; })()",
      "(() => { let a={}; for(let i=0;i<64;i++) a={child:a}; return a; })()"};
  for (auto value : invalid) {
    SCOPED_TRACE(value);
    Install(HostScriptInterceptor::Domain::kBTS);
    std::string source =
        "interceptor.on('jsb.result', (e,c) => c.proceed({value:[" +
        std::string(value) + "]}))";
    Script(source.c_str());
    lepus::Value event(lepus::Dictionary::Create());
    event.SetProperty("value", lepus::Value(1));
    auto result = provider_->Dispatch(InterceptKind::kResult, event);
    EXPECT_TRUE(result.failed);
    EXPECT_FALSE(result.patch.IsTable());
    EXPECT_EQ(event.GetProperty("value").Number(), 1);
    EXPECT_FALSE(env_.IsExceptionPending());
    provider_->Uninstall();
    provider_.reset();
  }
}

TEST_F(HostScriptInterceptorTest, ThreadRegistrationAndUnload) {
  Install();
  EXPECT_EQ(EvalString("try { interceptor.on('jsb.call', ()=>{}); 'bad' } "
                       "catch(e) { 'ok' }"),
            "ok");
  Script(
      "globalThis.oldApi=interceptor; "
      "interceptor.on('view.loadTemplate',(e,c)=>c.proceed())");
  provider_->Uninstall();
  EXPECT_EQ(Interceptor::Current({InterceptKind::kLoadTemplate},
                                 Interceptor::Domain::kUI),
            nullptr);
  EXPECT_EQ(EvalString("try { oldApi.on('view.loadTemplate',()=>{}); 'bad' } "
                       "catch(e) { 'ok' }"),
            "ok");
  provider_.reset();
  Install();
  EXPECT_FALSE(provider_->HasHandlers(InterceptKind::kLoadTemplate));
}

TEST_F(HostScriptInterceptorTest,
       SavedCallbacksDoNotRetainOrAffectNewProvider) {
  Install();
  Script(R"(
    globalThis.savedOn = interceptor.on;
    globalThis.savedDispose = savedOn('view.loadTemplate', (e,c) => c.proceed()).dispose;
  )");
  std::weak_ptr<HostScriptInterceptor> previous = provider_;
  provider_->Uninstall();
  provider_.reset();
  EXPECT_TRUE(previous.expired());
  Script("savedDispose(); savedDispose()");
  EXPECT_EQ(EvalString("try { savedOn('view.loadTemplate',()=>{}); 'bad' } "
                       "catch(e) { 'ok' }"),
            "ok");
  Install();
  Script(R"(
    interceptor.on('view.loadTemplate', (e,c) => c.proceed({url:'patched-3'}));
    savedDispose();
  )");
  EXPECT_EQ(Dispatch().patch.GetProperty("url").StdString(), "patched-3");
}

TEST_F(HostScriptInterceptorTest,
       SavedChainFunctionsOutliveDispatchAndProvider) {
  Install();
  Script(R"(
    globalThis.subscription = interceptor.on('view.loadTemplate', (e,c) => {
      globalThis.savedProceed = c.proceed;
      globalThis.savedMock = c.mock;
      globalThis.savedNext = c.next;
      const proceed = c.proceed;
      c.proceed = () => { throw Error('modified chain leaked'); };
      return proceed();
    });
  )");
  EXPECT_FALSE(Dispatch().failed);
  Script(R"(
    subscription.dispose();
    interceptor.on('view.loadTemplate', (e,c) => c.proceed({url:'patched-2'}));
  )");
  EXPECT_EQ(Dispatch().patch.GetProperty("url").StdString(), "patched-2");
  provider_->Uninstall();
  provider_.reset();
  LEPUS_RunGC(runtime_);
  Script(R"(
    for (const [fn, action] of [
      [savedProceed,'proceed'], [savedMock,'mock'], [savedNext,'next']
    ]) {
      const payload = {value:7};
      const result = fn(payload);
      if (result.action !== action || result.payload !== payload)
        throw Error('invalid saved decision');
      if (Object.keys(fn().payload).length !== 0)
        throw Error('invalid default payload');
    }
    delete globalThis.savedProceed;
    delete globalThis.savedMock;
    delete globalThis.savedNext;
  )");
  LEPUS_RunGC(runtime_);
}

TEST_F(HostScriptInterceptorTest, MockValidatesAllCallbacksBeforeCommit) {
  Install(HostScriptInterceptor::Domain::kBTS);
  lepus::Value event(lepus::Dictionary::Create());
  auto indices = lepus::CArray::Create();
  indices->push_back(lepus::Value(0));
  indices->push_back(lepus::Value(2));
  event.SetProperty("callbackIndices", lepus::Value(indices));
  Script(
      "globalThis.d = "
      "interceptor.on('jsb.call',(e,c)=>c.mock({returnValue:7,callbacks:[{"
      "argumentIndex:0,args:[1]},{argumentIndex:2,args:[2]}]}))");
  auto result = provider_->Dispatch(InterceptKind::kCall, event);
  EXPECT_FALSE(result.failed);
  EXPECT_EQ(result.mock.GetProperty("returnValue").Number(), 7);
  EXPECT_EQ(result.mock.GetProperty("callbacks").GetLength(), 2);
  Script(
      "d.dispose(); "
      "globalThis.d = "
      "interceptor.on('jsb.call',(e,c)=>c.mock({callbacks:[{argumentIndex:0,"
      "args:[]},{argumentIndex:1,args:[]}]}))");
  result = provider_->Dispatch(InterceptKind::kCall, event);
  EXPECT_TRUE(result.failed);
  EXPECT_FALSE(result.mock.IsTable());
  Script(
      "d.dispose(); "
      "interceptor.on('jsb.call',(e,c)=>c.mock({returnValue:7,callbacks:[{"
      "argumentIndex:0,args:[1]},{argumentIndex:2,args:[2]},"
      "{argumentIndex:0,args:[3]}]}))");
  result = provider_->Dispatch(InterceptKind::kCall, event);
  EXPECT_TRUE(result.failed);
  EXPECT_FALSE(result.mock.IsTable());
  EXPECT_FALSE(result.patch.IsTable());
}

TEST_F(HostScriptInterceptorTest, LoadTemplateUrlDoesNotUpdateStoredViewUrl) {
  const char* committed_url = "https://example.com/current";
  Interceptor::UpdateViewUrl(1, committed_url);
  Install();
  Script(R"(
    globalThis.snapshots = [];
    interceptor.on('view.loadTemplate', (e, c) => {
      snapshots.push(e);
      return c.proceed({url: 'https://example.com/redirected'});
    });
  )");
  for (const char* url :
       {"https://example.com/first", "https://example.com/next", ""}) {
    lepus::Value request(lepus::Dictionary::Create());
    request.SetProperty("url", lepus::Value(url));
    lepus::Value event(lepus::Dictionary::Create());
    event.SetProperty("viewId", lepus::Value("1"));
    event.SetProperty("request", request);
    auto result = provider_->Dispatch(InterceptKind::kLoadTemplate, event);
    EXPECT_FALSE(result.failed);
    EXPECT_EQ(EvalString("snapshots[snapshots.length - 1].viewURL"), url);
    EXPECT_EQ(Interceptor::ViewUrl(1), committed_url);
  }
  EXPECT_EQ(EvalString("snapshots[0].viewURL"), "https://example.com/first");
  Interceptor::ClearViewUrl(1);
}

TEST_F(HostScriptInterceptorTest,
       TracksCurrentViewUrlAcrossLoadsAndShellReplacement) {
  const char* url = "https://example.com/card?query=a%2Fb#entry";
  const char* next_url = "https://example.com/next";
  for (auto [domain, kind, name] :
       {std::tuple{Interceptor::Domain::kUI, InterceptKind::kUpdateMetaData,
                   "view.updateMetaData"},
        std::tuple{Interceptor::Domain::kBTS, InterceptKind::kCall,
                   "jsb.call"}}) {
    Install(domain);
    Script(("globalThis.urls = [];"
            "globalThis.snapshots = [];"
            "interceptor.on('" +
            std::string(name) + R"(', (e, c) => {
              const d = Object.getOwnPropertyDescriptor(e, 'viewURL');
              urls.push(d.enumerable && !d.writable && !d.configurable
                            ? e.viewURL : 'mutable');
              snapshots.push(e);
              return c.proceed();
            });)")
               .c_str());
    EXPECT_FALSE(Dispatch(kind).failed);
    EXPECT_EQ(EvalString("urls.pop()"), "");
    Interceptor::UpdateViewUrl(1, url);
    EXPECT_FALSE(Dispatch(kind).failed);
    EXPECT_EQ(EvalString("urls.pop()"), url);
    Interceptor::UpdateViewUrl(1, next_url);
    // Shell replacement preserves the latest URL associated with the View.
    Interceptor::RegisterView(1);
    Interceptor::DestroyView(1);
    EXPECT_FALSE(Dispatch(kind).failed);
    EXPECT_EQ(EvalString("urls.pop()"), next_url);
    EXPECT_EQ(EvalString("snapshots[1].viewURL"), url);
    Interceptor::UpdateViewUrl(1, "");
    EXPECT_FALSE(Dispatch(kind).failed);
    EXPECT_EQ(EvalString("urls.pop()"), "");
    Interceptor::UpdateViewUrl(1, next_url);
    Interceptor::ClearViewUrl(1);
    EXPECT_FALSE(Dispatch(kind).failed);
    EXPECT_EQ(EvalString("urls.pop()"), "");
    provider_->Uninstall();
    provider_.reset();
  }
}

TEST_F(HostScriptInterceptorTest, CreatePatchesValidateEveryField) {
  Install();
  lepus::Value request(lepus::Dictionary::Create());
  request.SetProperty("threadStrategy", lepus::Value(1));
  request.SetProperty("fontScale", lepus::Value(1));
  lepus::Value device(lepus::Dictionary::Create());
  device.SetProperty("width", lepus::Value(1080));
  device.SetProperty("height", lepus::Value(2400));
  request.SetProperty("deviceScreenSize", device);
  // A View under construction has no identity.
  lepus::Value event(lepus::Dictionary::Create());
  event.SetProperty("request", request);
  Script(R"(
    globalThis.patch = undefined;
    interceptor.on('view.create', (e, c, v) => {
      if (e.viewURL !== '' || v !== null || 'viewId' in e)
        throw Error('unexpected View identity');
      return c.proceed(patch);
    });
  )");
  Script(R"(patch = {threadStrategy: 3, enableJSRuntime: false,
      enableAirStrictMode: true, enableMTSModule: true,
      forceDarkAllowed: true, density: 2.5, fontScale: 1.5,
      presetMeasuredSpec: {width: 0x400002d0, height: -2147482368},
      screenSize: {width: 720, height: 1280}, lynxViewConfig: {a: 'b'},
      deviceScreenSize: {width: 1080, height: 2400}})");
  auto result = provider_->Dispatch(InterceptKind::kCreate, event);
  ASSERT_FALSE(result.failed);
  EXPECT_EQ(result.patch.GetProperty("threadStrategy").Number(), 3);
  EXPECT_EQ(result.patch.GetProperty("presetMeasuredSpec").GetLength(), 2);
  EXPECT_EQ(result.patch.GetProperty("presetMeasuredSpec")
                .GetProperty("height")
                .Number(),
            -2147482368.0);
  for (const char* invalid :
       {"{threadStrategy: 4}", "{threadStrategy: 1.5}",
        "{deviceScreenSize: {width: 1, height: 2400}}",
        "{presetMeasuredSpec: {widthMode: 'fill'}}", "{presetMeasuredSpec: {}}",
        "{screenSize: {width: 0, height: 1}}", "{screenSize: {width: 1}}",
        "{density: 0}", "{fontScale: -1}", "{enableJSRuntime: 1}",
        "{lynxViewConfig: {a: 1}}", "{backgroundColor: 0xff00ff00}",
        "{independentJSContext: true}"}) {
    Script(("patch = " + std::string(invalid)).c_str());
    EXPECT_TRUE(provider_->Dispatch(InterceptKind::kCreate, event).failed)
        << invalid;
  }
}

TEST_F(HostScriptInterceptorTest, ViewportPatchesAreValidatedPerField) {
  Install();
  Script(R"(
    globalThis.patch = undefined;
    interceptor.on('view.updateViewport', (e, c) => c.proceed(patch));
  )");
  Script("patch = {height: -2147482048}");
  auto result = Dispatch(InterceptKind::kUpdateViewport);
  ASSERT_FALSE(result.failed);
  EXPECT_EQ(result.patch.GetProperty("height").Number(), -2147482048.0);
  for (const char* invalid :
       {"{width: -1}", "{width: 2147483648}", "{width: 1.5}",
        "{widthMode: 'exact'}", "{height: '1'}", "{url: 'x'}"}) {
    Script(("patch = " + std::string(invalid)).c_str());
    EXPECT_TRUE(Dispatch(InterceptKind::kUpdateViewport).failed) << invalid;
  }
}

TEST_F(HostScriptInterceptorTest, ResourceFetchRedirectsOrAnswersWithBytes) {
  Install();
  Script(R"(
    globalThis.decide = undefined;
    interceptor.on('resource.fetch', (e, c) => decide(e.request, c));
  )");
  Script("decide = (r, c) => c.proceed({...r, url: r.url + '-patched'})");
  auto result = Dispatch(InterceptKind::kResourceFetch);
  ASSERT_FALSE(result.failed);
  EXPECT_EQ(result.patch.GetProperty("url").StdString(), "original-patched");
  Script("decide = (r, c) => c.mock({bytes: new Uint8Array([1, 2]).buffer})");
  result = Dispatch(InterceptKind::kResourceFetch);
  ASSERT_FALSE(result.failed);
  EXPECT_EQ(result.mock.GetProperty("bytes").ByteArray()->GetLength(), 2u);
  for (const char* invalid :
       {"(r, c) => c.mock({bytes: new ArrayBuffer(0)})",
        "(r, c) => c.mock({bytes: new ArrayBuffer(1), url: 'x'})",
        "(r, c) => c.mock({})", "(r, c) => c.proceed({type: 'lazyBundle'})",
        "(r, c) => c.proceed({url: ''})"}) {
    Script(("decide = " + std::string(invalid)).c_str());
    result = Dispatch(InterceptKind::kResourceFetch);
    EXPECT_TRUE(result.failed) << invalid;
    EXPECT_FALSE(result.mock.IsTable()) << invalid;
  }
}

TEST_F(HostScriptInterceptorTest, ResourceFetchRechecksViewAndHandlerOnUI) {
  Install(HostScriptInterceptor::Domain::kUI);
  Interceptor::RegisterView(7);
  Interceptor::UpdateViewUrl(7, "page");
  EXPECT_FALSE(Interceptor::HasResourceFetchHandler(7));
  Script(R"(
    globalThis.seen = [];
    globalThis.subscription = interceptor.on('resource.fetch', (e, c) => {
      seen.push([e.viewId, e.viewURL, e.request.url].join(' '));
      return c.proceed({url: 'target'});
    });
  )");
  // Checking from the requester must not enter the UI provider.
  std::thread requester([] {
    EXPECT_TRUE(Interceptor::HasResourceFetchHandler(7));
    EXPECT_FALSE(Interceptor::HasResourceFetchHandler(-1));
  });
  requester.join();
  auto result = Interceptor::DispatchResourceFetch(7, "original");
  EXPECT_EQ(result.patch.GetProperty("url").StdString(), "target");

  Interceptor::DestroyView(7);
  Interceptor::ClearViewUrl(7);
  EXPECT_FALSE(
      Interceptor::DispatchResourceFetch(7, "destroyed").patch.IsTable());
  Interceptor::RegisterView(7);
  Script("subscription.dispose()");
  EXPECT_FALSE(Interceptor::HasResourceFetchHandler(7));
  EXPECT_FALSE(
      Interceptor::DispatchResourceFetch(7, "disposed").patch.IsTable());
  EXPECT_EQ(EvalString("seen.join(',')"), "7 page original");
  Interceptor::DestroyView(7);
}

TEST_F(HostScriptInterceptorTest, TracksExistingLogContextIDsIncludingZero) {
  Interceptor::RegisterView(-1);
  EXPECT_FALSE(Interceptor::IsViewAlive(-1));
  Interceptor::RegisterView(0);
  Interceptor::RegisterView(42);
  // A replacement engine for the same View may register before the old one
  // is destroyed.
  Interceptor::RegisterView(0);
  EXPECT_TRUE(Interceptor::IsViewAlive(0));
  Interceptor::DestroyView(0);
  EXPECT_TRUE(Interceptor::IsViewAlive(0));
  Interceptor::DestroyView(0);
  EXPECT_FALSE(Interceptor::IsViewAlive(0));
  Interceptor::DestroyView(0);
  EXPECT_FALSE(Interceptor::IsViewAlive(0));
  EXPECT_TRUE(Interceptor::IsViewAlive(42));
  Interceptor::DestroyView(42);
}

TEST_F(HostScriptInterceptorTest,
       UiChainMergesIncrementalNextPatchesAndPreservesTemplateBytes) {
  Install();
  lepus::Value source(lepus::Dictionary::Create());
  source.SetProperty("kind", lepus::Value("url"));
  lepus::Value request(lepus::Dictionary::Create());
  request.SetProperty("url", lepus::Value("https://example.com/card-a"));
  request.SetProperty("source", source);
  lepus::Value event(lepus::Dictionary::Create());
  event.SetProperty("viewId", lepus::Value("1"));
  event.SetProperty("origin", lepus::Value("host"));
  event.SetProperty("request", request);
  Script(R"(
    globalThis.seen = [];
    interceptor.on('view.loadTemplate', (e, c) => {
      seen.push(['s1', e.request.source.kind, e.request.initialData]);
      return c.next({
        source: {kind: 'bytes', bytes: new Uint8Array([7, 8, 9]).buffer},
        initialData: {step: 1},
      });
    });
    interceptor.on('view.loadTemplate', (e, c) => {
      seen.push([
        's2',
        e.request.source.kind,
        e.request.source.size,
        'bytes' in e.request.source,
        e.request.initialData.step,
      ]);
      // Spreading e.request passes the public source descriptor unchanged;
      // native must retain the binary replacement from slot 1.
      return c.next({...e.request, globalProps: {theme: 'dark'}});
    });
    globalThis.stopper = interceptor.on('view.loadTemplate', (e, c) => {
      seen.push(['s3', e.request.globalProps.theme]);
      return c.proceed({url: 'https://example.com/card-a-v2'});
    });
    interceptor.on('view.loadTemplate', (_e, c) => {
      seen.push(['s4-skipped']);
      return c.next({globalProps: {theme: 'light'}});
    });
  )");
  auto result = provider_->Dispatch(InterceptKind::kLoadTemplate, event);
  ASSERT_FALSE(result.failed);
  EXPECT_EQ(EvalString("JSON.stringify(seen)"),
            R"([["s1","url",null],["s2","bytes",3,false,1],["s3","dark"]])");
  EXPECT_EQ(result.patch.GetProperty("url").StdString(),
            "https://example.com/card-a-v2");
  EXPECT_EQ(
      result.patch.GetProperty("initialData").GetProperty("step").Number(), 1);
  EXPECT_EQ(
      result.patch.GetProperty("globalProps").GetProperty("theme").StdString(),
      "dark");
  auto patched_source = result.patch.GetProperty("source");
  EXPECT_EQ(patched_source.GetProperty("kind").StdString(), "bytes");
  auto bytes = patched_source.GetProperty("bytes").ByteArray();
  ASSERT_NE(bytes, nullptr);
  ASSERT_EQ(bytes->GetLength(), 3u);
  EXPECT_EQ(bytes->GetPtr()[0], 7);
  EXPECT_EQ(bytes->GetPtr()[2], 9);
  // Caller input remains unmutated.
  EXPECT_EQ(request.GetProperty("url").StdString(),
            "https://example.com/card-a");
  EXPECT_EQ(request.GetProperty("source").GetProperty("kind").StdString(),
            "url");

  // Disposing the proceed stopper lets the chain flow to slot 4 and commit
  // all accumulated next() patches at the end of the chain.
  Script("seen = []; stopper.dispose();");
  auto full_chain = provider_->Dispatch(InterceptKind::kLoadTemplate, event);
  ASSERT_FALSE(full_chain.failed);
  EXPECT_EQ(full_chain.patch.GetProperty("globalProps")
                .GetProperty("theme")
                .StdString(),
            "light");
  EXPECT_EQ(full_chain.patch.GetProperty("source")
                .GetProperty("bytes")
                .ByteArray()
                ->GetLength(),
            3u);
}

TEST_F(HostScriptInterceptorTest,
       UiChainSupportsCreateViewportUpdateAndResourceFetch) {
  Install();
  Script(R"(
    interceptor.on('view.create', (_e, c) =>
      c.next({threadStrategy: 3, lynxViewConfig: {a: '1', shared: 'first'}}));
    interceptor.on('view.create', (e, c) => {
      if (e.request.threadStrategy !== 3 || e.request.lynxViewConfig.a !== '1')
        throw Error('create patch did not propagate');
      return c.next({fontScale: 1.5, lynxViewConfig: {b: '2', shared: 'second'}});
    });
    interceptor.on('view.updateMetaData', (_e, c) =>
      c.next({updateData: {count: 2}}));
    interceptor.on('view.updateMetaData', (_e, c) =>
      c.next({globalProps: {flag: true}}));
    interceptor.on('view.updateViewport', (_e, c) => c.next({width: 0x400002d0}));
    interceptor.on('view.updateViewport', (_e, c) => c.next({height: 0x40000500}));
    globalThis.mockResource = false;
    interceptor.on('resource.fetch', (e, c) =>
      c.next({url: e.request.url + '?step=1'}));
    interceptor.on('resource.fetch', (e, c) =>
      mockResource
        ? c.mock({bytes: new Uint8Array([4, 5]).buffer})
        : c.next({url: e.request.url + '&step=2'}));
  )");

  lepus::Value create_req(lepus::Dictionary::Create());
  create_req.SetProperty("threadStrategy", lepus::Value(0));
  create_req.SetProperty("fontScale", lepus::Value(1.0));
  lepus::Value create_event(lepus::Dictionary::Create());
  create_event.SetProperty("request", create_req);
  auto created = provider_->Dispatch(InterceptKind::kCreate, create_event);
  ASSERT_FALSE(created.failed);
  EXPECT_EQ(created.patch.GetProperty("threadStrategy").Number(), 3);
  EXPECT_EQ(created.patch.GetProperty("fontScale").Number(), 1.5);
  auto cfg = created.patch.GetProperty("lynxViewConfig");
  EXPECT_EQ(cfg.GetProperty("a").StdString(), "1");
  EXPECT_EQ(cfg.GetProperty("b").StdString(), "2");
  EXPECT_EQ(cfg.GetProperty("shared").StdString(), "second");

  auto updated = Dispatch(InterceptKind::kUpdateMetaData);
  ASSERT_FALSE(updated.failed);
  EXPECT_EQ(
      updated.patch.GetProperty("updateData").GetProperty("count").Number(), 2);
  EXPECT_TRUE(
      updated.patch.GetProperty("globalProps").GetProperty("flag").Bool());

  auto viewport = Dispatch(InterceptKind::kUpdateViewport);
  ASSERT_FALSE(viewport.failed);
  EXPECT_EQ(viewport.patch.GetProperty("width").Number(), 0x400002d0);
  EXPECT_EQ(viewport.patch.GetProperty("height").Number(), 0x40000500);

  auto redirected = Dispatch(InterceptKind::kResourceFetch);
  ASSERT_FALSE(redirected.failed);
  EXPECT_EQ(redirected.patch.GetProperty("url").StdString(),
            "original?step=1&step=2");
  Script("mockResource = true;");
  auto mocked = Dispatch(InterceptKind::kResourceFetch);
  ASSERT_FALSE(mocked.failed);
  EXPECT_FALSE(mocked.patch.IsTable());
  EXPECT_EQ(mocked.mock.GetProperty("bytes").ByteArray()->GetLength(), 2u);
}

TEST_F(HostScriptInterceptorTest,
       NativeScopeBypassesNonMatchingHandlersWithoutCallingJS) {
  Install(Interceptor::Domain::kUI);
  Script(R"(
    globalThis.invocations = [];
    // If invoked for an out-of-scope View, this throws and fails dispatch.
    interceptor.on('view.loadTemplate', {viewId: '1'}, () => {
      invocations.push('wrong-view');
      throw Error('out-of-scope viewId ran');
    });
    interceptor.on('view.loadTemplate',
      {urlIncludes: 'weather', origin: 'host'},
      (_e, c) => {
        invocations.push('weather-1');
        return c.next({url: 'http://127.0.0.1/local.bin', initialData: {w: 1}});
      });
    // viewURL stays anchored to the load's initial URL across the chain,
    // so a second subscriber with the same urlIncludes still matches.
    interceptor.on('view.loadTemplate',
      {urlPrefix: 'https://cdn.example.com/', urlIncludes: 'weather', origin: 'host'},
      (_e, c) => {
        invocations.push('weather-2');
        return c.next({globalProps: {w: 2}});
      });
  )");
  auto make_load = [](const char* view_id, const char* url,
                      const char* origin) {
    lepus::Value req(lepus::Dictionary::Create());
    req.SetProperty("url", lepus::Value(url));
    lepus::Value ev(lepus::Dictionary::Create());
    ev.SetProperty("viewId", lepus::Value(view_id));
    ev.SetProperty("origin", lepus::Value(origin));
    ev.SetProperty("request", req);
    return ev;
  };
  const auto before_failures = Interceptor::FailureCount();
  // Unrelated card passes straight through without calling any JS handler.
  auto other = provider_->Dispatch(
      InterceptKind::kLoadTemplate,
      make_load("2", "https://cdn.example.com/news/card.js", "host"));
  EXPECT_FALSE(other.failed);
  EXPECT_FALSE(other.patch.IsTable());
  // Script-originated load skips origin:'host' handlers.
  auto from_script = provider_->Dispatch(
      InterceptKind::kLoadTemplate,
      make_load("2", "https://cdn.example.com/weather/card.js", "host-script"));
  EXPECT_FALSE(from_script.failed);
  EXPECT_FALSE(from_script.patch.IsTable());
  EXPECT_EQ(EvalString("invocations.length"), "0");

  auto matched = provider_->Dispatch(
      InterceptKind::kLoadTemplate,
      make_load("2", "https://cdn.example.com/weather/card.js", "host"));
  ASSERT_FALSE(matched.failed);
  EXPECT_EQ(EvalString("invocations.join(',')"), "weather-1,weather-2");
  EXPECT_EQ(matched.patch.GetProperty("url").StdString(),
            "http://127.0.0.1/local.bin");
  EXPECT_EQ(matched.patch.GetProperty("initialData").GetProperty("w").Number(),
            1);
  EXPECT_EQ(matched.patch.GetProperty("globalProps").GetProperty("w").Number(),
            2);
  EXPECT_EQ(Interceptor::FailureCount(), before_failures);
  provider_->Uninstall();
  provider_.reset();

  // BTS module & method scopes.
  Install(Interceptor::Domain::kBTS);
  Interceptor::UpdateViewUrl(9, "https://cdn.example.com/profile");
  Script(R"(
    globalThis.jsbHits = [];
    interceptor.on('jsb.call', {module: 'other'}, () => {
      throw Error('wrong module ran');
    });
    interceptor.on('jsb.call',
      {viewId: '9', urlIncludes: 'profile', module: 'storage', method: 'getItem'},
      (_e, c) => {
        jsbHits.push('hit');
        return c.mock({returnValue: 'cached'});
      });
  )");
  auto make_call = [](const char* view, const char* mod, const char* method) {
    lepus::Value ev(lepus::Dictionary::Create());
    ev.SetProperty("viewId", lepus::Value(view));
    ev.SetProperty("module", lepus::Value(mod));
    ev.SetProperty("method", lepus::Value(method));
    ev.SetProperty("args", lepus::Value(lepus::CArray::Create()));
    ev.SetProperty("callbackIndices", lepus::Value(lepus::CArray::Create()));
    ev.SetProperty("opaqueIndices", lepus::Value(lepus::CArray::Create()));
    return ev;
  };
  EXPECT_FALSE(
      provider_
          ->Dispatch(InterceptKind::kCall, make_call("9", "storage", "setItem"))
          .mock.IsTable());
  EXPECT_FALSE(
      provider_
          ->Dispatch(InterceptKind::kCall, make_call("8", "storage", "getItem"))
          .mock.IsTable());
  auto jsb_matched = provider_->Dispatch(InterceptKind::kCall,
                                         make_call("9", "storage", "getItem"));
  ASSERT_FALSE(jsb_matched.failed);
  EXPECT_EQ(jsb_matched.mock.GetProperty("returnValue").StdString(), "cached");
  EXPECT_EQ(EvalString("jsbHits.join(',')"), "hit");
  Interceptor::ClearViewUrl(9);
}

TEST_F(HostScriptInterceptorTest,
       PriorityOrdersHandlersDescendingAndPreservesFifoTies) {
  Install(Interceptor::Domain::kBTS);
  Script(R"(
    globalThis.order = [];
    interceptor.on('jsb.callback', (_e, c) => {
      order.push('p0#1');
      return c.next();
    });
    interceptor.on('jsb.callback', {priority: -10}, (e, c) => {
      order.push('p-10:' + e.args[0]);
      return c.next();
    });
    interceptor.on('jsb.callback', {priority: 10}, async (_e, c) => {
      order.push('p10#1');
      await Promise.resolve();
      return c.next({args: ['from-p10#1']});
    });
    interceptor.on('jsb.callback', (_e, c) => {
      order.push('p0#2');
      return c.next();
    });
    interceptor.on('jsb.callback', {priority: 10}, (e, c) => {
      order.push('p10#2:' + e.args[0]);
      return c.next({args: ['from-p10#2']});
    });
  )");
  lepus::Value event(lepus::Dictionary::Create());
  auto args = lepus::CArray::Create();
  args->push_back(lepus::Value("initial"));
  event.SetProperty("args", lepus::Value(args));
  auto result = provider_->Dispatch(InterceptKind::kCallback, event);
  ASSERT_TRUE(result.pending);
  bool completed = false;
  result.pending([&](InterceptResult settled) {
    completed = true;
    ASSERT_FALSE(settled.failed);
    EXPECT_EQ(settled.patch.GetProperty("args").GetProperty(0u).StdString(),
              "from-p10#2");
  });
  DrainJobs();
  EXPECT_TRUE(completed);
  EXPECT_EQ(EvalString("order.join(',')"),
            "p10#1,p10#2:from-p10#1,p0#1,p0#2,p-10:from-p10#2");
}

TEST_F(HostScriptInterceptorTest,
       LaterInvalidDecisionRollsBackEarlierNextPatches) {
  Install();
  lepus::Value request(lepus::Dictionary::Create());
  request.SetProperty("url", lepus::Value("https://example.com/original"));
  lepus::Value event(lepus::Dictionary::Create());
  event.SetProperty("request", request);
  Script(R"(
    interceptor.on('view.loadTemplate', (_e, c) =>
      c.next({url: 'https://example.com/step1', initialData: {ok: true}}));
    interceptor.on('view.loadTemplate', () => { throw Error('boom'); });
  )");
  auto result = provider_->Dispatch(InterceptKind::kLoadTemplate, event);
  EXPECT_TRUE(result.failed);
  EXPECT_FALSE(result.patch.IsTable());
  EXPECT_EQ(request.GetProperty("url").StdString(),
            "https://example.com/original");
}

TEST_F(HostScriptInterceptorTest, RejectsInvalidOptionsAtRegistrationTime) {
  Install(Interceptor::Domain::kUI);
  Script(R"(
    // Explicit undefined options and fields are accepted.
    interceptor.on('view.loadTemplate', undefined, (_e, c) => c.next()).dispose();
    interceptor.on('view.loadTemplate',
      {viewId: undefined, priority: undefined, origin: undefined},
      (_e, c) => c.next()).dispose();
    const invalidCalls = [
      () => interceptor.on('view.loadTemplate'),
      () => interceptor.on('view.loadTemplate', {}, (_e, c) => c.next(), 1),
      () => interceptor.on('view.loadTemplate', null, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', [], (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', new Date(), (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {unknown: 1}, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {priority: NaN}, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {priority: Infinity}, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {priority: '1'}, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {viewId: ''}, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {urlIncludes: ''}, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {urlPrefix: ''}, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {origin: 'page'}, (_e, c) => c.next()),
      () => interceptor.on('view.loadTemplate', {module: 'bridge'}, (_e, c) => c.next()),
      () => interceptor.on('view.create', {viewId: '1'}, (_e, c) => c.next()),
      () => interceptor.on('view.updateViewport', {origin: 'host'}, (_e, c) => c.next()),
    ];
    for (let i = 0; i < invalidCalls.length; ++i) {
      let threw = false;
      try { invalidCalls[i](); } catch (e) { threw = e instanceof TypeError; }
      if (!threw) throw Error('expected TypeError at index ' + i);
    }
  )");
  provider_->Uninstall();
  provider_.reset();

  Install(Interceptor::Domain::kBTS);
  Script(R"(
    for (const call of [
      () => interceptor.on('jsb.call', {origin: 'host'}, (_e, c) => c.next()),
      () => interceptor.on('jsb.call', {module: ''}, (_e, c) => c.next()),
      () => interceptor.on('jsb.call', {method: ''}, (_e, c) => c.next()),
      () => interceptor.on('jsb.resolveModule', {method: 'm'}, (_e, c) => c.next()),
    ]) {
      let threw = false;
      try { call(); } catch (e) { threw = e instanceof TypeError; }
      if (!threw) throw Error('expected BTS TypeError');
    }
  )");
}

}  // namespace
}  // namespace shell
}  // namespace lynx
