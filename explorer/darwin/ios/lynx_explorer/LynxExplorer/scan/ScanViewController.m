// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ScanViewController.h"
#import "TasmDispatcher.h"

@interface ScanViewController ()

@property(nonatomic, strong) AVCaptureSession *captureSession;
@property(nonatomic, strong) AVCaptureVideoPreviewLayer *captureLayer;
@property(nonatomic, strong) UIView *sanFrameView;

@end

@implementation ScanViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  self.edgesForExtendedLayout = UIRectEdgeNone;
  self.navigationItem.title = @"Scan";

  [self prepareForScan];
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
  [self.view.layer addSublayer:_captureLayer];
  [_captureSession startRunning];
}

- (void)viewDidDisappear:(BOOL)animated {
  [super viewDidDisappear:animated];

  [_captureLayer removeFromSuperlayer];
  [_captureSession stopRunning];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
    didOutputMetadataObjects:(NSArray *)metadataObjects
              fromConnection:(AVCaptureConnection *)connection {
  [_captureLayer removeFromSuperlayer];
  [_captureSession stopRunning];
  if (metadataObjects.count > 0) {
    AVMetadataMachineReadableCodeObject *metadataObject = [metadataObjects objectAtIndex:0];
    NSString *result = metadataObject.stringValue;
    [self pushLynxViewShellVCWithUrl:result];
  }
}

- (void)pushLynxViewShellVCWithUrl:(NSString *)url {
  [[TasmDispatcher sharedInstance] openTargetUrl:url];
}

@end
