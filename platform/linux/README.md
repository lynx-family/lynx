# Build the Lynx SDK for Linux

## System requirements

- A supported x64 Linux host
- Git and Python 3.9 or later

## Install dependencies

Run the following commands from the repository root:

```sh
source tools/envsetup.sh
tools/hab sync . --target clay
```

## Build the SDK

Generate a release build and package the Linux SDK:

```sh
python3 platform/linux/build_release.py --target-cpu x64
```

The command writes these files to `out/Default`:

- `lynx_sdk_linux_x64.zip`
- `lynx_sdk_linux_x64.zip.sha256`

The SDK archive contains `lib/liblynx.so`, public C API headers, the Lynx core
JavaScript files, and ICU data.
