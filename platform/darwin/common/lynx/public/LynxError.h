// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxErrorCode.h"

NS_ASSUME_NONNULL_BEGIN

/// LynxError's domain.
FOUNDATION_EXPORT NSString* const LynxErrorDomain;

#pragma mark - LynxError UserInfo
FOUNDATION_EXPORT NSString* const LynxErrorUserInfoKeyMessage;
FOUNDATION_EXPORT NSString* const LynxErrorUserInfoKeySourceError;
FOUNDATION_EXPORT NSString* const LynxErrorUserInfoKeyCustomInfo;
FOUNDATION_EXPORT NSString* const LynxErrorUserInfoKeyStackInfo;

// Some commonly used keys of LynxError's customInfo
FOUNDATION_EXPORT NSString* const LynxErrorKeyResourceType;
FOUNDATION_EXPORT NSString* const LynxErrorKeyResourceUrl;

/// LynxError's level
FOUNDATION_EXPORT NSString* const LynxErrorLevelError;
FOUNDATION_EXPORT NSString* const LynxErrorLevelWarn;

// Some commonly used suggestions
FOUNDATION_EXPORT NSString* const LynxErrorSuggestionRefOfficialSite;

// LynxError instance is not thread safe,
// should not use it in multi thread
@interface LynxError : NSError

@property(nonatomic, readonly) BOOL isFatal;

// Indicates whether the error only needs to be displayed
// using LogBox and does not require reporting.
@property(nonatomic, readwrite) BOOL isLogBoxOnly;

/** Required fields */
// error code for the error
@property(nonatomic, readonly) NSInteger errorCode;
// a summary message of the error
@property(nonatomic, readonly) NSString* summaryMessage;
// url of the template that reported the error
@property(nonatomic, readwrite) NSString* templateUrl;
// version of the card that reported the error
@property(nonatomic, readwrite) NSString* cardVersion;
// error level
@property(nonatomic, readonly) NSString* level;

/** Optional fields */
// fix suggestion for the error
@property(nonatomic, readonly) NSString* fixSuggestion;
// the call stack when the error occurred
@property(nonatomic, readwrite) NSString* callStack;
// the origin cause of the error, usually comes from outside
@property(nonatomic, readwrite) NSString* rootCause;

/** Custom fields */
// some custom info of the error
@property(nonatomic, readonly) NSMutableDictionary* customInfo;

// LynxError creator for new error code
+ (instancetype)lynxErrorWithCode:(NSInteger)code message:(NSString*)errorMsg;

+ (instancetype)lynxErrorWithCode:(NSInteger)code
                          message:(NSString*)errorMsg
                    fixSuggestion:(NSString*)suggestion
                            level:(NSString*)level;
+ (instancetype)lynxErrorWithCode:(NSInteger)code
                          message:(NSString*)errorMsg
                    fixSuggestion:(NSString*)suggestion
                            level:(NSString*)level
                       customInfo:(NSDictionary* _Nullable)customInfo;
+ (instancetype)lynxErrorWithCode:(NSInteger)code
                          message:(NSString*)errorMsg
                    fixSuggestion:(NSString*)suggestion
                            level:(NSString*)level
                       customInfo:(NSDictionary* _Nullable)customInfo
                     isLogBoxOnly:(BOOL)isLogBoxOnly;

- (NSInteger)getSubCode;

- (BOOL)isValid;
- (BOOL)isJSError;
- (BOOL)isLepusError;

- (void)addCustomInfo:(NSString*)value forKey:(NSString*)key;
- (void)setCustomInfo:(NSDictionary*)customInfo;
- (NSDictionary*)getContextInfo;

/// Deprecated, LynxError create by this method may miss some fields
+ (instancetype)lynxErrorWithCode:(NSInteger)code userInfo:(NSDictionary*)userInfo;
/// Deprecated, mainly for create NSError quickly, be careful use it construct lynx error
+ (instancetype)lynxErrorWithCode:(NSInteger)code description:(nonnull NSString*)message;

/// LynxError created by following method may missing some fields
+ (instancetype)lynxErrorWithCode:(NSInteger)code
                      sourceError:(NSError*)source
    __attribute__((deprecated("Use lynxErrorWithCode:message:fixSuggestion:level")));
- (instancetype)init
    __attribute__((unavailable("Use lynxErrorWithCode:message:fixSuggestion:level")));
- (instancetype)new
    __attribute__((unavailable("Use lynxErrorWithCode:message:fixSuggestion:level")));
- (instancetype)initWithDomain:(NSErrorDomain)domain
                          code:(NSInteger)code
                      userInfo:(nullable NSDictionary<NSErrorUserInfoKey, id>*)dict
    __attribute__((unavailable("Use lynxErrorWithCode:message:fixSuggestion:level")));
+ (instancetype)errorWithDomain:(NSErrorDomain)domain
                           code:(NSInteger)code
                       userInfo:(nullable NSDictionary<NSErrorUserInfoKey, id>*)dict
    __attribute__((unavailable("Use lynxErrorWithCode:message:fixSuggestion:level")));

@end

NS_ASSUME_NONNULL_END
