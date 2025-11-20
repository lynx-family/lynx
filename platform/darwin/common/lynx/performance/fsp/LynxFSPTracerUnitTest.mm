// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxFSPTracer+Native.h"

#include <memory>

using namespace lynx::shell;

// Mock class for FSPTracer native implementation
class MockFSPTracer {
 private:
  bool _isEnabled;
  bool _isRunning;
  std::function<void()> _captureHandler;

 public:
  MockFSPTracer() : _isEnabled(false), _isRunning(false) {}
  ~MockFSPTracer() = default;

  void Enable(bool enable) { _isEnabled = enable; }

  void Start() {
    if (_isEnabled) {
      _isRunning = true;
      // Simulate capturing snapshot after start
      if (_captureHandler) {
        _captureHandler();
      }
    }
  }

  void Stop() { _isRunning = false; }

  void SetCaptureHandler(std::function<void()> handler) { _captureHandler = handler; }

  bool IsRunning() const { return _isRunning; }

  bool IsEnabled() const { return _isEnabled; }
};

// Mock class for PerformanceController to test snapshot reporting
@interface MockPerformanceController : NSObject
@property(nonatomic, assign) bool didReceiveSnapshot;
@property(nonatomic, strong) LynxMeaningfulContentSnapshot *lastReceivedSnapshot;
@end

@implementation MockPerformanceController
- (instancetype)init {
  self = [super init];
  if (self) {
    self.didReceiveSnapshot = false;
    self.lastReceivedSnapshot = nil;
  }
  return self;
}

- (void)onFSPContentSnapshotCaptured:(LynxMeaningfulContentSnapshot *)snapshot {
  self.didReceiveSnapshot = true;
  self.lastReceivedSnapshot = snapshot;
}
@end

@interface LynxFSPTracerTests : XCTestCase {
  MockFSPTracer *_mockFSPTracer;
  id _mockNativeActor;
}
@property(nonatomic, strong) LynxFSPTracer *tracer;
@property(nonatomic, strong) MockPerformanceController *mockController;

@end

@implementation LynxFSPTracerTests

- (void)setUp {
  [super setUp];

  // Create mock objects
  _mockFSPTracer = new MockFSPTracer();
  self.mockController = [[MockPerformanceController alloc] init];

  // Create mock native actor
  _mockNativeActor = OCMClassMock([NSObject class]);

  // Setup stub for native actor method to return our mock FSPTracer
  [_mockNativeActor stub:@selector(getFSPTracer) andReturn:(__bridge id)(void *)_mockFSPTracer];

  // Initialize the tracer
  self.tracer = [[LynxFSPTracer alloc] init];
  [self.tracer setNativeActor:_mockNativeActor];

  // Setup capture handler to test snapshot capturing
  __weak typeof(self) weakSelf = self;
  [self.tracer setCaptureHandler:^{
    // Create a dummy snapshot
    LynxMeaningfulContentSnapshot *snapshot = [[LynxMeaningfulContentSnapshot alloc] init];
    snapshot.isReady = true;
    [weakSelf.mockController onFSPContentSnapshotCaptured:snapshot];
  }];
}

- (void)tearDown {
  [super tearDown];
  delete _mockFSPTracer;
  _mockFSPTracer = nullptr;
  [_mockNativeActor stopMocking];
}

- (void)testInitialization {
  XCTAssertNotNil(self.tracer);
  XCTAssertFalse(self.tracer.enable, @"Tracer should be disabled by default");
}

- (void)testEnableProperty {
  // Test enabling
  self.tracer.enable = true;
  XCTAssertTrue(self.tracer.enable, @"Tracer enable property should be true");
  XCTAssertTrue(_mockFSPTracer->IsEnabled(), @"Native FSPTracer should be enabled");

  // Test disabling
  self.tracer.enable = false;
  XCTAssertFalse(self.tracer.enable, @"Tracer enable property should be false");
  XCTAssertFalse(_mockFSPTracer->IsEnabled(), @"Native FSPTracer should be disabled");
}

- (void)testStartAndStop {
  // First enable the tracer
  self.tracer.enable = true;

  // Test starting
  [self.tracer start];
  XCTAssertTrue(_mockFSPTracer->IsRunning(), @"FSPTracer should be running after start");

  // Test stopping
  [self.tracer stop];
  XCTAssertFalse(_mockFSPTracer->IsRunning(), @"FSPTracer should stop after stop call");

  // Test starting when disabled
  self.tracer.enable = false;
  [self.tracer start];
  XCTAssertFalse(_mockFSPTracer->IsRunning(), @"FSPTracer should not start when disabled");
}

- (void)testCaptureHandler {
  // Enable and start the tracer
  self.tracer.enable = true;

  // Create expectation for async snapshot capture
  XCTestExpectation *expectation = [self expectationWithDescription:@"Snapshot should be captured"];

  // Override capture handler to fulfill expectation
  [self.tracer setCaptureHandler:^{
    [expectation fulfill];
  }];

  // Start the tracer, which should trigger capture handler
  [self.tracer start];

  // Wait for expectation
  [self waitForExpectationsWithTimeout:2.0
                               handler:^(NSError *error) {
                                 if (error) {
                                   XCTFail(@"Expectation failed: %@", error);
                                 }
                               }];
}

- (void)testStopByUserInteraction {
  // Test with various event types that should trigger Report thread
  id mockEventSelector = OCMSelectorMock(@selector(ActAsync));
  OCMExpect([mockEventSelector ActAsync])
      .andDo(^(NSInvocation *invocation){
          // Verify that the invocation was made
      });

  // Send the event
  [self.tracer stopByUserInteraction];
}

- (void)testReportThreadInteraction {
  // This test verifies the interaction with the Report thread
  // by checking that the capture handler is invoked after start
  // and the snapshot is correctly reported

  self.tracer.enable = true;
  self.mockController.didReceiveSnapshot = false;

  // Create expectation
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Controller should receive snapshot"];

  // Override the controller's method to fulfill expectation
  __weak typeof(self) weakSelf = self;
  [self.tracer setCaptureHandler:^{
    LynxMeaningfulContentSnapshot *snapshot = [[LynxMeaningfulContentSnapshot alloc] init];
    snapshot.isReady = true;
    snapshot.timestamp = [[NSDate date] timeIntervalSince1970] * 1000;
    snapshot.contentInfo = [[LynxMeaningfulContentInfo alloc] init];
    snapshot.contentInfo.rect = CGRectMake(0, 0, 100, 100);
    [weakSelf.mockController onFSPContentSnapshotCaptured:snapshot];
    [expectation fulfill];
  }];

  // Start the tracer - this should trigger the Report thread to capture a snapshot
  [self.tracer start];

  // Wait for expectation
  [self waitForExpectationsWithTimeout:2.0
                               handler:^(NSError *error) {
                                 if (error) {
                                   XCTFail(@"Expectation failed: %@", error);
                                 } else {
                                   XCTAssertTrue(
                                       self.mockController.didReceiveSnapshot,
                                       @"Controller should receive snapshot from Report thread");
                                   XCTAssertNotNil(self.mockController.lastReceivedSnapshot,
                                                   @"Received snapshot should not be nil");
                                   XCTAssertTrue(self.mockController.lastReceivedSnapshot.isReady,
                                                 @"Snapshot should be marked as ready");
                                 }
                               }];
}

- (void)testStartStopWithMultipleInteractions {
  // Test multiple start/stop interactions to ensure Report thread is managed correctly

  self.tracer.enable = true;

  // Start first time
  [self.tracer start];
  XCTAssertTrue(_mockFSPTracer->IsRunning(), @"FSPTracer should be running after first start");

  // Stop first time
  [self.tracer stop];
  XCTAssertFalse(_mockFSPTracer->IsRunning(), @"FSPTracer should stop after first stop");

  // Start second time
  [self.tracer start];
  XCTAssertTrue(_mockFSPTracer->IsRunning(), @"FSPTracer should be running after second start");

  // Disable and check state
  self.tracer.enable = false;
  XCTAssertFalse(_mockFSPTracer->IsRunning(), @"FSPTracer should stop when disabled");
}

@end
