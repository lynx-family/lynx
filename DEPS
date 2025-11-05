import os
import platform

system = platform.system().lower()
machine = platform.machine().lower()
machine = "x86_64" if machine == "amd64" else machine

python_path = "python3"
if system == "windows":
    python_path = "python"

deps = {
    'platform/android/gradle/wrapper/gradle-6.7.1-all.zip': {
        "type": "http",
        "url": "https://services.gradle.org/distributions/gradle-6.7.1-all.zip",
        "decompress": False,
    },
    'explorer/android/gradle/wrapper/gradle-6.7.1-all.zip': {
        "type": "http",
        "url": "https://services.gradle.org/distributions/gradle-6.7.1-all.zip",
        "decompress": False,
    },
    'buildtools/ninja': {
        "type": "http",
        "url": {
            "linux": "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip",
            "darwin": "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-mac.zip",
            "windows": "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-win.zip"
        }.get(system, None),
        "sha256": {
            "linux": "b901ba96e486dce377f9a070ed4ef3f79deb45f4ffe2938f8e7ddc69cfb3df77",
            "darwin": "482ecb23c59ae3d4f158029112de172dd96bb0e97549c4b1ca32d8fad11f873e",
            "windows": "524b344a1a9a55005eaf868d991e090ab8ce07fa109f1820d40e74642e289abc"
        }.get(system, None),
        "ignore_in_git": True,
        "condition": system in ['linux', 'darwin', 'windows']
    },
    'buildtools/emsdk': {
        "type": "git",
        "url": "https://github.com/emscripten-core/emsdk.git",
        "commit": "2346baa7bb44a4a0571cc75f1986ab9aaa35aa03",
        "ignore_in_git": True,
        "condition": target == "tasm"
    },
    'config_emsdk': {
        "type": "action",
        "cwd": os.path.join(root_dir, "buildtools/emsdk/"),
        "commands": [
            "./emsdk install latest",
            "./emsdk activate latest",
            ". ./emsdk_env.sh",
        ],
        "require": ['buildtools/emsdk']
    },
    'buildtools/node': {
        "type": "http",
        "url": {
            "linux-x86_64": "https://nodejs.org/dist/v18.19.1/node-v18.19.1-linux-x64.tar.gz",
            "linux-arm64": "https://nodejs.org/dist/v18.19.1/node-v18.19.1-linux-arm64.tar.gz",
            "darwin-x86_64": "https://nodejs.org/dist/v18.19.1/node-v18.19.1-darwin-x64.tar.gz",
            "darwin-arm64": "https://nodejs.org/dist/v18.19.1/node-v18.19.1-darwin-arm64.tar.gz",
            "windows-x86_64": "https://nodejs.org/dist/v18.19.1/node-v18.19.1-win-x64.zip"
        }.get(f'{system}-{machine}', None),
        "sha256": {
            "linux-x86_64": "724802c45237477dbe5777923743e6c77906830cae03a82b5653ebd75b301dda",
            "linux-arm64": "2913e8544d95c8be9e6034c539ec0584014532166a088bf742629756c3ec42e2",
            "darwin-x86_64": "ab67c52c0d215d6890197c951e1bd479b6140ab630212b96867395e21d813016",
            "darwin-arm64": "0c7249318868877032ed21cc0ed450015ee44b31b9b281955521cd3fc39fbfa3",
            "windows-x86_64": "ff08f8fe253fba9274992d7052e9d9a70141342d7b36ddbd6e84cbe823e312c6"
        }.get(f'{system}-{machine}', None),
        "ignore_in_git": True,
        "condition": system in ['linux', 'darwin', 'windows']
    },
    "third_party/xhook": {
        'type': 'git',
        'url': 'https://github.com/iqiyi/xHook.git',
        'commit': 'e59285034feadfdd4ba9b65e1eea1d381da83ed3',
        "patches": os.path.join(root_dir, 'patches', 'xhook', '*.patch'),
        "ignore_in_git": True,
    },
    'python_venv_set': {
        "type": "action",
        "commands": [
            python_path + " tools/vpython_tools/vpython_env_setup.py --root_dir " + root_dir,
        ],
    },
    'change_executable_permission': {
        "type": "action",
        "commands": [
            "chmod +x buildtools/ninja/ninja",
        ],
        "require": ["buildtools/ninja"],
        "condition": system in ['linux', 'darwin']
    },
    'buildtools/gn': {
        "type": "http",
        "url": f"https://github.com/lynx-family/buildtools/releases/download/gn-cc28efe6/buildtools-gn-{system}-{machine}.tar.gz",
        "ignore_in_git": True,
        "condition": system in ['linux', 'darwin', 'windows']
    },
    'buildtools/llvm': {
        "type": "http",
        'url': f"https://github.com/lynx-family/buildtools/releases/download/llvm-020d2fb7/buildtools-llvm-{system}-{machine}.tar.gz",
        "ignore_in_git": True,
        "decompress": True,
        "condition": system in ['linux', 'darwin'],
    },
    'update_gradle_local_properties': {
        "type": "action",
        "cwd": os.path.join(root_dir),
        "commands": [
            f"{python_path} " + os.path.join(root_dir, "tools", "android_tools", "update_local_properties.py") + ' '
            "-f "
            f'{root_dir}/platform/android/local.properties '
            f'{root_dir}/explorer/android/local.properties '
            "-p "
            f'ndk.dir={os.environ.get("ANDROID_NDK", "")} '
            f'sdk.dir={os.environ.get("ANDROID_HOME", "")} '
            f'cmake.dir={os.path.join(root_dir, "buildtools", "cmake")}'
        ],
        "condition": system in ['linux', 'darwin', 'windows']
    },
    'buildtools/cmake': {
        "type": "http",
        "url": {
            "linux": f"https://cmake.org/files/v3.18/cmake-3.18.1-Linux-x86_64.tar.gz",
            "darwin": f"https://dl.google.com/android/repository/ba34c321f92f6e6fd696c8354c262c122f56abf8.cmake-3.18.1-darwin.zip",
            "windows": f"https://cmake.org/files/v3.18/cmake-3.18.1-win64-x64.zip"
        }.get(system, None),
        "sha256": {
            "linux": "537de8ad3a7fb4ec9b8517870db255802ad211aec00002c651e178848f7a769e",
            "darwin": "b15d6d7ab5615a48bb14962f5a931be6cd9a0c187f4bd6be404bdd46a7bef60b",
            "windows": "2c6c06da43c1088fc3a673e4440c8ebb1531bb6511134892c0589aa0b94f11ad"
        }.get(system, None),
        "ignore_in_git": True,
        "condition": system in ['linux', 'darwin', 'windows']
    },
    'buildtools/android_sdk_manager': {
        "type": "http",
        "url": {
            "darwin": "https://dl.google.com/android/repository/commandlinetools-mac-8512546_latest.zip",
            "linux": "https://dl.google.com/android/repository/commandlinetools-linux-8512546_latest.zip",
            "windows": "https://dl.google.com/android/repository/commandlinetools-win-8512546_latest.zip"
        }.get(system, None),
        "ignore_in_git": True,
        "condition": system in ['linux', 'darwin', 'windows']
    },
    'third_party/gyp': {
        "type": "git",
        "url": "https://chromium.googlesource.com/external/gyp",
        "commit": "9d09418933ea2f75cc416e5ce38d15f62acd5c9a",
        "ignore_in_git": True,
        "condition": system in ['linux', 'darwin', 'windows'],
    },
    'build': {
        "type": "git",
        "url": "https://github.com/lynx-family/buildroot.git",
        "commit": "9fa98cbce6384bac89a3c0157b9ac119608badf1",
        "ignore_in_git": True,
        "condition": system in ['linux', 'darwin', 'windows']
    },
    "build/linux/debian_sid_amd64-sysroot": {
        "type": "http",
        "url": "https://commondatastorage.googleapis.com/chrome-linux-sysroot/toolchain/79a7783607a69b6f439add567eb6fcb48877085c/debian_sid_amd64_sysroot.tar.xz",
        "ignore_in_git": True,
        "condition": machine == "x86_64" and system == "linux",
        "require": ["build"]
    },
    'third_party/libcxx': {
        "type": "git",
        "url": "https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxx",
        "commit": "64d36e572d3f9719c5d75011a718f33f11126851",
        "patches": os.path.join(root_dir, 'patches', 'libcxx', '*.patch'),
        "ignore_in_git": True,
    },
    'third_party/libcxxabi': {
        "type": "git",
        "url": "https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxxabi",
        "commit": "9572e56a12c88c011d504a707ca94952be4664f9",
        "ignore_in_git": True,
    },
    'third_party/googletest': {
        'type': 'git',
        'url': 'https://github.com/google/googletest.git',
        'commit': '04cf2989168a3f9218d463bea6f15f8ade2032fd',
        'ignore_in_git': True,
    },
    'third_party/checkstyle/checkstyle.jar':{
        "type": "http",
        "url": "https://github.com/checkstyle/checkstyle/releases/download/checkstyle-9.2.1/checkstyle-9.2.1-all.jar",
        "ignore_in_git": True,
        "decompress": False,
        "condition": system in ['linux', 'darwin'],
    },
    'third_party/zlib': {
        'type': 'git',
        'url': 'https://chromium.googlesource.com/chromium/src/third_party/zlib',
        'commit': 'f5fd0ad2663e239a31184ad4c9919991dda16f46',
        "patches": os.path.join(root_dir, 'patches', 'zlib', '*.patch'),
        "ignore_in_git": True,
    },
    'third_party/NativeScript/include': {
        "type": "http",
        "url": "https://github.com/lynx-family/v8-build/releases/download/11.1.277.1/v8_include_headers.zip",
        "ignore_in_git": True,
        "condition": system in ['darwin']
    },
    'third_party/NativeScript/lib/macOS': {
        "type": "http",
        "url": "https://github.com/lynx-family/v8-build/releases/download/11.1.277.1/v8_lib_macOS.zip",
        "ignore_in_git": True,
        "condition": system in ['darwin']
    },
    'third_party/v8/11.1.277/include': {
        "type": "http",
        "url": "https://github.com/lynx-family/v8-build/releases/download/11.1.277.1/v8_include_headers.zip",
        "ignore_in_git": True,
    },
    'third_party/jacoco':{
        "type":"http",
        "url":f"https://repo1.maven.org/maven2/org/jacoco/jacoco/0.8.12/jacoco-0.8.12.zip",
        "sha256": "35cf2a2b8937659ecbc8d0d3902dfa7f55b7ed2250e82424036a3e1d12462cd7",
        "ignore_in_git":True,
        "decompress":True,
        "condition": system in ['linux', 'darwin'],
    },
    'third_party/perfetto': {
        'type': 'git',
        'url': 'https://github.com/lynx-family/lynx-trace.git',
        'commit': 'a5e90f9c8d2433622be6f3382c17cc6cbf254f81',
        "patches": [
              os.path.join(root_dir, 'patches', 'perfetto', '*.patch')
         ],
         "ignore_in_git": True,
         "paths": [ "sdk" ],
      },
    "third_party/benchmark": {
        'type': 'git',
        'url': 'https://github.com/google/benchmark.git',
        'commit': 'f91b6b42b1b9854772a90ae9501464a161707d1e',
        "ignore_in_git": True,
    },
    "third_party/quickjs/src": {
        "type": "git",
        "url": "https://github.com/lynx-family/primjs.git",
        "commit": "1174a5aa8b187babc18489a444493a5163053d03",
        "ignore_in_git": True,
    },
    "third_party/debug_router/src": {
        'type': 'git',
        'url': 'https://github.com/lynx-family/debug-router.git',
        'commit': '031a44bc1b1fffc6a001a1245dda41b977316062',
        "ignore_in_git": True,
        "enable_lfs": False,
    },
    "third_party/boringssl/src":{
        "type": "git",
        "url": "https://github.com/google/boringssl.git",
        "commit": "71147d3763466094df692df97afef42e4ce26a4f",
        "ignore_in_git": True,
    },
    'third_party/nasm': {
        "type": "git",
        "url": "https://chromium.googlesource.com/chromium/deps/nasm",
        "commit": "7268fcfbd10140026d1f27fb562e0fc4954d2af9",
        "ignore_in_git": True,
        "condition": system in ['windows'],
        "patches": os.path.join(root_dir, 'patches', 'nasm', '*.patch'),
    },
    "third_party/httplib": {
        'type': 'git',
        'url': 'https://github.com/yhirose/cpp-httplib.git',
        'commit': 'ea850cbfa74e2dff228c49bf94542ce5331d73b5',
        'patches': os.path.join(root_dir, 'patches', 'httplib', '*.patch'),
        "ignore_in_git": True,
    },
    'third_party/modp_b64': {
        'type': 'git',
        'url': 'https://chromium.googlesource.com/chromium/src/third_party/modp_b64',
        'commit': '50685101d51ef9aabbd60c94f52d9e026d39c509',
        "patches": os.path.join(root_dir, 'patches', 'modp_b64', '*.patch'),
        "ignore_in_git": True,
    },
    'buildtools/corepack/pnpm/7.33.6': {
        "type": "http",
        "url": "https://registry.npmjs.org/pnpm/-/pnpm-7.33.6.tgz",
        "sha256": "f0c52b41f8128da92160f6826b53a105aad31c6c7cdc00b907fde507c5ca09b5",
        "ignore_in_git": True
    },
    # setup corepack and pnpm
    'setup_corepack_pnpm': {
        "type": "action",
        "cwd": root_dir,
        "env": {
            'NODE_CHANNEL_FD': '1' if not system == 'windows' else '',
            'COREPACK_HOME': os.path.join(root_dir, 'buildtools', 'corepack'),
            # Windows does not have `node/bin` dir
            # And it also use `;` as seperator of PATH
            'PATH': f"{os.path.join(root_dir, 'buildtools', 'node')};{os.environ.get('PATH')}"
               if system == "windows"
               else f"{os.path.join(root_dir, 'buildtools', 'node', 'bin')}:{os.environ.get('PATH')}"
        },
        "commands": [
            "corepack prepare pnpm@7.33.6 --activate",
            "corepack enable",
            "pnpm install --frozen-lockfile"
        ],
        "require": [
            "buildtools/node",
            "buildtools/corepack/pnpm/7.33.6",
        ],
        "condition": system in ['linux', 'darwin', 'windows']
    },
    ### AUTO GENERATED SCRIPT START
    'gen_feature_count': {
        "type": "action",
        "cwd": root_dir,
        "commands": [
            python_path + " tools/feature_count/generate_feature_count.py",
        ]
    },
    'gen_error_code': {
        "type": "action",
        "cwd": root_dir,
        "commands": [
            python_path + " tools/error_code/gen_error_code.py",
        ]
    },
    'gen_lynx_perfromance_entry': {
        "type": "action",
        "cwd": root_dir,
        "commands": [
            python_path + " tools/performance/performance_observer/generate_performance_entry.py",
        ]
    },
    ### AUTO GENERATED SCRIPT END
    'explorer/darwin/ios/lynx_explorer/xctestrunner': {
        "type": "http",
        "url": "https://github.com/google/xctestrunner/releases/download/0.2.15/ios_test_runner.par",
        "decompress": False,
        "condition": system in ['darwin'],
    },
    "./tools_shared": {
        "type": "solution",
        "url": "https://github.com/lynx-family/tools-shared.git",
        "commit": "f13686a7f3ea237475af97b106c894652b2c0ef2",
        'deps_file': 'dependencies/DEPS',
        "ignore_in_git": True,
    },
    'buildtools/doxygen': {
        "type": "http",
        "url": f"https://github.com/lynx-family/buildtools/releases/download/v1.13.2/buildtools-doxygen-{system}-{machine}.tar.gz",
        "ignore_in_git": True,
        "condition": system in ['linux', 'darwin']
    },
}
