// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <windows.h>
// clang-format off
#include <winhttp.h>
// clang-format on

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "platform/embedder/http_service/lynx_http_service_impl.h"

// cspell:ignore ADDREQ WINHTTP

namespace lynx {
namespace embedder {

namespace {

// The status code reported for transport/SDK level failures (no HTTP response
// was received), matching the convention used by the macOS service.
constexpr int kSdkErrorStatusCode = 499;

// A WinHTTP handle wrapper that closes itself on scope exit.
class ScopedInternetHandle {
 public:
  ScopedInternetHandle() = default;
  explicit ScopedInternetHandle(HINTERNET handle) : handle_(handle) {}
  ~ScopedInternetHandle() {
    if (handle_) {
      ::WinHttpCloseHandle(handle_);
    }
  }

  ScopedInternetHandle(const ScopedInternetHandle&) = delete;
  ScopedInternetHandle& operator=(const ScopedInternetHandle&) = delete;

  HINTERNET get() const { return handle_; }
  explicit operator bool() const { return handle_ != nullptr; }

 private:
  HINTERNET handle_ = nullptr;
};

std::wstring Utf8ToWide(const std::string& utf8) {
  if (utf8.empty()) {
    return std::wstring();
  }
  int size = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                   static_cast<int>(utf8.size()), nullptr, 0);
  if (size <= 0) {
    return std::wstring();
  }
  std::wstring wide(static_cast<size_t>(size), L'\0');
  ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                        wide.data(), size);
  return wide;
}

std::string WideToUtf8(const wchar_t* wide, size_t length) {
  if (wide == nullptr || length == 0) {
    return std::string();
  }
  int size = ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(length),
                                   nullptr, 0, nullptr, nullptr);
  if (size <= 0) {
    return std::string();
  }
  std::string utf8(static_cast<size_t>(size), '\0');
  ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(length), utf8.data(),
                        size, nullptr, nullptr);
  return utf8;
}

// Completes the response with a transport-level failure.
void FailResponse(const std::shared_ptr<pub::LynxHttpResponse>& response,
                  const std::string& message) {
  response->SetStatusCode(kSdkErrorStatusCode);
  response->SetStatusText(message.c_str());
}

// Reads all the response headers and forwards them to |response|. WinHTTP
// returns the raw header block as CRLF-separated "Key: Value" lines terminated
// by an empty line; the first line is the status line and is skipped.
void CopyResponseHeaders(
    HINTERNET request, const std::shared_ptr<pub::LynxHttpResponse>& response) {
  DWORD size = 0;
  ::WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                        &size, WINHTTP_NO_HEADER_INDEX);
  if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
    return;
  }
  std::wstring raw(size / sizeof(wchar_t), L'\0');
  if (!::WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                             WINHTTP_HEADER_NAME_BY_INDEX, raw.data(), &size,
                             WINHTTP_NO_HEADER_INDEX)) {
    return;
  }

  size_t raw_len =
      raw.find(L'\0') == std::wstring::npos ? raw.size() : raw.find(L'\0');
  const std::string headers = WideToUtf8(raw.c_str(), raw_len);
  size_t start = 0;
  bool first_line = true;
  while (start < headers.size()) {
    size_t end = headers.find("\r\n", start);
    if (end == std::string::npos) {
      end = headers.size();
    }
    const std::string line = headers.substr(start, end - start);
    start = end + 2;
    if (line.empty()) {
      continue;
    }
    // Skip the HTTP status line (e.g. "HTTP/1.1 200 OK").
    if (first_line) {
      first_line = false;
      continue;
    }
    size_t colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    std::string key = line.substr(0, colon);
    size_t value_start = colon + 1;
    while (value_start < line.size() && line[value_start] == ' ') {
      ++value_start;
    }
    std::string value = line.substr(value_start);
    if (!key.empty()) {
      response->AddHeader(key, value);
    }
  }
}

// Performs the whole WinHTTP transfer on a worker thread. |request| and
// |response| are kept alive for the duration; the response completion callback
// fires when the shared_ptr drops (via LynxHttpResponse's destructor).
void PerformRequest(std::shared_ptr<pub::LynxHttpRequest> request,
                    std::shared_ptr<pub::LynxHttpResponse> response) {
  const std::wstring url = Utf8ToWide(request->GetUrl());

  URL_COMPONENTS components = {};
  components.dwStructSize = sizeof(components);
  wchar_t host[256] = {0};
  wchar_t path[4096] = {0};
  components.lpszHostName = host;
  components.dwHostNameLength = ARRAYSIZE(host);
  components.lpszUrlPath = path;
  components.dwUrlPathLength = ARRAYSIZE(path);
  if (!::WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0,
                         &components)) {
    FailResponse(response, "failed to parse url");
    return;
  }
  const bool is_https = components.nScheme == INTERNET_SCHEME_HTTPS;
  // WinHttpCrackUrl fills the buffers and reports the actual lengths (the
  // written text is not guaranteed to be null-terminated), so build the
  // strings from the reported lengths.
  const std::wstring host_name(host, components.dwHostNameLength);
  const std::wstring url_path(path, components.dwUrlPathLength);

  ScopedInternetHandle session(
      ::WinHttpOpen(L"LynxEmbedder/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
  if (!session) {
    FailResponse(response, "failed to open WinHTTP session");
    return;
  }

  ScopedInternetHandle connect(
      ::WinHttpConnect(session.get(), host_name.c_str(), components.nPort, 0));
  if (!connect) {
    FailResponse(response, "failed to connect");
    return;
  }

  const std::wstring method =
      Utf8ToWide(request->GetMethod().empty() ? "GET" : request->GetMethod());
  ScopedInternetHandle req(::WinHttpOpenRequest(
      connect.get(), method.c_str(), url_path.c_str(), nullptr,
      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
      is_https ? WINHTTP_FLAG_SECURE : 0));
  if (!req) {
    FailResponse(response, "failed to open request");
    return;
  }

  // Request headers.
  for (const auto& header : request->GetHeaders()) {
    std::wstring header_line = Utf8ToWide(header.first + ": " + header.second);
    ::WinHttpAddRequestHeaders(
        req.get(), header_line.c_str(), static_cast<DWORD>(header_line.size()),
        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
  }

  // Request body.
  const std::vector<uint8_t>& body = request->GetBody();
  LPVOID body_ptr = body.empty() ? WINHTTP_NO_REQUEST_DATA
                                 : const_cast<uint8_t*>(body.data());
  DWORD body_size = static_cast<DWORD>(body.size());
  if (!::WinHttpSendRequest(req.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            body_ptr, body_size, body_size, 0)) {
    FailResponse(response, "failed to send request");
    return;
  }

  if (!::WinHttpReceiveResponse(req.get(), nullptr)) {
    FailResponse(response, "failed to receive response");
    return;
  }

  // Status code.
  DWORD status_code = 0;
  DWORD status_code_size = sizeof(status_code);
  ::WinHttpQueryHeaders(req.get(),
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status_code,
                        &status_code_size, WINHTTP_NO_HEADER_INDEX);
  response->SetStatusCode(static_cast<int>(status_code));

  // Status text.
  DWORD status_text_size = 0;
  ::WinHttpQueryHeaders(req.get(), WINHTTP_QUERY_STATUS_TEXT,
                        WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                        &status_text_size, WINHTTP_NO_HEADER_INDEX);
  if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER && status_text_size > 0) {
    std::wstring status_text(status_text_size / sizeof(wchar_t), L'\0');
    if (::WinHttpQueryHeaders(req.get(), WINHTTP_QUERY_STATUS_TEXT,
                              WINHTTP_HEADER_NAME_BY_INDEX, status_text.data(),
                              &status_text_size, WINHTTP_NO_HEADER_INDEX)) {
      size_t len = status_text.find(L'\0');
      if (len == std::wstring::npos) {
        len = status_text.size();
      }
      response->SetStatusText(WideToUtf8(status_text.c_str(), len).c_str());
    }
  }

  // Response headers.
  CopyResponseHeaders(req.get(), response);

  // Response body: read until WinHttpReadData returns 0 bytes.
  std::string response_body;
  for (;;) {
    DWORD available = 0;
    if (!::WinHttpQueryDataAvailable(req.get(), &available)) {
      break;
    }
    if (available == 0) {
      break;
    }
    std::string chunk(available, '\0');
    DWORD read = 0;
    if (!::WinHttpReadData(req.get(), chunk.data(), available, &read) ||
        read == 0) {
      break;
    }
    response_body.append(chunk.data(), read);
  }

  if (!response_body.empty()) {
    response->SetBody(reinterpret_cast<uint8_t*>(response_body.data()),
                      response_body.size());
  }
}

}  // namespace

void LynxHttpServiceImpl::Request(
    std::shared_ptr<pub::LynxHttpRequest> request,
    std::shared_ptr<pub::LynxHttpResponse> response) {
  // Request() is invoked on the JS thread; WinHTTP is used synchronously, so
  // run the transfer on a detached worker thread. The response completion
  // (which marshals the result back to JS) fires when |response| drops.
  std::thread([request = std::move(request),
               response = std::move(response)]() mutable {
    PerformRequest(std::move(request), std::move(response));
  }).detach();
}

}  // namespace embedder
}  // namespace lynx
