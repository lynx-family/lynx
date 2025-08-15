// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.network;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import org.junit.Before;
import org.junit.Test;

public class HttpStreamingDelegateTest {
  class MockLynxFetchModuleEventSender extends LynxFetchModuleEventSender {
    public ArrayList<JavaOnlyMap> eventList = new ArrayList<>();
    @Override
    public void sendGlobalEvent(String name, JavaOnlyArray params) {
      eventList.add(params.getMap(0));
    }
  }

  private HttpStreamingDelegate processor;
  private MockLynxFetchModuleEventSender sender;

  @Before
  public void setUp() {
    sender = new MockLynxFetchModuleEventSender();
    processor = new HttpStreamingDelegate("", sender);
  }

  @Test
  public void testGetStreamingBytesToReadValidChunk() throws IOException {
    String input = "A\r\n";
    InputStream is = new ByteArrayInputStream(input.getBytes());
    BufferedInputStream bi = new BufferedInputStream(is);
    StringBuilder sb = new StringBuilder();

    int result = processor.getStreamingBytesToRead(bi, sb);

    assertEquals(10, result); // A in hex is 10 in decimal
    assertEquals(0, sb.length()); // StringBuilder should be cleared
  }

  @Test
  public void testGetStreamingBytesToReadMalformedEnding() throws IOException {
    String input = "A\rX"; // missing \n
    InputStream is = new ByteArrayInputStream(input.getBytes());
    BufferedInputStream bi = new BufferedInputStream(is);
    StringBuilder sb = new StringBuilder();

    int result = processor.getStreamingBytesToRead(bi, sb);

    assertEquals(-1, result);
    assertEquals(sender.eventList.size(), 1);
    assertEquals(sender.eventList.get(0).getString("event"), "onError");
    assertEquals(sender.eventList.get(0).getString("error"),
        HttpStreamingDelegate.ERROR_STREAMING_MALFORMED_RESPONSE);
  }

  @Test
  public void testGetStreamingBytesToReadEmptyStream() throws IOException {
    String input = "";
    InputStream is = new ByteArrayInputStream(input.getBytes());
    BufferedInputStream bi = new BufferedInputStream(is);
    StringBuilder sb = new StringBuilder();

    int result = processor.getStreamingBytesToRead(bi, sb);

    assertEquals(-1, result);
  }

  @Test
  public void testGetStreamingChunkValidChunk() throws IOException {
    String input = "1234567890\r\n";
    InputStream is = new ByteArrayInputStream(input.getBytes());
    BufferedInputStream bi = new BufferedInputStream(is);

    byte[] result = processor.getStreamingChunk(10, bi);

    assertEquals(10, result.length);
    assertEquals("1234567890", new String(result));
  }

  @Test
  public void testGetStreamingChunkMalformedEnding() throws IOException {
    String input = "1234567890\rX"; // missing \n
    InputStream is = new ByteArrayInputStream(input.getBytes());
    BufferedInputStream bi = new BufferedInputStream(is);

    byte[] result = processor.getStreamingChunk(10, bi);

    assertEquals(0, result.length);
    assertEquals(sender.eventList.size(), 1);
    assertEquals(sender.eventList.get(0).getString("event"), "onError");
    assertEquals(sender.eventList.get(0).getString("error"),
        HttpStreamingDelegate.ERROR_STREAMING_MALFORMED_RESPONSE);
  }

  @Test
  public void testGetStreamingChunkIncompleteData() throws IOException {
    String input = "12345\r\n"; // only 5 bytes instead of 10
    InputStream is = new ByteArrayInputStream(input.getBytes());
    BufferedInputStream bi = new BufferedInputStream(is);

    byte[] result = processor.getStreamingChunk(10, bi);

    assertEquals(0, result.length);
    assertEquals(sender.eventList.size(), 1);
    assertEquals(sender.eventList.get(0).getString("event"), "onError");
    assertEquals(sender.eventList.get(0).getString("error"),
        HttpStreamingDelegate.ERROR_STREAMING_MALFORMED_RESPONSE);
  }

  @Test
  public void testStreamingBodySingleChunk() throws IOException {
    String input = "A\r\n1234567890\r\n0\r\n\r\n";
    InputStream is = new ByteArrayInputStream(input.getBytes());

    processor.streamingBody(is);
    assertEquals(sender.eventList.size(), 1);
    assertArrayEquals(sender.eventList.get(0).getByteArray("data"), "1234567890".getBytes());
  }

  @Test
  public void testStreamingBodyMultipleChunks() throws IOException {
    String input = "5\r\n12345\r\n5\r\n67890\r\n0\r\n\r\n";
    InputStream is = new ByteArrayInputStream(input.getBytes());

    processor.streamingBody(is);

    assertEquals(sender.eventList.size(), 2);
    assertArrayEquals(sender.eventList.get(0).getByteArray("data"), "12345".getBytes());
    assertArrayEquals(sender.eventList.get(1).getByteArray("data"), "67890".getBytes());
  }

  @Test
  public void testStreamingBodyEmptyChunk() throws IOException {
    String input = "0\r\n\r\n";
    InputStream is = new ByteArrayInputStream(input.getBytes());

    processor.streamingBody(is);

    assertEquals(sender.eventList.size(), 0);
  }

  @Test
  public void testStreamingBodyMalformedChunk() throws IOException {
    String input = "5\r\n1234\r\n"; // missing \r\n after data
    InputStream is = new ByteArrayInputStream(input.getBytes());

    processor.streamingBody(is);

    assertEquals(sender.eventList.size(), 1);
    assertEquals(sender.eventList.get(0).getString("event"), "onError");
    assertEquals(sender.eventList.get(0).getString("error"),
        HttpStreamingDelegate.ERROR_STREAMING_MALFORMED_RESPONSE);
  }

  @Test
  public void testStreamingBodySSE() throws IOException {
    String input = "12345\n\n1234567890\n\n";
    InputStream is = new ByteArrayInputStream(input.getBytes());

    processor.streamingBodySSE(is);

    assertEquals(sender.eventList.size(), 2);
    assertArrayEquals(sender.eventList.get(0).getByteArray("data"), "12345\n\n".getBytes());
    assertArrayEquals(sender.eventList.get(1).getByteArray("data"), "1234567890\n\n".getBytes());
  }
}
