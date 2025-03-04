// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.content.ClipData;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.hardware.Sensor;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.MediaMetadataRetriever;
import android.os.RemoteException;
import android.view.View;
import androidx.annotation.NonNull;

public interface ILynxSystemInvokeService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxSystemInvokeService.class;
  }
  /**
   * Sets the current primary clip on the clipboard.
   * see android.content.ClipboardManager.setPrimaryClip(ClipData) for details.
   */
  void setPrimaryClip(@NonNull ClipData clip) throws RemoteException;

  /**
   * Creates a new Camera object to access a particular hardware camera.
   * see android.hardware.Camera.open(int) for details.
   */
  Camera openCamera(int cameraId) throws RuntimeException;

  /**
   * Disconnects and releases the Camera object resources.
   * You must call this as soon as you're done with the Camera object.
   * see android.hardware.Camera.release() for details.
   */
  void releaseCamera(Camera camera);

  /**
   * Retrieves the meta data value associated with the keyCode.
   * see android.media.MediaMetadataRetriever.extractMetadata(int) for details.
   */
  String extractMetadata(MediaMetadataRetriever retriever, int keyCode);

  /**
   * Registers a SensorEventListener for the given sensor at the given sampling frequency.
   * see android.hardware.SensorManager.registerListener(SensorEventListener, Sensor, int) for
   * details.
   */
  boolean registerSensorListener(
      SensorManager manager, SensorEventListener listener, Sensor sensor, int samplingPeriodUs);

  /**
   *  Unregisters a listener for all sensors.
   * see android.hardware.SensorManager.unregisterListener(SensorEventListener) for details.
   */
  void unregisterSensorListener(SensorManager manager, SensorEventListener listener);

  /**
   * Take screenshot for the given view
   */
  Bitmap takeScreenshot(View view, Bitmap.Config config);

  /**
   * Get locale
   */
  String getLocale();
}
