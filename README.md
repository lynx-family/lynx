<p align="center">
  <a href="https://lynxjs.org">
    <img width="500" alt="Lynx" src="https://github.com/user-attachments/assets/23e35f90-1506-4b1d-8114-6bb2b8b643e7" />
  </a>
</p>

<p align="center">
  <a href="https://github.com/lynx-family/lynx/blob/develop/LICENSE" style="text-decoration:none">
    <img src="https://img.shields.io/badge/License-Apache--2.0-cyan?logo=apache" alt="Apache Licensed">
  </a>
  <a href="https://github.com/lynx-family/lynx/releases" style="text-decoration:none">
    <img src="https://img.shields.io/github/v/release/lynx-family/lynx.svg" alt="Latest release version">
  </a>
  <a href="https://github.com/lynx-family/lynx/actions" style="text-decoration:none">
    <img src="https://img.shields.io/github/actions/workflow/status/lynx-family/lynx/ci.yml" alt="CI status">
  </a><a href="https://x.com/lynxjs_org" style="text-decoration:none">
    <img src="https://img.shields.io/twitter/url?url=https%3A%2F%2Fx.com%2Flynxjs_org&style=social&label=Lynx" alt="X (formerly Twitter) URL">
  </a>
</p>


<p align="center">
Empower the web community and invite more to build across platforms
</p>


## Content
- [About Lynx](#About-Lynx)
- [Documentation](#Documentation)
- [How to Use Lynx](#How-to-Use-Lynx)
- [How to Contribute](#How-to-Contribute)
- [Discussions](#Discussions)
- [Credits](#Credits)
- [License](#License)

## About Lynx

Lynx is a _family_ of open-source technologies empowering developers to use their existing web skills to create truly native UIs for both mobile and web from a single codebase, featuring performance at scale and velocity.
- **💫 Write Once, Render Anywhere.** Enjoy native rendering on Android, iOS, and Web, or pixel-perfect consistency across mobile and desktop via our custom renderer.
- **🌐 Web-Inspired Design.** Leverage your existing knowledge of CSS and React. We designed Lynx with the web knowledge and libraries in mind.
- **⚡ Performance at Scale.** Achieve instant launch and silky UI responsiveness via our multithreaded engine, whether standalone or embedded.

This repository contains the **core engine** of Lynx. For other repositories in the Lynx family, visit our [org homepage](https://github.com/lynx-family).


## Documentation
You shall find documentation for Lynx on [lynxjs.org](http://lynxjs.org).

## How to Use Lynx
### Requirements
Lynx apps may target iOS 10 and Android 5.0 (API 21) or newer.

We recommend using macOS as the development operating system. Windows and Linux are not yet verified or guaranteed, so you may encounter problems. If you need assistance, please file an issue, and we will be more than happy to help you address it.

### Getting Started guide
- Trying out Lynx with [hello world](https://lynxjs.org/guide/start/quick-start.html)
- [Integrating Lynx with an Existing Application](https://lynxjs.org/guide/start/integrate-with-existing-apps.html)

## How to Contribute
### Get the Code
Instead of cloning the repository right away, please consider taking an extra step.

To effectively manage dependencies using our custom tool, [Habitat](https://github.com/lynx-family/habitat), it is recommended to create an additional directory.
For instance, you could name this directory `src`:

```
git clone git@github.com:lynx-family/lynx.git src/lynx
```

For now, `src` contains only `lynx`. However as you proceed to build Lynx, you'll notice several installed dependencies there.
This additional `src` directory helps keep your workplace organized.

### [Code of Conduct][coc]
We are devoted to ensuring a positive, inclusive, and safe environment for all contributors. Please find our [Code of Conduct][coc] for detailed information.

[coc]: CODE_OF_CONDUCT.md

### [Contributing Guide][contributing]
We welcome you to join and become a member of Lynx Authors. It's people like you that make this project great.

Please refer to our [contributing guide][contributing] for details.

[contributing]: CONTRIBUTING.md

## Discussions
Large discussions and proposals are discussed in [Github Discussions](https://github.com/lynx-family/lynx/discussions)

## Credits
Lynx makes use of several third-party libraries and draws inspiration from various projects. We would like to express our sincere gratitude to these sources.
### Third Party Libraries
Lynx incorporates the following third-party libraries, which have significantly contributed to its functionality. We appreciate the efforts of the developers and the open-source community behind these projects:
- [aes](https://github.com/SergeyBel/AES)
- [benchmark](https://github.com/google/benchmark)
- [binding](https://chromium.googlesource.com/chromium/blink/+/refs/heads/main/Source/bindings)
- [checkstyle](https://github.com/checkstyle/checkstyle)
- [buildroot](https://github.com/flutter/buildroot)
- [double-conversion](https://github.com/google/double-conversion)
- [googletest](https://github.com/google/googletest)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- [modp_b64](https://github.com/Piasy/modp_b64)
- [node-addon-api](https://github.com/nodejs/node-addon-api)
- [NativeScript](https://github.com/NativeScript/NativeScript)
- [perfetto](https://android.googlesource.com/platform//external/perfetto/)
- [rapidjson](https://github.com/Tencent/rapidjson)
- [v8](https://chromium.googlesource.com/v8/v8.git)
- [xctestrunner](https://github.com/google/xctestrunner)
- [xhook](https://github.com/iqiyi/xHook.git)
- [zlib](https://chromium.googlesource.com/chromium/src/third_party/zlib)

### Referenced API Design and Implementations
The design of some APIs and some implementations in Lynx have been inspired by and referenced from the following outstanding projects. Their innovative designs and solutions have been invaluable in shaping Lynx:
- [chromium](https://chromium.googlesource.com/chromium/)
- [react-native](https://github.com/facebook/react-native)
- [flutter engine](https://github.com/flutter/engine)

We respect the intellectual property rights of all these projects and adhere to relevant open-source licenses and usage guidelines.

## [License][license]
Lynx is Apache licensed, as found in the [LICENSE][license] file.

[license]: LICENSE
