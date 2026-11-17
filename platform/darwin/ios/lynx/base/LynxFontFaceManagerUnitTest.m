// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreText/CoreText.h>
#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxFontFaceManager.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

@interface LynxDeferredCascadeManager : LynxFontFaceManager
@property(nonatomic, weak) id<LynxFontFaceObserver> requestedObserver;
@end

@implementation LynxDeferredCascadeManager
- (void)requestFontfaceItem:(LynxFontSrcItem *)item
            fontFaceContext:(LynxFontFaceContext *)context
           fontFaceObserver:(id<LynxFontFaceObserver>)observer
                   cacheKey:(NSString *)cacheKey {
  self.requestedObserver = observer;
}
@end

@interface LynxCascadeObserver : NSObject <LynxFontFaceObserver>
@end
@implementation LynxCascadeObserver
@end

static BOOL LynxCascadeEnabled(id self, SEL selector) { return YES; }
static BOOL LynxCascadeDisabled(id self, SEL selector) { return NO; }

@interface LynxFontFaceManagerUnitTest : XCTestCase {
  IMP _originalCascadeSetting;
  IMP _originalFontFamilyCascadeSetting;
}
@property(nonatomic, strong) LynxFontFaceManager *manager;
@property(nonatomic, strong) LynxFontFaceContext *fontFaceContext;
@end

@implementation LynxFontFaceManagerUnitTest
- (void)setUp {
  [super setUp];
  Method method = class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontCascadeOpt));
  _originalCascadeSetting = method_setImplementation(method, (IMP)LynxCascadeDisabled);
  Method familyMethod =
      class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontFamilyCascade));
  _originalFontFamilyCascadeSetting =
      method_setImplementation(familyMethod, (IMP)LynxCascadeEnabled);
  self.manager = [LynxFontFaceManager new];
  self.fontFaceContext = [LynxFontFaceContext new];
  self.fontFaceContext.enableTextFontFamilyCascade = YES;
}

- (void)tearDown {
  Method method = class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontCascadeOpt));
  method_setImplementation(method, _originalCascadeSetting);
  Method familyMethod =
      class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontFamilyCascade));
  method_setImplementation(familyMethod, _originalFontFamilyCascadeSetting);
  [super tearDown];
}

- (UIFont *)fontForFamily:(NSString *)family weight:(CGFloat)weight style:(LynxFontStyleType)style {
  return [self.manager generateFontWithSize:18
                                     weight:weight
                                      style:style
                             fontFamilyName:family
                            fontFaceContext:self.fontFaceContext
                           fontFaceObserver:nil];
}

- (NSArray<UIFontDescriptor *> *)cascadeOfFont:(UIFont *)font {
  return [font.fontDescriptor
      objectForKey:(__bridge UIFontDescriptorAttributeName)kCTFontCascadeListAttribute];
}

- (NSString *)resolvedFontName:(UIFont *)font text:(NSString *)text {
  NSAttributedString *string =
      [[NSAttributedString alloc] initWithString:text attributes:@{NSFontAttributeName : font}];
  CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)string);
  NSArray *runs = (__bridge NSArray *)CTLineGetGlyphRuns(line);
  CTFontRef resolvedFont = (__bridge CTFontRef)((__bridge NSDictionary *)CTRunGetAttributes(
      (__bridge CTRunRef)runs.firstObject))[(__bridge NSString *)kCTFontAttributeName];
  NSString *name = CFBridgingRelease(CTFontCopyPostScriptName(resolvedFont));
  CFRelease(line);
  return name;
}

- (void)testFontFamilyCascadePageConfigDefaultsToLegacySelection {
  // An enabled page must not populate a cascade that leaks into a legacy page.
  UIFont *enabled = [self fontForFamily:@"Helvetica, 'Hiragino Sans'"
                                 weight:UIFontWeightRegular
                                  style:LynxFontStyleNormal];
  XCTAssertTrue([[self resolvedFontName:enabled text:@"\u80a9"] hasPrefix:@"HiraginoSans"]);

  LynxFontFaceContext *legacyContext = [LynxFontFaceContext new];
  XCTAssertFalse(legacyContext.enableTextFontFamilyCascade);
  LynxDeferredCascadeManager *manager = [LynxDeferredCascadeManager new];
  LynxFontFace *face =
      [[LynxFontFace alloc] initWithFamilyName:@"DeferredCascade"
                                        andSrc:@"url(https://example.com/cascade.ttf)"
                               withLynxContext:nil];
  [legacyContext addFontFace:face];
  BOOL didRequestAsync = NO;
  UIFont *legacy = [manager generateFontWithSize:18
                                          weight:UIFontWeightRegular
                                           style:LynxFontStyleNormal
                                  fontFamilyName:@"Helvetica, 'Hiragino Sans', DeferredCascade"
                                 fontFaceContext:legacyContext
                                fontFaceObserver:[LynxCascadeObserver new]
                                 didRequestAsync:&didRequestAsync];
  UIFont *expected = [UIFont fontWithName:@"Helvetica" size:18];
  XCTAssertEqualObjects(legacy.fontDescriptor, expected.fontDescriptor);
  XCTAssertFalse(didRequestAsync);
  XCTAssertNil(manager.requestedObserver);

  UIFont *withoutContext = [manager generateFontWithSize:18
                                                  weight:UIFontWeightRegular
                                                   style:LynxFontStyleNormal
                                          fontFamilyName:@"Helvetica, 'Hiragino Sans'"
                                         fontFaceContext:nil
                                        fontFaceObserver:nil];
  XCTAssertEqualObjects(withoutContext.fontDescriptor, expected.fontDescriptor);
}

- (void)testFontFamilyCascadeRequiresPageAndGlobalSettings {
  Method setting = class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontFamilyCascade));
  for (NSNumber *globalEnabled in @[ @NO, @YES ]) {
    method_setImplementation(
        setting, globalEnabled.boolValue ? (IMP)LynxCascadeEnabled : (IMP)LynxCascadeDisabled);
    for (NSNumber *pageEnabled in @[ @NO, @YES ]) {
      self.fontFaceContext.enableTextFontFamilyCascade = pageEnabled.boolValue;
      UIFont *font = [self fontForFamily:@"Helvetica, 'Hiragino Sans'"
                                  weight:UIFontWeightRegular
                                   style:LynxFontStyleNormal];
      XCTAssertEqual([self cascadeOfFont:font].count > 0,
                     globalEnabled.boolValue && pageEnabled.boolValue);
    }
  }
}

- (void)testFontFamilyCascadeExternalEnvironmentSetting {
  LynxEnv *env = LynxEnv.sharedInstance;
  NSString *key = [LynxEnv _keyStringFromType:LynxEnvEnableTextFontFamilyCascade];
  XCTAssertEqualObjects(key, @"enable_text_font_family_cascade");
  NSString *previous = [env stringFromExternalEnv:LynxEnvEnableTextFontFamilyCascade];
  for (NSString *value in @[ @"", @"true", @"1", @"false", @"0" ]) {
    [env updateExternalEnvCacheForKey:key withValue:value];
    BOOL expected = ![value isEqualToString:@"false"] && ![value isEqualToString:@"0"];
    XCTAssertEqual([env boolFromExternalEnv:LynxEnvEnableTextFontFamilyCascade defaultValue:YES],
                   expected);
  }
  [env updateExternalEnvCacheForKey:key withValue:previous ?: @""];
}

- (void)testFontFamilyCascadeDisabledRestoresLegacySelection {
  // Populate the cascade cache first; disabling must bypass it as well as resource loading.
  [self fontForFamily:@"Helvetica, 'PingFang SC'"
               weight:UIFontWeightRegular
                style:LynxFontStyleNormal];
  Method familyMethod =
      class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontFamilyCascade));
  method_setImplementation(familyMethod, (IMP)LynxCascadeDisabled);
  Method localeMethod = class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontCascadeOpt));
  for (NSNumber *enabled in @[ @NO, @YES ]) {
    method_setImplementation(
        localeMethod, enabled.boolValue ? (IMP)LynxCascadeEnabled : (IMP)LynxCascadeDisabled);
    UIFont *single = [self fontForFamily:@"Helvetica"
                                  weight:UIFontWeightRegular
                                   style:LynxFontStyleNormal];
    UIFont *font = [self fontForFamily:@"Helvetica, 'PingFang SC'"
                                weight:UIFontWeightRegular
                                 style:LynxFontStyleNormal];
    XCTAssertEqualObjects(font, single);

    LynxDeferredCascadeManager *manager = [LynxDeferredCascadeManager new];
    LynxFontFaceContext *context = [LynxFontFaceContext new];
    context.enableTextFontFamilyCascade = YES;
    LynxFontFace *face =
        [[LynxFontFace alloc] initWithFamilyName:@"DeferredCascade"
                                          andSrc:@"url(https://example.com/cascade.ttf)"
                                 withLynxContext:nil];
    [context addFontFace:face];
    BOOL didRequestAsync = NO;
    font = [manager generateFontWithSize:18
                                  weight:UIFontWeightRegular
                                   style:LynxFontStyleNormal
                          fontFamilyName:@"Helvetica, DeferredCascade"
                         fontFaceContext:context
                        fontFaceObserver:[LynxCascadeObserver new]
                         didRequestAsync:&didRequestAsync];
    XCTAssertEqualObjects(font, single);
    XCTAssertFalse(didRequestAsync);
    XCTAssertNil(manager.requestedObserver);
  }
}

- (void)testFontFamilyCascadeDisabledSkipsUnavailablePrimary {
  Method method = class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontFamilyCascade));
  method_setImplementation(method, (IMP)LynxCascadeDisabled);
  UIFont *font = [self fontForFamily:@"MissingCascadeFont, , Helvetica, 'PingFang SC'"
                              weight:UIFontWeightRegular
                               style:LynxFontStyleNormal];
  XCTAssertEqualObjects(font, [UIFont fontWithName:@"Helvetica" size:18]);
  XCTAssertNil([self cascadeOfFont:font]);
}

- (void)testExplicitCascadeDoesNotDependOnLocaleOptimization {
  Method method = class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontCascadeOpt));
  for (NSNumber *enabled in @[ @NO, @YES ]) {
    method_setImplementation(
        method, enabled.boolValue ? (IMP)LynxCascadeEnabled : (IMP)LynxCascadeDisabled);
    UIFont *font = [self fontForFamily:@"Helvetica, 'PingFang SC', 'Hiragino Sans'"
                                weight:UIFontWeightRegular
                                 style:LynxFontStyleNormal];
    NSArray *cascade = [self cascadeOfFont:font];
    XCTAssertEqual(cascade.count, 2u);
    XCTAssertEqualObjects([UIFont fontWithDescriptor:cascade[0] size:18].familyName,
                          @"PingFang SC");
    XCTAssertEqualObjects([UIFont fontWithDescriptor:cascade[1] size:18].familyName,
                          @"Hiragino Sans");
    XCTAssertTrue([[self resolvedFontName:font text:@"\u80a9"] hasPrefix:@"PingFangSC"]);
  }
}

- (void)testSingleFamilyKeepsLocaleOptimizationSeparate {
  UIFont *font = [self fontForFamily:@"Helvetica"
                              weight:UIFontWeightRegular
                               style:LynxFontStyleNormal];
  XCTAssertNil([self cascadeOfFont:font]);
  Method method = class_getInstanceMethod(LynxEnv.class, @selector(enableTextFontCascadeOpt));
  method_setImplementation(method, (IMP)LynxCascadeEnabled);
  UIFont *optimized = [self fontForFamily:@"Helvetica"
                                   weight:UIFontWeightRegular
                                    style:LynxFontStyleNormal];
  XCTAssertEqualObjects(optimized.fontName, font.fontName);
  XCTAssertGreaterThan([self cascadeOfFont:optimized].count, 0u);
}

- (void)testFontFamilyListResolvesMissingGlyphsInOrder {
  UIFont *font = [self fontForFamily:@"Helvetica, 'PingFang SC', 'Hiragino Sans'"
                              weight:UIFontWeightRegular
                               style:LynxFontStyleNormal];
  UIFont *latin = [self fontForFamily:@"Helvetica"
                               weight:UIFontWeightRegular
                                style:LynxFontStyleNormal];
  XCTAssertEqualObjects(font.fontName, latin.fontName);
  NSArray *cascade = [self cascadeOfFont:font];
  XCTAssertGreaterThanOrEqual(cascade.count, 2u);
  XCTAssertEqualObjects([UIFont fontWithDescriptor:cascade[0] size:18].familyName, @"PingFang SC");
  XCTAssertEqualObjects([UIFont fontWithDescriptor:cascade[1] size:18].familyName,
                        @"Hiragino Sans");
  XCTAssertEqualObjects([self resolvedFontName:font text:@"A"], latin.fontName);
  XCTAssertTrue([[self resolvedFontName:font text:@"\u80a9"] hasPrefix:@"PingFangSC"]);

  font = [self fontForFamily:@"Helvetica, 'Hiragino Sans', 'PingFang SC'"
                      weight:UIFontWeightRegular
                       style:LynxFontStyleNormal];
  XCTAssertTrue([[self resolvedFontName:font text:@"\u80a9"] hasPrefix:@"HiraginoSans"]);
}

- (void)testFontFamilyListSkipsUnavailableAndEmptyCandidates {
  UIFont *font =
      [self fontForFamily:@"MissingCascadeFont, , Helvetica, '', MissingFallback, 'PingFang SC'"
                   weight:UIFontWeightRegular
                    style:LynxFontStyleNormal];
  XCTAssertEqualObjects(font.fontName, [UIFont fontWithName:@"Helvetica" size:18].fontName);
  NSArray *cascade = [self cascadeOfFont:font];
  XCTAssertGreaterThanOrEqual(cascade.count, 1u);
  XCTAssertEqualObjects([UIFont fontWithDescriptor:cascade[0] size:18].familyName, @"PingFang SC");
  XCTAssertTrue([[self resolvedFontName:font text:@"\u80a9"] hasPrefix:@"PingFangSC"]);
}

- (void)testFontFamilyListPreservesFallbackWeightAndItalic {
  for (NSNumber *weight in @[ @(UIFontWeightRegular), @(UIFontWeightSemibold) ]) {
    for (NSNumber *style in @[ @(LynxFontStyleNormal), @(LynxFontStyleItalic) ]) {
      UIFont *font = [self fontForFamily:@"Helvetica, 'PingFang SC'"
                                  weight:weight.doubleValue
                                   style:style.unsignedIntegerValue];
      UIFont *expected = [self fontForFamily:@"PingFang SC"
                                      weight:weight.doubleValue
                                       style:style.unsignedIntegerValue];
      UIFont *fallback = [UIFont fontWithDescriptor:[self cascadeOfFont:font][0] size:18];
      XCTAssertEqualObjects(fallback.fontName, expected.fontName);
      XCTAssertEqual(fallback.fontDescriptor.symbolicTraits,
                     expected.fontDescriptor.symbolicTraits);
      XCTAssertTrue(CGAffineTransformEqualToTransform(fallback.fontDescriptor.matrix,
                                                      expected.fontDescriptor.matrix));
    }
  }
}

- (void)testFontFamilyListKeepsRegisteredCascadeAfterExplicitCandidates {
  UIFontDescriptor *japanese = [UIFontDescriptor
      fontDescriptorWithFontAttributes:@{UIFontDescriptorFamilyAttribute : @"Hiragino Sans"}];
  UIFont *latin = [UIFont fontWithName:@"Helvetica" size:18];
  UIFontDescriptor *descriptor = [latin.fontDescriptor fontDescriptorByAddingAttributes:@{
    (__bridge UIFontDescriptorAttributeName)kCTFontCascadeListAttribute : @[ japanese ]
  }];
  [self.manager registerFont:[UIFont fontWithDescriptor:descriptor size:18]
                     forName:@"CascadeLatin"];
  UIFont *font = [self fontForFamily:@"CascadeLatin, 'PingFang SC'"
                              weight:UIFontWeightRegular
                               style:LynxFontStyleNormal];
  NSArray *cascade = [self cascadeOfFont:font];
  XCTAssertEqual(cascade.count, 2u);
  XCTAssertEqualObjects([UIFont fontWithDescriptor:cascade[0] size:18].familyName, @"PingFang SC");
  XCTAssertEqualObjects([UIFont fontWithDescriptor:cascade[1] size:18].familyName,
                        @"Hiragino Sans");
  XCTAssertTrue([[self resolvedFontName:font text:@"\u80a9"] hasPrefix:@"PingFangSC"]);
}

- (void)testFontFamilyListSurvivesFontFeatureSettings {
  UIFont *font = [self fontForFamily:@"Helvetica, 'PingFang SC'"
                              weight:UIFontWeightRegular
                               style:LynxFontStyleNormal];
  UIFont *featured = [LynxFontFaceManager generateFontWithBaseFont:font
                                             fontVariationSettings:nil
                                               fontFeatureSettings:@[ @"kern", @1 ]
                                                 fontOpticalSizing:NO];
  XCTAssertEqualObjects([self cascadeOfFont:featured], [self cascadeOfFont:font]);
  XCTAssertTrue([[self resolvedFontName:featured text:@"\u80a9"] hasPrefix:@"PingFangSC"]);
  XCTAssertTrue([[self resolvedFontName:featured text:@"\U0001F600"] containsString:@"Emoji"]);
}

- (void)testFontFamilyListReusesResolvedCombination {
  UIFont *first = [self fontForFamily:@"Helvetica, 'PingFang SC'"
                               weight:UIFontWeightRegular
                                style:LynxFontStyleNormal];
  UIFont *second = [self fontForFamily:@"Helvetica, 'PingFang SC'"
                                weight:UIFontWeightRegular
                                 style:LynxFontStyleNormal];
  XCTAssertEqual(first, second);
  UIFont *single = [self fontForFamily:@"Helvetica"
                                weight:UIFontWeightRegular
                                 style:LynxFontStyleNormal];
  XCTAssertEqualObjects(single.fontName, first.fontName);
  UIFont *missing = [self fontForFamily:@"MissingCascadeFont, MissingFallback"
                                 weight:UIFontWeightSemibold
                                  style:LynxFontStyleNormal];
  XCTAssertEqualObjects(missing.fontName,
                        [UIFont systemFontOfSize:18 weight:UIFontWeightSemibold].fontName);
}

- (void)testFontFamilyListReloadsDeferredFallback {
  LynxDeferredCascadeManager *manager = [LynxDeferredCascadeManager new];
  LynxFontFaceContext *context = [LynxFontFaceContext new];
  context.enableTextFontFamilyCascade = YES;
  LynxFontFace *face =
      [[LynxFontFace alloc] initWithFamilyName:@"DeferredCascade"
                                        andSrc:@"url(https://example.com/cascade.ttf)"
                               withLynxContext:nil];
  [context addFontFace:face];
  LynxCascadeObserver *observer = [LynxCascadeObserver new];
  BOOL didRequestAsync = NO;
  UIFont *pending = [manager generateFontWithSize:18
                                           weight:UIFontWeightRegular
                                            style:LynxFontStyleNormal
                                   fontFamilyName:@"Helvetica, DeferredCascade"
                                  fontFaceContext:context
                                 fontFaceObserver:observer
                                  didRequestAsync:&didRequestAsync];
  XCTAssertTrue(didRequestAsync);
  XCTAssertEqual(manager.requestedObserver, observer);
  XCTAssertEqualObjects(pending.fontName, [UIFont fontWithName:@"Helvetica" size:18].fontName);

  // Font loading supplies a PostScript name before the observer requests a new layout.
  [face srcAtIndex:0].dataFontName = @"PingFangSC-Regular";
  didRequestAsync = NO;
  UIFont *loaded = [manager generateFontWithSize:18
                                          weight:UIFontWeightRegular
                                           style:LynxFontStyleNormal
                                  fontFamilyName:@"Helvetica, DeferredCascade"
                                 fontFaceContext:context
                                fontFaceObserver:observer
                                 didRequestAsync:&didRequestAsync];
  XCTAssertFalse(didRequestAsync);
  XCTAssertTrue([[self resolvedFontName:loaded text:@"\u80a9"] hasPrefix:@"PingFangSC"]);
}
@end
