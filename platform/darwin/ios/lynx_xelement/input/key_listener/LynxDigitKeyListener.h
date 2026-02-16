// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxNumberKeyListener.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxDigitKeyListener : LynxNumberKeyListener {
    BOOL _decimal;
    BOOL _sign;
}

@property(nonatomic, readonly) NSArray<NSString*>* CHARACTERS;
@property(nonatomic, copy, readwrite) NSString* mDecimalPointChars;
@property(nonatomic, copy, readwrite) NSString* mSignChars;

// init function
- (instancetype)initWithParamsNeedsDecimal:(BOOL)decimal sign:(BOOL)sign;

- (NSInteger)getInputType;
- (NSString*)getAcceptedChars;
- (NSString*)filter:(NSString*)source start:(NSInteger)start end:(NSInteger)end dest:(NSString*)dest dstart:(NSInteger)dstart dend:(NSInteger)dend;

@end

NS_ASSUME_NONNULL_END
