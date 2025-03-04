// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/event_tracker_platform_impl.h"

#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import "LynxEventReporter.h"
#import "LynxVersion.h"

static void UnitTestLynxClassSwizzle(Class clazz, SEL originalSelector, SEL swizzledSelector) {
  Class metaClass = object_getClass(clazz);
  Method originMethod = class_getClassMethod(clazz, originalSelector);
  Method swizzledMethod = class_getClassMethod(clazz, swizzledSelector);
  if (originMethod && swizzledMethod) {
    IMP originIMP = method_getImplementation(originMethod);
    IMP swizzledIMP = method_getImplementation(swizzledMethod);
    if (originIMP && swizzledIMP && originIMP != swizzledIMP) {
      const char *originMethodType = method_getTypeEncoding(originMethod);
      const char *swizzledMethodType = method_getTypeEncoding(swizzledMethod);
      if (originMethodType && swizzledMethodType) {
        if (strcmp(originMethodType, swizzledMethodType) == 0) {
          class_replaceMethod(metaClass, swizzledSelector, originIMP, originMethodType);
          class_replaceMethod(metaClass, originalSelector, swizzledIMP, originMethodType);
        }
      }
    }
  }
}

static BOOL test_on_event_ret = NO;

@interface LynxEventReporter (UnitTest)

// Must be accessed on report thread.
@property(nonatomic, strong)
    NSMutableDictionary<NSNumber *, NSMutableDictionary<NSString *, NSObject *> *> *allGenericInfo;

+ (instancetype)sharedInstance;

@end

@interface LynxEventReporter (UnitTestHook)

@end

@implementation LynxEventReporter (UnitTestHook)

+ (void)load {
  UnitTestLynxClassSwizzle([LynxEventReporter class], @selector(onEvent:instanceId:props:),
                           @selector(unitTestHook_onEvent:instanceId:props:));
}

+ (void)unitTestHook_onEvent:(nonnull NSString *)eventName
                  instanceId:(int32_t)instanceId
                       props:(nullable NSDictionary<NSString *, NSObject *> *)props {
  test_on_event_ret = [eventName isEqual:@"event_test"] && instanceId == 1 &&
                      [props[@"source_url"] isEqual:@"sourceURL.c_str()"];
  [self unitTestHook_onEvent:eventName instanceId:instanceId props:props];
}

@end

@interface EventTrackerPlatformImplUnitTest : XCTestCase

@end

@implementation EventTrackerPlatformImplUnitTest

- (void)setUp {
  test_on_event_ret = NO;
}

- (void)testOnEvents {
  int32_t instanceId = 1;
  lynx::tasm::report::MoveOnlyEvent event;
  event.SetName("event_test");
  event.SetProps("source_url", "sourceURL.c_str()");
  event.SetProps("enable", true);
  double duration = 10000.01;
  event.SetProps("duration", duration);
  std::vector<lynx::tasm::report::MoveOnlyEvent> stack;
  stack.push_back(std::move(event));
  lynx::tasm::report::EventTrackerPlatformImpl::OnEvents(instanceId, std::move(stack));
  XCTAssertEqual(test_on_event_ret, YES);
}

- (void)testUpdateGenericInfo {
  int32_t instanceId = 1;
  std::unordered_map<std::string, std::string> info;
  info.insert({"generic_info_k_1", "generic_info_v_1"});
  lynx::tasm::report::EventTrackerPlatformImpl::UpdateGenericInfo(instanceId, std::move(info));
  lynx::tasm::report::EventTrackerPlatformImpl::UpdateGenericInfo(instanceId, "generic_info_k_2",
                                                                  "generic_info_v_2");
  BOOL ret = NO;
  dispatch_semaphore_t test_case_lock = dispatch_semaphore_create(0);
  lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [&ret, instanceId, &test_case_lock]() {
        NSMutableDictionary<NSString *, NSObject *> *genericInfo =
            [[LynxEventReporter sharedInstance].allGenericInfo objectForKey:@(instanceId)];
        ret = [genericInfo[@"generic_info_k_1"] isEqual:@"generic_info_v_1"] &&
              [genericInfo[@"generic_info_k_2"] isEqual:@"generic_info_v_2"] &&
              [genericInfo[@"lynx_sdk_version"] isEqual:[LynxVersion versionString]];
        dispatch_semaphore_signal(test_case_lock);
      });
  dispatch_semaphore_wait(test_case_lock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(ret, YES);
}

@end
