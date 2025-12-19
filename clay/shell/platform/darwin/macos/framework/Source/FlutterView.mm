// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterView.h"

#import "clay/shell/platform/darwin/macos/framework/Source/ClayViewProvider_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterSurfaceManager.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterThreadSynchronizer.h"

#import <QuartzCore/QuartzCore.h>

@interface FlutterView () <FlutterSurfaceManagerDelegate> {
  __weak id<FlutterViewReshapeListener> _reshapeListener;
  FlutterThreadSynchronizer* _threadSynchronizer;
  FlutterSurfaceManager* _surfaceManager;
}

@end

@implementation FlutterView

- (instancetype)initWithMTLDevice:(id<MTLDevice>)device
                     commandQueue:(id<MTLCommandQueue>)commandQueue
                  reshapeListener:(id<FlutterViewReshapeListener>)reshapeListener {
  self = [super initWithFrame:NSZeroRect];
  if (self) {
    [self setWantsLayer:YES];
    [self setBackgroundColor:[NSColor clearColor]];
    [self setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];
    _reshapeListener = reshapeListener;
    _threadSynchronizer = [[FlutterThreadSynchronizer alloc] init];
    _surfaceManager = [[FlutterSurfaceManager alloc] initWithDevice:device
                                                       commandQueue:commandQueue
                                                              layer:self.layer
                                                           delegate:self];
    NSArray* allowedTypes = @[ NSFilenamesPboardType, NSPasteboardTypeString ];
    [self registerForDraggedTypes:allowedTypes];
  }
  return self;
}

- (void)onPresent:(CGSize)frameSize withBlock:(dispatch_block_t)block {
  [_threadSynchronizer performCommit:frameSize notify:block];
}

- (FlutterSurfaceManager*)surfaceManager {
  return _surfaceManager;
}

- (FlutterThreadSynchronizer*)threadSynchronizer {
  return _threadSynchronizer;
}

- (void)reshaped {
  if ([self isHidden]) {
    return;
  }
  CGSize scaledSize = [self convertSizeToBacking:self.bounds.size];
  [_threadSynchronizer beginResize:scaledSize
                            notify:^{
                              [_reshapeListener viewDidReshape:self];
                            }];
}

- (void)setBackgroundColor:(NSColor*)color {
  self.layer.backgroundColor = color.CGColor;
}

#pragma mark - NSView overrides

- (void)setFrameSize:(NSSize)newSize {
  [super setFrameSize:newSize];
  [self reshaped];
}

/**
 * Declares that the view uses a flipped coordinate system, consistent with Flutter conventions.
 */
- (BOOL)isFlipped {
  return YES;
}

- (BOOL)isOpaque {
  return YES;
}

/**
 * Declares that the initial mouse-down when the view is not in focus will send an event to the
 * view.
 */
- (BOOL)acceptsFirstMouse:(NSEvent*)event {
  return YES;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)cursorUpdate:(NSEvent*)event {
  // When adding/removing views AppKit will schedule call to current hit-test view
  // cursorUpdate: at the end of frame to determine possible cursor change. If
  // the view doesn't implement cursorUpdate: AppKit will set the default (arrow) cursor
  // instead. This would replace the cursor set by FlutterMouseCursorPlugin.
  // Empty cursorUpdate: implementation prevents this behavior.
  // https://github.com/flutter/flutter/issues/111425
}

- (void)viewDidChangeBackingProperties {
  [super viewDidChangeBackingProperties];
  // Force redraw
  [_reshapeListener viewDidReshape:self];
}

- (BOOL)layer:(CALayer*)layer
    shouldInheritContentsScale:(CGFloat)newScale
                    fromWindow:(NSWindow*)window {
  return YES;
}

- (void)shutdown {
  [_threadSynchronizer shutdown];
}
#pragma mark - NSAccessibility overrides

- (BOOL)isAccessibilityElement {
  return YES;
}

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityGroupRole;
}

- (NSString*)accessibilityLabel {
  // TODO(chunhtai): Provides a way to let developer customize the accessibility
  // label.
  // https://github.com/flutter/flutter/issues/75446
  NSString* applicationName =
      [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
  if (!applicationName) {
    applicationName = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleName"];
  }
  return applicationName;
}

#pragma mark - NSResponder

#define DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(METHOD) \
  self.provider ? [self.provider METHOD:event] : [super METHOD:event]

// Forward NSResponder events to provider.
- (void)mouseDown:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(mouseDown);
}

- (void)rightMouseDown:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(rightMouseDown);
}

- (void)otherMouseDown:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(otherMouseDown);
}

- (void)mouseUp:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(mouseUp);
}

- (void)rightMouseUp:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(rightMouseUp);
}

- (void)otherMouseUp:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(otherMouseUp);
}

- (void)mouseMoved:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(mouseMoved);
}

- (void)mouseDragged:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(mouseDragged);
}

- (void)scrollWheel:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(scrollWheel);
}

- (void)magnifyWithEvent:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(magnifyWithEvent);
}

- (void)rightMouseDragged:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(rightMouseDragged);
}

- (void)otherMouseDragged:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(otherMouseDragged);
}

- (void)mouseEntered:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(mouseEntered);
}

- (void)mouseExited:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(mouseExited);
}

- (void)keyDown:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(keyDown);
}

- (void)keyUp:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(keyUp);
}

- (void)flagsChanged:(NSEvent*)event {
  DELEGATE_EVENT_TO_PROVIDER_OR_SUPER(flagsChanged);
}

#undef DELEGATE_EVENT_TO_PROVIDER_OR_SUPER

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
  if (self.provider) {
    NSPoint locationInWindow = [self convertPoint:[sender draggingLocation] fromView:nil];
    CGFloat scale = self.window.backingScaleFactor;
    NSPoint dropPoint = NSMakePoint(locationInWindow.x * scale, locationInWindow.y * scale);
    [self.provider performMouseDragEnterAndOverAtPoint:dropPoint];
  }
  if ([sender draggingSourceOperationMask] & NSDragOperationCopy) {
    if ([sender draggingPasteboard] == [NSPasteboard pasteboardWithName:NSDragPboard]) {
      return NSDragOperationCopy;
    } else {
      return NSDragOperationGeneric;
    }
  }
  return NSDragOperationNone;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
  if (self.provider) {
    NSPoint locationInWindow = [self convertPoint:[sender draggingLocation] fromView:nil];
    CGFloat scale = self.window.backingScaleFactor;
    NSPoint dropPoint = NSMakePoint(locationInWindow.x * scale, locationInWindow.y * scale);
    [self.provider performMouseDragEnterAndOverAtPoint:dropPoint];
  }
  if ([sender draggingSourceOperationMask] & NSDragOperationCopy) {
    if ([sender draggingPasteboard] == [NSPasteboard pasteboardWithName:NSDragPboard]) {
      return NSDragOperationCopy;
    } else {
      return NSDragOperationGeneric;
    }
  }
  return NSDragOperationNone;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
  if (self.provider) {
    [self.provider performMouseDragLeave];
  }
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
  if (!self.provider) {
    return NO;
  }
  NSPoint locationInWindow = [self convertPoint:[sender draggingLocation] fromView:nil];
  CGFloat scale = self.window.backingScaleFactor;
  NSPoint dropPoint = NSMakePoint(locationInWindow.x * scale, locationInWindow.y * scale);
  NSPasteboard* pasteboard = [sender draggingPasteboard];
  NSArray* urls = [pasteboard readObjectsForClasses:@[ [NSURL class] ] options:nil];
  NSString* text = [pasteboard stringForType:NSPasteboardTypeString];
  if ([urls count] > 0) {
    NSMutableArray* pathArray = [NSMutableArray array];
    for (NSURL* url in urls) {
      NSString* filePath = url.path;
      NSDictionary* fileAttributes = [[NSFileManager defaultManager] attributesOfItemAtPath:filePath
                                                                                      error:nil];
      if (fileAttributes) {
        NSDateFormatter* dateFormatter = [[NSDateFormatter alloc] init];
        dateFormatter.dateStyle = NSDateFormatterMediumStyle;
        dateFormatter.timeStyle = NSDateFormatterMediumStyle;

        NSString* fileName = [filePath lastPathComponent];
        NSString* fileType = [fileAttributes objectForKey:NSFileType];
        NSNumber* fileSize = [fileAttributes objectForKey:NSFileSize];
        NSDate* fileModificationDate = [fileAttributes objectForKey:NSFileModificationDate];
        NSString* fileModificationDateString = [dateFormatter stringFromDate:fileModificationDate];
        NSMutableDictionary* file_dictionary = [NSMutableDictionary dictionary];
        [file_dictionary setObject:filePath forKey:kDragDropPathKey];
        [file_dictionary setObject:fileName forKey:kDragDropNameKey];
        [file_dictionary setObject:fileType forKey:kDragDropTypeKey];
        [file_dictionary setObject:fileSize forKey:kDragDropSizeKey];
        [file_dictionary setObject:fileModificationDateString forKey:kDragDropLastModifiedKey];
        [pathArray addObject:file_dictionary];
      }
    }
    [self.provider performMouseDragDropAtPoint:dropPoint type:kDragFileType dropContent:pathArray];
    return YES;
  } else if (text != nil) {
    [self.provider performMouseDragDropAtPoint:dropPoint type:kDragTextType dropContent:text];
    return YES;
  } else {
    return NO;
  }
}

@end
