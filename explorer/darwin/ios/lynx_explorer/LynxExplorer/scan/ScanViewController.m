// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ScanViewController.h"
#import "../LynxExplorerSwiftInterop.h"

static NSString *const kPreferredContainerKey = @"preferredContainer";
static NSString *const kLegacyQRContainerPreferenceKey = @"qrContainerPreference";
static NSString *const kQRContainerPreferenceLegacy = @"legacy";
static NSString *const kQRContainerPreferenceSparkling = @"sparkling";

@interface ScanViewController () <UIAdaptivePresentationControllerDelegate,
                                  UIPopoverPresentationControllerDelegate>

@property(nonatomic, strong) AVCaptureSession *captureSession;
@property(nonatomic, strong) AVCaptureVideoPreviewLayer *captureLayer;
@property(nonatomic, strong) UIView *sanFrameView;
@property(nonatomic, copy) NSString *pendingScanResult;
@property(nonatomic, assign) BOOL presentingContainerChoice;

@end

@implementation ScanViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  self.edgesForExtendedLayout = UIRectEdgeNone;
  self.navigationItem.title = @"Scan";
  self.navigationItem.leftBarButtonItem =
      [[UIBarButtonItem alloc] initWithTitle:@"Back"
                                       style:UIBarButtonItemStylePlain
                                      target:self
                                      action:@selector(backButtonTapped)];

  [self prepareForScan];
}

- (LXRouteCoordinator *)routeCoordinator {
  return [LXRouteCoordinator currentBridge];
}

- (void)backButtonTapped {
  LXRouteCoordinator *coordinator = [self routeCoordinator];
  if (coordinator == nil) {
    return;
  }
  [coordinator closeAnimated:YES
                    callback:^(__unused id payload){
                    }];
}

- (void)prepareForScan {
#if !(TARGET_IPHONE_SIMULATOR)
  _captureSession = [[AVCaptureSession alloc] init];
  [_captureSession setSessionPreset:AVCaptureSessionPresetHigh];
  AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
  AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:nil];
  AVCaptureMetadataOutput *output = [[AVCaptureMetadataOutput alloc] init];
  if (output && input && device) {
    [output setMetadataObjectsDelegate:self queue:dispatch_get_main_queue()];
    [_captureSession addInput:input];
    [_captureSession addOutput:output];
    output.metadataObjectTypes = @[
      AVMetadataObjectTypeQRCode, AVMetadataObjectTypeEAN13Code, AVMetadataObjectTypeEAN8Code,
      AVMetadataObjectTypeCode128Code
    ];
  }

  _captureLayer = [AVCaptureVideoPreviewLayer layerWithSession:_captureSession];
  _captureLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
  _captureLayer.frame = self.view.layer.bounds;
#endif
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self.navigationController setNavigationBarHidden:NO];
  [self resumeScanning];
}

- (void)viewDidDisappear:(BOOL)animated {
  [super viewDidDisappear:animated];

  [_captureLayer removeFromSuperlayer];
  [_captureSession stopRunning];
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  self.captureLayer.frame = self.view.layer.bounds;
}

- (void)resumeScanning {
  if (self.captureLayer != nil && self.captureLayer.superlayer == nil) {
    [self.view.layer insertSublayer:self.captureLayer atIndex:0];
  }
  if (self.captureSession != nil && !self.captureSession.isRunning) {
    [self.captureSession startRunning];
  }
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
    didOutputMetadataObjects:(NSArray *)metadataObjects
              fromConnection:(AVCaptureConnection *)connection {
  if (self.presentingContainerChoice || metadataObjects.count == 0) {
    return;
  }

  AVMetadataMachineReadableCodeObject *metadataObject = metadataObjects.firstObject;
  NSString *result = metadataObject.stringValue;
  if (result.length == 0) {
    [self resumeScanning];
    return;
  }

  self.presentingContainerChoice = YES;
  self.pendingScanResult = result;
  [self.captureLayer removeFromSuperlayer];
  [self.captureSession stopRunning];

  NSString *preference =
      [[NSUserDefaults standardUserDefaults] stringForKey:kPreferredContainerKey];
  if (preference.length == 0) {
    preference =
        [[NSUserDefaults standardUserDefaults] stringForKey:kLegacyQRContainerPreferenceKey];
  }
  if ([preference isEqualToString:kQRContainerPreferenceLegacy]) {
    [self openScannedURL:result container:LXRequestedContainerLegacy];
  } else if ([preference isEqualToString:kQRContainerPreferenceSparkling] &&
             [LXRouteCoordinator supportsSparklingContainer]) {
    [self openScannedURL:result container:LXRequestedContainerSparkling];
  } else {
    [self openScannedURL:result container:LXRequestedContainerLegacy];
  }
}

- (void)openScannedURL:(NSString *)url container:(LXRequestedContainer)container {
  LXRouteCoordinator *coordinator = [LXRouteCoordinator currentBridge];
  if (coordinator == nil) {
    [self resetPendingScanAndResume];
    return;
  }

  __weak typeof(self) weakSelf = self;
  [coordinator openScannerURL:url
           requestedContainer:container
                     callback:^(id payload) {
                       NSDictionary *result =
                           [payload isKindOfClass:NSDictionary.class] ? payload : nil;
                       if (![result[@"success"] boolValue]) {
                         [weakSelf resetPendingScanAndResume];
                       }
                     }];
}

- (void)resetPendingScanAndResume {
  self.presentingContainerChoice = NO;
  self.pendingScanResult = nil;
  [self resumeScanning];
}

#pragma mark - UIAdaptivePresentationControllerDelegate

// Invoked when the container-choice sheet is dismissed interactively (e.g. a
// swipe-to-dismiss) without the user picking a container. Drop the pending scan
// and resume the capture session so scanning keeps working.
- (void)presentationControllerDidDismiss:(UIPresentationController *)presentationController {
  [self resetPendingScanAndResume];
}

#pragma mark - UIPopoverPresentationControllerDelegate

- (void)popoverPresentationControllerDidDismissPopover:
    (UIPopoverPresentationController *)popoverPresentationController {
  [self resetPendingScanAndResume];
}

@end
