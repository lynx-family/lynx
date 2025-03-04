# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

_BASE85_CHARACTERS = ('0123456789'
                      'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
                      'abcdefghijklmnopqrstuvwxyz'
                      '!#$%&()*+-;<=>?@^_`{|}~')

# pylint: disable=invalid-name
_char_to_value = {}


def decode_base85(encoded_str):
    """Decodes a base85 string.

    The input string length must be a multiple of 5, and the resultant
    binary length is always a multiple of 4.
    """
    if len(encoded_str) % 5 != 0:
        raise ValueError('Input string length is not a multiple of 5; ' +
                         str(len(encoded_str)))
    if not _char_to_value:
        for i, ch in enumerate(_BASE85_CHARACTERS):
            _char_to_value[ch] = i

    result = ''
    i = 0
    while i < len(encoded_str):
        acc = 0
        for _ in range(5):
            ch = encoded_str[i]
            if ch not in _char_to_value:
                raise ValueError('Invalid base85 character; "{}"'.format(ch))
            new_acc = acc * 85 + _char_to_value[ch]
            assert new_acc >= acc
            acc = new_acc
            i += 1
        for _ in range(4):
            result += chr(acc >> 24)
            acc = (acc & 0x00ffffff) << 8
            assert acc >= 0
    return result
