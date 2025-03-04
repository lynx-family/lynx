// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testing.base.instrumentation;

import android.app.Instrumentation;
import android.os.Bundle;
import android.util.Log;
import android.util.Xml;
import androidx.test.internal.runner.listener.InstrumentationRunListener;
import androidx.test.platform.app.InstrumentationRegistry;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;
import java.util.TimeZone;
import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;
import org.xmlpull.v1.XmlSerializer;

public class LynxInstrumentTestListener extends InstrumentationRunListener {
  private static final String TAG = LynxInstrumentTestListener.class.getSimpleName();

  private static final String NAMESPACE = null;

  private static final String TAG_SUITES = "testsuites";
  private static final String TAG_SUITE = "testsuite";
  private static final String TAG_CASE = "testcase";
  private static final String TAG_FAILURE = "failure";
  private static final String TAG_SKIPPED = "skipped";

  private static final String ATTRIBUTE_CLASS = "classname";
  private static final String ATTRIBUTE_ERRORS = "errors";
  private static final String ATTRIBUTE_FAILURES = "failures";
  private static final String ATTRIBUTE_MESSAGE = "message";
  private static final String ATTRIBUTE_NAME = "name";
  private static final String ATTRIBUTE_SKIPPED = "skipped";
  private static final String ATTRIBUTE_TESTS = "tests";
  private static final String ATTRIBUTE_TIME = "time";
  private static final String ATTRIBUTE_TIMESTAMP = "timestamp";

  private FileOutputStream mOutputStream;

  private final XmlSerializer mXmlSerializer;

  private TaskResult mTaskResult;

  private String mModuleName;

  private boolean mInitSuccess = true;

  private long mStartTime = 0;

  private long mEndTime = 0;

  public LynxInstrumentTestListener() {
    this(Xml.newSerializer());
  }

  public LynxInstrumentTestListener(XmlSerializer xmlSerializer) {
    this.mXmlSerializer = xmlSerializer;
  }

  @Override
  public void setInstrumentation(Instrumentation instrumentation) {
    super.setInstrumentation(instrumentation);

    Bundle extraAttrs = InstrumentationRegistry.getArguments();
    if (extraAttrs != null) {
      if (extraAttrs.containsKey("module")) {
        mModuleName = extraAttrs.getString("module");
      }
    }

    try {
      File outputFile = getOutputFile(instrumentation);
      mOutputStream = new FileOutputStream(outputFile);
    } catch (IOException e) {
      Log.e(TAG, "Unable to open report file", e);
      mInitSuccess = false;
      return;
    }

    try {
      mXmlSerializer.setOutput(mOutputStream, "utf-8");
      mXmlSerializer.startDocument("utf-8", true);
    } catch (IOException e) {
      Log.e(TAG, "Unable to open serializer", e);
      mInitSuccess = false;
      return;
    }
  }

  protected File getOutputFile(Instrumentation instrumentation) throws IOException {
    String fileName = getFile(
        ((mModuleName != null && mModuleName.length() > 0) ? mModuleName + "_" : "") + "output.xml",
        instrumentation);
    return new File(instrumentation.getTargetContext().getExternalFilesDir(null), fileName);
  }

  private String getFile(String fileName, Instrumentation instr) throws IOException {
    File file = new File(instr.getTargetContext().getExternalFilesDir(null), fileName);
    if (file.exists()) {
      // If report file is already exist, rename the old file format like
      // $moduleName_output_$fileCreateTime.xml.
      long lastModified = file.lastModified();
      Date date = new Date(lastModified);
      SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd_HH:mm:ss");
      String formatDate = dateFormat.format(date);
      String newFileName = fileName.substring(0, fileName.length() - 4) + "_" + formatDate + ".xml";
      File newFile = new File(instr.getTargetContext().getExternalFilesDir(null), newFileName);
      file.renameTo(newFile);
      file = new File(instr.getTargetContext().getExternalFilesDir(null), fileName);
    }
    return file.getName();
  }

  @Override
  public void testRunStarted(Description description) throws Exception {
    mStartTime = System.currentTimeMillis();
    mTaskResult = new TaskResult();
    mTaskResult.runStarted(description);
  }

  @Override
  public void testRunFinished(Result result) throws Exception {
    mEndTime = System.currentTimeMillis();
    mTaskResult.runFinished(result);
    try {
      outputResult();
    } catch (IOException e) {
      Log.e(TAG, "Write report xml error!", e);
    } finally {
      try {
        if (mOutputStream != null) {
          mOutputStream.close();
        }
      } catch (IOException e) {
        Log.e(TAG, "Close OutputStream failed!", e);
      }
    }
  }

  @Override
  public void testStarted(Description description) throws Exception {
    mTaskResult.testStarted(description);
  }

  @Override
  public void testFinished(Description description) throws Exception {
    mTaskResult.testFinished(description);
  }

  @Override
  public void testFailure(Failure failure) throws Exception {
    mTaskResult.testFailure(failure);
  }

  @Override
  public void testIgnored(Description description) throws Exception {
    mTaskResult.testIgnored(description);
  }

  private void outputResult() throws IOException {
    if (!mInitSuccess) {
      throw new IOException("xmlSerializer init failed, skip output result to xml.");
    }
    mXmlSerializer.startTag(NAMESPACE, TAG_SUITES);
    mXmlSerializer.startTag(NAMESPACE, TAG_SUITE);
    String name = mTaskResult.getTestSuiteName();
    if (name != null && name.isEmpty()) {
      mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_NAME, name);
    }
    mXmlSerializer.attribute(
        NAMESPACE, ATTRIBUTE_TESTS, Integer.toString(mTaskResult.getCaseMap().size()));
    mXmlSerializer.attribute(
        NAMESPACE, ATTRIBUTE_FAILURES, Integer.toString(mTaskResult.getFailedTests().size()));
    mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_NAME, mModuleName == null ? "" : mModuleName);

    mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_ERRORS, "0");
    mXmlSerializer.attribute(
        NAMESPACE, ATTRIBUTE_SKIPPED, Integer.toString(mTaskResult.getIgnoredTests().size()));
    float caseSetElapsedTime = (float) (mEndTime - mStartTime) / 1000;
    mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_TIME, String.format("%.4f", caseSetElapsedTime));
    mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_TIMESTAMP, formatDateString(mEndTime));

    Map<String, CaseResult> mTestResults = mTaskResult.getCaseMap();
    for (Map.Entry<String, CaseResult> testEntry : mTestResults.entrySet()) {
      mXmlSerializer.startTag(NAMESPACE, TAG_CASE);
      String className = testEntry.getValue().getDescriptor().getClassName();
      String caseName = testEntry.getValue().getDescriptor().getMethodName();
      mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_NAME, className + "." + caseName);
      mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_CLASS, className);
      float elapsedTime = (float) testEntry.getValue().getElapsedTime() / 1000;
      mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_TIME, String.format("%.4f", elapsedTime));
      mXmlSerializer.attribute(
          NAMESPACE, ATTRIBUTE_TIMESTAMP, formatDateString(testEntry.getValue().getEndTime()));

      switch (testEntry.getValue().getStatus()) {
        case FAILURE:
          Failure failure = testEntry.getValue().getFailure();
          mXmlSerializer.startTag(NAMESPACE, TAG_FAILURE);

          mXmlSerializer.attribute(NAMESPACE, ATTRIBUTE_MESSAGE, sanitize(failure.getTrace()));
          mXmlSerializer.endTag(NAMESPACE, TAG_FAILURE);
          break;

        case IGNORED:
          mXmlSerializer.startTag(NAMESPACE, TAG_SKIPPED);
          mXmlSerializer.endTag(NAMESPACE, TAG_SKIPPED);
          break;
      }

      mXmlSerializer.endTag(NAMESPACE, TAG_CASE);
    }

    mXmlSerializer.endTag(NAMESPACE, TAG_SUITE);
    mXmlSerializer.endTag(NAMESPACE, TAG_SUITES);
    mXmlSerializer.endDocument();
    mXmlSerializer.flush();
  }

  /**
   * Returns the text in a format that is safe for use in an XML document.
   */
  private String sanitize(String text) {
    return text.replace("\0", "<\\0>");
  }

  private String formatDateString(long timestamp) {
    DateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm'Z'");
    df.setTimeZone(TimeZone.getTimeZone("UTC"));
    return df.format(new Date(timestamp));
  }
}
