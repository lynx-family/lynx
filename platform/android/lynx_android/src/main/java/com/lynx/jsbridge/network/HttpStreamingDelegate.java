// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.network;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class HttpStreamingDelegate {
  private final String mStringId;
  private final LynxFetchModuleEventSender mSender;
  static final String ERROR_STREAMING_MALFORMED_RESPONSE = "errorStreamingMalformedResponse";

  public HttpStreamingDelegate(String streamingId, LynxFetchModuleEventSender sender) {
    this.mStringId = streamingId;
    this.mSender = sender;
  }

  public void onData(byte[] bytes) {
    JavaOnlyMap result = new JavaOnlyMap();
    result.putString("event", "onData");
    result.putByteArray("data", bytes);
    JavaOnlyArray params = new JavaOnlyArray();
    params.pushMap(result);
    mSender.sendGlobalEvent(mStringId, params);
  }

  public void onEnd() {
    JavaOnlyMap result = new JavaOnlyMap();
    result.putString("event", "onEnd");
    JavaOnlyArray params = new JavaOnlyArray();
    params.pushMap(result);
    mSender.sendGlobalEvent(mStringId, params);
  }

  public void onError(String error) {
    JavaOnlyMap result = new JavaOnlyMap();
    result.putString("event", "onError");
    result.putString("error", error);
    JavaOnlyArray params = new JavaOnlyArray();
    params.pushMap(result);
    mSender.sendGlobalEvent(mStringId, params);
  }

  // find chunk length follow by '\r\n'
  int getStreamingBytesToRead(BufferedInputStream bi, StringBuilder lengthStringBuilder)
      throws IOException {
    int lengthChar;
    while (true) {
      lengthChar = bi.read();
      if (lengthChar == -1 || lengthChar == '\r') {
        break;
      }
      lengthStringBuilder.append((char) lengthChar);
    }

    if (lengthChar != '\r' || bi.read() != '\n') {
      onError(ERROR_STREAMING_MALFORMED_RESPONSE);
      return -1;
    }

    String lengthString = lengthStringBuilder.toString();
    lengthStringBuilder.setLength(0);

    return Integer.parseInt(lengthString, 16);
  }

  // read chunk content follow by '\r\n'
  byte[] getStreamingChunk(int bytesToRead, BufferedInputStream bi) throws IOException {
    byte[] bytes = new byte[bytesToRead];
    int totalBytesRead = 0;
    while (totalBytesRead < bytesToRead) {
      int bytesRead = bi.read(bytes, totalBytesRead, bytesToRead - totalBytesRead);
      if (bytesRead == -1) {
        break;
      }
      totalBytesRead += bytesRead;
    }

    if (totalBytesRead != bytesToRead || bi.read() != '\r' || bi.read() != '\n') {
      onError(ERROR_STREAMING_MALFORMED_RESPONSE);
      return new byte[0];
    }
    return bytes;
  }

  // streaming chunk split by '\n\n'
  // see:
  // https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events
  public void streamingBodySSE(InputStream in) throws IOException {
    ByteArrayOutputStream buffer = new ByteArrayOutputStream();
    int prev = -1;
    int curr;
    while ((curr = in.read()) != -1) {
      buffer.write(curr);
      if (prev == '\n' && curr == '\n') {
        onData(buffer.toByteArray());
        buffer.reset();
        prev = -1;
        continue;
      }
      prev = curr;
    }
  }

  // split chunk defined by `Transfer-Encoding: chunked`:
  // see: https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Transfer-Encoding
  public void deprecatedChunkedStreamingBody(InputStream in) throws IOException {
    BufferedInputStream bi = new BufferedInputStream(in);
    StringBuilder lengthStringBuilder = new StringBuilder();
    while (true) {
      int bytesToRead = getStreamingBytesToRead(bi, lengthStringBuilder);
      if (bytesToRead <= 0) {
        break;
      }

      byte[] chunk = getStreamingChunk(bytesToRead, bi);
      if (chunk.length == 0) {
        break;
      }
      onData(chunk);
    }
    bi.close();
  }

  public void streamingBody(InputStream in) throws IOException {
    try (BufferedInputStream bufferedIn = new BufferedInputStream(in)) {
      byte[] transportBuffer = new byte[8192];
      int bytesRead;

      while ((bytesRead = bufferedIn.read(transportBuffer)) != -1) {
        byte[] actualData = new byte[bytesRead];
        System.arraycopy(transportBuffer, 0, actualData, 0, bytesRead);
        onData(actualData);
      }
    }
  }
}
