// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.scan;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import com.google.zxing.BarcodeFormat;
import com.google.zxing.ResultPoint;
import com.journeyapps.barcodescanner.BarcodeCallback;
import com.journeyapps.barcodescanner.BarcodeResult;
import com.journeyapps.barcodescanner.DecoratedBarcodeView;
import com.journeyapps.barcodescanner.DefaultDecoderFactory;
import com.lynx.explorer.R;
import com.lynx.explorer.shell.TemplateDispatcher;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

public class QRScanActivity extends Activity {
  private static int cameraPermissionReqCode = 250;

  private DecoratedBarcodeView mBarcodeView;
  private String mUrl;

  private BarcodeCallback callback = new BarcodeCallback() {
    @Override
    public void barcodeResult(BarcodeResult result) {
      mUrl = result.getText();

      TemplateDispatcher.dispatchUrl(getApplication(), mUrl);

      finish();
    }

    @Override
    public void possibleResultPoints(List<ResultPoint> resultPoints) {}
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.scan);

    mBarcodeView = findViewById(R.id.barcode_scanner);
    Collection<BarcodeFormat> formats = Arrays.asList(BarcodeFormat.QR_CODE);
    mBarcodeView.getBarcodeView().setDecoderFactory(new DefaultDecoderFactory(formats));
    mBarcodeView.initializeFromIntent(getIntent());
    mBarcodeView.decodeSingle(callback);
  }

  @Override
  protected void onResume() {
    super.onResume();
    if (Build.VERSION.SDK_INT >= 23) {
      openCameraWithPermission();
    } else {
      mBarcodeView.resume();
    }
  }

  @Override
  protected void onPause() {
    super.onPause();
    mBarcodeView.pause();
  }

  @TargetApi(23)
  private void openCameraWithPermission() {
    if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
        == PackageManager.PERMISSION_GRANTED) {
      mBarcodeView.resume();
    } else {
      ActivityCompat.requestPermissions(
          this, new String[] {Manifest.permission.CAMERA}, cameraPermissionReqCode);
    }
  }
}
