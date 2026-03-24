// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_TRANSITION_DATA_H_
#define CORE_STYLE_TRANSITION_DATA_H_

#include "base/include/vector.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/style/timing_function_data.h"

namespace lynx {
namespace starlight {

struct SingleTransition {
  AnimationPropertyType property;
  long duration;
  long delay;
  TimingFunctionData timing_func;
};

struct TransitionData {
  TransitionData() = default;
  ~TransitionData() = default;

  base::InlineVector<AnimationPropertyType, 1> properties;
  base::InlineVector<long, 1> durations;
  base::InlineVector<long, 1> delays;
  base::InlineVector<TimingFunctionData, 1> timing_funcs;

  class ConstTransitionProxy {
   protected:
    const TransitionData* data_;
    size_t index_;

   public:
    ConstTransitionProxy(const TransitionData* data, size_t index)
        : data_(data), index_(index) {}

    AnimationPropertyType property() const { return data_->properties[index_]; }

    long duration() const {
      if (data_->durations.empty()) return 0;
      return data_->durations[index_ % data_->durations.size()];
    }

    long delay() const {
      if (data_->delays.empty()) return 0;
      return data_->delays[index_ % data_->delays.size()];
    }

    TimingFunctionData timing_func() const {
      if (data_->timing_funcs.empty()) return TimingFunctionData{};
      return data_->timing_funcs[index_ % data_->timing_funcs.size()];
    }
  };

  class TransitionProxy : public ConstTransitionProxy {
   public:
    TransitionProxy(TransitionData* data, size_t index)
        : ConstTransitionProxy(data, index) {}

    operator SingleTransition() const {
      return SingleTransition{
          .property = property(),
          .duration = duration(),
          .delay = delay(),
          .timing_func = timing_func(),
      };
    }
  };

  size_t size() const { return properties.size(); }
  bool empty() const { return properties.empty(); }

  TransitionProxy operator[](size_t index) {
    return TransitionProxy(this, index);
  }

  ConstTransitionProxy operator[](size_t index) const {
    return ConstTransitionProxy(this, index);
  }

  class Iterator {
    TransitionData* data_;
    size_t index_;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = TransitionProxy;
    using difference_type = std::ptrdiff_t;
    using pointer = TransitionProxy*;
    using reference = TransitionProxy;

    Iterator(TransitionData* data, size_t index) : data_(data), index_(index) {}
    TransitionProxy operator*() { return TransitionProxy(data_, index_); }
    Iterator& operator++() {
      ++index_;
      return *this;
    }
    bool operator!=(const Iterator& other) { return index_ != other.index_; }
  };

  class ConstIterator {
    const TransitionData* data_;
    size_t index_;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = ConstTransitionProxy;
    using difference_type = std::ptrdiff_t;
    using pointer = ConstTransitionProxy*;
    using reference = ConstTransitionProxy;

    ConstIterator(const TransitionData* data, size_t index)
        : data_(data), index_(index) {}
    ConstTransitionProxy operator*() const {
      return ConstTransitionProxy(data_, index_);
    }
    ConstIterator& operator++() {
      ++index_;
      return *this;
    }
    bool operator!=(const ConstIterator& other) const {
      return index_ != other.index_;
    }
  };

  Iterator begin() { return Iterator(this, 0); }
  Iterator end() { return Iterator(this, size()); }
  ConstIterator begin() const { return ConstIterator(this, 0); }
  ConstIterator end() const { return ConstIterator(this, size()); }

  void clear() {
    properties.clear();
    durations.clear();
    delays.clear();
    timing_funcs.clear();
  }

  size_t GetTransitionCount() const { return properties.size(); }

  bool operator==(const TransitionData& rhs) const {
    return properties == rhs.properties && durations == rhs.durations &&
           delays == rhs.delays && timing_funcs == rhs.timing_funcs;
  }
  bool operator!=(const TransitionData& rhs) const { return !(*this == rhs); }
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_TRANSITION_DATA_H_
