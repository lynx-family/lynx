// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#ifndef PLATFORM_LINUX_GTK_GTK_TEXT_INPUT_H_
#define PLATFORM_LINUX_GTK_GTK_TEXT_INPUT_H_
#include <gtk/gtk.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace lynx::gtk {
// Offsets in GTK surrounding text are UTF-8 bytes; GTK preedit/delete offsets
// are Unicode characters; Clay selections are UTF-16 code units.
class TextOffsets {
 public:
  explicit TextOffsets(const std::string& text) {
    valid = g_utf8_validate(text.c_str(), text.size(), nullptr);
    bytes.push_back(0);
    utf16.push_back(0);
    if (!valid) return;
    const char* begin = text.c_str();
    const char* p = begin;
    while (*p) {
      auto c = g_utf8_get_char(p);
      p = g_utf8_next_char(p);
      bytes.push_back(p - begin);
      utf16.push_back(utf16.back() + (c > 0xffff ? 2 : 1));
    }
  }
  int Clamp16(uint64_t offset) const {
    int value = offset > uint64_t(utf16.back()) ? utf16.back() : int(offset);
    return utf16[std::upper_bound(utf16.begin(), utf16.end(), value) -
                 utf16.begin() - 1];
  }
  int Byte16(uint64_t offset) const {
    auto value = Clamp16(offset);
    return bytes[std::lower_bound(utf16.begin(), utf16.end(), value) -
                 utf16.begin()];
  }
  int Character16(int character) const {
    return utf16[std::clamp(character, 0, int(utf16.size() - 1))];
  }
  int Character(uint64_t offset) const {
    return std::lower_bound(utf16.begin(), utf16.end(), Clamp16(offset)) -
           utf16.begin();
  }
  bool valid;
  std::vector<int> bytes, utf16;
};

class TextInput {
 public:
  using Update =
      std::function<bool(int, const std::string&, int, int, int, int)>;
  TextInput(GtkWidget* widget, Update update,
            std::function<void(GtkIMContext*)> set_context = {})
      : widget_(widget),
        update_(std::move(update)),
        set_context_(std::move(set_context)) {
    CreateContext();
    focus_ = gtk_event_controller_focus_new();
    g_signal_connect(focus_, "enter",
                     G_CALLBACK(+[](GtkEventControllerFocus*, gpointer p) {
                       auto* self = static_cast<TextInput*>(p);
                       if (self->client_ >= 0 && self->shown_)
                         gtk_im_context_focus_in(self->context_);
                     }),
                     this);
    g_signal_connect(focus_, "leave",
                     G_CALLBACK(+[](GtkEventControllerFocus*, gpointer p) {
                       auto* self = static_cast<TextInput*>(p);
                       self->ResetContext();
                     }),
                     this);
    gtk_widget_add_controller(widget_, focus_);
  }
  ~TextInput() {
    client_ = -1;
    gtk_im_context_reset(context_);
    g_signal_handlers_disconnect_by_data(context_, this);
    gtk_im_context_set_client_widget(context_, nullptr);
    g_object_unref(context_);
    g_signal_handlers_disconnect_by_data(focus_, this);
    gtk_widget_remove_controller(widget_, focus_);
  }
  void SetClient(int id, const char*, const char* type) {
    ClearClient();
    // A new context per client keeps delayed IME replies attached to the old
    // client object, whose callbacks have been disconnected.
    g_signal_handlers_disconnect_by_data(context_, this);
    gtk_im_context_set_client_widget(context_, nullptr);
    g_object_unref(context_);
    CreateContext();
    client_ = id;
    std::string input = type ? type : "";
    password_ = input == "TextInputType.password";
    auto purpose = password_ ? GTK_INPUT_PURPOSE_PASSWORD
                   : input == "TextInputType.number" ? GTK_INPUT_PURPOSE_NUMBER
                   : input == "TextInputType.phone"  ? GTK_INPUT_PURPOSE_PHONE
                   : input == "TextInputType.emailAddress"
                       ? GTK_INPUT_PURPOSE_EMAIL
                       : GTK_INPUT_PURPOSE_FREE_FORM;
    g_object_set(context_, "input-purpose", purpose, "input-hints",
                 password_ ? GTK_INPUT_HINT_PRIVATE : GTK_INPUT_HINT_NONE,
                 nullptr);
  }
  void ClearClient() {
    client_ = -1;
    if (set_context_) set_context_(nullptr);
    gtk_im_context_focus_out(context_);
    gtk_im_context_reset(context_);
    text_.clear();
    has_preedit_backup_ = false;
    preedit_backup_.clear();
    base_ = extent_ = 0;
    composing_base_ = composing_extent_ = -1;
    shown_ = false;
    gtk_im_context_set_surrounding_with_selection(context_, "", 0, 0, 0);
  }
  void SetState(uint64_t base, uint64_t composing_extent, const std::string&,
                const std::string& text, bool, uint64_t extent,
                uint64_t composing_base) {
    if (client_ < 0) return;
    TextOffsets offsets(text);
    if (!offsets.valid) return;
    if (text != text_) has_preedit_backup_ = false;
    text_ = text;
    base_ = offsets.Clamp16(base);
    extent_ = offsets.Clamp16(extent);
    if (composing_base == UINT64_MAX || composing_extent == UINT64_MAX ||
        composing_base == composing_extent)
      composing_base_ = composing_extent_ = -1;
    else {
      composing_base_ = offsets.Clamp16(composing_base);
      composing_extent_ = offsets.Clamp16(composing_extent);
    }
  }
  void Show(bool show) {
    shown_ = show;
    if (!show) {
      ResetContext(true);
      return;
    }
    if (set_context_) set_context_(client_ >= 0 ? context_ : nullptr);
    if (show && client_ >= 0 && gtk_widget_has_focus(widget_))
      gtk_im_context_focus_in(context_);
    else {
      ResetContext();
    }
  }
  void Caret(float x, float y, float width, float height) {
    if (g_getenv("LYNX_GTK_INPUT_TRACE"))
      g_print("GTK4_CARET %.2f %.2f %.2f %.2f\n", x, y, width, height);
    GdkRectangle rect{int(std::floor(x)), int(std::floor(y)),
                      std::max(1, int(std::ceil(width))),
                      std::max(1, int(std::ceil(height)))};
    gtk_im_context_set_cursor_location(context_, &rect);
  }
  GtkIMContext* context() const { return context_; }
  bool IsPassword() const { return password_; }
  void Commit(const char* text) {
    if (client_ < 0 || !text || resetting_) return;
    TextOffsets offsets(text);
    if (!offsets.valid) return;
    Replace(text);
    has_preedit_backup_ = false;
    composing_base_ = composing_extent_ = -1;
    Publish();
  }
  bool Surrounding() {
    if (client_ < 0 || password_) {
      gtk_im_context_set_surrounding_with_selection(context_, "", 0, 0, 0);
      return false;
    }
    auto visible = SurroundingText();
    TextOffsets offsets(visible);
    gtk_im_context_set_surrounding_with_selection(
        context_, visible.c_str(), visible.size(),
        offsets.Byte16(WithoutComposition(extent_)),
        offsets.Byte16(WithoutComposition(base_)));
    return true;
  }
  bool DeleteSurrounding(int offset, int count) {
    if (client_ < 0 || password_ || count < 0) return false;
    auto visible = SurroundingText();
    TextOffsets offsets(visible);
    int64_t caret = offsets.Character(WithoutComposition(extent_));
    int first =
        std::clamp<int64_t>(caret + offset, 0, offsets.bytes.size() - 1);
    int last = std::clamp<int64_t>(caret + int64_t(offset) + count, first,
                                   offsets.bytes.size() - 1);
    int start = offsets.utf16[first], end = offsets.utf16[last];
    auto adjust = [=](int p) {
      return p <= start ? p : p >= end ? p - (end - start) : start;
    };
    int new_base = adjust(WithoutComposition(base_)),
        new_extent = adjust(WithoutComposition(extent_));
    visible.erase(offsets.bytes[first],
                  offsets.bytes[last] - offsets.bytes[first]);
    if (composing_base_ >= 0) {
      TextOffsets original(text_);
      int cb = std::min(composing_base_, composing_extent_),
          ce = std::max(composing_base_, composing_extent_);
      auto composition = text_.substr(
          original.Byte16(cb), original.Byte16(ce) - original.Byte16(cb));
      int insertion = adjust(cb);
      TextOffsets after(visible);
      visible.insert(after.Byte16(insertion), composition);
      auto restore = [&](int old, int position) {
        return old >= cb && old <= ce
                   ? insertion + (old - cb)
                   : position + (position >= insertion ? ce - cb : 0);
      };
      new_base = restore(base_, new_base);
      new_extent = restore(extent_, new_extent);
      composing_base_ = insertion;
      composing_extent_ = insertion + (ce - cb);
      has_preedit_backup_ = false;
    }
    text_ = std::move(visible);
    base_ = new_base;
    extent_ = new_extent;
    return Publish();
  }

 private:
  void CreateContext() {
    context_ = gtk_im_multicontext_new();
    gtk_im_context_set_client_widget(context_, widget_);
    g_signal_connect(
        context_, "commit",
        G_CALLBACK(+[](GtkIMContext*, const char* text, gpointer p) {
          static_cast<TextInput*>(p)->Commit(text);
        }),
        this);
    g_signal_connect(context_, "preedit-changed",
                     G_CALLBACK(+[](GtkIMContext*, gpointer p) {
                       static_cast<TextInput*>(p)->Preedit();
                     }),
                     this);
    g_signal_connect(context_, "preedit-end",
                     G_CALLBACK(+[](GtkIMContext*, gpointer p) {
                       auto* self = static_cast<TextInput*>(p);
                       if (!self->resetting_) self->CancelPreedit();
                     }),
                     this);
    g_signal_connect(context_, "retrieve-surrounding",
                     G_CALLBACK(+[](GtkIMContext*, gpointer p) -> gboolean {
                       return static_cast<TextInput*>(p)->Surrounding();
                     }),
                     this);
    g_signal_connect(context_, "delete-surrounding",
                     G_CALLBACK(+[](GtkIMContext*, int offset, int count,
                                    gpointer p) -> gboolean {
                       return static_cast<TextInput*>(p)->DeleteSurrounding(
                           offset, count);
                     }),
                     this);
  }
  int WithoutComposition(int position) const {
    if (composing_base_ < 0) return position;
    int start = std::min(composing_base_, composing_extent_),
        end = std::max(composing_base_, composing_extent_);
    return position <= start ? position
           : position >= end ? position - (end - start)
                             : start;
  }
  void CancelPreedit() {
    if (client_ < 0 || (composing_base_ < 0 && !has_preedit_backup_)) return;
    if (has_preedit_backup_) {
      text_ = preedit_backup_;
      base_ = backup_base_;
      extent_ = backup_extent_;
      has_preedit_backup_ = false;
    } else {
      Replace("");
    }
    composing_base_ = composing_extent_ = -1;
    Publish();
  }
  void ResetContext(bool detach_key_context = false) {
    // Restore while the old client is still connected. IBus/Fcitx reset
    // replies can be asynchronous, so waiting for preedit-end is too late.
    auto* context = GTK_IM_CONTEXT(g_object_ref(context_));
    CancelPreedit();  // May reenter and replace the current client/context.
    bool previous = resetting_;
    resetting_ = true;
    // Detaching GtkEventControllerKey can itself make Fcitx commit preedit.
    // Keep those reset-time signals suppressed after restoring the old model.
    if (detach_key_context && context_ == context && set_context_)
      set_context_(nullptr);
    gtk_im_context_reset(context);
    gtk_im_context_focus_out(context);
    resetting_ = previous;
    g_object_unref(context);
  }
  std::string SurroundingText() const {
    auto text = text_;
    if (composing_base_ >= 0) {
      TextOffsets offsets(text);
      int start = offsets.Byte16(std::min(composing_base_, composing_extent_));
      int end = offsets.Byte16(std::max(composing_base_, composing_extent_));
      text.erase(start, end - start);
    }
    return text;
  }
  void Replace(const std::string& replacement) {
    TextOffsets before(text_), after(replacement);
    int start = composing_base_ >= 0
                    ? std::min(composing_base_, composing_extent_)
                    : std::min(base_, extent_);
    int end = composing_base_ >= 0
                  ? std::max(composing_base_, composing_extent_)
                  : std::max(base_, extent_);
    text_.replace(before.Byte16(start),
                  before.Byte16(end) - before.Byte16(start), replacement);
    base_ = extent_ = start + after.utf16.back();
    composing_base_ = start;
    composing_extent_ = extent_;
  }
  void Preedit() {
    if (client_ < 0 || resetting_) return;
    char* text = nullptr;
    PangoAttrList* attrs = nullptr;
    int cursor = 0;
    gtk_im_context_get_preedit_string(context_, &text, &attrs, &cursor);
    TextOffsets offsets(text ? text : "");
    if (offsets.valid && (offsets.utf16.back() > 0 || composing_base_ >= 0)) {
      if (composing_base_ < 0 && offsets.utf16.back() > 0) {
        has_preedit_backup_ = true;
        preedit_backup_ = text_;
        backup_base_ = base_;
        backup_extent_ = extent_;
      }
      Replace(text ? text : "");
      base_ = extent_ = composing_base_ + offsets.Character16(cursor);
      Publish();
    }
    if (attrs) pango_attr_list_unref(attrs);
    g_free(text);
  }
  bool Publish() {
    if (client_ < 0) return false;
    // Copy state: synchronous Clay callbacks may change client or editing
    // state.
    const auto text = text_;
    const int client = client_, base = base_, extent = extent_,
              cb = composing_base_, ce = composing_extent_;
    return update_(client, text, base, extent, cb, ce);
  }
  GtkWidget* widget_;
  GtkIMContext* context_;
  GtkEventController* focus_;
  Update update_;
  std::function<void(GtkIMContext*)> set_context_;
  int client_ = -1, base_ = 0, extent_ = 0, composing_base_ = -1,
      composing_extent_ = -1;
  bool has_preedit_backup_ = false;
  std::string preedit_backup_;
  int backup_base_ = 0, backup_extent_ = 0;
  std::string text_;
  bool password_ = false, shown_ = false;
  bool resetting_ = false;
};
}  // namespace lynx::gtk
#endif
