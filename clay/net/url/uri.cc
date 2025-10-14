// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/url/uri.h"

#include <string_view>

#include "clay/fml/logging.h"
#include "clay/net/macros.h"

namespace clay {

namespace url {

Uri::Uri(const std::string& uri) : spec_(uri) { DoParse(); }

std::string_view Uri::PathView() const {
  if (!valid_) {
    return "";
  }
  return std::string_view(spec_.c_str() + path_parsed_.begin, path_parsed_.len);
}

std::string_view Uri::SchemeView() const {
  if (!valid_) {
    return {};
  }
  return std::string_view(spec_.c_str() + scheme_parsed_.begin,
                          scheme_parsed_.len);
}

std::string_view Uri::SchemeHostPortView() const {
  if (!valid_) {
    return "";
  }
  if (port_parsed_.is_valid()) {
    return std::string_view(spec_.c_str() + scheme_parsed_.begin,
                            port_parsed_.end() - scheme_parsed_.begin);
  }
  return std::string_view(spec_.c_str() + scheme_parsed_.begin,
                          host_parsed_.end() - scheme_parsed_.begin);
}

std::string Uri::SchemeHostPort() const {
  if (!valid_) {
    return "";
  }
  if (port_parsed_.is_valid()) {
    return spec_.substr(scheme_parsed_.begin,
                        port_parsed_.end() - scheme_parsed_.begin);
  }
  return spec_.substr(scheme_parsed_.begin,
                      host_parsed_.end() - scheme_parsed_.begin);
}

const std::vector<std::string_view>& Uri::QueryKeys() {
  if (!query_valid_) {
    DoParseQuery();
  }
  return query_keys_;
}

const std::vector<std::string_view>& Uri::QueryValues() {
  if (!query_valid_) {
    DoParseQuery();
  }
  return query_values_;
}

std::string_view Uri::GetQueryParameter(const std::string& key) {
  FML_DCHECK(valid_);

  if (!query_valid_) {
    DoParseQuery();
  }

  FML_DCHECK(query_keys_.size() == query_values_.size());
  size_t i = 0;
  for (; i < query_keys_.size(); ++i) {
    if (query_keys_[i] == key) {
      break;
    }
  }
  if (i != query_keys_.size()) {
    return query_values_[i];
  }
  return {};
}

void Uri::DoParse() {
  if (valid_ || spec_.empty()) {
    return;
  }

  Parsed input_parsed;
  ParseStandardURL(spec_.data(), spec_.size(), &input_parsed);
  if (!input_parsed.host.is_valid() || !input_parsed.scheme.is_valid()) {
    return;
  }
  // Diff between scheme and host should be "://"
  if (input_parsed.scheme.is_valid() &&
      (input_parsed.scheme.end() + 3 != input_parsed.host.begin)) {
    return;
  }
  scheme_parsed_ = input_parsed.scheme;
  host_parsed_ = input_parsed.host;
  port_parsed_ = input_parsed.port;
  path_parsed_ = input_parsed.path;
  query_parsed_ = input_parsed.query;

  valid_ = true;

  NET_LOG << "lgl parsed to Scheme: " << scheme_parsed_.begin
          << " Host:" << host_parsed_.begin << " Port: " << port_parsed_.begin
          << " Path:" << path_parsed_.begin;
}

void Uri::DoParseQuery() {
  FML_DCHECK(!query_valid_);
  url::Component query = query_parsed_;
  url::Component key;
  url::Component value;
  while (url::ExtractQueryKeyValue(spec_.c_str(), &query, &key, &value)) {
    query_keys_.emplace_back(spec_.c_str() + key.begin, key.len);
    query_values_.emplace_back(spec_.c_str() + value.begin, value.len);
  }
  query_valid_ = true;
}

}  // namespace url

}  // namespace clay
