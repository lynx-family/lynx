// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service.security;

/**
 * Verify Result of SecurityService
 */
public class SecurityResult {
  private SecurityResult() {}
  /**
   * verified: true if passed, else false;
   */
  private boolean verified = false;

  /**
   * errorMsg: reason for verify failed, null if success;
   */
  private String errorMsg;

  /**
   * Acquire the reason for verify failed.
   *
   * @return reason for failed.
   */
  public String getErrorMsg() {
    return this.errorMsg;
  }

  /**
   * Check if the TASM has been verified.
   *
   * @return true if passed, else false;
   */
  public boolean isVerified() {
    return this.verified;
  }

  public static SecurityResult onSuccess() {
    SecurityResult result = new SecurityResult();
    result.verified = true;
    return result;
  }

  public static SecurityResult onReject(String error) {
    SecurityResult result = new SecurityResult();
    result.verified = false;
    result.errorMsg = error;
    return result;
  }
}
