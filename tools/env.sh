#!/bin/bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# Keep the no-side-effect environment setup used by both standalone Lynx and
# downstream repositories in the open-source tree.
LYNX_OSS_ENV_ANDROID_NDK_VERSION="21.1.6352462"
LYNX_OSS_ENV_HARMONY_SDK_VERSION="6.0.0.868"
LYNX_OSS_ENV_COMMON_PATH_SUFFIXES=(
  "tools_shared"
  "tools/gn_tools"
)
LYNX_OSS_ENV_BUILDTOOLS_PATH_SUFFIXES=(
  "llvm/bin"
  "gn"
  "ninja"
  "sccache"
  "emsdk"
  "emsdk/upstream/emscripten"
  "node/bin"
  "ktlint"
)
LYNX_OSS_ENV_PYTHONPATH_SUFFIXES=(
  "third_party/py_deps"
)
LYNX_OSS_ENV_TOOL_CANDIDATES=(
  gn
  ninja
  sccache
  node
  npm
  npx
  pnpm
  corepack
  python3
  pip3
  clang
  clang++
  emcc
  em++
  ktlint
  java
  adb
  git
)

lynx_oss_env_is_sourced() {
  if [[ -n "${ZSH_EVAL_CONTEXT:-}" ]]; then
    case ":${ZSH_EVAL_CONTEXT}:" in
      *:file:*) return 0 ;;
    esac
  fi

  if [[ -n "${BASH_SOURCE[0]:-}" ]]; then
    [[ "${BASH_SOURCE[0]}" != "$0" ]]
    return
  fi

  return 1
}

lynx_oss_env_usage() {
  cat <<'EOF'
Usage:
  tools/env.sh --list-tools
  tools/env.sh -- <command> [args...]
  tools/env.sh <command> [args...]

Description:
  Sets up the Lynx no-side-effect development environment.
  When executed directly, it runs the given command with the environment applied.
  When sourced, it only exports the environment variables.

Examples:
  tools/env.sh --list-tools
  tools/env.sh node -v
  tools/env.sh pnpm --version
  tools/env.sh bash -lc 'echo "$LYNX_DIR"'
EOF
}

lynx_oss_env_realpath() {
  if [[ $# -ne 1 ]]; then
    echo "env.sh: expected exactly one path argument" >&2
    return 1
  fi

  local target="$1"
  local target_dir
  target_dir="$(dirname -- "$target")" || return 1
  local target_base
  target_base="$(basename -- "$target")" || return 1

  (
    cd -- "$target_dir" >/dev/null 2>&1 || exit 1
    printf '%s/%s\n' "$(pwd -P)" "$target_base"
  )
}

lynx_oss_env_script_path() {
  if [[ -n "${BASH_SOURCE[0]:-}" ]]; then
    printf '%s\n' "${BASH_SOURCE[0]}"
    return 0
  fi

  if [[ -n "${ZSH_VERSION:-}" ]]; then
    eval 'printf "%s\n" "${(%):-%x}"'
    return
  fi

  printf '%s\n' "$0"
}

lynx_oss_env_prepend_path() {
  local dir="$1"
  if [[ -z "$dir" || ! -d "$dir" ]]; then
    return 0
  fi

  case ":${PATH:-}:" in
    *":$dir:"*) ;;
    *)
      if [[ -n "${PATH:-}" ]]; then
        PATH="$dir:$PATH"
      else
        PATH="$dir"
      fi
      ;;
  esac
}

lynx_oss_env_prepend_lynx_paths() {
  local path_suffix

  for path_suffix in "${LYNX_OSS_ENV_BUILDTOOLS_PATH_SUFFIXES[@]}"; do
    lynx_oss_env_prepend_path "${BUILDTOOLS_DIR}/${path_suffix}"
  done

  for path_suffix in "${LYNX_OSS_ENV_COMMON_PATH_SUFFIXES[@]}"; do
    lynx_oss_env_prepend_path "${LYNX_DIR}/${path_suffix}"
  done
}

lynx_oss_env_prepend_pythonpath() {
  local dir="$1"
  if [[ -z "$dir" || ! -d "$dir" ]]; then
    return 0
  fi

  case ":${PYTHONPATH:-}:" in
    *":$dir:"*) ;;
    *)
      if [[ -n "${PYTHONPATH:-}" ]]; then
        PYTHONPATH="$dir:$PYTHONPATH"
      else
        PYTHONPATH="$dir"
      fi
      ;;
  esac
}

lynx_oss_env_prepend_lynx_python_paths() {
  local path_suffix

  for path_suffix in "${LYNX_OSS_ENV_PYTHONPATH_SUFFIXES[@]}"; do
    lynx_oss_env_prepend_pythonpath "${LYNX_DIR}/${path_suffix}"
  done
}

lynx_oss_env_setup_common() {
  local script_real_path
  script_real_path="$(lynx_oss_env_realpath "$(lynx_oss_env_script_path)")" || return 1

  local tools_dir
  tools_dir="$(dirname -- "$script_real_path")" || return 1

  export LYNX_DIR
  LYNX_DIR="$(dirname -- "$tools_dir")"
  export LYNX_ROOT_DIR="${LYNX_ROOT_DIR:-$LYNX_DIR}"
  export BUILDTOOLS_DIR="${BUILDTOOLS_DIR:-${LYNX_DIR}/buildtools}"
  export COREPACK_HOME="${COREPACK_HOME:-${BUILDTOOLS_DIR}/corepack}"
  # create pnpm shim in $COREPACK_HOME/pnpm (configured in dependencies/DEPS, "prepare_pnpm_shim")
  lynx_oss_env_prepend_path "$COREPACK_HOME/pnpm"

  lynx_oss_env_prepend_lynx_paths
  export PATH
  lynx_oss_env_prepend_lynx_python_paths
  export PYTHONPATH

  if [[ "${OSTYPE:-}" == darwin* && -z "${JAVA_HOME:-}" && -x /usr/libexec/java_home ]]; then
    local detected_java_home
    detected_java_home="$(
      /usr/libexec/java_home 2>/dev/null
    )"
    if [[ -n "$detected_java_home" ]]; then
      export JAVA_HOME="$detected_java_home"
      lynx_oss_env_prepend_path "${JAVA_HOME}/bin"
      export CLASS_PATH="${JAVA_HOME}/lib"
      export PATH
    fi
  fi
}

lynx_oss_env_setup_android() {
  if [[ -z "${ANDROID_HOME:-}" ]]; then
    return 0
  fi

  export ANDROID_SDK="${ANDROID_SDK:-$ANDROID_HOME}"

  if [[ -n "${ANDROID_NDK_PATH:-}" ]]; then
    export ANDROID_NDK="${ANDROID_NDK_PATH}"
  elif [[ -z "${ANDROID_NDK:-}" ]]; then
    export ANDROID_NDK="${ANDROID_HOME}/ndk/${LYNX_OSS_ENV_ANDROID_NDK_VERSION}"
  fi

  export ANDROID_NDK_21="${ANDROID_HOME}/ndk/${LYNX_OSS_ENV_ANDROID_NDK_VERSION}"
}

lynx_oss_env_setup_harmony() {
  local harmony_home="${HARMONY_HOME:-}"

  if [[ -z "$harmony_home" && -n "${COMMANDLINE_TOOL_BASE_DIR:-}" ]]; then
    local commandline_tool_dir="${COMMANDLINE_TOOL_BASE_DIR}/${LYNX_OSS_ENV_HARMONY_SDK_VERSION}/command-line-tools"
    if [[ -d "${commandline_tool_dir}/sdk" ]]; then
      export COMMANDLINE_TOOL_DIR="${commandline_tool_dir}"
      harmony_home="${commandline_tool_dir}/sdk"
      export HARMONY_HOME="${harmony_home}"
    fi
  fi

  if [[ -z "$harmony_home" ]]; then
    return 0
  fi

  lynx_oss_env_prepend_path "$(dirname -- "$harmony_home")/bin"

  export HOS_SDK_HOME="${HOS_SDK_HOME:-$harmony_home}"
  export DEVECO_SDK_HOME="${DEVECO_SDK_HOME:-$harmony_home}"
  export DEVECO_NODE_HOME="${DEVECO_NODE_HOME:-${BUILDTOOLS_DIR}/node}"
  export PATH
}

lynx_oss_env_setup() {
  lynx_oss_env_setup_common || return 1
  lynx_oss_env_setup_android || return 1
  lynx_oss_env_setup_harmony || return 1
}

lynx_oss_env_list_tools() {
  local tool_name
  local tool_path
  local tool_origin
  local found_any=0

  printf 'LYNX_DIR=%s\n' "$LYNX_DIR"
  printf 'LYNX_ROOT_DIR=%s\n' "$LYNX_ROOT_DIR"
  printf 'BUILDTOOLS_DIR=%s\n' "$BUILDTOOLS_DIR"
  printf 'Available tools:\n'

  for tool_name in "${LYNX_OSS_ENV_TOOL_CANDIDATES[@]}"; do
    tool_path="$(command -v "$tool_name" 2>/dev/null || true)"
    if [[ -z "$tool_path" ]]; then
      continue
    fi

    tool_origin="system"
    if [[ "$tool_path" == "$LYNX_DIR/"* || "$tool_path" == "$LYNX_ROOT_DIR/"* ]]; then
      tool_origin="repo"
    fi

    printf '  %-10s [%s] %s\n' "$tool_name" "$tool_origin" "$tool_path"
    found_any=1
  done

  if [[ "$found_any" -eq 0 ]]; then
    printf '  (no known tools were resolved)\n'
  fi
}

lynx_oss_env_main() {
  local list_tools=0

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --help|-h)
        lynx_oss_env_usage
        return 0
        ;;
      --list-tools)
        list_tools=1
        shift
        ;;
      --)
        shift
        break
        ;;
      *)
        break
        ;;
    esac
  done

  lynx_oss_env_setup || {
    echo "env.sh: failed to prepare environment" >&2
    return 1
  }

  if [[ "$list_tools" -eq 1 ]]; then
    lynx_oss_env_list_tools
    return 0
  fi

  if lynx_oss_env_is_sourced; then
    if [[ $# -gt 0 ]]; then
      echo "env.sh: source this script without command arguments" >&2
      return 1
    fi
    return 0
  fi

  if [[ $# -eq 0 ]]; then
    lynx_oss_env_usage >&2
    return 1
  fi

  exec "$@"
}

lynx_oss_env_main "$@"
