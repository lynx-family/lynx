// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/common/service/service_manager.h"
#include "clay/testing/thread_test.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

// We enable all sync calls from other threads to platform thread in tests.
template <>
struct puppet_can_sync_call_actor<Owner::kRaster, Owner::kPlatform> {
  static constexpr bool value = true;
};

template <>
struct puppet_can_sync_call_actor<Owner::kIO, Owner::kPlatform> {
  static constexpr bool value = true;
};

namespace testing {

// NOLINTNEXTLINE(build/namespaces)
using namespace ::testing;

class BaseServiceTest : public clay::testing::ThreadTest {
 public:
  explicit BaseServiceTest(Owner owner)
      : owner_(owner),
        task_runners_(std::make_tuple(
            GetCurrentTaskRunner(), GetCurrentTaskRunner(),
            CreateNewThread("mock_raster"), CreateNewThread("mock_io"))) {
    static_assert(is_owner_same_thread<Owner::kPlatform, Owner::kUI>::value,
                  "Test is under assumption that platform thread is the same "
                  "as UI thread");
  }

  void SetUp() override {
    service_manager_ = ServiceManager::Create(task_runners_);
    Attach(owner_);
  }

  void TearDown() override {
    std::apply(
        [&](auto... arg) {
          (
              [&] {
                constexpr Owner o = decltype(arg)::value;
                fml::TaskRunner::RunNowOrPostTask(
                    std::get<+o>(task_runners_),
                    [service_manager = service_manager_] {
                      service_manager->Detach<o>();
                    });
              }(),
              ...);
        },
        Owners());
  }

  void Attach(Owner owner) {
    std::apply(
        [&](auto... arg) {
          (
              [&] {
                constexpr Owner o = decltype(arg)::value;
                if (owner == o) {
                  fml::TaskRunner::RunNowOrPostTask(
                      std::get<+o>(task_runners_),
                      [service_manager = service_manager_] {
                        service_manager->Attach<o>(ServiceContext<o>{});
                      });
                }
              }(),
              ...);
        },
        Owners());
  }

  Owner owner_;
  ServiceTaskRunners::TaskRunnersTuple task_runners_;
  std::shared_ptr<ServiceManager> service_manager_;

  template <typename T>
  Puppet<Owner::kPlatform, T> GetService() {
    return service_manager_->GetService<T>();
  }
};

template <Owner owner>
class TestService : public Service<TestService<owner>, owner> {
 public:
  using ContextType = typename Service<TestService<owner>, owner>::ContextType;
  static std::shared_ptr<TestService> Create() {
    return std::make_shared<TestService>();
  }

  TestService() {
    EXPECT_CALL(*this, OnInit);
    EXPECT_CALL(*this, OnDestroy);
  }

  MOCK_METHOD(void, OnInit, (ServiceManager&, const ContextType&), (override));

  MOCK_METHOD(void, OnDestroy, (), (override));

  MOCK_METHOD(void, Hello, (int), (const));
};

template <typename T>
class ServiceTest;

template <Owner owner>
class ServiceTest<OwnerEnum<owner>> : public BaseServiceTest {
 public:
  ServiceTest() : BaseServiceTest(owner) {}
  static constexpr Owner kOwner = owner;

  using ServiceType = TestService<owner>;

  fml::RefPtr<fml::TaskRunner> task_runner_ = std::get<+owner>(task_runners_);
};

using Owners =
    ::testing::Types<OwnerEnum<Owner::kPlatform>, OwnerEnum<Owner::kUI>,
                     OwnerEnum<Owner::kRaster>, OwnerEnum<Owner::kIO>>;

TYPED_TEST_SUITE(ServiceTest, Owners);

TYPED_TEST(ServiceTest, ManageServices) {
  class TestService : public Service<TestService, TestFixture::kOwner> {
   public:
    MOCK_METHOD(void, Hello, (int), (const));

    static std::shared_ptr<TestService> Create() {
      return std::make_shared<TestService>();
    }
  };
  auto service = this->template GetService<TestService>();
  EXPECT_TRUE(service);
  service.Act([](auto& impl) { EXPECT_CALL(impl, Hello(233)); });
  service.Act([](auto& impl) { impl.Hello(233); });
  this->task_runner_->PostSyncTask([] {});
}

TYPED_TEST(ServiceTest, ManualRegisterServices) {
  class TestService : public Service<TestService, TestFixture::kOwner,
                                     ServiceFlags::kManualRegister> {
   public:
    explicit TestService(int a) : a_(a) {}
    int a_;
  };
  Puppet<Owner::kPlatform, TestService> service =
      this->template GetService<TestService>();
  EXPECT_FALSE(service);
  this->service_manager_->template RegisterService<TestService>(
      std::make_shared<TestService>(233));
  service = this->template GetService<TestService>();
  EXPECT_TRUE(service);
  service.Act([](auto& impl) { EXPECT_EQ(impl.a_, 233); });
  service.Act([](auto& impl) { impl.a_++; });
  service.Act([](auto& impl) { EXPECT_EQ(impl.a_, 234); });
  this->task_runner_->PostSyncTask([] {});
}

TYPED_TEST(ServiceTest, CanGetOtherService) {
  class DepService : public Service<DepService, Owner::kIO> {
   public:
    static std::shared_ptr<DepService> Create() {
      return std::make_shared<DepService>();
    }
  };
  class TestService : public Service<TestService, TestFixture::kOwner> {
   public:
    ServiceManager* manager;
    Puppet<TestFixture::kOwner, DepService> dep_service;

    MOCK_METHOD(void, Die, ());
    MOCK_METHOD(void, Hello, ());

    TestService() {
      EXPECT_CALL(*this, Die);
      EXPECT_CALL(*this, Hello);
    }

    ~TestService() { Die(); }

    void OnInit(ServiceManager& manager,
                const ServiceContext<TestFixture::kOwner>& context) override {
      this->manager = &manager;
      dep_service = manager.template GetService<DepService>();
      dep_service.Act([](auto&) { return 1; },
                      [weak_self = this->GetWeakPtr()](auto) {
                        if (weak_self) {
                          weak_self->Hello();
                        }
                      });
    }

    static std::shared_ptr<TestService> Create() {
      return std::make_shared<TestService>();
    }
  };

  auto service = this->template GetService<TestService>();

  service.Act([](auto& impl) { EXPECT_TRUE(impl.dep_service); });

  if constexpr (TestFixture::kOwner != Owner::kIO) {
    this->Attach(Owner::kIO);
  }
  if constexpr (is_owner_same_thread<TestFixture::kOwner,
                                     Owner::kPlatform>::value) {
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  } else {
    this->task_runner_->PostSyncTask([] {});
  }
  std::get<+Owner::kIO>(this->task_runners_)->PostSyncTask([] {});
  if constexpr (is_owner_same_thread<TestFixture::kOwner,
                                     Owner::kPlatform>::value) {
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  } else {
    this->task_runner_->PostSyncTask([] {});
  }
  std::get<+Owner::kIO>(this->task_runners_)->PostSyncTask([] {});
}

TYPED_TEST(ServiceTest, CallsOnInit) {
  auto service = this->template GetService<typename TestFixture::ServiceType>();

  this->task_runner_->PostSyncTask([] {});
}

TYPED_TEST(ServiceTest, CallsOnInitEvenNoAttachedBefore) {
  auto service = this->template GetService<typename TestFixture::ServiceType>();

  class TestPlatformService
      : public Service<TestPlatformService, Owner::kPlatform> {
   public:
    using ContextType [[maybe_unused]] =
        typename Service<TestPlatformService, Owner::kPlatform>::ContextType;

    MOCK_METHOD(void, OnInit, (ServiceManager&, const ContextType&),
                (override));

    TestPlatformService() { EXPECT_CALL(*this, OnInit); }

    static std::shared_ptr<TestPlatformService> Create() {
      return std::make_shared<TestPlatformService>();
    }
  };

  this->task_runner_->PostSyncTask([this] {
    Puppet<TestFixture::kOwner, TestPlatformService> puppet =
        this->service_manager_->template GetService<TestPlatformService>();
  });
  if constexpr (TestFixture::kOwner != Owner::kPlatform) {
    this->Attach(Owner::kPlatform);
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  }
}

TYPED_TEST(ServiceTest, CreateNewActor) {
  class MockObj : public ActorObject<TestFixture::kOwner> {
   public:
    MOCK_METHOD(int, Hello, (int a, int b));
  };

  class TestService : public Service<TestService, TestFixture::kOwner> {
   public:
    static std::shared_ptr<TestService> Create() {
      return std::make_shared<TestService>();
    }
  };

  MockFunction<void(int)> result_callback;

  this->task_runner_->PostSyncTask([] {});

  auto mock_obj = std::make_shared<MockObj>();
  EXPECT_CALL(*mock_obj, Hello(233, 123)).WillOnce(Return(123456789));
  EXPECT_CALL(result_callback, Call(123456789))
      .WillOnce([current_runner =
                     fml::MessageLoop::GetCurrent().GetTaskRunner()](auto) {
        EXPECT_TRUE(current_runner->RunsTasksOnCurrentThread());
      });
  Puppet<Owner::kPlatform, TestService> service =
      this->template GetService<TestService>();
  Puppet<Owner::kPlatform, std::shared_ptr<MockObj>> mock_obj_puppet =
      service.CreateObjectInActorThread(
          [mock_obj](auto& impl) -> std::shared_ptr<MockObj> {
            return mock_obj;
          });

  mock_obj_puppet.Act([](auto& impl) { return impl.Hello(233, 123); },
                      result_callback.AsStdFunction());
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();

  this->task_runner_->PostSyncTask([] {});
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();

  this->task_runner_->PostSyncTask([] {});
}

TYPED_TEST(ServiceTest, ActorCallActor) {
  class Listener : public ActorObject<Owner::kPlatform> {
   public:
    virtual int CallHello(int a, int b) = 0;
  };

  using ListenerActor = fml::WeakPtr<Listener>;

  class OtherObj : public ActorObject<TestFixture::kOwner> {
   public:
    Puppet<TestFixture::kOwner, ListenerActor> platform_puppet;

    void Hello() {
      platform_puppet.Act(
          [](auto& impl) { EXPECT_EQ(impl.CallHello(233, 123), 123456789); });
    }
  };

  class PlatformObj : public Listener {
   public:
    MOCK_METHOD(int, Hello, (int a, int b));

    int CallHello(int a, int b) override { return this->Hello(a, b); }

    Puppet<Owner::kPlatform, std::unique_ptr<OtherObj>> other_puppet;
    fml::WeakPtrFactory<Listener> weak_factory;
    PlatformObj() : weak_factory(this) {}
  };

  PlatformObj obj;
  EXPECT_CALL(obj, Hello(233, 123)).WillOnce(Return(123456789));

  class TestService : public Service<TestService, TestFixture::kOwner> {
   public:
    static std::shared_ptr<TestService> Create() {
      return std::make_shared<TestService>();
    }
  };

  Puppet<Owner::kPlatform, TestService> service =
      this->template GetService<TestService>();

  Puppet<Owner::kPlatform, std::unique_ptr<OtherObj>> actor_puppet =
      service.CreateObjectInActorThread(
          [](auto&) { return std::make_unique<OtherObj>(); });

  actor_puppet.PostObjectToActorThread(
      obj.weak_factory.GetWeakPtr(),
      [](OtherObj& obj, auto actor) { obj.platform_puppet = actor; });

  actor_puppet.Act([](auto& impl) { impl.Hello(); });

  this->task_runner_->PostSyncTask([] {});
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();

  this->task_runner_->PostSyncTask([] {});
}

TYPED_TEST(ServiceTest, EnableSyncCall) {
  if constexpr (TestFixture::kOwner != Owner::kPlatform) {
    this->Attach(Owner::kPlatform);
  }
  if constexpr (puppet_can_sync_call_actor<TestFixture::kOwner,
                                           Owner::kPlatform>::value) {
    class TestPlatformService
        : public Service<TestPlatformService, Owner::kPlatform> {
     public:
      MOCK_METHOD(void, Hello, (int), ());

      static std::shared_ptr<TestPlatformService> Create() {
        return std::make_shared<TestPlatformService>();
      }
    };

    Puppet<Owner::kPlatform, TestPlatformService> service =
        this->template GetService<TestPlatformService>();
    EXPECT_CALL(*service, Hello(233));

    std::future<std::optional<bool>> f;
    this->task_runner_->PostSyncTask([&] {
      Puppet<TestFixture::kOwner, TestPlatformService> service =
          this->service_manager_->template GetService<TestPlatformService>();
      std::unique_ptr<int> u = std::make_unique<int>(233);
      f = service.ActWithPromise([u = std::move(u)](auto& impl) {
        impl.Hello(*u);
        return true;
      });
    });
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
    this->task_runner_->PostSyncTask([&] { EXPECT_EQ(f.get(), true); });
  }
}

TYPED_TEST(ServiceTest, PendingCalls) {
  class TestPlatformService
      : public Service<TestPlatformService, Owner::kPlatform> {
   public:
    using ContextType [[maybe_unused]] =
        typename Service<TestPlatformService, Owner::kPlatform>::ContextType;

    MOCK_METHOD(void, OnInit, (ServiceManager&, const ContextType&),
                (override));

    TestPlatformService() { EXPECT_CALL(*this, OnInit); }

    static std::shared_ptr<TestPlatformService> Create() {
      return std::make_shared<TestPlatformService>();
    }
  };

  MockFunction<void(TestPlatformService&)> callback;
  MockFunction<void(int)> num_callback;
  EXPECT_CALL(callback, Call);
  EXPECT_CALL(num_callback, Call(233))
      .WillOnce([current_runner = std::get<+TestFixture::kOwner>(
                     this->task_runners_)](auto) {
        EXPECT_TRUE(current_runner->RunsTasksOnCurrentThread());
      });
  this->task_runner_->PostSyncTask([&] {
    Puppet<TestFixture::kOwner, TestPlatformService> puppet =
        this->service_manager_->template GetService<TestPlatformService>();
    puppet.Act(callback.AsStdFunction());
    std::unique_ptr<int> u = std::make_unique<int>(233);
    puppet.Act([u = std::move(u)](auto& impl) { return 233; },
               num_callback.AsStdFunction());
  });
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  Puppet<Owner::kPlatform, TestPlatformService> puppet =
      this->template GetService<TestPlatformService>();
  if constexpr (TestFixture::kOwner != Owner::kPlatform) {
    this->Attach(Owner::kPlatform);
  }
  this->task_runner_->PostSyncTask([&] {});
  EXPECT_TRUE(puppet->IsValid());
}

class TestServiceWithServiceId
    : public Service<TestServiceWithServiceId, Owner::kPlatform,
                     ServiceFlags::kManualRegister> {
 public:
  static constexpr ServiceId kServiceId = 123456789;
};

TYPED_TEST(ServiceTest, RegisterServiceWithServiceId) {
  this->service_manager_->template RegisterService<TestServiceWithServiceId>(
      std::make_shared<TestServiceWithServiceId>());
  auto p = this->template GetService<TestServiceWithServiceId>();
  if constexpr (TestFixture::kOwner != Owner::kPlatform) {
    this->Attach(Owner::kPlatform);
  }
  EXPECT_EQ(p->GetServiceId(), TestServiceWithServiceId::kServiceId);
}

// Compile only, just make sure code editing works.
namespace test_service_typing {
template <Owner owner>
class TypingsTestService : public Service<TypingsTestService<owner>, owner,
                                          ServiceFlags::kManualRegister |
                                              ServiceFlags::kMultiThread> {
 public:
  int ConstMethod() const { return a; }
  void NonConstMethod() { a++; }

  int a = 3;
};

struct ActorObj : public ActorObject<Owner::kPlatform> {
  int a = 3;
  int ConstMethod() const { return a; }
  void NonConstMethod() { a++; }
};

struct Test {
  void TestServiceTyping() {
    void([] {
      std::shared_ptr<ServiceManager> service_manager =
          ServiceManager::Create({nullptr, nullptr, nullptr, nullptr});

      {
        using S = TypingsTestService<Owner::kPlatform>;
        Puppet<Owner::kPlatform, S> p = service_manager->GetService<S>();
        p->NonConstMethod();
        p->ConstMethod();
        p->a++;
        p.Act([](auto&) {});
        p.Act([](auto&) { return 233; }, [](auto) {});
        // p->shared_from_this();
      }

      {
        using S = TypingsTestService<Owner::kRaster>;
        Puppet<Owner::kPlatform, S> p = service_manager->GetService<S>();
        p->NonConstMethod();
        p->ConstMethod();
        p->a++;
        p.Act([](auto&) {});
        p.Act([](auto&) { return 233; }, [](auto) {});
        // p.ActWithPromise([](auto&) { return true; }).get();
      }

      {
        using S = TypingsTestService<Owner::kPlatform>;
        Puppet<Owner::kUI, S> p = service_manager->GetService<S>();
        p->NonConstMethod();
        p->ConstMethod();
        p->a++;
        p.Act([](auto&) {});
        p.Act([](auto&) { return 233; }, [](auto) {});
        p.ActWithPromise([](auto&) { return true; }).get();
      }
    });
  }

  void TestActorTyping() {
    void([] {
      std::shared_ptr<ServiceManager> service_manager =
          ServiceManager::Create({nullptr, nullptr, nullptr, nullptr});
      {
        using S = TypingsTestService<Owner::kPlatform>;
        Puppet<Owner::kPlatform, S> p = service_manager->GetService<S>();

        using A = std::unique_ptr<ActorObj>;
        Puppet<Owner::kPlatform, A> a = p.CreateObjectInActorThread(
            [](auto&) { return std::make_unique<ActorObj>(); });
        a->NonConstMethod();
        a->ConstMethod();
        a->a++;
        a.Act([](auto&) {});
        a.Act([](auto&) { return 233; }, [](auto) {});
        // a.ActWithPromise([](auto&) { return 233; });
      }

      {
        using S = TypingsTestService<Owner::kIO>;
        Puppet<Owner::kPlatform, S> p = service_manager->GetService<S>();

        using A = std::unique_ptr<ActorObj>;
        Puppet<Owner::kPlatform, A> a = p.CreateObjectInActorThread(
            [](auto&) { return std::make_unique<ActorObj>(); });
        // a->NonConstMethod();
        // a->ConstMethod();
        // a->a++;
        a.Act([](auto&) {});
        a.Act([](auto&) { return 233; }, [](auto) {});
        // a.ActWithPromise([](auto&) {});
      }

      {
        using S = TypingsTestService<Owner::kPlatform>;
        Puppet<Owner::kUI, S> p = service_manager->GetService<S>();

        using A = std::unique_ptr<ActorObj>;
        Puppet<Owner::kUI, A> a = p.CreateObjectInActorThread(
            [](auto&) { return std::make_unique<ActorObj>(); });
        // a->NonConstMethod();
        // a->ConstMethod();
        // a->a++;
        a.Act([](auto&) {});
        a.Act([](auto&) { return 233; }, [](auto) {});
        a.ActWithPromise([](auto&) { return 233; }).get();
      }
    });
  }

  void TestMultiThreadService() {
    void([] {
      std::shared_ptr<ServiceManager> service_manager =
          ServiceManager::Create({nullptr, nullptr, nullptr, nullptr});
      {
        using S = TypingsTestService<Owner::kPlatform>;
        auto s = service_manager->GetMultiThreadService<S>();
        s->NonConstMethod();
        s->ConstMethod();
        s->a++;
      }
    });
  }
};

}  // namespace test_service_typing

}  // namespace testing
}  // namespace clay
