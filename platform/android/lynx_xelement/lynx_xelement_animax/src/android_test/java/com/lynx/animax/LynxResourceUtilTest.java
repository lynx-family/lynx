// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax;

import static junit.framework.TestCase.assertNotNull;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.animax.util.LynxResourceUtil;
import com.lynx.tasm.provider.LynxResResponse;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxResourceUtilTest {
  @Rule public TemporaryFolder tempFolder = new TemporaryFolder();

  private static class MockBlockingInputStream extends InputStream {
    private final byte[] data;
    private int position = 0;
    private boolean firstRead = true;

    public MockBlockingInputStream(byte[] data) {
      this.data = data;
    }

    @Override
    public int read() throws IOException {
      if (position < data.length) {
        if (firstRead) {
          firstRead = false;
          return 0; // Simulating the initial blocking behavior
        }
        return data[position++];
      } else {
        return -1; // End of data
      }
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
      if (b == null) {
        throw new NullPointerException();
      } else if (off < 0 || len < 0 || len > b.length - off) {
        throw new IndexOutOfBoundsException();
      } else if (len == 0) {
        return 0;
      }

      if (position >= data.length) {
        return -1; // End of data
      }

      if (firstRead) {
        firstRead = false;
        b[off] = data[position]; // Simulating the initial blocking behavior
        position++;
        return 1;
      }

      int readLength = Math.min(len, data.length - position);
      System.arraycopy(data, position, b, off, readLength);
      position += readLength;
      return readLength;
    }

    @Override
    public int available() throws IOException {
      return firstRead ? 0 : data.length - position;
    }
  }

  @Test
  public void testGetResponseContentLength() {
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    Map<String, List<String>> headers = new HashMap<>();
    headers.put("content-length", Arrays.asList("100"));
    when(mockResponse.getResponseHeaders()).thenReturn(headers);

    assertEquals(100, LynxResourceUtil.getLynxResResponseContentLength(mockResponse));
  }

  @Test
  public void testGetResponseContentLengthNoHeader() {
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getResponseHeaders()).thenReturn(null);

    assertEquals(0, LynxResourceUtil.getLynxResResponseContentLength(mockResponse));
  }

  @Test
  public void testGetTotalLengthForLynxResResponse() throws IOException {
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    InputStream mockStream = new ByteArrayInputStream(new byte[100]);
    when(mockResponse.getInputStream()).thenReturn(mockStream);
    when(mockResponse.getResponseHeaders()).thenReturn(new HashMap<String, List<String>>() {
      { put("content-length", Arrays.asList("100")); }
    });

    assertEquals(100, LynxResourceUtil.getTotalLengthForLynxResResponse(mockResponse));
  }

  @Test
  public void testGetTotalLengthForLynxResResponseWithNullStream() {
    // Mock LynxResResponse to return null for getInputStream
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(null);

    // Call the method under test
    int totalLength = LynxResourceUtil.getTotalLengthForLynxResResponse(mockResponse);

    // Assert that the total length is 0
    assertEquals(0, totalLength);
  }

  @Test
  public void testGetTotalLengthForLynxResResponseWithBlockingStream() throws IOException {
    // Create fake data
    byte[] fakeData = new byte[100];

    // Create our custom MockBlockingInputStream
    InputStream mockStream = new MockBlockingInputStream(fakeData);

    // Mock the LynxResResponse
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(mockStream);

    when(mockResponse.getResponseHeaders()).thenReturn(new HashMap<String, List<String>>() {
      { put("content-length", Arrays.asList("100")); }
    });

    // Call the method under test
    int totalLength = LynxResourceUtil.getTotalLengthForLynxResResponse(mockResponse);

    // Assert the expected length (depends on your method's logic)
    assertEquals(100, totalLength);
  }

  @Test
  public void testGetByteArrayFromLynxResResponse() throws IOException {
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getResponseHeaders()).thenReturn(new HashMap<String, List<String>>() {
      { put("content-length", Arrays.asList("5")); }
    });
    byte[] data = {1, 2, 3, 4, 5};
    InputStream mockStream = new ByteArrayInputStream(data);
    when(mockResponse.getInputStream()).thenReturn(mockStream);

    assertArrayEquals(data, LynxResourceUtil.getByteArrayFromLynxResResponse(mockResponse));
  }

  @Test
  public void testGetByteArrayFromLynxResResponseWithBlockingStream() throws IOException {
    // Create fake data
    byte[] fakeData = new byte[100];
    for (int i = 0; i < fakeData.length; i++) {
      fakeData[i] = (byte) (i % 256); // Fill with some dummy data
    }

    // Create our custom MockBlockingInputStream
    InputStream mockStream = new MockBlockingInputStream(fakeData);

    // Mock the LynxResResponse
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(mockStream);
    when(mockResponse.getResponseHeaders()).thenReturn(new HashMap<String, List<String>>() {
      { put("content-length", Arrays.asList("100")); }
    });

    // Call the method under test
    byte[] result = LynxResourceUtil.getByteArrayFromLynxResResponse(mockResponse);

    // Assert the expected byte array (depends on your method's logic)
    assertNotNull(result);
    assertArrayEquals(fakeData, result);
  }

  @Test
  public void testGetByteArrayFromLynxResResponseWithIOException() throws IOException {
    // Create a mock InputStream
    InputStream mockInputStream = mock(InputStream.class);

    // When read is called on the mock, throw an IOException
    when(mockInputStream.read(any(byte[].class), anyInt(), anyInt()))
        .thenThrow(new IOException("Simulated IOException"));

    // Mock the available() method to return a value
    when(mockInputStream.available()).thenReturn(1000);

    // Mock the LynxResResponse to return the mocked InputStream
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(mockInputStream);

    // Call the method under test
    byte[] result = LynxResourceUtil.getByteArrayFromLynxResResponse(mockResponse);

    // Assert that the result is null
    assertNull(result);

    // Verify that the InputStream was closed
    verify(mockInputStream).close();
  }

  @Test
  public void testGetByteArrayFromLynxResResponseWithIOExceptionOnAvailable() throws IOException {
    // Create a mock InputStream
    InputStream mockInputStream = mock(InputStream.class);

    // Configure the mock to throw an IOException when available() is called
    when(mockInputStream.available())
        .thenThrow(new IOException("Simulated IOException on available"));
    when(mockInputStream.read(any(byte[].class), anyInt(), anyInt()))
        .thenThrow(new IOException("Simulated IOException on read"));

    // Mock the LynxResResponse to return the mocked InputStream
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(mockInputStream);

    // Call the method under test
    byte[] result = LynxResourceUtil.getByteArrayFromLynxResResponse(mockResponse);

    // Assert that the result is null
    assertNull(result);

    // Verify that the InputStream's close method was called
    verify(mockInputStream).close();
  }

  @Test
  public void testGetByteArrayFromLynxResResponseWithLargeContentLength() throws IOException {
    // Create fake data to return from the InputStream
    byte[] fakeData = {1, 2, 3, 4, 5}; // Small amount of data
    InputStream fakeInputStream = new ByteArrayInputStream(fakeData);

    // Mock the LynxResResponse
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(fakeInputStream);

    // Mock the response headers to return a large content-length
    Map<String, List<String>> headers =
        Collections.singletonMap("content-length", Collections.singletonList("999999999"));
    when(mockResponse.getResponseHeaders()).thenReturn(headers);

    // Call the method under test
    byte[] result = LynxResourceUtil.getByteArrayFromLynxResResponse(mockResponse);

    // Assert that the result matches the actual data read
    assertArrayEquals(fakeData, result);
  }

  @Test
  public void testSaveFileFromLynxResResponseWithLargeInlineJson() throws IOException {
    // Large inline JSON string (truncated for brevity)
    String jsonString = "{"
        + "\"v\":\"5.5.7\",\"fr\":30,\"ip\":0,\"op\":90,\"w\":1080,\"h\":1920,"
        + new String(new char[100])
              .replace("\0",
                  "\"layers\":[{\"ddd\":0,\"ind\":1,\"ty\":4,\"nm\":\"Shape Layer 1\",\"sr\":1,\"ks\":{...},\"ao\":0,\"shapes\":[{...}],\"ip\":0,\"op\":90,\"st\":0,\"bm\":0}],")
        + "\"assets\":[],\"layers\":[{\"ddd\":0,\"ind\":1,\"ty\":4,\"nm\":\"Shape Layer 1\",\"sr\":1,\"ks\":{...},\"ao\":0,\"shapes\":[{...}],\"ip\":0,\"op\":90,\"st\":0,\"bm\":0}]"
        + "}";
    byte[] jsonData = jsonString.getBytes();

    // Create a ByteArrayInputStream with the JSON data
    InputStream fakeInputStream = new ByteArrayInputStream(jsonData);

    // Mock LynxResResponse to return the fake InputStream
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(fakeInputStream);

    // Create a temporary file for the test
    File tempFile = tempFolder.newFile("output.json");
    tempFile.deleteOnExit(); // Ensure the file is deleted when tests are done

    // Call the method under test
    boolean result =
        LynxResourceUtil.saveFileFromLynxResResponse(mockResponse, tempFile.getAbsolutePath());

    // Verify the result
    assertTrue(result);

    // Read back the file and assert its contents
    byte[] fileContent = Files.readAllBytes(tempFile.toPath());
    assertArrayEquals(jsonData, fileContent);

    tempFile.delete();
  }

  @Test
  public void testGetByteArrayFromLynxResResponseWithNullInputStream() {
    // Mock LynxResResponse to return null for getInputStream
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(null);

    // Call the method under test
    byte[] result = LynxResourceUtil.getByteArrayFromLynxResResponse(mockResponse);

    // Assert that the result is null
    assertNull(result);
  }

  @Test
  public void testGetByteArrayFromLynxResResponseWithMockBlockingStreamWithoutHeader() {
    // Create fake data
    byte[] fakeData = {1, 2, 3, 4, 5};
    InputStream mockStream = new MockBlockingInputStream(fakeData);

    // Mock the LynxResResponse to return the mocked InputStream
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(mockStream);
    when(mockResponse.getResponseHeaders()).thenReturn(null); // No content-length header

    // Call the method under test
    byte[] result = LynxResourceUtil.getByteArrayFromLynxResResponse(mockResponse);

    // Assert that the result matches the fake data
    assertArrayEquals(fakeData, result);
  }

  @Test
  public void testSaveFileFromLynxResResponseWithMockBlockingStreamWithoutHeader()
      throws Exception {
    // Create fake data
    byte[] fakeData = {1, 2, 3, 4, 5};
    InputStream mockStream = new MockBlockingInputStream(fakeData);

    // Mock the LynxResResponse to return the mocked InputStream
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    when(mockResponse.getInputStream()).thenReturn(mockStream);
    when(mockResponse.getResponseHeaders()).thenReturn(null); // No content-length header

    // Create a temporary file for the test
    File tempFile = tempFolder.newFile("output.dat");

    // Call the method under test
    boolean result =
        LynxResourceUtil.saveFileFromLynxResResponse(mockResponse, tempFile.getAbsolutePath());

    // Verify the result
    assertTrue(result);

    // Read back the file and assert its contents
    byte[] fileContent = Files.readAllBytes(tempFile.toPath());
    assertArrayEquals(fakeData, fileContent);
  }

  @Test
  public void testSaveFileFromLynxResResponseWithIOExceptionInCopyStream() throws Exception {
    // Mock LynxResResponse and InputStream
    LynxResResponse mockResponse = mock(LynxResResponse.class);
    InputStream mockInputStream = mock(InputStream.class);

    // Configure the mock InputStream to throw an IOException when read is called
    when(mockInputStream.read(any(byte[].class), anyInt(), anyInt()))
        .thenThrow(new IOException("Simulated IOException"));
    when(mockResponse.getInputStream()).thenReturn(mockInputStream);

    // Use a real file path for the test, or a mock if your environment supports it
    File tempFile = File.createTempFile("test", null);
    tempFile.deleteOnExit();

    // Call the method under test
    boolean result =
        LynxResourceUtil.saveFileFromLynxResResponse(mockResponse, tempFile.getAbsolutePath());

    // Assert that the result is false, indicating the method did not succeed
    assertFalse(result);

    // Verify that read method on the mock InputStream was called
    verify(mockInputStream).read(any(byte[].class), anyInt(), anyInt());

    // Clean up the temporary file
    tempFile.delete();
  }
}
