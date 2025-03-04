// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_LAZY_COMPONENT_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_LAZY_COMPONENT_H_

#include <memory>
#include <string>

#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"

namespace lynx {

namespace tasm {

/**
 *    The LazyComponent serves a crucial role in Lynx DSL by modularizing
 * complex Lynx template structure into separate, manageable template files. It
 * empowers developers to isolate specific parts of Lynx page - such as headers,
 * footers, sidebars, or other lazy content sections into individual template
 * modules. This modularization aids not only in enhancing code readability and
 * maintainability but also in increasing development efficiency by allowing
 * multiple teams or developers to work on separate components concurrently
 * without interfering with each other's code.
 *
 *   Benefits of using the LazyComponent includes:
 *
 *   1. **Improved Maintainability**: Changes to a specific part of a Lynx page
 * can be made independently of others, minimize the risk of introducing issues
 * into the system and easing the debugging process;
 *   2. **Reusability**: Components can be reused across different pages and
 * projects. This reduces the effort and time required to develop new Lynx pages
 * or features that utilize similar elements, ensuring consistency across
 * projects.
 *   3. **Scalability**: As projects grow, managing individual components rather
 * than monolithic page structures can be more manageable. This class structure
 *                           supports a scalable architecture that adapters to
 * increasing complexity without significant reworks.
 *   4. **Just-In-Time Component Loading**: load components when they are
 * needed, rather than during the initial Lynx page loading phase. This strategy
 *                           considerably decreases unnecessary resource
 * loading, template decoding process, which can lead to performance gains,
 * lower consumption, and improved responsiveness.
 *   5. **Separation of Concerns**: Each component can be developed, tested, and
 * debugged independently, adhering to software development best practises.
 *                           This separation not only makes the development
 * process more systematic but also enhances the clarity of the system
 * architecture.
 *
 *
 *   Example Usage:
 *   Consider implementing a user profile section that appears across multiple
 * pages in a Lynx page. By developing a Lazy Component for the user profile,
 * the team can focus on creating, testing, and refining this element
 * separately. Once perfected, it can be integrated as needed across various
 * Lynx pages, contributing to a cohesive user interface with minimal repetition
 * in the development workflow.
 *
 *   In summary, the Lazy Component is a strategic asset in modern UI
 * development. Its integration into projects supports more structured
 * codebases, efficient development practices, and results in robust,
 * high-performance Lynx pages.
 */

class TemplateAssembler;

using LazyBundleState = lazy_bundle::LazyBundleState;

class RadonLazyComponent : public RadonComponent {
 public:
  RadonLazyComponent(
      TemplateAssembler* tasm, const std::string& entry_name,
      PageProxy* page_proxy, int tid, CSSFragment* style_sheet,
      std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
      ComponentMould* mould, lepus::Context* context, uint32_t node_index,
      const lepus::Value& global_props,
      const base::String& tag_name = BASE_STATIC_STRING(kRadonComponentTag));

  // construct an empty component
  // context_, mould_, intrinsic_style_sheet_ are nullptr;
  // path_ is empty;
  RadonLazyComponent(
      TemplateAssembler* tasm, const std::string& entry_name,
      PageProxy* page_proxy, int tid, uint32_t node_index,
      const base::String& tag_name = BASE_STATIC_STRING(kRadonComponentTag));

  RadonLazyComponent(const RadonLazyComponent& node, PtrLookupMap& map);

  void InitLazyComponent(
      CSSFragment* style_sheet,
      std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
      ComponentMould* mould, lepus::Context* context);

  void SetGlobalProps(const lepus::Value& global_props);
  void DeriveFromMould(ComponentMould* data) override;

  virtual lepus::Value& GetComponentInfoMap(
      const std::string& entry_name = "") override;

  virtual lepus::Value& GetComponentPathMap(
      const std::string& entry_name = "") override;

  void UpdateDynamicCompTopLevelVariables(ComponentMould* data,
                                          const lepus::Value& global_props);

  bool NeedsExtraData() const override;

  bool LoadLazyBundle(const std::string& url, TemplateAssembler* tasm,
                      const uint32_t uid);

  bool LoadLazyBundleFromJS(const std::string& url);

  virtual const std::string& GetEntryName() const override;
  virtual bool UpdateGlobalProps(const lepus::Value& table) override;

  virtual bool CanBeReusedBy(const RadonBase* const radon_base) const override;

  virtual void SetProperties(const base::String& key, const lepus::Value& value,
                             bool strict_prop_type) override;

  virtual void SetData(const base::String& key,
                       const lepus::Value& value) override;

  static RadonLazyComponent* CreateRadonLazyComponent(TemplateAssembler* tasm,
                                                      const std::string& url,
                                                      const base::String& name,
                                                      int tid, uint32_t index);

  bool SetContext(TemplateAssembler* tasm);

  void OnComponentAdopted();

  void SwapElement(const std::unique_ptr<RadonBase>&,
                   const DispatchOption&) override;

  inline void SetLazyBundleState(LazyBundleState state,
                                 const lepus::Value& msg) {
    state_ = state;
    event_msg_ = msg;

    // only in sync mode, we need to send bindEvent here;
    need_send_event_ = true;
  }

  uint32_t Uid() const { return uid_; }

  void AddFallback(std::unique_ptr<RadonPlug> fallback);

  // try to render fallback if it exists
  bool RenderFallback();

  // If "is" of this componet is undefined, it will be marked as a js component,
  // which means its entry name can be controlled by js.
  void MarkJSComponent() { is_js_component_ = true; }

 private:
  // create a new slot to adopt fallback plug
  void CreateAndAdoptFallback(std::unique_ptr<RadonPlug> fallback);
  // dispatch for render when LoadComponentWithCallback or show fallback if
  // failed
  void DispatchForRender();

  bool LoadLazyBundleInternal();

  void UpdateSystemInfoToContext(const lepus::Value& system_info);

  // for lazy component event
  bool need_send_event_{false};
  LazyBundleState state_{LazyBundleState::STATE_UNKNOWN};
  lepus::Value event_msg_;

  TemplateAssembler* tasm_;
  virtual void RenderRadonComponent(RenderOption&) override;

  // only use for lazy component require callback
  uint32_t uid_;
  static uint32_t uid_generator_;

  std::unique_ptr<RadonPlug> fallback_{nullptr};

  bool is_js_component_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_LAZY_COMPONENT_H_
