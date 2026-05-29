// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/event_dispatcher.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/event/event.h"
#include "core/event/event_listener.h"
#include "core/event/event_target.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace event {
namespace test {
namespace {

std::string PhaseName(Event::PhaseType phase) {
  switch (phase) {
    case Event::PhaseType::kCapturingPhase:
      return "capture";
    case Event::PhaseType::kAtTarget:
      return "target";
    case Event::PhaseType::kBubblingPhase:
      return "bubble";
    case Event::PhaseType::kGlobal:
      return "global";
    case Event::PhaseType::kNone:
      return "none";
  }
  return "unknown";
}

std::string TargetName(const fml::WeakPtr<EventTarget>& target) {
  return target ? target->GetUniqueID() : "null";
}

class RecordingEvent : public Event {
 public:
  RecordingEvent(const std::string& type, Capture capture, Bubbles bubbles)
      : Event(type, EventType::kCustomEvent, capture, bubbles, Cancelable::kYes,
              ComposedMode::kComposed, PhaseType::kNone) {}

  DispatchEventResult DispatchEvent(EventDispatcher& dispatcher) override {
    ++dispatch_event_count_;
    return Event::DispatchEvent(dispatcher);
  }

  bool HandleEventConflictAndParam() override {
    ++conflict_check_count_;
    return should_cancel_before_dispatch_;
  }

  void HandleEventCustomDetail() override { ++custom_detail_count_; }

  void set_should_cancel_before_dispatch(bool value) {
    should_cancel_before_dispatch_ = value;
  }

  int dispatch_event_count() const { return dispatch_event_count_; }
  int conflict_check_count() const { return conflict_check_count_; }
  int custom_detail_count() const { return custom_detail_count_; }

 private:
  bool should_cancel_before_dispatch_{false};
  int dispatch_event_count_{0};
  int conflict_check_count_{0};
  int custom_detail_count_{0};
};

class RecordingEventListener : public EventListener {
 public:
  RecordingEventListener(std::string name, std::vector<std::string>* records,
                         const Options& options = Options())
      : EventListener(Type::kClosureEventListener, options),
        name_(std::move(name)),
        records_(records) {}

  void Invoke(fml::RefPtr<Event> event) override {
    records_->push_back(name_ + "@" + TargetName(event->current_target()) +
                        "#" + PhaseName(event->event_phase()) + "->" +
                        TargetName(event->target()));
  }

  bool Matches(EventListener* listener) override { return listener == this; }

 private:
  std::string name_;
  std::vector<std::string>* records_;
};

class DispatchChainTarget
    : public EventTarget,
      public fml::EnableWeakFromThis<DispatchChainTarget> {
 public:
  explicit DispatchChainTarget(std::string id, EventTarget* parent = nullptr)
      : id_(std::move(id)), parent_(parent) {}

  EventTarget* GetParentTarget() override { return parent_; }

  fml::WeakPtr<EventTarget> GetWeakTarget() override { return WeakFromThis(); }

  std::string GetUniqueID() override { return id_; }

  void HandleGlobalEvent(fml::RefPtr<Event> event) override {
    ++handle_global_event_count_;
    event->set_event_phase(Event::PhaseType::kGlobal);
    for (const auto& item : event->event_path()) {
      if (event->is_stop_propagation() ||
          event->is_stop_immediate_propagation()) {
        break;
      }
      if (!item) {
        continue;
      }
      event->set_current_target(item);
      auto result = item->DispatchEvent(event);
      if (result.IsCanceled() || event->is_stop_propagation() ||
          event->is_stop_immediate_propagation()) {
        break;
      }
    }
  }

  int handle_global_event_count() const { return handle_global_event_count_; }

 private:
  std::string id_;
  EventTarget* parent_{nullptr};
  int handle_global_event_count_{0};
};

std::shared_ptr<RecordingEventListener> Listener(
    const std::string& name, std::vector<std::string>* records,
    const EventListener::Options& options = EventListener::Options()) {
  return std::make_shared<RecordingEventListener>(name, records, options);
}

TEST(EventDispatcherTest, DispatchEventRunsWholePropagationChain) {
  auto root = std::make_unique<DispatchChainTarget>("root");
  auto parent = std::make_unique<DispatchChainTarget>("parent", root.get());
  auto child = std::make_unique<DispatchChainTarget>("child", parent.get());
  std::vector<std::string> records;

  root->AddEventListener(
      "tap", Listener("root-global", &records,
                      EventListener::Options(false, false, false, false, false,
                                             true)));
  root->AddEventListener(
      "tap", Listener("root-capture", &records, EventListener::Options(true)));
  root->AddEventListener("tap", Listener("root-bubble", &records));
  parent->AddEventListener("tap", Listener("parent-capture", &records,
                                           EventListener::Options(true)));
  parent->AddEventListener("tap", Listener("parent-bubble", &records));
  child->AddEventListener(
      "tap", Listener("child-capture", &records, EventListener::Options(true)));
  child->AddEventListener("tap", Listener("child-bubble", &records));

  auto event = fml::MakeRefCounted<RecordingEvent>("tap", Event::Capture::kYes,
                                                   Event::Bubbles::kYes);
  auto result = EventDispatcher::DispatchEvent(*child, event);

  EXPECT_EQ(result.cancel_type, EventCancelType::kNotCanceled);
  EXPECT_TRUE(result.consumed);
  EXPECT_EQ(event->dispatch_event_count(), 1);
  EXPECT_EQ(event->conflict_check_count(), 1);
  EXPECT_EQ(event->custom_detail_count(), 1);
  EXPECT_EQ(child->handle_global_event_count(), 1);
  ASSERT_EQ(event->event_path().size(), 3u);
  EXPECT_EQ(event->event_path()[0].get(), child.get());
  EXPECT_EQ(event->event_path()[1].get(), parent.get());
  EXPECT_EQ(event->event_path()[2].get(), root.get());
  EXPECT_EQ(event->target().get(), child.get());
  EXPECT_EQ(records, (std::vector<std::string>{
                         "root-global@root#global->child",
                         "root-capture@root#capture->child",
                         "parent-capture@parent#capture->child",
                         "child-capture@child#target->child",
                         "child-bubble@child#target->child",
                         "parent-bubble@parent#bubble->child",
                         "root-bubble@root#bubble->child",
                     }));
}

TEST(EventDispatcherTest, DispatchEventStopsGlobalDispatchAfterCanceling) {
  auto root = std::make_unique<DispatchChainTarget>("root");
  auto parent = std::make_unique<DispatchChainTarget>("parent", root.get());
  auto child = std::make_unique<DispatchChainTarget>("child", parent.get());
  std::vector<std::string> records;

  child->AddEventListener(
      "tap",
      Listener("child-global-catch", &records,
               EventListener::Options(false, false, false, false, true, true)));
  parent->AddEventListener(
      "tap", Listener("parent-global", &records,
                      EventListener::Options(false, false, false, false, false,
                                             true)));
  root->AddEventListener(
      "tap", Listener("root-global", &records,
                      EventListener::Options(false, false, false, false, false,
                                             true)));

  auto event = fml::MakeRefCounted<RecordingEvent>("tap", Event::Capture::kYes,
                                                   Event::Bubbles::kYes);
  EventDispatcher::DispatchEvent(*child, event);

  EXPECT_EQ(child->handle_global_event_count(), 1);
  EXPECT_EQ(records, (std::vector<std::string>{
                         "child-global-catch@child#global->child",
                     }));
}

TEST(EventDispatcherTest, DispatchEventStopsWhenCaptureListenerCancels) {
  auto root = std::make_unique<DispatchChainTarget>("root");
  auto parent = std::make_unique<DispatchChainTarget>("parent", root.get());
  auto child = std::make_unique<DispatchChainTarget>("child", parent.get());
  std::vector<std::string> records;

  root->AddEventListener(
      "tap", Listener("root-capture-catch", &records,
                      EventListener::Options(true, false, false, false, true)));
  parent->AddEventListener("tap", Listener("parent-capture", &records,
                                           EventListener::Options(true)));
  child->AddEventListener("tap", Listener("child-bubble", &records));
  root->AddEventListener("tap", Listener("root-bubble", &records));

  auto event = fml::MakeRefCounted<RecordingEvent>("tap", Event::Capture::kYes,
                                                   Event::Bubbles::kYes);
  auto result = EventDispatcher::DispatchEvent(*child, event);

  EXPECT_EQ(result.cancel_type, EventCancelType::kCanceledByEventHandler);
  EXPECT_TRUE(result.consumed);
  EXPECT_EQ(event->dispatch_event_count(), 1);
  EXPECT_EQ(event->conflict_check_count(), 1);
  EXPECT_EQ(event->custom_detail_count(), 1);
  EXPECT_EQ(records, (std::vector<std::string>{
                         "root-capture-catch@root#capture->child",
                     }));
}

TEST(EventDispatcherTest, DispatchEventReturnsCanceledWhenEventConflict) {
  auto target = std::make_unique<DispatchChainTarget>("target");
  std::vector<std::string> records;
  target->AddEventListener("tap", Listener("target-bubble", &records));

  auto event = fml::MakeRefCounted<RecordingEvent>("tap", Event::Capture::kYes,
                                                   Event::Bubbles::kYes);
  event->set_should_cancel_before_dispatch(true);
  auto result = EventDispatcher::DispatchEvent(*target, event);

  EXPECT_EQ(result.cancel_type, EventCancelType::kCanceledByEventHandler);
  EXPECT_FALSE(result.consumed);
  EXPECT_EQ(event->dispatch_event_count(), 1);
  EXPECT_EQ(event->conflict_check_count(), 1);
  EXPECT_EQ(event->custom_detail_count(), 0);
  EXPECT_TRUE(records.empty());
}

}  // namespace
}  // namespace test
}  // namespace event
}  // namespace lynx
