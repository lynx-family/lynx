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
       "[['view.created','view.loadTemplate','view.updateMetaData'],"
       "['view.create','jsb.call','jsb.result','jsb.callback']]"},
      {Interceptor::Domain::kBTS,
       "[['view.created','jsb.call','jsb.result','jsb.callback'],"
       "['view.create','view.loadTemplate','view.updateMetaData']]"},
      {Interceptor::Domain::kMTS,
       "[['view.created'],['view.create','view.loadTemplate',"
       "'view.updateMetaData','jsb.call','jsb.result','jsb.callback']]"}};
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

TEST_F(HostScriptInterceptorTest, RejectsDuplicateHandlersForEachKind) {
  for (auto domain : {Interceptor::Domain::kUI, Interceptor::Domain::kBTS}) {
    Install(domain);
    const char* names =
        domain == Interceptor::Domain::kUI
            ? "['view.created','view.loadTemplate',"
              "'view.updateMetaData']"
            : "['view.created','jsb.call','jsb.result','jsb.callback']";
    std::string script = R"(
      globalThis.calls = [];
      globalThis.names = )" +
                         std::string(names) + R"(;
      for (const name of names) {
        // Without View bindings every proxy is null and creation never fires.
        interceptor.on(name, (e,c,v) => {
          calls.push(v === null ? name : 'proxy');
          return c.proceed();
        });
        let message = '';
        try { interceptor.on(name, () => { throw Error('duplicate ran'); }); }
        catch (e) { message = e.message; }
        if (!message.includes('already registered'))
          throw Error('duplicate registration was not rejected');
      }
    )";
    Script(script.c_str());
    for (size_t i = 0; i < static_cast<size_t>(InterceptKind::kCount); ++i) {
      auto kind = static_cast<InterceptKind>(i);
      if (provider_->HasHandlers(kind)) EXPECT_FALSE(Dispatch(kind).failed);
    }
    EXPECT_EQ(EvalString("calls.join(',')"),
              EvalString("names.slice(1).join(',')"));
    provider_->Uninstall();
    provider_.reset();
  }
}

TEST_F(HostScriptInterceptorTest, ProceedCommitsSingleHandlerPatch) {
  Install();
  Script(R"(
    interceptor.on('view.loadTemplate', (e,c) => {
      if ('next' in c || Object.keys(c).sort().join() !== 'mock,proceed')
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

TEST_F(HostScriptInterceptorTest, RejectsRemovedNextDecision) {
  Install();
  Script(R"(
    interceptor.on('view.loadTemplate', () => ({action:'next', payload:{}}));
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
    LEPUSContext* pending_context = nullptr;
    int status;
    while ((status = LEPUS_ExecutePendingJob(runtime_, &pending_context)) > 0) {
    }
    ASSERT_EQ(status, 0);
    EXPECT_TRUE(completed);
  }
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
    for (const [fn, action] of [[savedProceed,'proceed'], [savedMock,'mock']]) {
      const payload = {value:7};
      const result = fn(payload);
      if (result.action !== action || result.payload !== payload)
        throw Error('invalid saved decision');
      if (Object.keys(fn().payload).length !== 0)
        throw Error('invalid default payload');
    }
    delete globalThis.savedProceed;
    delete globalThis.savedMock;
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

TEST_F(HostScriptInterceptorTest, ExposesInitialViewUrlUntilCleared) {
  const char* url = "https://example.com/card?query=a%2Fb#entry";
  Interceptor::BindInitialViewUrl(1, "");
  EXPECT_TRUE(Interceptor::InitialViewUrl(1).empty());
  Interceptor::BindInitialViewUrl(1, url);
  Interceptor::BindInitialViewUrl(1, "https://example.com/next");
  // The URL outlives the native shells registered for the View.
  Interceptor::RegisterView(1);
  Interceptor::DestroyView(1);
  for (auto [domain, kind, name] :
       {std::tuple{Interceptor::Domain::kUI, InterceptKind::kLoadTemplate,
                   "view.loadTemplate"},
        std::tuple{Interceptor::Domain::kBTS, InterceptKind::kCall,
                   "jsb.call"}}) {
    Install(domain);
    Script(("globalThis.urls = [];"
            "interceptor.on('" +
            std::string(name) + R"(', (e, c) => {
              const d = Object.getOwnPropertyDescriptor(e, 'viewURL');
              urls.push(d.enumerable && !d.writable && !d.configurable
                            ? e.viewURL : 'mutable');
              return c.proceed();
            });)")
               .c_str());
    EXPECT_FALSE(Dispatch(kind).failed);
    EXPECT_EQ(EvalString("urls.join()"), url);
    provider_->Uninstall();
    provider_.reset();
  }
  Interceptor::ClearInitialViewUrl(1);
  EXPECT_TRUE(Interceptor::InitialViewUrl(1).empty());
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

}  // namespace
}  // namespace shell
}  // namespace lynx
