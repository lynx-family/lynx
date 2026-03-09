// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/ClayOverlayView.h"

#import "clay/shell/platform/darwin/macos/framework/Headers/ClayViewProvider.h"

namespace {

NSPoint DragPointInBacking(NSView* view, id<NSDraggingInfo> sender) {
  NSPoint location_in_view = [view convertPoint:[sender draggingLocation] fromView:nil];
  CGFloat scale = view.window ? view.window.backingScaleFactor : 1.0;
  return NSMakePoint(location_in_view.x * scale, location_in_view.y * scale);
}

NSDragOperation DragOperationForSender(id<NSDraggingInfo> sender) {
  if (!([sender draggingSourceOperationMask] & NSDragOperationCopy)) {
    return NSDragOperationNone;
  }
  if ([sender draggingPasteboard] == [NSPasteboard pasteboardWithName:NSDragPboard]) {
    return NSDragOperationCopy;
  }
  return NSDragOperationGeneric;
}

}  // namespace

@implementation ClayOverlayView {
  NSArray<NSValue*>* _opaqueRects;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self) {
    _opaqueRects = @[];
    // Host overlay content in a transparent layer.
    self.wantsLayer = YES;
    self.layer.opaque = NO;
    self.layer.geometryFlipped = YES;

    self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    self.layer.masksToBounds = NO;

    NSArray* allowedTypes = @[ NSFilenamesPboardType, NSPasteboardTypeString ];
    [self registerForDraggedTypes:allowedTypes];
  }
  return self;
}

- (BOOL)isFlipped {
  return YES;
}

- (BOOL)isOpaque {
  return NO;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
  return YES;
}

- (void)updateOpaqueRects:(NSArray<NSValue*>*)rects {
  _opaqueRects = [rects copy] ?: @[];
  [self.window invalidateCursorRectsForView:self];
}

- (NSRect)viewRectFromDevicePixelRect:(NSRect)rect contentsScale:(CGFloat)scale {
  if (scale <= 0.0) {
    scale = self.window ? self.window.backingScaleFactor : self.layer.contentsScale;
  }
  if (scale <= 0.0) {
    scale = 1.0;
  }

  NSRect view_rect = NSMakeRect(rect.origin.x / scale, rect.origin.y / scale,
                                rect.size.width / scale, rect.size.height / scale);
  return view_rect;
}

- (void)viewDidChangeBackingProperties {
  [super viewDidChangeBackingProperties];
  // Keep overlay content aligned with the current backing scale.
  CGFloat scale = self.window ? self.window.backingScaleFactor : 1.0;
  self.layer.contentsScale = scale;
  self.layer.geometryFlipped = YES;
  [self.window invalidateCursorRectsForView:self];
}

- (NSView*)hitTest:(NSPoint)point {
  // Pass through events outside painted overlay regions.
  for (NSValue* val in _opaqueRects) {
    if (NSPointInRect(point, [val rectValue])) {
      return self;
    }
  }
  return nil;
}

#define FORWARD_EVENT_TO_DELEGATE_OR_SUPER(METHOD) \
  self.eventDelegate ? [self.eventDelegate METHOD:event] : [super METHOD:event]

- (void)mouseDown:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(mouseDown);
}

- (void)rightMouseDown:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(rightMouseDown);
}

- (void)otherMouseDown:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(otherMouseDown);
}

- (void)mouseUp:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(mouseUp);
}

- (void)rightMouseUp:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(rightMouseUp);
}

- (void)otherMouseUp:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(otherMouseUp);
}

- (void)mouseMoved:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(mouseMoved);
}

- (void)mouseDragged:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(mouseDragged);
}

- (void)rightMouseDragged:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(rightMouseDragged);
}

- (void)otherMouseDragged:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(otherMouseDragged);
}

- (void)mouseEntered:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(mouseEntered);
}

- (void)mouseExited:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(mouseExited);
}

- (void)scrollWheel:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(scrollWheel);
}

- (void)magnifyWithEvent:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(magnifyWithEvent);
}

- (void)rotateWithEvent:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(rotateWithEvent);
}

- (void)swipeWithEvent:(NSEvent*)event {
  FORWARD_EVENT_TO_DELEGATE_OR_SUPER(swipeWithEvent);
}

#undef FORWARD_EVENT_TO_DELEGATE_OR_SUPER

- (void)cursorUpdate:(NSEvent*)event {
  // Keep cursor updates delegated to the Flutter cursor plugin.
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
  [self.eventDelegate performMouseDragEnterAndOverAtPoint:DragPointInBacking(self, sender)];
  return DragOperationForSender(sender);
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
  [self.eventDelegate performMouseDragEnterAndOverAtPoint:DragPointInBacking(self, sender)];
  return DragOperationForSender(sender);
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
  [self.eventDelegate performMouseDragLeave];
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
  if (!self.eventDelegate) {
    return NO;
  }
  NSPoint dropPoint = DragPointInBacking(self, sender);
  NSPasteboard* pasteboard = [sender draggingPasteboard];
  NSArray* urls = [pasteboard readObjectsForClasses:@[ [NSURL class] ] options:nil];
  NSString* text = [pasteboard stringForType:NSPasteboardTypeString];
  if ([urls count] > 0) {
    NSMutableArray* pathArray = [NSMutableArray array];
    for (NSURL* url in urls) {
      NSString* filePath = url.path;
      NSDictionary* fileAttributes = [[NSFileManager defaultManager] attributesOfItemAtPath:filePath
                                                                                      error:nil];
      if (!fileAttributes) {
        continue;
      }
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
    [self.eventDelegate performMouseDragDropAtPoint:dropPoint
                                               type:kDragFileType
                                        dropContent:pathArray];
    return YES;
  }
  if (text != nil) {
    [self.eventDelegate performMouseDragDropAtPoint:dropPoint type:kDragTextType dropContent:text];
    return YES;
  }
  return NO;
}

@end
