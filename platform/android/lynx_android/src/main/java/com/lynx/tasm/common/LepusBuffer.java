// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.common;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.json.JSONArray;
import org.json.JSONObject;

@SuppressWarnings({"ConstantConditions", "SameParameterValue"})
public class LepusBuffer implements MessageCodec {
  public static final LepusBuffer INSTANCE = new LepusBuffer();

  private static final String TAG = "LepusBuffer";
  private static final String TRACE_ENCODE_MESSAGE = "LepusBuffer::EncodeMessage";
  private static final String TRACE_DECODE_MESSAGE = "LepusBuffer::DecodeMessage";

  @Deprecated
  public void setDebug(boolean debug) {}

  @Override
  @Nullable
  public ByteBuffer encodeMessage(@NonNull Object message) {
    TraceEvent.beginSection(TRACE_ENCODE_MESSAGE);
    if (message == null) {
      TraceEvent.endSection(TRACE_ENCODE_MESSAGE);
      return null;
    }
    final ExposedByteArrayOutputStream stream = new ExposedByteArrayOutputStream();
    try {
      writeValue(stream, message);
    } catch (IllegalArgumentException e) {
      LynxError error = new LynxError(e.toString(), LynxError.JAVA_ERROR);
      LynxEnv.inst().getLynxViewClient().onReceivedError(error);
      LLog.e(TAG, error.toString());
      TraceEvent.endSection(TRACE_ENCODE_MESSAGE);
      return null;
    }
    final ByteBuffer buffer = ByteBuffer.allocateDirect(stream.size());
    buffer.put(stream.buffer(), 0, stream.size());
    TraceEvent.endSection(TRACE_ENCODE_MESSAGE);
    return buffer;
  }

  @Override
  @Nullable
  public Object decodeMessage(@NonNull ByteBuffer message) {
    TraceEvent.beginSection(TRACE_DECODE_MESSAGE);
    if (message == null) {
      TraceEvent.endSection(TRACE_DECODE_MESSAGE);
      return null;
    }
    try {
      message.order(ByteOrder.nativeOrder());
      final Object value = readValue(message);
      if (message.hasRemaining()) {
        throw new IllegalArgumentException("Message corrupted");
      }
      TraceEvent.endSection(TRACE_DECODE_MESSAGE);
      return value;
    } catch (IllegalArgumentException e) {
      LynxError error = new LynxError(e.toString(), LynxError.JAVA_ERROR);
      if (LynxEnv.inst().getLynxViewClient() != null) {
        LynxEnv.inst().getLynxViewClient().onReceivedError(error);
      }
      TraceEvent.endSection(TRACE_DECODE_MESSAGE);
      return null;
    }
  }

  private static final boolean LITTLE_ENDIAN = ByteOrder.nativeOrder() == ByteOrder.LITTLE_ENDIAN;
  private static final Charset UTF8 = Charset.forName("UTF8");
  private static final byte NULL = 0;
  private static final byte TRUE = 1;
  private static final byte FALSE = 2;
  private static final byte INT = 3;
  private static final byte LONG = 4;
  private static final byte DOUBLE = 5;
  private static final byte STRING = 6;
  private static final byte LIST = 7;
  private static final byte MAP = 8;
  private static final byte BYTE_ARRAY = 9;
  private static final byte TEMPLATE_DATA = 10;
  private static final byte UNDEFINED = 100;

  /**
   * Writes an int representing a size to the specified stream.
   * Uses an expanding code of 1 to 5 bytes to optimize for small values.
   */
  protected static void writeSize(ByteArrayOutputStream stream, int value) {
    if (value < 254) {
      stream.write(value);
    } else if (value <= 0xffff) {
      stream.write(254);
      writeChar(stream, value);
    } else {
      stream.write(255);
      writeInt(stream, value);
    }
  }

  /**
   * Writes the least significant two bytes of the specified int to the
   * specified stream.
   */
  protected static void writeChar(ByteArrayOutputStream stream, int value) {
    if (LITTLE_ENDIAN) {
      stream.write(value);
      stream.write(value >>> 8);
    } else {
      stream.write(value >>> 8);
      stream.write(value);
    }
  }

  /**
   * Writes the specified int as 4 bytes to the specified stream.
   */
  protected static void writeInt(ByteArrayOutputStream stream, int value) {
    if (LITTLE_ENDIAN) {
      stream.write(value);
      stream.write(value >>> 8);
      stream.write(value >>> 16);
      stream.write(value >>> 24);
    } else {
      stream.write(value >>> 24);
      stream.write(value >>> 16);
      stream.write(value >>> 8);
      stream.write(value);
    }
  }

  /**
   * Writes the specified long as 8 bytes to the specified stream.
   */
  protected static void writeLong(ByteArrayOutputStream stream, long value) {
    if (LITTLE_ENDIAN) {
      stream.write((byte) value);
      stream.write((byte) (value >>> 8));
      stream.write((byte) (value >>> 16));
      stream.write((byte) (value >>> 24));
      stream.write((byte) (value >>> 32));
      stream.write((byte) (value >>> 40));
      stream.write((byte) (value >>> 48));
      stream.write((byte) (value >>> 56));
    } else {
      stream.write((byte) (value >>> 56));
      stream.write((byte) (value >>> 48));
      stream.write((byte) (value >>> 40));
      stream.write((byte) (value >>> 32));
      stream.write((byte) (value >>> 24));
      stream.write((byte) (value >>> 16));
      stream.write((byte) (value >>> 8));
      stream.write((byte) value);
    }
  }

  /**
   * Writes the specified double as 8 bytes to the specified stream.
   */
  protected static void writeDouble(ByteArrayOutputStream stream, double value) {
    writeLong(stream, Double.doubleToLongBits(value));
  }

  /**
   * Writes the length and then the actual bytes of the specified array to
   * the specified stream.
   */
  protected static void writeBytes(ByteArrayOutputStream stream, byte[] bytes) {
    writeSize(stream, bytes.length);
    stream.write(bytes, 0, bytes.length);
  }

  /**
   * Writes a number of padding bytes to the specified stream to ensure that
   * the next value is aligned to a whole multiple of the specified alignment.
   * An example usage with alignment = 8 is to ensure doubles are word-aligned
   * in the stream.
   */
  protected static void writeAlignment(ByteArrayOutputStream stream, int alignment) {
    final int mod = stream.size() % alignment;
    if (mod != 0) {
      for (int i = 0; i < alignment - mod; i++) {
        stream.write(0);
      }
    }
  }

  /**
   * Writes a type discriminator byte and then a byte serialization of the
   * specified value to the specified stream.
   *
   * <p>Subclasses can extend the codec by overriding this method, calling
   * super for values that the extension does not handle.</p>
   */
  protected void writeValue(ByteArrayOutputStream stream, Object value) {
    recursiveWriteValue(stream, value, new LinkedList<Object>());
  }

  private void recursiveWriteValue(
      ByteArrayOutputStream stream, Object value, LinkedList<Object> allObjects) {
    if (value == null || value.equals(null)) {
      stream.write(NULL);
    } else if (value instanceof Boolean) {
      stream.write((boolean) value ? TRUE : FALSE);
    } else if (value instanceof Number) {
      if (value instanceof Integer || value instanceof Short || value instanceof Byte) {
        stream.write(INT);
        writeInt(stream, ((Number) value).intValue());
      } else if (value instanceof Long) {
        stream.write(LONG);
        writeLong(stream, (long) value);
      } else if (value instanceof Float || value instanceof Double) {
        stream.write(DOUBLE);
        writeAlignment(stream, 8);
        writeDouble(stream, ((Number) value).doubleValue());
      } else {
        stream.write(NULL);
        LLog.e(TAG, "Unsupported Number type: " + value.getClass() + " value: " + value);
      }
    } else if (value instanceof String) {
      stream.write(STRING);
      writeBytes(stream, ((String) value).getBytes(UTF8));
    } else if (value instanceof List) {
      if (!((List) value).isEmpty() && shallowContains(allObjects, value)) {
        stream.write(NULL);
        LLog.DTHROW(new IllegalArgumentException("writeValue has cycle array!"));
        return;
      }
      allObjects.addLast(value);
      stream.write(LIST);
      final List<?> list = (List) value;
      writeSize(stream, list.size());
      for (final Object o : list) {
        recursiveWriteValue(stream, o, allObjects);
      }
      allObjects.removeLast();
    } else if (value instanceof Map) {
      if (!((Map) value).isEmpty() && shallowContains(allObjects, value)) {
        stream.write(NULL);
        LLog.DTHROW(new IllegalArgumentException("writeValue has cycle dict!"));
        return;
      }
      allObjects.addLast(value);
      stream.write(MAP);
      final Map<?, ?> map = (Map) value;
      writeSize(stream, map.size());
      for (final Entry<?, ?> entry : map.entrySet()) {
        recursiveWriteValue(stream, entry.getKey(), allObjects);
        recursiveWriteValue(stream, entry.getValue(), allObjects);
      }
      allObjects.removeLast();
    } else if (value instanceof JSONObject) {
      final JSONObject jsonObject = ((JSONObject) value);
      if (jsonObject.length() > 0 && shallowContains(allObjects, value)) {
        stream.write(NULL);
        LLog.DTHROW(new IllegalArgumentException("writeValue has cycle JSONObject!"));
        return;
      }
      allObjects.addLast(value);
      stream.write(MAP);
      writeSize(stream, jsonObject.length());
      Iterator<String> keys = jsonObject.keys();
      while (keys.hasNext()) {
        String key = keys.next();
        recursiveWriteValue(stream, key, allObjects);
        recursiveWriteValue(stream, jsonObject.opt(key), allObjects);
      }
      allObjects.removeLast();
    } else if (value instanceof JSONArray) {
      final JSONArray jsonArray = ((JSONArray) value);
      if (jsonArray.length() > 0 && shallowContains(allObjects, value)) {
        stream.write(NULL);
        LLog.DTHROW(new IllegalArgumentException("writeValue has cycle JSONArray!"));
        return;
      }

      allObjects.addLast(value);
      stream.write(LIST);
      writeSize(stream, jsonArray.length());
      for (int i = 0; i < jsonArray.length(); ++i) {
        recursiveWriteValue(stream, jsonArray.opt(i), allObjects);
      }
      allObjects.removeLast();
    } else if (value instanceof byte[]) {
      stream.write(BYTE_ARRAY);
      writeBytes(stream, (byte[]) value);
    } else if (value instanceof TemplateData) {
      ((TemplateData) value).flush();
      stream.write(TEMPLATE_DATA);
      writeLong(stream, ((TemplateData) value).getNativePtr());
    } else {
      stream.write(NULL);
      LLog.e(TAG, "Unsupported type: " + value.getClass() + " value: " + value);
    }
  }

  /**
   * Only judge whether it is the same object
   * @param allObjects containers
   * @param o object to be judged.
   * @return true contains.
   */
  private static boolean shallowContains(@NonNull Collection<Object> allObjects, Object o) {
    if (o == null) {
      return false;
    }
    for (Object item : allObjects) {
      // only compare pointer.
      if (o == item) {
        return true;
      }
    }
    return false;
  }

  /**
   * Reads an int representing a size as written by writeSize.
   */
  protected static int readSize(ByteBuffer buffer) {
    if (!buffer.hasRemaining()) {
      throw new IllegalArgumentException("Message corrupted");
    }
    final int value = buffer.get() & 0xff;
    if (value < 254) {
      return value;
    } else if (value == 254) {
      return buffer.getChar();
    } else {
      return buffer.getInt();
    }
  }

  /**
   * Reads a byte array as written by writeBytes.
   */
  protected static byte[] readBytes(ByteBuffer buffer) {
    final int length = readSize(buffer);
    final byte[] bytes = new byte[length];
    buffer.get(bytes);
    return bytes;
  }

  /**
   * Reads alignment padding bytes as written by writeAlignment.
   */
  protected static void readAlignment(ByteBuffer buffer, int alignment) {
    final int mod = buffer.position() % alignment;
    if (mod != 0) {
      buffer.position(buffer.position() + alignment - mod);
    }
  }

  /**
   * Reads a value as written by writeValue.
   */
  protected final Object readValue(ByteBuffer buffer) {
    if (!buffer.hasRemaining()) {
      throw new IllegalArgumentException("Message corrupted");
    }
    final byte type = buffer.get();
    return readValueOfType(type, buffer);
  }

  /**
   * Reads a value of the specified type.
   *
   * <p>Subclasses may extend the codec by overriding this method, calling
   * super for types that the extension does not handle.</p>
   */
  protected Object readValueOfType(byte type, ByteBuffer buffer) {
    final Object result;
    switch (type) {
      case NULL:
      case UNDEFINED:
        result = null;
        break;
      case TRUE:
        result = true;
        break;
      case FALSE:
        result = false;
        break;
      case INT:
        readAlignment(buffer, 8);
        result = buffer.getInt();
        break;
      case LONG:
        readAlignment(buffer, 8);
        result = buffer.getLong();
        break;
      case DOUBLE:
        readAlignment(buffer, 8);
        result = buffer.getDouble();
        break;
      case STRING: {
        final byte[] bytes = readBytes(buffer);
        result = new String(bytes, UTF8);
        break;
      }
      case LIST: {
        final int size = readSize(buffer);
        final List<Object> list = new ArrayList<>(size);
        for (int i = 0; i < size; i++) {
          list.add(readValue(buffer));
        }
        result = list;
        break;
      }
      case MAP: {
        final int size = readSize(buffer);
        final Map<Object, Object> map = new HashMap<>();
        for (int i = 0; i < size; i++) {
          map.put(readValue(buffer), readValue(buffer));
        }
        result = map;
        break;
      }
      default:
        throw new IllegalArgumentException("Message corrupted");
    }
    return result;
  }

  static final class ExposedByteArrayOutputStream extends ByteArrayOutputStream {
    byte[] buffer() {
      return buf;
    }
  }
}
