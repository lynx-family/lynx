// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXTEMPLATEDATA_H_
#define DARWIN_COMMON_LYNX_LYNXTEMPLATEDATA_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

/**
 * @apidoc
 * @brief TemplateData is a data structure provided by Lynx.
 * It is used to store the data types accepted by Lynx at runtime
 * and can be constructed using either a string or a map type.
 */
@interface LynxTemplateData : NSObject

- (instancetype)init NS_UNAVAILABLE;

/**
 * @apidoc
 * @brief Input data in json and return the parsed `TemplateData` object.
 * @param json `NSString*` type data in json format.
 * @return `TemplateData` constructed by `json`.
 */
- (instancetype)initWithJson:(NSString *)json;

/**
 * @apidoc
 * @brief Input data in key-value format and return the parsed `TemplateData` object.
 * @param dictionary `NSDictionary*` type data.
 * @return `TemplateData` constructed by `data`.
 */
- (instancetype)initWithDictionary:(NSDictionary *)dictionary;

/**
 * Init a TemplateData with a json/dictionary
 * @param useBoolLiterals if useBoolLiterals, convert @YES/@NO to lepus bool true/false, else to
 * lepus number 1/0
 */
- (instancetype)initWithJson:(NSString *)json useBoolLiterals:(BOOL)useBoolLiterals;
- (instancetype)initWithDictionary:(NSDictionary *)dictionary useBoolLiterals:(BOOL)useBoolLiterals;

- (void)updateWithJson:(NSString *)json;
- (void)updateWithDictionary:(NSDictionary *)dictionary;

- (void)setObject:(id)object
          withKey:(NSString *)key __attribute__((deprecated("Use updateObject:forKey: instead.")));

- (void)updateObject:(id)object forKey:(NSString *)key;
- (void)updateBool:(BOOL)value forKey:(NSString *)key;
- (void)updateInteger:(NSInteger)value forKey:(NSString *)key;
- (void)updateDouble:(CGFloat)value forKey:(NSString *)key;

- (void)remove:(NSString *)key;

// will convert TemplateData to lepus value
- (void)updateWithTemplateData:(LynxTemplateData *)value;

- (BOOL)checkIsLegalData;

- (NSDictionary *)dictionary;

#pragma mark -preprocess
/**
 * @apidoc
 * @brief Mark the current TemplateData with the associated dataProcessor name.
 * When this TemplateData is used in [UpdateData](/api/lynx-native-api/lynx-view/update-meta-data),
 * the corresponding dataProcessor will be found based on this name for data preprocessing.
 * @param name Marked name.
 */
- (void)markState:(NSString *)name;

/**
 * TemplateData will be sync to Native. For Thread-Safety, we will clone the value in Native Side.
 * In some case, this may result in performance-loss, If your data won't change any more, Please
 * call this method to mark value Read-Only, so we'll no longer clone the value any more to improve
 * performance.
 */
- (void)markReadOnly;
- (BOOL)isReadOnly;
- (LynxTemplateData *)deepClone;

@end

#endif  // DARWIN_COMMON_LYNX_LYNXTEMPLATEDATA_H_
