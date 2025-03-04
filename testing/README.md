# Testing Overview

In the Lynx repository, we will cover different, multi-level ways to ensure the availability and stability, which include unit tests and integration tests.

## Unit Tests

In the Lynx repository, unit tests consist of three parts: C++ unit tests, Android unit tests, and iOS unit tests.

We have encapsulated a general-purpose unit testing tool [RTF](../tools/rtf/README.md) to drive and manage the testing process.

For the specific methods of running and adding unit tests, please refer to: [Usage of Unit Tests](README_UT.md)

## Integration Tests (End-to-End Tests)

Integration Tests is a self-driving testing for Lynx on devices and emulators. It is driven by the [Lynx-E2E](https://pypi.org/project/lynx-e2e-appium/) framework which is a self-developed UI automation framework of Lynx.

For more detailed introductions and usage guides, please refer to the documentation [Integration Test Guide](./integration_test/README.md)