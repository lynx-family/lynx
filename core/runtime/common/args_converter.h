// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_ARGS_CONVERTER_H_
#define CORE_RUNTIME_COMMON_ARGS_CONVERTER_H_

#include <memory>

namespace lynx {
namespace piper {
template <typename Out, typename In, typename Conv>
class ArgsConverterImpl {
 public:
  ArgsConverterImpl(size_t argc, In argv, const Conv& closure) {
    Out* destination = stack_args;
    if (argc > MAX_STACK_ARGS) {
      heap_args = std::make_unique<Out[]>(argc);
      destination = heap_args.get();
    }

    for (size_t i = 0; i < argc; ++i) {
      destination[i] = closure(argv[i]);
    }
  }

  /* implicit */ operator Out*() {
    return heap_args ? heap_args.get() : stack_args;
  }

  ArgsConverterImpl(ArgsConverterImpl&) = delete;
  ArgsConverterImpl& operator=(ArgsConverterImpl&) = delete;

  ArgsConverterImpl(ArgsConverterImpl&&) = delete;
  ArgsConverterImpl& operator=(ArgsConverterImpl&&) = delete;

 private:
  constexpr static unsigned MAX_STACK_ARGS = 8;
  Out stack_args[MAX_STACK_ARGS];
  std::unique_ptr<Out[]> heap_args;
};

// `ArgsConverter` is used to convert the argument array from one type to
// another.
//
// The conversion is done by the closure passed to the constructor, which is
// called for each argument and the result is stored in the internal array.
// The array is allocated on the stack if the number of arguments is less than
// or equal to MAX_STACK_ARGS, otherwise it is allocated on the heap.
// The array is returned by the implicit conversion operator.
template <typename Out, typename In, typename Conv,
          std::enable_if_t<std::is_pointer_v<In>, bool> = true>
inline auto ArgsConverter(size_t argc, const In argv, const Conv& closure) {
  return ArgsConverterImpl<Out, In, Conv>(argc, argv, closure);
}

template <typename Out, typename In, typename Conv,
          std::enable_if_t<!std::is_pointer_v<In>, bool> = true>
inline auto ArgsConverter(size_t argc, const In& argv, const Conv& closure) {
  return ArgsConverterImpl<Out, const In&, Conv>(argc, argv, closure);
}

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_ARGS_CONVERTER_H_
