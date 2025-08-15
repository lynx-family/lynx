// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxHttpStreamingDelegate.h>
#import <XCTest/XCTWaiter.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>
#import "LynxFetchModule.h"

@interface LynxHttpStreamingDelegate ()
- (void)processChunkedData:(NSMutableData *)buffer withData:(NSData *)data;

- (void)streamingChunk:(NSMutableData *)buffer
             chunkSize:(NSInteger)chunkSize
               nextIdx:(NSUInteger)nextIdx;
- (void)getStreamingLength:(NSMutableData *)buffer
                 chunkSize:(NSInteger *)chunkSize
                   nextIdx:(NSUInteger *)nextIdx;

@end
static NSString *const ERROR_STREAMING_MALFORMED_RESPONSE = @"errorStreamingMalformedResponse";

@interface LynxHttpStreamingDelegateTests : XCTestCase
@property(nonatomic, strong) LynxFetchModuleEventSender *mockSender;
@property(nonatomic, strong) LynxHttpStreamingDelegate *mockDelegate;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *eventArray;
@property(nonatomic, strong) NSMutableData *buffer;
@end

@implementation LynxHttpStreamingDelegateTests

- (void)sendGlobalEvent:(NSString *)name withParams:(nullable NSArray *)params {
  [self.eventArray addObject:params[0]];
}

- (void)setUp {
  [super setUp];
  self.mockSender = [[LynxFetchModuleEventSender alloc] init];
  self.mockSender.eventSender = self;
  self.mockDelegate = [[LynxHttpStreamingDelegate alloc] initWithParam:self.mockSender
                                                       withStreamingId:@"_"];
  self.eventArray = [[NSMutableArray alloc] init];
  self.buffer = [[NSMutableData alloc] init];
}

- (void)tearDown {
  [super tearDown];
}

#pragma mark - getStreamingLength Tests

- (void)testGetStreamingLength_ValidChunk {
  NSString *chunkHeader = @"A\r\n";  // 10 bytes chunk
  [self.buffer appendData:[chunkHeader dataUsingEncoding:NSUTF8StringEncoding]];

  NSInteger chunkSize = -1;
  NSUInteger nextIdx = 0;

  [self.mockDelegate getStreamingLength:self.buffer chunkSize:&chunkSize nextIdx:&nextIdx];

  XCTAssertEqual(chunkSize, 10);
  XCTAssertEqual(nextIdx, 3);
  XCTAssertEqual([self.eventArray count], 0);
}

- (void)testGetStreamingLength_MissingNewline {
  NSString *chunkHeader = @"A\rX";  // Missing \n after \r
  [self.buffer appendData:[chunkHeader dataUsingEncoding:NSUTF8StringEncoding]];

  NSInteger chunkSize = -1;
  NSUInteger nextIdx = 0;

  [self.mockDelegate getStreamingLength:self.buffer chunkSize:&chunkSize nextIdx:&nextIdx];

  XCTAssertEqual(chunkSize, -1);
  XCTAssertEqual(nextIdx, 0);
  XCTAssertEqual([self.eventArray count], 1);
  XCTAssertEqual(self.eventArray[0][@"event"], @"onError");
  XCTAssertEqual(self.eventArray[0][@"error"], ERROR_STREAMING_MALFORMED_RESPONSE);
}

- (void)testGetStreamingLength_IncompleteHeader {
  NSString *chunkHeader = @"F";  // Only partial header
  [self.buffer appendData:[chunkHeader dataUsingEncoding:NSUTF8StringEncoding]];

  NSInteger chunkSize = -1;
  NSUInteger nextIdx = 0;

  [self.mockDelegate getStreamingLength:self.buffer chunkSize:&chunkSize nextIdx:&nextIdx];

  XCTAssertEqual(chunkSize, -1);
  XCTAssertEqual(nextIdx, 0);
  XCTAssertEqual([self.eventArray count], 0);
}

#pragma mark - streamingChunk Tests

- (void)testStreamingChunk_ValidChunk {
  NSString *chunk = @"A\r\n1234567890\r\n";
  [self.buffer appendData:[chunk dataUsingEncoding:NSUTF8StringEncoding]];
  NSInteger chunkSize = 10;
  NSUInteger nextIdx = 3;

  [self.mockDelegate streamingChunk:self.buffer chunkSize:chunkSize nextIdx:nextIdx];

  NSString *expectedString = @"1234567890";
  NSData *expectedData = [expectedString dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[0][@"data"], expectedData);
}

- (void)testStreamingChunk_MissingTerminator {
  NSString *chunk = @"A\r\n1234567890X";  // Missing \r\n terminator
  [self.buffer appendData:[chunk dataUsingEncoding:NSUTF8StringEncoding]];
  NSInteger chunkSize = 10;
  NSUInteger nextIdx = 3;

  [self.mockDelegate streamingChunk:self.buffer chunkSize:chunkSize nextIdx:nextIdx];

  XCTAssertEqual(self.eventArray[0][@"event"], @"onError");
  XCTAssertEqual(self.eventArray[0][@"error"], ERROR_STREAMING_MALFORMED_RESPONSE);
}

#pragma mark - processChunkedData Tests

- (void)testProcessChunkedData_SingleChunk {
  NSString *chunk = @"A\r\n1234567890\r\n";
  NSData *data = [chunk dataUsingEncoding:NSUTF8StringEncoding];

  [self.mockDelegate processChunkedData:self.buffer withData:data];

  NSString *expectedString = @"1234567890";
  NSData *expectedData = [expectedString dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[0][@"data"], expectedData);
  XCTAssertEqual(self.buffer.length, 0);
}

- (void)testProcessChunkedData_MultipleChunks {
  NSString *chunk1 = @"5\r\nHello\r\n";
  NSString *chunk2 = @"6\r\n World\r\n";
  NSString *chunk3 = @"0\r\n\r\n";  // End chunk

  [self.mockDelegate processChunkedData:self.buffer
                               withData:[chunk1 dataUsingEncoding:NSUTF8StringEncoding]];

  [self.mockDelegate processChunkedData:self.buffer
                               withData:[chunk2 dataUsingEncoding:NSUTF8StringEncoding]];

  [self.mockDelegate processChunkedData:self.buffer
                               withData:[chunk3 dataUsingEncoding:NSUTF8StringEncoding]];

  NSData *expectedData = [@"Hello" dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[0][@"data"], expectedData);
  expectedData = [@" World" dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[1][@"data"], expectedData);
  XCTAssertEqualObjects(self.eventArray[2][@"event"], @"onEnd");
}

- (void)testProcessChunkedData_MultipleChunksSingle {
  NSString *chunk1 = @"5\r\nHello\r\n6\r\n World\r\n0\r\n\r\n";

  [self.mockDelegate processChunkedData:self.buffer
                               withData:[chunk1 dataUsingEncoding:NSUTF8StringEncoding]];

  NSData *expectedData = [@"Hello" dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[0][@"data"], expectedData);
  expectedData = [@" World" dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[1][@"data"], expectedData);
  XCTAssertEqualObjects(self.eventArray[2][@"event"], @"onEnd");
}

- (void)testProcessChunkedData_IncompleteNumber {
  NSString *chunk = @"A\r";  // Only part of the chunk
  NSData *data = [chunk dataUsingEncoding:NSUTF8StringEncoding];

  [self.mockDelegate processChunkedData:self.buffer withData:data];

  XCTAssertEqual([self.eventArray count], 0);

  chunk = @"\n1234567890\r\n";
  data = [chunk dataUsingEncoding:NSUTF8StringEncoding];
  [self.mockDelegate processChunkedData:self.buffer withData:data];

  NSString *expectedString = @"1234567890";
  NSData *expectedData = [expectedString dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[0][@"data"], expectedData);
  XCTAssertEqual(self.buffer.length, 0);
}

- (void)testProcessChunkedData_IncompleteChunk {
  NSString *chunk = @"A\r\n12345";  // Only part of the chunk
  NSData *data = [chunk dataUsingEncoding:NSUTF8StringEncoding];

  [self.mockDelegate processChunkedData:self.buffer withData:data];

  XCTAssertEqual([self.eventArray count], 0);

  chunk = @"67890\r\n";
  data = [chunk dataUsingEncoding:NSUTF8StringEncoding];
  [self.mockDelegate processChunkedData:self.buffer withData:data];

  NSString *expectedString = @"1234567890";
  NSData *expectedData = [expectedString dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[0][@"data"], expectedData);
  XCTAssertEqual(self.buffer.length, 0);
}

- (void)testProcessChunkedData_ZeroLengthChunk {
  NSString *chunk = @"0\r\n\r\n";  // End chunk
  NSData *data = [chunk dataUsingEncoding:NSUTF8StringEncoding];

  [self.mockDelegate processChunkedData:self.buffer withData:data];

  XCTAssertEqual([self.eventArray count], 1);
  XCTAssertEqualObjects(self.eventArray[0][@"event"], @"onEnd");
}

- (void)testProcessChunkedData_MalformedChunk {
  NSString *chunk = @"A\rX\n1234567890\r\n";  // Malformed chunk header
  NSData *data = [chunk dataUsingEncoding:NSUTF8StringEncoding];

  [self.mockDelegate processChunkedData:self.buffer withData:data];

  XCTAssertEqual([self.eventArray count], 1);
  XCTAssertEqual(self.eventArray[0][@"event"], @"onError");
  XCTAssertEqual(self.eventArray[0][@"error"], ERROR_STREAMING_MALFORMED_RESPONSE);
}

- (void)testProcessSseData {
  NSString *chunk = @"12345\n\n1234567890\n\n";
  NSData *data = [chunk dataUsingEncoding:NSUTF8StringEncoding];

  [self.mockDelegate processSseData:self.buffer withData:data];

  XCTAssertEqual([self.eventArray count], 2);
  NSString *expectedString = @"12345\n\n";
  NSData *expectedData = [expectedString dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[0][@"data"], expectedData);

  expectedString = @"1234567890\n\n";
  expectedData = [expectedString dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(self.eventArray[1][@"data"], expectedData);
}

@end
