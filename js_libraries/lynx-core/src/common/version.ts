// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const numberRegExp = /\d+/;
class Version {
  major: number = 0;
  minor: number = 0;
  revision: number = 0;
  build: number = 0;

  // version: major.minor.revision.build
  constructor(version: string) {
    version = String(version);
    [
      this.major = 0,
      this.minor = 0,
      this.revision = 0,
      this.build = 0,
    ] = version.split('.').map((v) => {
      const result = numberRegExp.exec(v);
      if (result && result.length > 0) {
        return +result[0];
      }

      return 0;
    });
  }

  /**
   * Greater Than
   * @param version the version to be compared
   * @returns this > version
   */
  gt(version: string | Version): boolean {
    if (typeof version === 'string') {
      version = new Version(version);
    }

    if (this.major > version.major) {
      return true;
    } else if (this.major < version.major) {
      return false;
    }

    if (this.minor > version.minor) {
      return true;
    } else if (this.minor < version.minor) {
      return false;
    }

    if (this.revision > version.revision) {
      return true;
    } else if (this.revision < version.revision) {
      return false;
    }

    if (this.build > version.build) {
      return true;
    } else if (this.build < version.build) {
      return false;
    }

    // equals
    return false;
  }

  /**
   * EQual
   * @param version the version to be compared
   * @returns this == version
   */
  eq(version: string | Version): boolean {
    if (typeof version === 'string') {
      version = new Version(version);
    }

    return (
      this.major === version.major &&
      this.minor === version.minor &&
      this.revision === version.revision &&
      this.build === version.build
    );
  }

  /**
   * Less Than
   * @param version the version to be compared
   * @returns this < version
   */
  lt(version: string | Version): boolean {
    if (this.eq(version)) {
      return false;
    }

    return !this.gt(version);
  }

  /**
   * Greater Than or Equal
   * @param version the version to be compared
   * @returns this >= version
   */
  gte(version: string | Version): boolean {
    return this.eq(version) || this.gt(version);
  }

  /**
   * Less Than or Equal
   * @param version the version to be compared
   * @returns this <= version
   */
  lte(version: string | Version): boolean {
    return this.eq(version) || this.lt(version);
  }
}

export default Version;

export const version2_4 = new Version('2.4');
export const version2_7 = new Version('2.7');
export const version2_9 = new Version('2.9');
export const version2_12 = new Version('2.12');
export const version2_14 = new Version('2.14');
