// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEnv.h>
#import <Lynx/LynxMeaningfulContentSnapshot.h>
#include "base/trace/native/trace_event.h"
#include "core/services/trace/service_trace_event_def.h"

@implementation LynxMeaningfulContentInfo

@end

@implementation LynxMeaningfulContentSnapshot

- (void)captureView:(UIView*)rootView {
#if ENABLE_TRACE_PERFETTO
  self.traceTimestampUs = (TRACE_TIME_NS() / 1000);
  self.traceThreadId = lynx::perfetto::ThreadTrack::Current();
  if (!rootView || ![LynxEnv sharedInstance].fspScreenshotEnabled) {
    return;
  }
  UIImage* snapshotImage = nil;
#if OS_IOS
  // 1. get UIImage from rootView
  UIGraphicsBeginImageContextWithOptions(rootView.bounds.size, NO, 0.0);
  [rootView.layer renderInContext:UIGraphicsGetCurrentContext()];
  snapshotImage = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  if (!snapshotImage) {
    return;
  }
  // 2. background thread: process image
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    // 2.1 downsample image if necessary
    UIImage* processedImage = snapshotImage;
    CGFloat targetMaxSize = 600.0f;
    CGFloat originalWidth = snapshotImage.size.width;
    CGFloat originalHeight = snapshotImage.size.height;
    CGFloat scaleRatio = 1.0f;
    if (originalWidth > targetMaxSize || originalHeight > targetMaxSize) {
      CGFloat widthRatio = targetMaxSize / originalWidth;
      CGFloat heightRatio = targetMaxSize / originalHeight;
      scaleRatio = MIN(widthRatio, heightRatio);
      // 2.1 downsample image if necessary
      CGSize scaledSize = CGSizeMake(originalWidth * scaleRatio, originalHeight * scaleRatio);
      UIGraphicsBeginImageContextWithOptions(scaledSize, NO, 0.0);
      [snapshotImage drawInRect:CGRectMake(0, 0, scaledSize.width, scaledSize.height)];
      processedImage = UIGraphicsGetImageFromCurrentImageContext();
      UIGraphicsEndImageContext();
    }
    // 2.2 compress image
    CGFloat quality = 0.3f;
    NSData* data = UIImageJPEGRepresentation(processedImage, quality);
    if (!data) {
      return;
    }
    // 2.3 encode image to base64
    NSString* snapshotImageBase64 = [NSString
        stringWithFormat:
            @"data:image/jpeg;base64,%@",
            [data base64EncodedStringWithOptions:NSDataBase64Encoding64CharacterLineLength]];
    // 3. main thread: send trace event
    dispatch_async(dispatch_get_main_queue(), ^{
      TRACE_EVENT_INSTANT(
          LYNX_TRACE_CATEGORY_VITALS, FSP_SNAPSHOT_BASE_64_BITMAP,
          [&, traceTimestampUs = self.traceTimestampUs](lynx::perfetto::EventContext ctx) {
            ctx.event()->set_timestamp_absolute_us(traceTimestampUs);
            if (snapshotImageBase64) {
              ctx.event()->add_debug_annotations(FSP_SNAPSHOT_BASE_64_BITMAP_PROP_DATA,
                                                 snapshotImageBase64.UTF8String);
            }
          });
    });
  });
#endif
#endif
}

@end
