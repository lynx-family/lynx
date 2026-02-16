// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxDigitKeyListener.h>
#import <Foundation/Foundation.h>

static NSString* const COMPATIBILITY_CHARACTERS = @"0123456789";
static NSString* const DEFAULT_DECIMAL_POINT_CHARS = @".";
static NSString* const DEFAULT_SIGN_CHARS = @"-+"; // maybe add the ASCII hyphen-minus U+2212 U+2013
static int const SIGN = 1;
static int const DECIMAL = 2;

@implementation LynxDigitKeyListener

- (instancetype)init {
    self = [super init];
    if (self) {
        _decimal = NO;
        _sign = NO;
        _mSignChars = DEFAULT_SIGN_CHARS;
        _mDecimalPointChars = [[NSLocale currentLocale] decimalSeparator] ? : DEFAULT_DECIMAL_POINT_CHARS;
    }
    return self;
}

- (instancetype)initWithParamsNeedsDecimal:(BOOL)decimal sign:(BOOL)sign {
    self = [super init];
    if (self) {
        _decimal = decimal;
        _sign = sign;
        _mSignChars = DEFAULT_SIGN_CHARS;
        _mDecimalPointChars = [[NSLocale currentLocale] decimalSeparator] ? : DEFAULT_DECIMAL_POINT_CHARS;
    }
    return self;
}


- (NSInteger)getInputType {
    NSInteger type = TYPE_CLASS_NUMBER;
    if (_decimal) {
        type |= TYPE_NUMBER_FLAG_DECIMAL;
    }
    if (_sign) {
        type |= TYPE_NUMBER_FLAG_SIGNED;
    }
    return type;
}

- (NSString*)getAcceptedChars {
    NSMutableString *accepted = [COMPATIBILITY_CHARACTERS mutableCopy];
    if (_sign) {
        [accepted appendString:_mSignChars];
    }
    if (_decimal) {
        NSString *decimalChars = _mDecimalPointChars.length > 0 ? _mDecimalPointChars : DEFAULT_DECIMAL_POINT_CHARS;
        [accepted appendString:decimalChars];
    }
    return accepted;
}

- (NSString *)filter:(NSString *)source start:(NSInteger)start end:(NSInteger)end dest:(NSString *)dest dstart:(NSInteger)dstart dend:(NSInteger)dend {
    NSMutableString *filteredText = [NSMutableString stringWithString:[super filter:source start:0 end:source.length dest:@"" dstart:dstart dend:dend]];

    if (!_sign && !_decimal) {
        return filteredText;
    }

    if (filteredText != nil) {
        source = filteredText;
        start = 0;
        end = [source length];
    }
    
    NSInteger sign = -1;
    NSInteger decimal = -1;
    NSInteger dlen = [dest length];
    
    /*
     * Find out if the existing text has a sign or decimal point characters.
     */
    int index = 0;
    while (index < dstart) {
        unichar c = [dest characterAtIndex:index];
        if ([super checkCharIsInCharacterSet:self.mSignChars character:c]) {
            sign = index;
        } else if ([super checkCharIsInCharacterSet:self.mDecimalPointChars character:c]) {
            decimal = index;
        }
        index++;
    }
    // assume dstart = dend before we move filter into shouldChangeCharactersInRange
    while (index < dlen) { // index >= dend
        unichar c = [dest characterAtIndex:index];
        if ([super checkCharIsInCharacterSet:self.mSignChars character:c]) {
            return @""; // Nothing can be inserted in front of a sign character.
        } else if ([super checkCharIsInCharacterSet:self.mDecimalPointChars character:c]) {
            decimal = index;
        }
        index++;
    }
    
    /*
     * If it does, we must strip them out from the source.
     * In addition, a sign character must be the very first character,
     * and nothing can be inserted before an existing sign character.
     * Go in reverse order so the offsets are stable.
     */
    NSMutableString *stripped = nil;
    for (NSInteger i = end - 1; i >= start; i--) {
        unichar c = [source characterAtIndex:i];
        BOOL strip = false;
        
        if ([super checkCharIsInCharacterSet:self.mSignChars character:c]) {
            if (i != start || dstart != 0) {
                strip = true;
            } else if (sign >= 0) {
                strip = true;
            } else {
                sign = i;
            }
        } else if ([super checkCharIsInCharacterSet:self.mDecimalPointChars character:c]) {
            if (decimal >= 0) {
                strip = true;
            } else {
                decimal = i;
            }
        }
        
        if (strip) {
            if (end == start + 1) {
                return @""; // Only one character, and it was stripped.
            }
            
            if (stripped == nil) {
                stripped = [NSMutableString stringWithString:[source substringWithRange:NSMakeRange(start, end - start)]];
            }
            
            [stripped deleteCharactersInRange:NSMakeRange(i - start, 1)];
        }
    }
    
    if (stripped != nil) {
        return stripped;
    } else if (filteredText != nil) {
        return filteredText;
    } else {
        return @"";
    }
}


@end
