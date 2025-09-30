// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_LOAD_META_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_LOAD_META_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "capi/lynx_load_meta_capi.h"
#include "lynx_template_bundle.h"
#include "lynx_template_data.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief `LynxLoadMeta` is the metadata structure used by LynxView to load
 * templates. It will be used as the input parameter of loadTemplate. Users can
 * use `LynxLoadMeta` to specify many parameters of loading templates, such as
 * specifying template content, template initial data, and other additional
 * configuration information. url, binary_data, bundle are the required
 * parameters for template loading. You can pass only one or all of them. The
 * priority relationship is as follows: bundle > binary_data > url.
 */
class LynxLoadMeta {
 public:
  LynxLoadMeta() : load_meta_(lynx_load_meta_create()) {}
  ~LynxLoadMeta() { lynx_load_meta_release(load_meta_); }

  /**
   * @apidoc
   * @brief Set the url of the template to be loaded.
   * @param url The url of the template to be loaded.
   */
  void SetUrl(const std::string& url) {
    url_ = url;
    lynx_load_meta_set_url(load_meta_, url_.c_str());
  }

  /**
   * @apidoc
   * @brief Set the binary data of the template to be loaded.
   * @param content The binary data of the template to be loaded.
   */
  void SetBinaryData(std::vector<uint8_t> content) {
    lynx_load_meta_set_binary_data(load_meta_, content.data(), content.size(),
                                   nullptr, nullptr);
  }

  /**
   * @apidoc
   * @brief Set the binary data of the template to be loaded.
   * @param content The binary data of the template to be loaded.
   * @param length The length of the binary data.
   * @param dtor The destructor of the binary data.
   * @param opaque The opaque pointer of the binary data.
   */
  void SetBinaryData(uint8_t* content, size_t length,
                     void (*dtor)(uint8_t*, size_t, void*) = nullptr,
                     void* opaque = nullptr) {
    lynx_load_meta_set_binary_data(load_meta_, content, length, dtor, opaque);
  }

  /**
   * @apidoc
   * @brief Set the template bundle of the template to be loaded.
   * @param template_bundle The template bundle of the template to be loaded.
   */
  void SetTemplateBundle(std::shared_ptr<LynxTemplateBundle> template_bundle) {
    template_bundle_ = std::move(template_bundle);
    lynx_load_meta_set_template_bundle(load_meta_, template_bundle_->Impl());
  }

  /**
   * @apidoc
   * @brief Set the initial data of the template to be loaded.
   * @param initial_data The initial data of the template to be loaded.
   */
  void SetInitialData(std::shared_ptr<LynxTemplateData> initial_data) {
    initial_data_ = std::move(initial_data);
    lynx_load_meta_set_initial_data(load_meta_, initial_data_->Impl());
  }

  /**
   * @apidoc
   * @brief Set the global props of the template to be loaded.
   * @param global_props The global props of the template to be loaded.
   */
  void SetGlobalProps(std::shared_ptr<LynxTemplateData> global_props) {
    global_props_ = std::move(global_props);
    lynx_load_meta_set_global_props(load_meta_, global_props_->Impl());
  }

  std::string GetUrl() const { return url_; }

  bool IsBundleValid() const {
    return template_bundle_ != nullptr && template_bundle_->IsValid();
  }

  lynx_load_meta_t* Impl() { return load_meta_; }

  LynxLoadMeta(const LynxLoadMeta&) = delete;
  LynxLoadMeta& operator=(const LynxLoadMeta&) = delete;

 private:
  lynx_load_meta_t* load_meta_;
  std::string url_;
  std::shared_ptr<LynxTemplateBundle> template_bundle_;
  std::shared_ptr<LynxTemplateData> initial_data_;
  std::shared_ptr<LynxTemplateData> global_props_;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_LOAD_META_H_
