// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_websocket_module.h"

#include <random>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <malloc.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSESOCKET closesocket
#define alloca _alloca
#else
#include <alloca.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define SOCKET int
#define CLOSESOCKET ::close
#endif

#include <openssl/ssl.h>

#include <cstring>

#include "clay/fml/base64.h"
#include "clay/lynx_adaptor/clay_value.h"

namespace lynx {

namespace {
std::string GenerateSecWebsocketKey() {
  // Generate 16-byte random numbers.
  std::vector<unsigned char> random_bytes(16);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned short> dis(0, 255);
  for (auto& byte : random_bytes) {
    byte = static_cast<unsigned char>(dis(gen));
  }
  // Encode with base64.
  size_t length =
      ::fml::Base64::Encode(random_bytes.data(), random_bytes.size(), nullptr);
  std::vector<char> base64_data(length);
  ::fml::Base64::Encode(random_bytes.data(), random_bytes.size(),
                        base64_data.data());
  std::string key(base64_data.begin(), base64_data.end());
  return key;
}
}  // namespace

class SimpleWebSocket {
 public:
  SimpleWebSocket(std::weak_ptr<shell::LynxRuntimeProxy> runtime_proxy,
                  const std::string& url, std::vector<std::string> protocols,
                  std::unordered_map<std::string, std::string> headers, int id)
      : id_(id),
        url_(url),
        protocols_(std::move(protocols)),
        headers_(std::move(headers)),
        runtime_proxy_(runtime_proxy) {
    thread_ = std::make_unique<std::thread>([this]() { run(); });
  }
  ~SimpleWebSocket() { Close(0, ""); }

  void Close(double code, const std::string& reason) {
    code_ = code;
    reason_ = reason;

    if (socket_) {
      CLOSESOCKET(socket_);
    }
    if (ssl_) {
      SSL_shutdown(ssl_);
    }
    if (thread_) {
      if (thread_->joinable()) {
        thread_->join();
      }
      thread_.reset();
    }
    if (ssl_) {
      SSL_free(ssl_);
      ssl_ = nullptr;
    }
    socket_ = 0;
  }

  void Ping() {
    if (!socket_) {
      return;
    }
    uint8_t txb[16];
    size_t txb_len = 2;

    txb[0] = 9 /*OP_PING*/ | 0x80 /*FIN*/;
    txb[1] = 0;

    // All frames sent from client to server have this bit set to 1.
    txb[1] |= 0x80 /*MASK*/;
    *reinterpret_cast<uint32_t*>(txb + txb_len) = 0;
    txb_len += 4;

    do_write((char*)txb, txb_len);
  }

  void Write(const std::string& data) {
    if (!socket_) {
      return;
    }
    const char* payload = data.data();
    size_t payload_len = data.size();
    uint8_t* txb = (uint8_t*)alloca(16 + payload_len);
    size_t txb_len = 2;

    txb[0] = 1 /*OP_TEXT*/ | 0x80 /*FIN*/;

    if (payload_len > 65535) {
      txb[1] = 127;
      *reinterpret_cast<uint32_t*>(txb + 2) = 0;
      txb[6] = payload_len >> 24;
      txb[7] = payload_len >> 16;
      txb[8] = payload_len >> 8;
      txb[9] = payload_len;
      txb_len += 8;
    } else if (payload_len > 125) {
      txb[1] = 126;
      txb[2] = payload_len >> 8;
      txb[3] = payload_len;
      txb_len += 2;
    } else {
      txb[1] = payload_len;
    }

    // All frames sent from client to server have this bit set to 1.
    txb[1] |= 0x80 /*MASK*/;
    *reinterpret_cast<uint32_t*>(txb + txb_len) = 0;
    txb_len += 4;

    memcpy(txb + txb_len, payload, payload_len);
    txb_len += payload_len;

    do_write((char*)txb, txb_len);
  }

 private:
  void emit(const std::string& event_name, clay::Value::Map map) {
    if (auto proxy = runtime_proxy_.lock()) {
      clay::Value::Array array_wrapper(2);
      array_wrapper[0] = clay::Value(event_name);
      clay::Value::Array array_args(1);
      array_args[0] = clay::Value(std::move(map));
      array_wrapper[1] = clay::Value(std::move(array_args));

      auto params = clay::Value(std::move(array_wrapper));
      proxy->CallJSFunction(
          "GlobalEventEmitter", "emit",
          std::make_unique<lynx::ClayValue>(std::move(params)));
    }
  }

  void run() {
    if (!Connect()) {
      clay::Value::Map map;
      map["id"] = clay::Value(id_);
      map["message"] = clay::Value("");
      emit("websocketFailed", std::move(map));
      return;
    }

    {
      clay::Value::Map map;
      map["id"] = clay::Value(id_);
      map["protocol"] = clay::Value(protocol_);
      emit("websocketOpen", std::move(map));
    }

    uint8_t opcode;
    std::string payload;
    while (Read(opcode, payload)) {
      if (opcode == 0x1) {  // OP_TEXT
        clay::Value::Map map;
        map["id"] = clay::Value(id_);
        map["type"] = clay::Value("text");
        map["data"] = clay::Value(payload);
        emit("websocketMessage", std::move(map));
      }
    }

    {
      clay::Value::Map map;
      map["id"] = clay::Value(id_);
      map["code"] = clay::Value(code_);
      map["reason"] = clay::Value(reason_);
      emit("websocketClosed", std::move(map));
    }
  }

  bool Connect() {
    const char* purl = url_.c_str();
    int port = 80;
    bool wss = false;
    if (memcmp(purl, "wss://", 6) == 0) {
      port = 443;
      wss = true;
      purl += 6;
    } else if (memcmp(purl, "ws://", 5) == 0) {
      purl += 5;
    } else {
      return false;
    }

    char host[256] = {0};
    char path[4000] = {0};
    if (sscanf(purl, "%[^:/]:%d/%s", host, &port, path) == 3) {
    } else if (sscanf(purl, "%[^:/]/%s", host, path) == 2) {
    } else if (sscanf(purl, "%[^:/]:%d", host, &port) == 2) {
    } else if (sscanf(purl, "%[^:/]", host) == 1) {
    } else {
      return false;
    }

    struct addrinfo ai, *sai;
    memset(&ai, 0, sizeof ai);
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_STREAM;
    char str_port[16];
    snprintf(str_port, sizeof(str_port), "%d", port);
    int ret = ::getaddrinfo(host, str_port, &ai, &sai);
    if (ret != 0) {
      return false;
    }

    for (auto p = sai; p != NULL; p = p->ai_next) {
      auto sockfd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (sockfd == -1) {
        continue;
      }
      if (::connect(sockfd, p->ai_addr, p->ai_addrlen) != -1) {
        socket_ = sockfd;
        break;
      }
      CLOSESOCKET(sockfd);
    }
    ::freeaddrinfo(sai);

    if (socket_ == 0) {
      return false;
    }

    if (wss) {
      SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
      SSL* ssl = SSL_new(ctx);
      SSL_set_fd(ssl, socket_);

      if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        CLOSESOCKET(socket_);
        socket_ = 0;
        return false;
      }
      SSL_CTX_free(ctx);
      ssl_ = ssl;
    }

    std::string protocols_str;
    for (const auto& proto : protocols_) {
      if (!protocols_str.empty()) {
        protocols_str += ',';
      }
      protocols_str += proto;
    }
    if (!protocols_str.empty()) {
      headers_["Sec-WebSocket-Protocol"] = protocols_str;
    }

    std::string headers_str;
    for (const auto& [key, val] : headers_) {
      headers_str += key;
      headers_str += ": ";
      headers_str += val;
      headers_str += "\r\n";
    }

    char buf[512 + strlen(path) + strlen(host) + headers_str.size()];
    // Generate `Sec-WebSocket-Key` using random numbers.
    std::string key = GenerateSecWebsocketKey();
    snprintf(buf, sizeof(buf),
             "GET /%s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Key: %s\r\n"
             "Sec-WebSocket-Version: 13\r\n"
             "%s\r\n",
             path, host, port, key.c_str(), headers_str.c_str());
    do_write(buf, strlen(buf));

    int status;
    if (read_line(buf, sizeof(buf)) < 10 ||
        sscanf(buf, "HTTP/1.1 %d Switching Protocols\r\n", &status) != 1 ||
        status != 101) {
      return false;
    }

    while (read_line(buf, sizeof(buf)) > 0 && buf[0] != '\r') {
      auto len = strlen(buf);
      buf[len - 2] = '\0';  // '\r\n' -> '\0\n'
      if (memcmp(buf, "Sec-WebSocket-Protocol: ", 24) == 0) {
        protocol_ = &buf[24];
      }
      // Prefer to validate handshake with `Sec-WebSocket-Accept` from server
      // here.
    }
    return true;
  }

  bool Read(uint8_t& opcode, std::string& payload) {
    struct {
      uint8_t flag_opcode;
      uint8_t mask_payload_len;
    } head;

    if (do_read((char*)&head, sizeof(head)) != sizeof(head)) {
      return false;
    }
    if ((head.flag_opcode & 0x80) == 0) {  // FIN
      return false;
    }
    uint8_t flags = head.flag_opcode >> 4;
    if ((head.mask_payload_len & 0x80) != 0) {  // masked payload
      return false;
    }
    size_t payload_len = head.mask_payload_len & 0x7f;
    bool deflated = (flags & 4 /*FLAG_RSV1*/) != 0;
    if (deflated) {
      return false;
    }

    if (payload_len == 126) {
      uint8_t len[2];
      do_read((char*)&len, sizeof(len));
      payload_len = (len[0] << 8) | len[1];
    } else if (payload_len == 127) {
      uint8_t len[8];
      do_read((char*)&len, sizeof(len));
      payload_len = (len[4] << 24) | (len[5] << 16) | (len[6] << 8) | len[7];
    }

    opcode = head.flag_opcode & 0x0F;
    if (payload_len == 0) {
      return true;
    }

    payload.resize(payload_len);

    if (do_read(payload.data(), payload_len) != payload_len) {
      return false;
    }
    return true;
  }

  int read_line(char* buf, size_t size) {
    char* out = buf;
    while (out - buf < size) {
      int res = do_read(out, 1);
      if (res == 1) {
        if (*out++ == '\n') {
          break;
        }
      } else if (res == -1) {
        // printf("recv errr: %d\n", errno);
        break;
      }
    }
    *out = '\0';
    // printf("RX: %s [%d bytes]\n", buf, int(out - buf));
    return out - buf;
  }

  int do_read(char* buf, size_t size) {
    if (ssl_) {
      return SSL_read(ssl_, buf, size);
    }
    return ::recv(socket_, buf, size, 0);
  }

  int do_write(char* buf, size_t size) {
    // printf("TX: %s [%d bytes]\n", buf, int(size));
    if (ssl_) {
      return SSL_write(ssl_, buf, size);
    }
    return ::send(socket_, buf, size, 0);
  }

  int id_;
  SOCKET socket_ = 0;
  SSL* ssl_ = nullptr;
  std::string url_;
  std::string protocol_;
  std::vector<std::string> protocols_;
  std::unordered_map<std::string, std::string> headers_;
  std::unique_ptr<std::thread> thread_;
  std::weak_ptr<shell::LynxRuntimeProxy> runtime_proxy_;
  double code_;
  std::string reason_;
};

const std::string LynxWebSocketModule::name_ = "LynxWebSocketModule";

LynxWebSocketModule::LynxWebSocketModule(
    uint32_t view_context_id, fml::RefPtr<fml::TaskRunner> task_runner)
    : LynxModuleBase(view_context_id, task_runner) {
  runtime::NativeModuleMethod connect_method("connect", 4);
  RegisterMethod(connect_method, &LynxWebSocketModule::connect);

  runtime::NativeModuleMethod send_method("send", 2);
  RegisterMethod(send_method, &LynxWebSocketModule::send);

  runtime::NativeModuleMethod ping_method("ping", 1);
  RegisterMethod(ping_method, &LynxWebSocketModule::ping);

  runtime::NativeModuleMethod close_method("close", 3);
  RegisterMethod(close_method, &LynxWebSocketModule::close);
}

LynxWebSocketModule::~LynxWebSocketModule() {
  for (const auto& [id, ws] : sockets_) {
    delete ws;
  }
}

std::unique_ptr<pub::Value> LynxWebSocketModule::connect(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap& callbacks) {
  std::string url = args->GetValueAtIndex(0)->str();
  auto protocols_array = args->GetValueAtIndex(1);
  auto options_object = args->GetValueAtIndex(2);
  int id = static_cast<int>(args->GetValueAtIndex(3)->Number());
  std::vector<std::string> protocols;
  std::unordered_map<std::string, std::string> headers;

  if (protocols_array->IsArray()) {
    protocols_array->ForeachArray(
        [&protocols](int64_t idx, const pub::Value& val) {
          protocols.push_back(val.str());
        });
  }
  if (options_object->IsMap()) {
    auto headers_object = options_object->GetValueForKey("headers");
    if (headers_object->IsMap()) {
      headers_object->ForeachMap(
          [&headers](const pub::Value& key, const pub::Value& val) {
            headers[key.str()] = val.str();
          });
    }
  }
  auto ws = new SimpleWebSocket(runtime_proxy_, url, std::move(protocols),
                                std::move(headers), id);
  if (auto search = sockets_.find(id); search != sockets_.end()) {
    delete search->second;
  }
  sockets_[id] = ws;
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
};

std::unique_ptr<pub::Value> LynxWebSocketModule::send(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap& callbacks) {
  std::string msg = args->GetValueAtIndex(0)->str();
  int id = static_cast<int>(args->GetValueAtIndex(1)->Number());
  if (auto search = sockets_.find(id); search != sockets_.end()) {
    search->second->Write(msg);
  }
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

std::unique_ptr<pub::Value> LynxWebSocketModule::ping(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap& callbacks) {
  int id = static_cast<int>(args->GetValueAtIndex(0)->Number());
  if (auto search = sockets_.find(id); search != sockets_.end()) {
    search->second->Ping();
  }
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

std::unique_ptr<pub::Value> LynxWebSocketModule::close(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap& callbacks) {
  auto code = args->GetValueAtIndex(0)->Number();
  auto reason = args->GetValueAtIndex(1)->str();
  int id = static_cast<int>(args->GetValueAtIndex(2)->Number());
  if (auto search = sockets_.find(id); search != sockets_.end()) {
    search->second->Close(code, reason);
    delete search->second;
    sockets_.erase(search);
  }
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

}  // namespace lynx
