// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testing.base.instrumentation;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TimeZone;
import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;

public class TaskResult {
  private static final String TAG = TaskResult.class.getSimpleName();

  private Description testRun;

  private Map<String, CaseResult> caseMap = new HashMap<>();

  private List<CaseResult> failedTests = new ArrayList<>();
  private List<CaseResult> ignoredTests = new ArrayList<>();
  private List<CaseResult> passedTests = new ArrayList<>();

  private long startTime = 0;

  public void runStarted(Description description) {
    startTime = System.currentTimeMillis();
    testRun = description;
  }

  public void testStarted(Description description) throws Exception {
    caseMap.put(getDescriptionKey(description), new CaseResult(description));
  }

  public void testFailure(Failure failure) throws Exception {
    CaseResult caseResult = caseMap.get(getDescriptionKey(failure.getDescription()));
    caseResult.recordFailure(failure);
    failedTests.add(caseResult);
  }

  public void testIgnored(Description description) throws Exception {
    CaseResult caseResult = caseMap.get(getDescriptionKey(description));

    if (caseResult == null) {
      caseResult = new CaseResult(description);
      caseMap.put(getDescriptionKey(description), caseResult);
    }

    caseResult.recordTestIgnored();
    ignoredTests.add(caseResult);
  }

  public void testFinished(Description description) throws Exception {
    CaseResult caseResult = caseMap.get(getDescriptionKey(description));

    if (caseResult == null) {
      return;
    }

    caseResult.recordFinished();

    if (caseResult.getStatus() == CaseResult.Status.STARTED) {
      passedTests.add(caseResult);
    }
  }

  public void runFinished(Result result) {
    return;
  }

  public String getTestSuiteName() {
    return testRun.getDisplayName();
  }

  public Map<String, CaseResult> getCaseMap() {
    return caseMap;
  }

  public List<CaseResult> getPassedTests() {
    return passedTests;
  }

  public List<CaseResult> getIgnoredTests() {
    return ignoredTests;
  }

  public List<CaseResult> getFailedTests() {
    return failedTests;
  }

  public String startTimeAsIso() {
    DateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm'Z'");
    df.setTimeZone(TimeZone.getTimeZone("UTC"));
    return df.format(new Date(startTime));
  }

  private String getDescriptionKey(Description description) {
    return description.getClassName() + "." + description.getMethodName();
  }
}
