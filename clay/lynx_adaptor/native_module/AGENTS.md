# AGENTS.md: Lynx Native Module

## Directory Purpose

This directory contains the C++ implementation for Lynx Native Modules, which are exposed to the JS runtime via the JSBridge. Its primary goal is to provide a consistent, cross-platform API for JS to access underlying native capabilities, such as UI manipulation, system events, and platform-specific features.

## Specification

Agents are designed to automatically discover and consume specifications from the `spec/` directory within this module. All technical specs, including API contracts, behavioral notes, and platform differences, are documented here.

| File | Description |
| :--- | :--- |
| `spec/lynx_ui_method.spec.md` | Defines the node location mechanism and cross-platform invocation chain for `invokeUIMethod`. |
