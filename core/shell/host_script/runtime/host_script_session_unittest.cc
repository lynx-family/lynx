// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_session.h"

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/common/napi/napi_runtime_proxy.h"
#include "core/runtime/common/napi/napi_runtime_proxy_quickjs.h"
#include "core/shell/host_script/runtime/host_script_interceptor.h"
#include "core/shell/host_script/runtime/host_script_module.h"
#include "quickjs/include/quickjs.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace {

using runtime::js::NapiEnvironment;
using runtime::js::NapiRuntimeProxy;
using runtime::js::NapiRuntimeProxyQuickjs;

struct FakeProxyState {
  bool accepts = true;
  std::vector<std::string> calls;
  std::string global_props;
  LynxViewRefLoadTemplateRequest load_template;
  LynxViewRefSsrRequest load_ssr;
  LynxViewRefSsrRequest hydrate_ssr;
  LynxViewRefUpdateMetaDataRequest update_data;
  LynxViewRefReloadTemplateRequest reload_template;
  LynxViewRefGlobalEventRequest global_event;
  int invalidate_count = 0;
};

class FakeLynxViewRefProxy final : public LynxViewRefProxy {
 public:
  explicit FakeLynxViewRefProxy(std::shared_ptr<FakeProxyState> state)
      : state_(std::move(state)) {}

  bool LoadTemplate(LynxViewRefLoadTemplateRequest request) override {
    state_->load_template = std::move(request);
    return Record("loadTemplate");
  }

  bool LoadSSR(LynxViewRefSsrRequest request) override {
    state_->load_ssr = std::move(request);
    return Record("loadSSR");
  }

  bool HydrateSSR(LynxViewRefSsrRequest request) override {
    state_->hydrate_ssr = std::move(request);
    return Record("hydrateSSR");
  }

  bool UpdateMetaData(LynxViewRefUpdateMetaDataRequest request) override {
    state_->update_data = std::move(request);
    return Record("updateMetaData");
  }

  bool ReloadTemplate(LynxViewRefReloadTemplateRequest request) override {
    state_->reload_template = std::move(request);
    return Record("reloadTemplate");
  }

  bool SendGlobalEvent(LynxViewRefGlobalEventRequest request) override {
    state_->global_event = std::move(request);
    return Record("sendGlobalEvent");
  }

  bool SetGlobalProps(std::string global_props) override {
    state_->global_props = std::move(global_props);
    return Record("setGlobalProps");
  }

  void Invalidate() override { ++state_->invalidate_count; }

 private:
  bool Record(std::string name) {
    state_->calls.push_back(std::move(name));
    return state_->accepts;
  }

  std::shared_ptr<FakeProxyState> state_;
};

class ModuleDelegate final : public NapiEnvironment::Delegate {
 public:
  void RegisterModule(const std::string& name,
                      std::unique_ptr<Module> module) override {
    modules_[name] = std::move(module);
  }

  Module* GetModule(const std::string& name) override {
    auto found = modules_.find(name);
    return found == modules_.end() ? nullptr : found->second.get();
  }

  void OnDetach(Napi::Env env) override {
    for (auto& entry : modules_) {
      entry.second->OnEnvDetach(env);
    }
  }

 private:
  std::unordered_map<std::string, std::unique_ptr<Module>> modules_;
};

class QueuedTaskRunner final : public fml::TaskRunner {
 public:
  QueuedTaskRunner() : fml::TaskRunner(nullptr) {}
  void PostTask(base::closure closure) override {
    tasks_.push_back(std::move(closure));
  }

  bool DrainTasks() {
    if (tasks_.empty()) {
      return false;
    }
    auto tasks = std::move(tasks_);
    tasks_.clear();
    for (auto& task : tasks) {
      task();
    }
    return true;
  }

 private:
  std::vector<base::closure> tasks_;
};

class HostScriptSessionTest : public ::testing::Test {
 public:
  HostScriptSessionTest()
      : runtime_(LEPUS_NewRuntime()),
        context_(LEPUS_NewContext(runtime_)),
        runtime_proxy_(static_cast<NapiRuntimeProxy*>(
            NapiRuntimeProxyQuickjs::Create(
                context_,
                std::make_shared<runtime::js::DelegateObserver>(task_runner_))
                .release())),
        delegate_(new ModuleDelegate()),
        environment_(std::unique_ptr<ModuleDelegate>(delegate_)),
        env_(runtime_proxy_->Env()) {
    environment_.SetRuntimeProxy(std::move(runtime_proxy_));
    environment_.Attach();
  }

  ~HostScriptSessionTest() override {
    if (session_) {
      session_->Detach();
    }
    environment_.Detach();
    session_.reset();
    LEPUS_FreeContext(context_);
    LEPUS_FreeRuntime(runtime_);
  }

 protected:
  void CreateSession(bool attach_first) {
    state_ = std::make_shared<FakeProxyState>();
    session_ = HostScriptSession::Create();
    if (attach_first) {
      EXPECT_TRUE(session_->Attach(env_));
    }
    EXPECT_TRUE(
        session_->BindView(std::make_unique<FakeLynxViewRefProxy>(state_)));
    if (!attach_first) {
      EXPECT_TRUE(session_->Attach(env_));
    }
  }

  void LoadModule() {
    auto* module = delegate_->GetModule("host_script");
    ASSERT_NE(module, nullptr);
    Napi::Object target = Napi::Object::New(env_);
    module->OnLoad(target);
    env_.Global().Set("viewRef", target);
  }

  void PumpJobs() {
    bool did_work = false;
    do {
      did_work = task_runner_->DrainTasks();
      LEPUSContext* pending_context = nullptr;
      while (LEPUS_ExecutePendingJob(runtime_, &pending_context) > 0) {
        did_work = true;
      }
    } while (did_work);
  }

  std::string EvalString(const char* script) {
    return env_.RunScript(script).ToString().Utf8Value();
  }

  LEPUSRuntime* runtime_;
  LEPUSContext* context_;
  fml::RefPtr<QueuedTaskRunner> task_runner_ =
      fml::MakeRefCounted<QueuedTaskRunner>();
  std::unique_ptr<NapiRuntimeProxy> runtime_proxy_;
  ModuleDelegate* delegate_;
  NapiEnvironment environment_;
  Napi::Env env_;
  std::shared_ptr<FakeProxyState> state_;
  std::shared_ptr<HostScriptSession> session_;
};

class HostScriptInterceptorTest : public HostScriptSessionTest {
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
        interceptor.on(name, (e,c) => { calls.push(name); return c.proceed(); });
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
    EXPECT_EQ(EvalString("calls.join(',')"), EvalString("names.join(',')"));
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

TEST_F(HostScriptInterceptorTest,
       InvalidNestedDataRollsBackAndClearsExceptions) {
  const char* invalid[] = {
      "() => {}",
      "Promise.resolve(1)",
      "new Date(0)",
      "new Uint8Array(1)",
      "Object.create({n:1})",
      "NaN",
      "Infinity",
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

TEST_F(HostScriptInterceptorTest, CreatedNotificationsIgnoreReturnValues) {
  for (auto domain : {Interceptor::Domain::kUI, Interceptor::Domain::kBTS,
                      Interceptor::Domain::kMTS}) {
    Install(domain);
    Script(R"(
      globalThis.created = false;
      interceptor.on('view.created', e => {
        if (e.viewId !== '1' || e.url !== '' || 'request' in e)
          throw Error('creation snapshot');
        created = true;
        return Promise.resolve({ignored:true});
      });
    )");
    auto result = Dispatch(InterceptKind::kCreated);
    EXPECT_FALSE(result.failed);
    EXPECT_FALSE(result.patch.IsTable());
    EXPECT_FALSE(result.mock.IsTable());
    EXPECT_EQ(EvalString("String(created)"), "true");
    provider_->Uninstall();
    provider_.reset();
  }
}

TEST_F(HostScriptInterceptorTest, CreatedNotificationsAllowReentrantDelivery) {
  Install();
  env_.Global().Set(
      "notifyCreated",
      Napi::Function::New(
          env_,
          [](const Napi::CallbackInfo& info) -> Napi::Value {
            auto* provider = static_cast<HostScriptInterceptor*>(info.Data());
            lepus::Value event(lepus::Dictionary::Create());
            EXPECT_FALSE(
                provider->Dispatch(InterceptKind::kCreated, event).failed);
            return info.Env().Undefined();
          },
          "notifyCreated", provider_.get()));
  Script(R"(
    globalThis.created = 0;
    interceptor.on('view.created', () => {
      if (++created === 1) notifyCreated();
    });
  )");
  EXPECT_FALSE(Dispatch(InterceptKind::kCreated).failed);
  EXPECT_EQ(EvalString("String(created)"), "2");
  env_.Global().Delete("notifyCreated");
}

TEST_F(HostScriptSessionTest, SupportsBothAttachAndBindOrders) {
  for (bool attach_first : {true, false}) {
    state_ = std::make_shared<FakeProxyState>();
    session_ = HostScriptSession::Create();
    if (attach_first) {
      ASSERT_TRUE(session_->Attach(env_));
    } else {
      ASSERT_TRUE(
          session_->BindView(std::make_unique<FakeLynxViewRefProxy>(state_)));
    }
    if (!attach_first) {
      ASSERT_TRUE(session_->Attach(env_));
    }
    env_.RunScript(R"(
      globalThis.readyCount = 0;
      globalThis.waiterResolved = false;
      globalThis.readyListener = () => ++readyCount;
    )");
    session_->AddListener(
        "ready", env_.Global().Get("readyListener").As<Napi::Function>());
    env_.Global().Set("waiter", session_->WaitForCurrentView(env_));
    env_.RunScript("waiter.then(() => waiterResolved = true)");
    if (attach_first) {
      ASSERT_TRUE(
          session_->BindView(std::make_unique<FakeLynxViewRefProxy>(state_)));
    }
    PumpJobs();

    EXPECT_TRUE(session_->HasCurrentView());
    EXPECT_EQ(EvalString("String(readyCount)"), "1");
    EXPECT_EQ(EvalString("String(waiterResolved)"), "true");
    session_->Detach();
    EXPECT_FALSE(session_->HasCurrentView());
    session_.reset();
  }
}

TEST_F(HostScriptSessionTest, CopiesAndAcceptsOperationsWithoutPromises) {
  CreateSession(true);
  LoadModule();
  EXPECT_EQ(EvalString(R"(
    [viewRef.loadTemplate(new Uint8Array([1,2,3]).buffer, 'lynx://page',
                         '{"initial":1}', '{"global":2}', 'processor', true),
     viewRef.loadSSR(new Uint8Array([4,5]).buffer, 'lynx://ssr', '{}'),
     viewRef.hydrateSSR(new Uint8Array([6]).buffer, 'lynx://hydrate', '{}'),
     viewRef.updateMetaData('{"update":1}', '{"props":2}'),
     viewRef.reloadTemplate('{"reload":1}', '{"props":1}'),
     viewRef.sendGlobalEvent('event', '[1,2]'),
     viewRef.setGlobalProps('{"theme":"dark"}')].every(v => v === true)
  )"),
            "true");
  EXPECT_EQ(state_->calls.size(), 7u);
  EXPECT_EQ(state_->load_template.template_data,
            (std::vector<uint8_t>{1, 2, 3}));
  EXPECT_EQ(state_->load_template.url, "lynx://page");
  EXPECT_EQ(state_->load_template.initial_data_json, "{\"initial\":1}");
  EXPECT_EQ(state_->load_template.global_props_json, "{\"global\":2}");
  EXPECT_EQ(state_->load_template.processor_name, "processor");
  EXPECT_TRUE(state_->load_template.read_only);
  EXPECT_EQ(state_->load_ssr.data, (std::vector<uint8_t>{4, 5}));
  EXPECT_EQ(state_->hydrate_ssr.data, (std::vector<uint8_t>{6}));
  EXPECT_TRUE(state_->update_data.has_data);
  EXPECT_TRUE(state_->update_data.has_global_props);
  EXPECT_EQ(state_->update_data.data_json, "{\"update\":1}");
  EXPECT_EQ(state_->update_data.global_props_json, "{\"props\":2}");
  EXPECT_EQ(state_->global_event.params_json, "[1,2]");
  EXPECT_EQ(state_->global_props, "{\"theme\":\"dark\"}");
}

TEST_F(HostScriptSessionTest, PreservesAbsentMetadataAndDoesNotCacheUpdates) {
  CreateSession(true);
  LoadModule();
  EXPECT_EQ(EvalString("viewRef.updateMetaData(undefined, '{}')"), "true");
  EXPECT_FALSE(state_->update_data.has_data);
  EXPECT_TRUE(state_->update_data.has_global_props);
  EXPECT_EQ(EvalString("viewRef.updateMetaData('{}')"), "true");
  EXPECT_TRUE(state_->update_data.has_data);
  EXPECT_FALSE(state_->update_data.has_global_props);
  EXPECT_EQ(state_->calls.size(), 2u);
  EXPECT_EQ(EvalString("viewRef.loadURL('lynx://url', '{}', '{}')"), "true");
  EXPECT_FALSE(state_->load_template.has_template);
  EXPECT_EQ(state_->load_template.url, "lynx://url");
  state_->accepts = false;
  EXPECT_EQ(EvalString("viewRef.updateMetaData('{}')"), "false");
  EXPECT_EQ(EvalString("viewRef.sendGlobalEvent('event', '[]')"), "false");
}

TEST_F(HostScriptSessionTest, ReportsAsyncErrorsAndDestroysTargetOnce) {
  CreateSession(true);
  LoadModule();
  env_.RunScript(R"(
    globalThis.events = [];
    viewRef.on('error', (code, message) => events.push('error:' + code + ':' + message));
    viewRef.on('destroyed', () => events.push('destroyed'));
  )");
  EXPECT_EQ(EvalString("viewRef.updateMetaData('{}')"), "true");
  session_->Notify("error", -1, "platform failed");
  session_->Notify("destroyed");
  session_->Notify("destroyed");
  session_->Notify("error", -1, "late");
  PumpJobs();
  EXPECT_EQ(EvalString("events.join(',')"),
            "error:-1:platform failed,destroyed");
  EXPECT_EQ(state_->invalidate_count, 1);
  EXPECT_EQ(EvalString("viewRef.updateMetaData('{}')"), "false");
}

TEST_F(HostScriptSessionTest, OrdersLifecycleEventsAndIgnoresLateEvents) {
  CreateSession(true);
  LoadModule();
  env_.RunScript(R"(
    globalThis.events = [];
    ['loadSuccess', 'firstScreen', 'pageUpdate', 'dataUpdated', 'destroyed']
      .forEach(name => viewRef.on(name, () => events.push(name)));
  )");

  session_->Notify("loadSuccess");
  session_->Notify("firstScreen");
  session_->Notify("pageUpdate");
  session_->Notify("dataUpdated");
  session_->Notify("destroyed");
  session_->Notify("destroyed");
  session_->Notify("loadSuccess");
  PumpJobs();

  EXPECT_EQ(EvalString("events.join(',')"),
            "loadSuccess,firstScreen,pageUpdate,dataUpdated,destroyed");
  EXPECT_EQ(state_->invalidate_count, 1);
}

TEST_F(HostScriptSessionTest, DetachRejectsWaiterAndDropsQueuedEvents) {
  state_ = std::make_shared<FakeProxyState>();
  session_ = HostScriptSession::Create();
  ASSERT_TRUE(session_->Attach(env_));
  LoadModule();
  EXPECT_EQ(EvalString("viewRef.updateMetaData('{}')"), "false");
  env_.RunScript(R"(
    globalThis.results = [];
    viewRef.waitForCurrent().catch(error => results.push(error.code));
  )");
  ASSERT_TRUE(
      session_->BindView(std::make_unique<FakeLynxViewRefProxy>(state_)));
  env_.RunScript("viewRef.on('error', () => results.push('late'))");
  session_->Notify("error", -1, "queued");
  session_->Detach();
  session_->Notify("error", -1, "late");
  PumpJobs();
  EXPECT_EQ(EvalString("results.join(',')"), "INVALID_STATE");
  EXPECT_EQ(EvalString("viewRef.loadURL('lynx://page')"), "false");
  EXPECT_EQ(state_->invalidate_count, 1);
}

TEST_F(HostScriptSessionTest, RejectsUnknownEvents) {
  CreateSession(true);
  LoadModule();

  EXPECT_EQ(EvalString(R"(
    (() => {
      try {
        viewRef.on('unknown', () => {});
        return 'accepted';
      } catch (error) {
        return error.code;
      }
    })()
  )"),
            "INVALID_ARGUMENT");
}

TEST_F(HostScriptSessionTest,
       OwnsProtocolWithoutViewAndStopsReportingAfterDetach) {
  std::vector<std::string> results;
  session_ = HostScriptSession::Create(
      [&](const std::string& status, const std::string& message) {
        results.push_back(status + ":" + message);
      });
  EXPECT_FALSE(session_->Attach(nullptr));
  ASSERT_TRUE(session_->Attach(env_));
  EXPECT_FALSE(session_->Attach(env_));
  LoadModule();
  EXPECT_EQ(EvalString("typeof viewRef.getProtocolVersion"), "undefined");
  EXPECT_EQ(EvalString("typeof viewRef.updateData"), "undefined");
  EXPECT_EQ(EvalString("String(viewRef.hasCurrent())"), "false");
  env_.RunScript(
      "viewRef.reportEntryResult('REGISTERED'); "
      "viewRef.reportEntryResult('READY');");
  EXPECT_EQ(results, (std::vector<std::string>{"REGISTERED:", "READY:"}));
  session_->Detach();
  session_->Detach();
  session_->ReportEntryResult("ERROR", "late");
  EXPECT_EQ(results.size(), 2u);
  EXPECT_FALSE(session_->Attach(env_));
}

TEST_F(HostScriptSessionTest, BindsOnceAndInvalidatesOnce) {
  CreateSession(false);
  EXPECT_FALSE(session_->BindView(nullptr));
  EXPECT_FALSE(
      session_->BindView(std::make_unique<FakeLynxViewRefProxy>(state_)));
  session_->InvalidateView();
  session_->InvalidateView();
  EXPECT_EQ(state_->invalidate_count, 1);
  EXPECT_FALSE(session_->HasCurrentView());
  EXPECT_FALSE(
      session_->BindView(std::make_unique<FakeLynxViewRefProxy>(state_)));
  EXPECT_FALSE(
      session_->Dispatch(&LynxViewRefProxy::SetGlobalProps, std::string("{}")));
  EXPECT_TRUE(state_->calls.empty());
}

TEST_F(HostScriptSessionTest, DestructorInvalidatesViewBeforeAttach) {
  auto state = std::make_shared<FakeProxyState>();
  {
    auto session = HostScriptSession::Create();
    EXPECT_TRUE(
        session->BindView(std::make_unique<FakeLynxViewRefProxy>(state)));
  }
  EXPECT_EQ(state->invalidate_count, 1);
}

TEST_F(HostScriptSessionTest,
       EnvironmentDetachOwnsCleanupAfterPlatformReleasesSession) {
  CreateSession(true);
  LoadModule();
  PumpJobs();
  env_.RunScript("globalThis.events = 0; viewRef.on('error', () => ++events)");
  session_->Notify("error", -1, "queued");
  auto weak = std::weak_ptr<HostScriptSession>(session_);
  session_.reset();
  EXPECT_FALSE(weak.expired());
  delegate_->OnDetach(env_);
  PumpJobs();
  EXPECT_EQ(EvalString("String(events)"), "0");
  EXPECT_EQ(EvalString("viewRef.updateMetaData('{}')"), "false");
  EXPECT_EQ(state_->invalidate_count, 1);
}

TEST_F(HostScriptSessionTest, RemovesLastMatchingListenerOnly) {
  CreateSession(true);
  LoadModule();
  env_.RunScript(R"(
    globalThis.events = [];
    const repeat = () => events.push('repeat');
    viewRef.on('pageUpdate', repeat);
    viewRef.on('pageUpdate', () => events.push('middle'));
    viewRef.on('pageUpdate', repeat);
    viewRef.off('pageUpdate', repeat);
  )");
  session_->Notify("pageUpdate");
  PumpJobs();
  EXPECT_EQ(EvalString("events.join(',')"), "repeat,middle");
  // The platform can invalidate between the public availability check and off.
  session_->Notify("destroyed");
  EXPECT_EQ(EvalString("String(viewRef.off('pageUpdate', repeat))"),
            "undefined");
  PumpJobs();
  EXPECT_EQ(EvalString("events.join(',')"), "repeat,middle");
}

TEST_F(HostScriptSessionTest, RejectsWaiterWhenViewIsDestroyedBeforeBinding) {
  session_ = HostScriptSession::Create();
  ASSERT_TRUE(session_->Attach(env_));
  LoadModule();
  env_.RunScript(
      "globalThis.result = ''; viewRef.waitForCurrent().catch(e => result = "
      "e.code);");
  session_->Notify("destroyed");
  PumpJobs();
  EXPECT_EQ(EvalString("result"), "INVALID_STATE");
  EXPECT_FALSE(session_->BindView(std::make_unique<FakeLynxViewRefProxy>(
      std::make_shared<FakeProxyState>())));
}

TEST_F(HostScriptSessionTest, SnapshotsListenersDuringSubscriptionChanges) {
  CreateSession(true);
  LoadModule();
  PumpJobs();
  env_.RunScript(R"(
    globalThis.events = [];
    const second = () => events.push('second');
    const third = () => events.push('third');
    viewRef.on('pageUpdate', () => {
      events.push('first');
      viewRef.off('pageUpdate', second);
      viewRef.on('pageUpdate', third);
    });
    viewRef.on('pageUpdate', second);
  )");
  session_->Notify("pageUpdate");
  PumpJobs();
  EXPECT_EQ(EvalString("events.join(',')"), "first,second");
  session_->Notify("pageUpdate");
  PumpJobs();
  EXPECT_EQ(EvalString("events.join(',')"), "first,second,first,third");
}

TEST_F(HostScriptSessionTest, RejectsInvalidArgumentsBeforePlatformDispatch) {
  CreateSession(true);
  LoadModule();
  EXPECT_EQ(EvalString(R"(
    (() => {
      const calls = [
        () => viewRef.loadTemplate(new ArrayBuffer(0), 'url'),
        () => viewRef.loadTemplate(undefined, ''),
        () => viewRef.loadSSR(new ArrayBuffer(0), 'url'),
        () => viewRef.hydrateSSR(new ArrayBuffer(1), ''),
        () => viewRef.updateMetaData(''),
        () => viewRef.reloadTemplate(1),
        () => viewRef.sendGlobalEvent('', '[]'),
        () => viewRef.on('firstScreen', 1),
        () => viewRef.reportEntryResult(1)
      ];
      return calls.map(call => { try { call(); return 'accepted'; } catch (e) { return e.code; } }).join(',');
    })()
  )"),
            "INVALID_ARGUMENT,INVALID_ARGUMENT,INVALID_ARGUMENT,INVALID_"
            "ARGUMENT,INVALID_ARGUMENT,INVALID_ARGUMENT,INVALID_ARGUMENT,"
            "INVALID_ARGUMENT,INVALID_ARGUMENT");
  EXPECT_TRUE(state_->calls.empty());
}

}  // namespace
}  // namespace shell
}  // namespace lynx
