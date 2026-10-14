// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderFixture.h>

#include <cstring>
#include <vector>

#include "core/services/replay/fixture_evaluator.h"
#include "third_party/zlib/contrib/minizip/unzip.h"

#if ENABLE_TESTBENCH_REPLAY
static const NSUInteger kMaxFixtureBytes = 128 * 1024 * 1024;
static const NSUInteger kMaxTemplateSourceBytes = 16 * 1024 * 1024;

#endif

static BOOL FixtureError(NSError **error, NSString *message) {
  if (error) {
    *error = [NSError errorWithDomain:@"LynxRecorderFixture"
                                 code:1
                             userInfo:@{NSLocalizedDescriptionKey : message}];
  }
  return NO;
}

#if ENABLE_TESTBENCH_REPLAY
static NSString *AssetPath(NSString *root, NSString *relative, NSError **error) {
  if (relative.length == 0 || [relative containsString:@"\\"] || [relative containsString:@":"] ||
      [relative rangeOfString:[NSString stringWithFormat:@"%C", (unichar)0]].location !=
          NSNotFound) {
    FixtureError(error, @"Invalid fixture asset path");
    return nil;
  }
  for (NSString *part in [relative componentsSeparatedByString:@"/"]) {
    if (part.length == 0 || [part isEqualToString:@"."] || [part isEqualToString:@".."]) {
      FixtureError(error, @"Invalid fixture asset path");
      return nil;
    }
  }
  return [root stringByAppendingPathComponent:relative];
}

static NSData *ReadAsset(NSString *path, NSUInteger limit, NSError **error) {
  NSNumber *size = [[NSFileManager defaultManager] attributesOfItemAtPath:path
                                                                    error:error][NSFileSize];
  if (!size || size.unsignedLongLongValue > limit) {
    FixtureError(error, @"Fixture asset is missing or exceeds size limit");
    return nil;
  }
  NSData *data = [NSData dataWithContentsOfFile:path options:0 error:error];
  if (data.length > limit) {
    FixtureError(error, @"Fixture asset exceeds size limit");
    return nil;
  }
  return data;
}

static BOOL Extract(NSData *data, NSString *directory, NSError **error) {
  NSString *archivePath = [directory stringByAppendingString:@".zip"];
  if (![data writeToFile:archivePath options:NSDataWritingAtomic error:error]) return NO;
  unzFile archive = unzOpen64(archivePath.fileSystemRepresentation);
  @try {
    if (!archive) return FixtureError(error, @"Invalid fixture ZIP");
    NSMutableSet *names = [NSMutableSet set];
    NSUInteger total = 0;
    int status = unzGoToFirstFile(archive);
    while (status == UNZ_OK) {
      unz_file_info64 info = {};
      if (unzGetCurrentFileInfo64(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK ||
          info.size_filename == 0 || info.size_filename > 4096 ||
          info.uncompressed_size > kMaxFixtureBytes - total) {
        return FixtureError(error, @"Invalid or oversized fixture entry");
      }
      std::vector<char> name(info.size_filename + 1);
      if (unzGetCurrentFileInfo64(archive, &info, name.data(), static_cast<uLong>(name.size()),
                                  nullptr, 0, nullptr, 0) != UNZ_OK ||
          std::memchr(name.data(), 0, info.size_filename)) {
        return FixtureError(error, @"Invalid fixture entry name");
      }
      NSString *relative = [[NSString alloc] initWithBytes:name.data()
                                                    length:info.size_filename
                                                  encoding:NSUTF8StringEncoding];
      BOOL isDirectory = [relative hasSuffix:@"/"];
      if (isDirectory) relative = [relative substringToIndex:relative.length - 1];
      NSString *target = AssetPath(directory, relative, error);
      if (!target || [names containsObject:relative] || names.count >= 65535) {
        return FixtureError(error, @"Invalid or duplicate fixture entry");
      }
      [names addObject:relative];
      NSFileManager *files = [NSFileManager defaultManager];
      // Write regular files only, including entries marked as Unix symbolic links.
      NSString *parent = isDirectory ? target : target.stringByDeletingLastPathComponent;
      if (![files createDirectoryAtPath:parent
              withIntermediateDirectories:YES
                               attributes:nil
                                    error:error])
        return NO;
      if (unzOpenCurrentFile(archive) != UNZ_OK)
        return FixtureError(error, @"Unreadable fixture entry");
      NSOutputStream *output =
          isDirectory ? nil : [NSOutputStream outputStreamToFileAtPath:target append:NO];
      [output open];
      BOOL valid = isDirectory || (output && output.streamStatus != NSStreamStatusError);
      int count = 0;
      uint8_t buffer[8192];
      while ((count = unzReadCurrentFile(archive, buffer, sizeof(buffer))) > 0) {
        if (isDirectory || static_cast<NSUInteger>(count) > kMaxFixtureBytes - total) {
          valid = NO;
          break;
        }
        total += count;
        NSInteger offset = 0;
        while (offset < count) {
          NSInteger written = [output write:buffer + offset maxLength:count - offset];
          if (written <= 0) {
            valid = NO;
            break;
          }
          offset += written;
        }
        if (!valid) break;
      }
      [output close];
      int closeStatus = unzCloseCurrentFile(archive);
      if (!valid || count < 0 || closeStatus != UNZ_OK) {
        return FixtureError(error, @"Corrupt, oversized or unwritable fixture entry");
      }
      status = unzGoToNextFile(archive);
    }
    return status == UNZ_END_OF_LIST_OF_FILE ||
           FixtureError(error, @"Invalid fixture ZIP directory");
  } @finally {
    if (archive) unzClose(archive);
    [[NSFileManager defaultManager] removeItemAtPath:archivePath error:nil];
  }
}

static id ParseJSON(const std::string &json, NSError **error) {
  NSData *data = [NSData dataWithBytes:json.data() length:json.size()];
  return [NSJSONSerialization JSONObjectWithData:data
                                         options:NSJSONReadingAllowFragments
                                           error:error];
}

#endif

@implementation LynxRecorderFixture

+ (instancetype)openData:(NSData *)data error:(NSError **)error {
#if !ENABLE_TESTBENCH_REPLAY
  FixtureError(error, @"Fixture replay is disabled in this build");
  return nil;
#else
  LynxRecorderFixture *fixture = [[self alloc] init];
  fixture->_directory = [NSTemporaryDirectory()
      stringByAppendingPathComponent:[@"lynx-fixture-"
                                         stringByAppendingString:NSUUID.UUID.UUIDString]];
  if (![[NSFileManager defaultManager] createDirectoryAtPath:fixture.directory
                                 withIntermediateDirectories:YES
                                                  attributes:nil
                                                       error:error] ||
      !Extract(data, fixture.directory, error))
    return nil;
  auto result = lynx::tasm::replay::EvaluateFixture(fixture.directory.fileSystemRepresentation);
  if (!result.has_value()) {
    FixtureError(error, [[NSString alloc] initWithBytes:result.error().data()
                                                 length:result.error().size()
                                               encoding:NSUTF8StringEncoding]
                            ?: @"Fixture evaluation failed");
    return nil;
  }
  NSMutableArray *actions = [NSMutableArray array];
  NSMutableDictionary *templates = [NSMutableDictionary dictionary];
  NSUInteger templateBytes = 0;
  for (const auto &action : result->actions) {
    NSString *name = [[NSString alloc] initWithBytes:action.function_name.data()
                                              length:action.function_name.size()
                                            encoding:NSUTF8StringEncoding];
    if (!name) {
      FixtureError(error, @"Invalid fixture action name");
      return nil;
    }
    id parsed = ParseJSON(action.params_json, error);
    if (![parsed isKindOfClass:NSDictionary.class]) {
      FixtureError(error, @"Invalid fixture action parameters");
      return nil;
    }
    NSMutableDictionary *params = [parsed mutableCopy];
    if ([name isEqualToString:@"loadTemplate"]) {
      NSString *relative = params[@"templateAsset"];
      if (![relative isKindOfClass:NSString.class]) {
        FixtureError(error, @"Missing fixture template asset path");
        return nil;
      }
      if ([relative hasPrefix:@"assets/"]) relative = [relative substringFromIndex:7];
      NSString *path =
          AssetPath([fixture.directory stringByAppendingPathComponent:@"assets"], relative, error);
      if (!path) return nil;
      NSString *source = templates[path];
      if (!source) {
        NSData *templateData =
            ReadAsset(path, (kMaxTemplateSourceBytes - templateBytes) / 4 * 3, error);
        if (!templateData.length) {
          FixtureError(error, @"Missing or empty fixture template");
          return nil;
        }
        source = [templateData base64EncodedStringWithOptions:0];
        templateBytes += source.length;
        templates[path] = source;
      }
      params[@"source"] = source;
    }
    [actions addObject:@{
      @"Function Name" : name,
      @"Record Time" : @0,
      @"RecordMillisecond" : @(action.delay_ms),
      @"Params" : params
    }];
  }
  NSMutableDictionary *shared = [NSMutableDictionary dictionary];
  for (const auto &item : result->shared_data) {
    NSString *key = [[NSString alloc] initWithBytes:item.key.data()
                                             length:item.key.size()
                                           encoding:NSUTF8StringEncoding];
    id value = ParseJSON(item.value_json, error);
    if (!key || !value) return nil;
    shared[key] = value;
  }
  NSMutableDictionary *record = [@{
    @"Action List" : actions,
    @"SharedData" : shared,
    @"Invoked Method Data" : @[],
    @"Callback" : @{}
  } mutableCopy];
  NSDictionary *metadata =
      @{@"config.json" : @"Config", @"component_list.json" : @"Component List"};
  for (NSString *file in metadata) {
    NSString *path = [fixture.directory stringByAppendingPathComponent:file];
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) continue;
    NSData *bytes = ReadAsset(path, 16 * 1024 * 1024, error);
    id value = bytes ? [NSJSONSerialization JSONObjectWithData:bytes options:0 error:error] : nil;
    Class type = [file isEqualToString:@"config.json"] ? NSDictionary.class : NSArray.class;
    if (![value isKindOfClass:type]) {
      FixtureError(error, @"Invalid fixture metadata");
      return nil;
    }
    record[metadata[file]] = value;
  }
  fixture->_record = record;
  return fixture;
#endif
}

- (void)dealloc {
  if (_directory) [[NSFileManager defaultManager] removeItemAtPath:_directory error:nil];
}
@end
