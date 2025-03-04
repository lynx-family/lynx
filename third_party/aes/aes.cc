// Copyright (c) 2019 SergeyBel
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "aes.h"
#include "third_party/modp_b64/modp_b64.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>

AES::AES(AESKeyLength key_length) {
  switch (key_length) {
    case AESKeyLength::AES_128: {
      nk_ = 4;
      nr_ = 10;
      break;
    }
    case AESKeyLength::AES_192: {
      nk_ = 6;
      nr_ = 12;
      break;
    }
    case AESKeyLength::AES_256: {
      nk_ = 8;
      nr_ = 14;
      break;
    }
  }
  block_bytes_len_ = 4 * nb_ * sizeof(u_char);
}

std::string AES::EncryptECB(const std::string& plain,
                            const std::string& key_temp) {
  std::string out;
  out.reserve((plain.size() + 16) * 2);
  uint32_t key_size = 4 * nb_ * (nr_ + 1);
  std::array<u_char, 16> key;
  key.fill(0);
  size_t key_length = (key_temp.size() > 16 ? 16 : key_temp.size());
  std::copy_n(key_temp.begin(), key_length, key.begin());
  std::string round_key(key_size, 0);
  KeyExpansion(key, round_key);
  for (uint32_t i = 0; i < plain.size(); i += block_bytes_len_) {
    std::array<u_char, 16> temp;
    temp.fill(0);
    if (i + block_bytes_len_ > plain.size()) {
      std::copy(plain.begin() + i, plain.end(), temp.begin());
    } else {
      std::copy(plain.begin() + i, plain.begin() + i + block_bytes_len_,
                temp.begin());
    }
    temp = Encrypt(temp, round_key);
    out.append(temp.begin(), temp.end());
  }
  modp_b64_encode(out);
  return out;
}

std::array<AES::u_char, 16> AES::Encrypt(const std::array<u_char, 16>& plain,
                                         const std::string& round_key) {
  std::array<u_char, 16> cipher;
  std::array<std::array<u_char, 4>, 4> state;
  for (uint32_t i = 0; i < 4; ++i) {
    for (uint32_t j = 0; j < 4; ++j) {
      state[i][j] = plain[i + 4 * j];
    }
  }
  AddRoundKey(state, round_key);

  for (uint32_t i = 1; i < nr_; ++i) {
    SubBytes(state);
    ShiftRows(state);
    MixColumns(state);
    AddRoundKey(state, round_key.substr(i * 4 * nb_, round_key.size()));
  }
  SubBytes(state);
  ShiftRows(state);
  AddRoundKey(state, round_key.substr(nr_ * 4 * nb_));
  for (uint32_t i = 0; i < 4; ++i) {
    for (uint32_t j = 0; j < nb_; ++j) {
      cipher[i + 4 * j] = state[i][j];
    }
  }
  return cipher;
}

std::string AES::DecryptECB(std::string cipher, const std::string& key_temp) {
  modp_b64_decode(cipher);
  std::string out;
  out.reserve(cipher.size());
  uint32_t key_size = 4 * nb_ * (nr_ + 1);
  std::array<u_char, 16> key;
  key.fill(0);
  size_t key_length = (key_temp.size() > 16 ? 16 : key_temp.size());
  std::copy_n(key_temp.begin(), key_length, key.begin());
  std::string round_key(key_size, 0);
  KeyExpansion(key, round_key);
  for (uint32_t i = 0; i < cipher.size(); i += block_bytes_len_) {
    if (i + block_bytes_len_ > cipher.size()) {
      std::cerr << "Length Error" << std::endl;
      break;
    }
    std::array<u_char, 16> temp;
    std::copy(cipher.begin() + i, cipher.begin() + i + block_bytes_len_,
              temp.data());
    temp = Decrypt(temp, round_key);
    std::for_each(temp.begin(), temp.end(), [&out](char c) {
      if (c) {
        out.push_back(c);
      }
    });
  }
  return out;
}

std::array<AES::u_char, 16> AES::Decrypt(const std::array<u_char, 16>& cipher,
                                         const std::string& round_key) {
  std::array<u_char, 16> plain;
  std::array<std::array<u_char, 4>, 4> state;
  for (uint32_t i = 0; i < 4; ++i) {
    for (uint32_t j = 0; j < 4; ++j) {
      state[i][j] = cipher[i + 4 * j];
    }
  }
  AddRoundKey(state, round_key.substr(nr_ * 4 * nb_));

  for (uint32_t i = nr_ - 1; i > 0; --i) {
    InvSubBytes(state);
    InvShiftRows(state);
    AddRoundKey(state, round_key.substr(i * 4 * nb_));
    InvMixColumns(state);
  }
  InvSubBytes(state);
  InvShiftRows(state);
  AddRoundKey(state, round_key);
  for (uint32_t i = 0; i < 4; ++i) {
    for (uint32_t j = 0; j < nb_; ++j) {
      plain[i + 4 * j] = state[i][j];
    }
  }
  return plain;
}

void AES::AddRoundKey(std::array<std::array<u_char, 4>, 4>& state,
                      const std::string& key) {
  for (uint32_t i = 0; i < 4; i++) {
    for (uint32_t j = 0; j < nb_; j++) {
      state[i][j] = state[i][j] ^ key[i + 4 * j];
    }
  }
}

void AES::SubBytes(std::array<std::array<u_char, 4>, 4>& state) {
  for (uint32_t i = 0; i < 4; ++i) {
    for (uint32_t j = 0; j < nb_; ++j) {
      unsigned char t = state[i][j];
      state[i][j] = sbox_[t / 16][t % 16];
    }
  }
}

void AES::ShiftRows(std::array<std::array<u_char, 4>, 4>& state) {
  for (uint32_t i = 1; i < 4; ++i) {
    ShiftRows(state, i, i);
  }
}

void AES::ShiftRows(std::array<std::array<u_char, 4>, 4>& state, uint32_t i,
                    uint32_t n) {
  std::array<u_char, nb_> temp;
  for (uint32_t j = 0; j < nb_; ++j) {
    temp[j] = state[i][(j + n) % nb_];
  }
  state[i] = std::move(temp);
}

void AES::MixColumns(std::array<std::array<u_char, 4>, 4>& state) {
  for (uint32_t i = 0; i < 4; i++) {
    u_char c1 = Multiple(2, state[0][i]) ^ Multiple(3, state[1][i]) ^
                state[2][i] ^ state[3][i];
    u_char c2 = state[0][i] ^ Multiple(2, state[1][i]) ^
                Multiple(3, state[2][i]) ^ state[3][i];
    u_char c3 = state[0][i] ^ state[1][i] ^ Multiple(2, state[2][i]) ^
                Multiple(3, state[3][i]);
    u_char c4 = Multiple(3, state[0][i]) ^ state[1][i] ^ state[2][i] ^
                Multiple(2, state[3][i]);
    state[0][i] = c1;
    state[1][i] = c2;
    state[2][i] = c3;
    state[3][i] = c4;
  }
}

void AES::InvSubBytes(std::array<std::array<u_char, 4>, 4>& state) {
  for (uint32_t i = 0; i < 4; ++i) {
    for (uint32_t j = 0; j < nb_; ++j) {
      unsigned char t = state[i][j];
      state[i][j] = inv_sbox_[t / 16][t % 16];
    }
  }
}

void AES::InvShiftRows(std::array<std::array<u_char, 4>, 4>& state) {
  for (uint32_t i = 1; i < 4; ++i) {
    ShiftRows(state, i, nb_ - i);
  }
}

void AES::InvMixColumns(std::array<std::array<u_char, 4>, 4>& state) {
  for (uint32_t i = 0; i < 4; i++) {
    u_char c1 = Multiple(0xE, state[0][i]) ^ Multiple(0xB, state[1][i]) ^
                Multiple(0xD, state[2][i]) ^ Multiple(0x9, state[3][i]);
    u_char c2 = Multiple(0x9, state[0][i]) ^ Multiple(0xE, state[1][i]) ^
                Multiple(0xB, state[2][i]) ^ Multiple(0xD, state[3][i]);
    u_char c3 = Multiple(0xD, state[0][i]) ^ Multiple(0x9, state[1][i]) ^
                Multiple(0xE, state[2][i]) ^ Multiple(0xB, state[3][i]);
    u_char c4 = Multiple(0xB, state[0][i]) ^ Multiple(0xD, state[1][i]) ^
                Multiple(0x9, state[2][i]) ^ Multiple(0xE, state[3][i]);
    state[0][i] = c1;
    state[1][i] = c2;
    state[2][i] = c3;
    state[3][i] = c4;
  }
}

void AES::RotWord(std::array<u_char, 4>& w) {
  u_char temp = w[0];
  w[0] = w[1];
  w[1] = w[2];
  w[2] = w[3];
  w[3] = temp;
}

void AES::SubWord(std::array<u_char, 4>& w) {
  for (uint32_t i = 0; i < 4; ++i) {
    w[i] = sbox_[w[i] / 16][w[i] % 16];
  }
}

std::array<AES::u_char, 4> AES::XorWords(const std::array<u_char, 4>& a,
                                         const std::array<u_char, 4>& b) {
  std::array<u_char, 4> res;
  for (uint32_t i = 0; i < 4; ++i) {
    res[i] = a[i] ^ b[i];
  }
  return res;
}

AES::u_char AES::Xtime(u_char b)  // multiply on x
{
  return (b << 1) ^ (((b >> 7) & 1) * 0x1b);
}

void AES::Rcon(std::array<u_char, 4>& rcon, uint32_t n) {
  u_char c = 1;
  for (uint32_t i = 0; i < n - 1; i++) {
    c = Xtime(c);
  }
  rcon[0] = c;
  rcon[1] = rcon[2] = rcon[3] = 0;
}

void AES::KeyExpansion(const std::array<u_char, 16>& key, std::string& w) {
  std::array<u_char, 4> temp;
  std::array<u_char, 4> rcon;
  for (uint32_t i = 0; i < 4 * nk_; ++i) {
    w[i] = key[i];
  }
  for (uint32_t i = 4 * nk_; i < 4 * nb_ * (nr_ + 1); i += 4) {
    temp[0] = w[i - 4 + 0];
    temp[1] = w[i - 4 + 1];
    temp[2] = w[i - 4 + 2];
    temp[3] = w[i - 4 + 3];
    if (i / 4 % nk_ == 0) {
      RotWord(temp);
      SubWord(temp);
      Rcon(rcon, i / (nk_ * 4));
      temp = XorWords(temp, rcon);
    } else if (nk_ > 6 && i / 4 % nk_ == 4) {
      SubWord(temp);
    }
    w[i + 0] = w[i + 0 - 4 * nk_] ^ temp[0];
    w[i + 1] = w[i + 1 - 4 * nk_] ^ temp[1];
    w[i + 2] = w[i + 2 - 4 * nk_] ^ temp[2];
    w[i + 3] = w[i + 3 - 4 * nk_] ^ temp[3];
  }
}

AES::u_char AES::Multiple(u_char num, u_char data) {
  u_char flag = 1;
  u_char res = 0;
  if (num == 1) {
    return data;
  } else if (num == 2) {
    if (data & 0x80) {
      return (data << 1) ^ 0x1B;
    } else {
      return data << 1;
    }
  } else {
    if ((num & (~num + 1)) == num) {
      return Multiple(num / 2, Multiple(2, data));
    }
  }
  while (num != 0) {
    if (num & 1) {
      res ^= Multiple(flag, data);
    }
    flag = flag << 1;
    num = num >> 1;
  }
  return res;
}
