/*
 * Regular Expression Engine
 *
 * Copyright (c) 2017-2018 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "third_party/legacy_regexp/legacy_regexp_compiler.h"

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
#include "quickjs/include/cutils.h"
#include "quickjs/include/libregexp.h"
}

namespace lynx {
namespace legacy_regexp {
static BOOL is_digit(int c) { return c >= '0' && c <= '9'; }
// The encoder preserves the established regexp encoding for existing runtimes.
/*
  TODO:

  - Add full unicode canonicalize rules for character ranges (not
    really useful but needed for exact "ignorecase" compatibility).

  - Add a lock step execution mode (=linear time execution guaranteed)
    when the regular expression is "simple" i.e. no backreference nor
    complicated lookahead. The opcodes are designed for this execution
    model.
*/

typedef enum {
#define DEF(id, size) REOP_##id,
#include "third_party/legacy_regexp/legacy_regexp_opcode.def"
#undef DEF
  REOP_COUNT,
} REOPCodeEnum;

#define CAPTURE_COUNT_MAX 255
#define STACK_SIZE_MAX 255

/* unicode code points */
#define CP_LS 0x2028
#define CP_PS 0x2029

#define TMP_BUF_SIZE 128

typedef struct {
  DynBuf byte_code;
  const uint8_t *buf_ptr;
  const uint8_t *buf_end;
  const uint8_t *buf_start;
  int re_flags;
  BOOL is_utf16;
  BOOL ignore_case;
  BOOL dotall;
  int capture_count;
  int total_capture_count; /* -1 = not computed yet */
  int has_named_captures;  /* -1 = don't know, 0 = no, 1 = yes */
  void *mem_opaque;
  DynBuf group_names;
  union {
    char error_msg[TMP_BUF_SIZE];
    char tmp_buf[TMP_BUF_SIZE];
  } u;
} REParseState;

typedef struct {
  uint8_t size;
} REOpCode;

static const REOpCode reopcode_info[REOP_COUNT] = {
#define DEF(id, size) {size},
#include "third_party/legacy_regexp/legacy_regexp_opcode.def"
#undef DEF
};

#define RE_HEADER_FLAGS 0
#define RE_HEADER_CAPTURE_COUNT 1
#define RE_HEADER_STACK_SIZE 2

#define RE_HEADER_LEN 7

/* insert 'len' bytes at position 'pos' */
static void dbuf_insert(DynBuf *s, int pos, int len) {
  dbuf_realloc(s, s->size + len);
  memmove(s->buf + pos + len, s->buf + pos, s->size - pos);
  s->size += len;
}

/* canonicalize with the specific LEPUS regexp rules */
static uint32_t lre_canonicalize(uint32_t c, BOOL is_utf16) {
  uint32_t res[LRE_CC_RES_LEN_MAX];
  int len;
  if (is_utf16) {
    if (likely(c < 128)) {
      if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    } else {
      lre_case_conv(res, c, 2);
      c = res[0];
    }
  } else {
    if (likely(c < 128)) {
      if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    } else {
      /* legacy regexp: to upper case if single char >= 128 */
      len = lre_case_conv(res, c, FALSE);
      if (len == 1 && res[0] >= 128) c = res[0];
    }
  }
  return c;
}

static const uint16_t char_range_d[] = {
    1,
    0x0030,
    0x0039 + 1,
};

/* code point ranges for Zs,Zl or Zp property */
static const uint16_t char_range_s[] = {
    10,
    0x0009,
    0x000D + 1,
    0x0020,
    0x0020 + 1,
    0x00A0,
    0x00A0 + 1,
    0x1680,
    0x1680 + 1,
    0x2000,
    0x200A + 1,
    /* 2028;LINE SEPARATOR;Zl;0;WS;;;;;N;;;;; */
    /* 2029;PARAGRAPH SEPARATOR;Zp;0;B;;;;;N;;;;; */
    0x2028,
    0x2029 + 1,
    0x202F,
    0x202F + 1,
    0x205F,
    0x205F + 1,
    0x3000,
    0x3000 + 1,
    /* FEFF;ZERO WIDTH NO-BREAK SPACE;Cf;0;BN;;;;;N;BYTE ORDER MARK;;;; */
    0xFEFF,
    0xFEFF + 1,
};

static void *lre_realloc_addjudge(__attribute__((unused)) void *opaque,
                                  void *ptr, size_t size,
                                  __attribute__((unused)) int alloc_tag = 0) {
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  return realloc(ptr, size);
}

BOOL lre_is_space(int c) {
  int i, n, low, high;
  n = (countof(char_range_s) - 1) / 2;
  for (i = 0; i < n; i++) {
    low = char_range_s[2 * i + 1];
    if (c < low) return FALSE;
    high = char_range_s[2 * i + 2];
    if (c < high) return TRUE;
  }
  return FALSE;
}

[[maybe_unused]] uint32_t const lre_id_start_table_ascii[4] = {
    /* $ A-Z _ a-z */
    0x00000000, 0x00000010, 0x87FFFFFE, 0x07FFFFFE};

[[maybe_unused]] uint32_t const lre_id_continue_table_ascii[4] = {
    /* $ 0-9 A-Z _ a-z */
    0x00000000, 0x03FF0010, 0x87FFFFFE, 0x07FFFFFE};

static const uint16_t char_range_w[] = {
    4,      0x0030,     0x0039 + 1, 0x0041,     0x005A + 1,
    0x005F, 0x005F + 1, 0x0061,     0x007A + 1,
};

#define CLASS_RANGE_BASE 0x40000000

typedef enum {
  CHAR_RANGE_d,
  CHAR_RANGE_D,
  CHAR_RANGE_s,
  CHAR_RANGE_S,
  CHAR_RANGE_w,
  CHAR_RANGE_W,
} CharRangeEnum;

static const uint16_t *char_range_table[] = {
    char_range_d,
    char_range_s,
    char_range_w,
};

static int cr_init_char_range(REParseState *s, CharRange *cr, uint32_t c) {
  BOOL invert;
  const uint16_t *c_pt;
  int len, i;

  invert = c & 1;
  c_pt = char_range_table[c >> 1];
  len = *c_pt++;
  cr_init(cr, s->mem_opaque, lre_realloc_addjudge);
  for (i = 0; i < len * 2; i++) {
    if (cr_add_point(cr, c_pt[i])) goto fail;
  }
  if (invert) {
    if (cr_invert(cr)) goto fail;
  }
  return 0;
fail:
  cr_free(cr);
  return -1;
}

static int cr_canonicalize(CharRange *cr) {
  CharRange a;
  uint32_t pt[2];
  int i, ret;
  cr_init(&a, cr->mem_opaque, lre_realloc_addjudge);
  pt[0] = 'a';
  pt[1] = 'z' + 1;
  ret = cr_op(&a, cr->points, cr->len, pt, 2, CR_OP_INTER);
  if (ret) goto fail;
  /* convert to upper case */
  /* XXX: the generic unicode case would be much more complicated
     and not really useful */
  for (i = 0; i < a.len; i++) {
    a.points[i] += 'A' - 'a';
  }
  /* Note: for simplicity we keep the lower case ranges */
  ret = cr_union1(cr, a.points, a.len);
fail:
  cr_free(&a);
  return ret;
}

static void re_emit_op(REParseState *s, int op) {
  dbuf_putc(&s->byte_code, op);
}

/* return the offset of the u32 value */
static int re_emit_op_u32(REParseState *s, int op, uint32_t val) {
  int pos;
  dbuf_putc(&s->byte_code, op);
  pos = s->byte_code.size;
  dbuf_put_u32(&s->byte_code, val);
  return pos;
}

static int re_emit_goto(REParseState *s, int op, uint32_t val) {
  int pos;
  dbuf_putc(&s->byte_code, op);
  pos = s->byte_code.size;
  dbuf_put_u32(&s->byte_code, val - (pos + 4));
  return pos;
}

static void re_emit_op_u8(REParseState *s, int op, uint32_t val) {
  dbuf_putc(&s->byte_code, op);
  dbuf_putc(&s->byte_code, val);
}

static void re_emit_op_u16(REParseState *s, int op, uint32_t val) {
  dbuf_putc(&s->byte_code, op);
  dbuf_put_u16(&s->byte_code, val);
}

static int __attribute__((format(printf, 2, 3)))
re_parse_error(REParseState *s, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(s->u.error_msg, sizeof(s->u.error_msg), fmt, ap);
  va_end(ap);
  return -1;
}

/* Return -1 in case of overflow */
static int parse_digits(const uint8_t **pp) {
  const uint8_t *p;
  uint64_t v;
  int c;

  p = *pp;
  v = 0;
  for (;;) {
    c = *p;
    if (c < '0' || c > '9') break;
    v = v * 10 + c - '0';
    if (v >= INT32_MAX) return -1;
    p++;
  }
  *pp = p;
  return v;
}

static int re_parse_expect(REParseState *s, const uint8_t **pp, int c) {
  const uint8_t *p;
  p = *pp;
  if (*p != c) return re_parse_error(s, "expecting '%c'", c);
  p++;
  *pp = p;
  return 0;
}

/* Parse an escape sequence, *pp points after the '\':
   allow_utf16 value:
   0 : no UTF-16 escapes allowed
   1 : UTF-16 escapes allowed
   2 : UTF-16 escapes allowed and escapes of surrogate pairs are
   converted to a unicode character (unicode regexp case).

   Return the unicode char and update *pp if recognized,
   return -1 if malformed escape,
   return -2 otherwise. */
int lre_parse_escape(const uint8_t **pp, int allow_utf16) {
  const uint8_t *p;
  uint32_t c;

  p = *pp;
  c = *p++;
  switch (c) {
    case 'b':
      c = '\b';
      break;
    case 'f':
      c = '\f';
      break;
    case 'n':
      c = '\n';
      break;
    case 'r':
      c = '\r';
      break;
    case 't':
      c = '\t';
      break;
    case 'v':
      c = '\v';
      break;
    case 'x':
    case 'u': {
      int h, n, i;
      uint32_t c1;

      if (*p == '{' && allow_utf16) {
        p++;
        c = 0;
        for (;;) {
          h = from_hex(*p++);
          if (h < 0) return -1;
          c = (c << 4) | h;
          if (c > 0x10FFFF) return -1;
          if (*p == '}') break;
        }
        p++;
      } else {
        if (c == 'x') {
          n = 2;
        } else {
          n = 4;
        }

        c = 0;
        for (i = 0; i < n; i++) {
          h = from_hex(*p++);
          if (h < 0) {
            return -1;
          }
          c = (c << 4) | h;
        }
        if (c >= 0xd800 && c < 0xdc00 && allow_utf16 == 2 && p[0] == '\\' &&
            p[1] == 'u') {
          /* convert an escaped surrogate pair into a
             unicode char */
          c1 = 0;
          for (i = 0; i < 4; i++) {
            h = from_hex(p[2 + i]);
            if (h < 0) break;
            c1 = (c1 << 4) | h;
          }
          if (i == 4 && c1 >= 0xdc00 && c1 < 0xe000) {
            p += 6;
            c = (((c & 0x3ff) << 10) | (c1 & 0x3ff)) + 0x10000;
          }
        }
      }
    } break;
    case '0' ... '7':
      c -= '0';
      if (allow_utf16 == 2) {
        /* only accept \0 not followed by digit */
        if (c != 0 || is_digit(*p)) return -1;
      } else {
        /* parse a legacy octal sequence */
        uint32_t v;
        v = *p - '0';
        if (v > 7) break;
        c = (c << 3) | v;
        p++;
        if (c >= 32) break;
        v = *p - '0';
        if (v > 7) break;
        c = (c << 3) | v;
        p++;
      }
      break;
    default:
      return -2;
  }
  *pp = p;
  return c;
}

/* return -1 if error otherwise the character or a class range
   (CLASS_RANGE_BASE). In case of class range, 'cr' is
   initialized. Otherwise, it is ignored. */
static int get_class_atom(REParseState *s, CharRange *cr, const uint8_t **pp,
                          BOOL inclass) {
  const uint8_t *p;
  uint32_t c;
  int ret;

  p = *pp;

  c = *p;
  switch (c) {
    case '\\':
      p++;
      if (p >= s->buf_end) goto unexpected_end;
      c = *p++;
      switch (c) {
        case 'd':
          c = CHAR_RANGE_d;
          goto class_range;
        case 'D':
          c = CHAR_RANGE_D;
          goto class_range;
        case 's':
          c = CHAR_RANGE_s;
          goto class_range;
        case 'S':
          c = CHAR_RANGE_S;
          goto class_range;
        case 'w':
          c = CHAR_RANGE_w;
          goto class_range;
        case 'W':
          c = CHAR_RANGE_W;
        class_range:
          if (cr_init_char_range(s, cr, c)) return -1;
          c = CLASS_RANGE_BASE;
          break;
        case 'c':
          c = *p;
          if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (((c >= '0' && c <= '9') || c == '_') && inclass &&
               !s->is_utf16)) { /* Annex B.1.4 */
            c &= 0x1f;
            p++;
          } else if (s->is_utf16) {
            goto invalid_escape;
          } else {
            /* otherwise return '\' and 'c' */
            p--;
            c = '\\';
          }
          break;
        default:
          p--;
          ret = lre_parse_escape(&p, s->is_utf16 * 2);
          if (ret >= 0) {
            c = ret;
          } else {
            if (ret == -2 && *p != '\0' && strchr("^$\\.*+?()[]{}|/", *p)) {
              /* always valid to escape these characters */
              goto normal_char;
            } else if (s->is_utf16) {
            invalid_escape:
              return re_parse_error(
                  s, "invalid escape sequence in regular expression");
            } else {
              /* just ignore the '\' */
              goto normal_char;
            }
          }
          break;
      }
      break;
    case '\0':
      if (p >= s->buf_end) {
      unexpected_end:
        return re_parse_error(s, "unexpected end");
      }
      /* fall thru */
    default:
    normal_char:
      /* normal char */
      if (c >= 128) {
        c = unicode_from_utf8(p, UTF8_CHAR_LEN_MAX, &p);
        if ((unsigned)c > 0xffff && !s->is_utf16) {
          /* XXX: should handle non BMP-1 code points */
          return re_parse_error(s, "malformed unicode char");
        }
      } else {
        p++;
      }
      break;
  }
  *pp = p;
  return c;
}

static int re_emit_range(REParseState *s, const CharRange *cr) {
  int len, i;
  uint32_t high;

  len = (unsigned)cr->len / 2;
  if (len >= 65535) return re_parse_error(s, "too many ranges");
  if (len == 0) {
    /* not sure it can really happen. Emit a match that is always
       false */
    re_emit_op_u32(s, REOP_char32, -1);
  } else {
    high = cr->points[cr->len - 1];
    if (high == UINT32_MAX) high = cr->points[cr->len - 2];
    if (high <= 0xffff) {
      /* can use 16 bit ranges with the conversion that 0xffff =
         infinity */
      re_emit_op_u16(s, REOP_range, len);
      for (i = 0; i < cr->len; i += 2) {
        dbuf_put_u16(&s->byte_code, cr->points[i]);
        high = cr->points[i + 1] - 1;
        if (high == UINT32_MAX - 1) high = 0xffff;
        dbuf_put_u16(&s->byte_code, high);
      }
    } else {
      re_emit_op_u16(s, REOP_range32, len);
      for (i = 0; i < cr->len; i += 2) {
        dbuf_put_u32(&s->byte_code, cr->points[i]);
        dbuf_put_u32(&s->byte_code, cr->points[i + 1] - 1);
      }
    }
  }
  return 0;
}

static int re_parse_char_class(REParseState *s, const uint8_t **pp) {
  const uint8_t *p;
  uint32_t c1, c2;
  CharRange cr_s, *cr = &cr_s;
  CharRange cr1_s, *cr1 = &cr1_s;
  BOOL invert;

  cr_init(cr, s->mem_opaque, lre_realloc_addjudge);
  p = *pp;
  p++; /* skip '[' */
  invert = FALSE;
  if (*p == '^') {
    p++;
    invert = TRUE;
  }
  for (;;) {
    if (*p == ']') break;
    c1 = get_class_atom(s, cr1, &p, TRUE);
    if ((int)c1 < 0) goto fail;
    if (*p == '-' && p[1] != ']') {
      const uint8_t *p0 = p + 1;
      if (c1 >= CLASS_RANGE_BASE) {
        if (s->is_utf16) {
          cr_free(cr1);
          goto invalid_class_range;
        }
        /* Annex B: match '-' character */
        goto class_atom;
      }
      c2 = get_class_atom(s, cr1, &p0, TRUE);
      if ((int)c2 < 0) goto fail;
      if (c2 >= CLASS_RANGE_BASE) {
        cr_free(cr1);
        if (s->is_utf16) {
          goto invalid_class_range;
        }
        /* Annex B: match '-' character */
        goto class_atom;
      }
      p = p0;
      if (c2 < c1) {
      invalid_class_range:
        re_parse_error(s, "invalid class range");
        goto fail;
      }
      if (cr_union_interval(cr, c1, c2)) goto memory_error;
    } else {
    class_atom:
      if (c1 >= CLASS_RANGE_BASE) {
        int ret;
        ret = cr_union1(cr, cr1->points, cr1->len);
        cr_free(cr1);
        if (ret) goto memory_error;
      } else {
        if (cr_union_interval(cr, c1, c1)) goto memory_error;
      }
    }
  }
  if (s->ignore_case) {
    if (cr_canonicalize(cr)) goto memory_error;
  }
  if (invert) {
    if (cr_invert(cr)) goto memory_error;
  }
  if (re_emit_range(s, cr)) goto fail;
  cr_free(cr);
  p++; /* skip ']' */
  *pp = p;
  return 0;
memory_error:
  re_parse_error(s, "out of memory");
fail:
  cr_free(cr);
  return -1;
}

/* Return:
   1 if the opcodes in bc_buf[] always advance the character pointer.
   0 if the character pointer may not be advanced.
   -1 if the code may depend on side effects of its previous execution
   (backreference)
*/
static int re_check_advance(const uint8_t *bc_buf, int bc_buf_len) {
  int pos, opcode, ret, len, i;
  uint32_t val, last;
  BOOL has_back_reference;
  uint8_t capture_bitmap[CAPTURE_COUNT_MAX];

  ret = -2; /* not known yet */
  pos = 0;
  has_back_reference = FALSE;
  memset(capture_bitmap, 0, sizeof(capture_bitmap));

  while (pos < bc_buf_len) {
    opcode = bc_buf[pos];
    len = reopcode_info[opcode].size;
    switch (opcode) {
      case REOP_range:
        val = get_u16(bc_buf + pos + 1);
        len += val * 4;
        goto simple_char;
      case REOP_range32:
        val = get_u16(bc_buf + pos + 1);
        len += val * 8;
        goto simple_char;
      case REOP_char:
      case REOP_char32:
      case REOP_dot:
      case REOP_any:
      simple_char:
        if (ret == -2) ret = 1;
        break;
      case REOP_line_start:
      case REOP_line_end:
      case REOP_push_i32:
      case REOP_push_char_pos:
      case REOP_drop:
      case REOP_word_boundary:
      case REOP_not_word_boundary:
      case REOP_prev:
        /* no effect */
        break;
      case REOP_save_start:
      case REOP_save_end:
        val = bc_buf[pos + 1];
        capture_bitmap[val] |= 1;
        break;
      case REOP_save_reset: {
        val = bc_buf[pos + 1];
        last = bc_buf[pos + 2];
        while (val < last) capture_bitmap[val++] |= 1;
      } break;
      case REOP_back_reference:
      case REOP_backward_back_reference:
        val = bc_buf[pos + 1];
        capture_bitmap[val] |= 2;
        has_back_reference = TRUE;
        break;
      default:
        /* safe behvior: we cannot predict the outcome */
        if (ret == -2) ret = 0;
        break;
    }
    pos += len;
  }
  if (has_back_reference) {
    /* check if there is back reference which references a capture
       made in the some code */
    for (i = 0; i < CAPTURE_COUNT_MAX; i++) {
      if (capture_bitmap[i] == 3) return -1;
    }
  }
  if (ret == -2) ret = 0;
  return ret;
}

/* return -1 if a simple quantifier cannot be used. Otherwise return
   the number of characters in the atom. */
static int re_is_simple_quantifier(const uint8_t *bc_buf, int bc_buf_len) {
  int pos, opcode, len, count;
  uint32_t val;

  count = 0;
  pos = 0;
  while (pos < bc_buf_len) {
    opcode = bc_buf[pos];
    len = reopcode_info[opcode].size;
    switch (opcode) {
      case REOP_range:
        val = get_u16(bc_buf + pos + 1);
        len += val * 4;
        goto simple_char;
      case REOP_range32:
        val = get_u16(bc_buf + pos + 1);
        len += val * 8;
        goto simple_char;
      case REOP_char:
      case REOP_char32:
      case REOP_dot:
      case REOP_any:
      simple_char:
        count++;
        break;
      case REOP_line_start:
      case REOP_line_end:
      case REOP_word_boundary:
      case REOP_not_word_boundary:
        break;
      default:
        return -1;
    }
    pos += len;
  }
  return count;
}

/* '*pp' is the first char after '<' */
static int re_parse_group_name(char *buf, int buf_size, const uint8_t **pp,
                               BOOL is_utf16) {
  const uint8_t *p;
  uint32_t c;
  char *q;

  p = *pp;
  q = buf;
  for (;;) {
    c = *p;
    if (c == '\\') {
      p++;
      if (*p != 'u') return -1;
      c = lre_parse_escape(&p, is_utf16 * 2);
    } else if (c == '>') {
      break;
    } else if (c >= 128) {
      c = unicode_from_utf8(p, UTF8_CHAR_LEN_MAX, &p);
    } else {
      p++;
    }
    if (c > 0x10FFFF) return -1;
    if (q == buf) {
      if (!lre_js_is_ident_first(c)) return -1;
    } else {
      if (!lre_js_is_ident_next(c)) return -1;
    }
    if ((q - buf + UTF8_CHAR_LEN_MAX + 1) > buf_size) return -1;
    if (c < 128) {
      *q++ = c;
    } else {
      q += unicode_to_utf8((uint8_t *)q, c);
    }
  }
  if (q == buf) return -1;
  *q = '\0';
  p++;
  *pp = p;
  return 0;
}

/* if capture_name = NULL: return the number of captures + 1.
   Otherwise, return the capture index corresponding to capture_name
   or -1 if none */
static int re_parse_captures(REParseState *s, int *phas_named_captures,
                             const char *capture_name) {
  const uint8_t *p;
  int capture_index;
  char name[TMP_BUF_SIZE];

  capture_index = 1;
  *phas_named_captures = 0;
  for (p = s->buf_start; p < s->buf_end; p++) {
    switch (*p) {
      case '(':
        if (p[1] == '?') {
          if (p[2] == '<' && p[3] != '=' && p[3] != '!') {
            *phas_named_captures = 1;
            /* potential named capture */
            if (capture_name) {
              p += 3;
              if (re_parse_group_name(name, sizeof(name), &p, s->is_utf16) ==
                  0) {
                if (!strcmp(name, capture_name)) return capture_index;
              }
            }
            capture_index++;
          }
        } else {
          capture_index++;
        }
        break;
      case '\\':
        p++;
        break;
      case '[':
        for (p += 1 + (*p == ']'); p < s->buf_end && *p != ']'; p++) {
          if (*p == '\\') p++;
        }
        break;
    }
  }
  if (capture_name)
    return -1;
  else
    return capture_index;
}

static int re_count_captures(REParseState *s) {
  if (s->total_capture_count < 0) {
    s->total_capture_count = re_parse_captures(s, &s->has_named_captures, NULL);
  }
  return s->total_capture_count;
}

static BOOL re_has_named_captures(REParseState *s) {
  if (s->has_named_captures < 0) re_count_captures(s);
  return s->has_named_captures;
}

static int find_group_name(REParseState *s, const char *name) {
  const char *p, *buf_end;
  size_t len, name_len;
  int capture_index;

  name_len = strlen(name);
  p = (char *)s->group_names.buf;
  buf_end = (char *)s->group_names.buf + s->group_names.size;
  capture_index = 1;
  while (p < buf_end) {
    len = strlen(p);
    if (len == name_len && memcmp(name, p, name_len) == 0) return capture_index;
    p += len + 1;
    capture_index++;
  }
  return -1;
}

static int re_parse_disjunction(REParseState *s, BOOL is_backward_dir);
static int parse_digits(const uint8_t **pp, BOOL allow_overflow) {
  const uint8_t *p;
  uint64_t v;
  uint8_t c;

  p = *pp;
  v = 0;
  for (;;) {
    c = *p;
    if (c < '0' || c > '9') break;
    v = v * 10 + c - '0';
    if (v >= INT32_MAX) {
      if (allow_overflow)
        v = INT32_MAX;
      else
        return -1;
    }
    p++;
  }
  *pp = p;
  return v;
}
static int re_parse_term(REParseState *s, BOOL is_backward_dir) {
  const uint8_t *p;
  int c, last_atom_start, quant_min, quant_max, last_capture_count;
  BOOL greedy, add_zero_advance_check, is_neg, is_backward_lookahead;
  CharRange cr_s, *cr = &cr_s;

  last_atom_start = -1;
  last_capture_count = 0;
  p = s->buf_ptr;
  c = *p;
  const uint8_t *p1 = nullptr;
  switch (c) {
    case '^':
      p++;
      re_emit_op(s, REOP_line_start);
      break;
    case '$':
      p++;
      re_emit_op(s, REOP_line_end);
      break;
    case '.':
      p++;
      last_atom_start = s->byte_code.size;
      last_capture_count = s->capture_count;
      if (is_backward_dir) re_emit_op(s, REOP_prev);
      re_emit_op(s, s->dotall ? REOP_any : REOP_dot);
      if (is_backward_dir) re_emit_op(s, REOP_prev);
      break;
    case '{':
      if (s->is_utf16) {
        return re_parse_error(s, "syntax error");
      } else if (!is_digit(p[1])) {
        /* Annex B: we accept '{' not followed by digits as a
           normal atom */
        goto parse_class_atom;
      } else {
        const uint8_t *p1 = p + 1;
        /* Annex B: error if it is like a repetition count */
        parse_digits(&p1, TRUE);
        if (*p1 == ',') {
          p1++;
          if (is_digit(*p1)) {
            parse_digits(&p1, TRUE);
          }
        }
        if (*p1 != '}') {
          goto parse_class_atom;
        }
      }
      /* fall tru */
    case '*':
    case '+':
    case '?':
      return re_parse_error(s, "nothing to repeat");
    case '(':
      if (p[1] == '?') {
        if (p[2] == ':') {
          p += 3;
          last_atom_start = s->byte_code.size;
          last_capture_count = s->capture_count;
          s->buf_ptr = p;
          if (re_parse_disjunction(s, is_backward_dir)) return -1;
          p = s->buf_ptr;
          if (re_parse_expect(s, &p, ')')) return -1;
        } else if ((p[2] == '=' || p[2] == '!')) {
          is_neg = (p[2] == '!');
          is_backward_lookahead = FALSE;
          p += 3;
          goto lookahead;
        } else if (p[2] == '<' && (p[3] == '=' || p[3] == '!')) {
          int pos;
          is_neg = (p[3] == '!');
          is_backward_lookahead = TRUE;
          p += 4;
          /* lookahead */
        lookahead:
          /* Annex B allows lookahead to be used as an atom for
             the quantifiers */
          if (!s->is_utf16 && !is_backward_lookahead) {
            last_atom_start = s->byte_code.size;
            last_capture_count = s->capture_count;
          }
          pos = re_emit_op_u32(s, REOP_lookahead + is_neg, 0);
          s->buf_ptr = p;
          if (re_parse_disjunction(s, is_backward_lookahead)) return -1;
          p = s->buf_ptr;
          if (re_parse_expect(s, &p, ')')) return -1;
          re_emit_op(s, REOP_match);
          /* jump after the 'match' after the lookahead is successful */
          put_u32(s->byte_code.buf + pos, s->byte_code.size - (pos + 4));
        } else if (p[2] == '<') {
          p += 3;
          if (re_parse_group_name(s->u.tmp_buf, sizeof(s->u.tmp_buf), &p,
                                  s->is_utf16)) {
            return re_parse_error(s, "invalid group name");
          }
          if (find_group_name(s, s->u.tmp_buf) > 0) {
            return re_parse_error(s, "duplicate group name");
          }
          /* group name with a trailing zero */
          dbuf_put(&s->group_names, (uint8_t *)s->u.tmp_buf,
                   strlen(s->u.tmp_buf) + 1);
          s->has_named_captures = 1;
          goto parse_capture;
        } else {
          return re_parse_error(s, "invalid group");
        }
      } else {
        int capture_index;
        p++;
        /* capture without group name */
        dbuf_putc(&s->group_names, 0);
      parse_capture:
        if (s->capture_count >= CAPTURE_COUNT_MAX)
          return re_parse_error(s, "too many captures");
        last_atom_start = s->byte_code.size;
        last_capture_count = s->capture_count;
        capture_index = s->capture_count++;
        re_emit_op_u8(s, REOP_save_start + is_backward_dir, capture_index);

        s->buf_ptr = p;
        if (re_parse_disjunction(s, is_backward_dir)) return -1;
        p = s->buf_ptr;

        re_emit_op_u8(s, REOP_save_start + 1 - is_backward_dir, capture_index);

        if (re_parse_expect(s, &p, ')')) return -1;
      }
      break;
    case '\\':
      switch (p[1]) {
        case 'b':
        case 'B':
          re_emit_op(s, REOP_word_boundary + (p[1] != 'b'));
          p += 2;
          break;
        case 'k': {
          const uint8_t *p1;
          int dummy_res;

          p1 = p;
          if (p1[2] != '<') {
            /* annex B: we tolerate invalid group names in non
               unicode mode if there is no named capture
               definition */
            if (s->is_utf16 || re_has_named_captures(s))
              return re_parse_error(s, "expecting group name");
            else
              goto parse_class_atom;
          }
          p1 += 3;
          if (re_parse_group_name(s->u.tmp_buf, sizeof(s->u.tmp_buf), &p1,
                                  s->is_utf16)) {
            if (s->is_utf16 || re_has_named_captures(s))
              return re_parse_error(s, "invalid group name");
            else
              goto parse_class_atom;
          }
          c = find_group_name(s, s->u.tmp_buf);
          if (c < 0) {
            /* no capture name parsed before, try to look
               after (inefficient, but hopefully not common */
            c = re_parse_captures(s, &dummy_res, s->u.tmp_buf);
            if (c < 0) {
              if (s->is_utf16 || re_has_named_captures(s))
                return re_parse_error(s, "group name not defined");
              else
                goto parse_class_atom;
            }
          }
          p = p1;
        }
          goto emit_back_reference;
        case '0':
          p += 2;
          c = 0;
          if (s->is_utf16) {
            if (is_digit(*p)) {
              return re_parse_error(
                  s, "invalid decimal escape in regular expression");
            }
          } else {
            /* Annex B.1.4: accept legacy octal */
            if (*p >= '0' && *p <= '7') {
              c = *p++ - '0';
              if (*p >= '0' && *p <= '7') {
                c = (c << 3) + *p++ - '0';
              }
            }
          }
          goto normal_char;
        case '1' ... '9': {
          const uint8_t *q;
          q = ++p;

          c = parse_digits(&p);
          if (c < 0 || (c >= s->capture_count && c >= re_count_captures(s))) {
            if (!s->is_utf16) {
              /* Annex B.1.4: accept legacy octal */
              p = q;
              if (*p <= '7') {
                c = 0;
                if (*p <= '3') c = *p++ - '0';
                if (*p >= '0' && *p <= '7') {
                  c = (c << 3) + *p++ - '0';
                  if (*p >= '0' && *p <= '7') {
                    c = (c << 3) + *p++ - '0';
                  }
                }
              } else {
                c = *p++;
              }
              goto normal_char;
            }
            return re_parse_error(
                s, "back reference out of range in reguar expression");
          }
        emit_back_reference:
          last_atom_start = s->byte_code.size;
          last_capture_count = s->capture_count;
          re_emit_op_u8(s, REOP_back_reference + is_backward_dir, c);
        } break;
        default:
          goto parse_class_atom;
      }
      break;
    case '[':
      last_atom_start = s->byte_code.size;
      last_capture_count = s->capture_count;
      if (is_backward_dir) re_emit_op(s, REOP_prev);
      if (re_parse_char_class(s, &p)) return -1;
      if (is_backward_dir) re_emit_op(s, REOP_prev);
      break;
    case ']':
    case '}':
      if (s->is_utf16) return re_parse_error(s, "syntax error");
      goto parse_class_atom;
    default:
    parse_class_atom:
      c = get_class_atom(s, cr, &p, FALSE);
      if ((int)c < 0) return -1;
    normal_char:
      last_atom_start = s->byte_code.size;
      last_capture_count = s->capture_count;
      if (is_backward_dir) re_emit_op(s, REOP_prev);
      if (c >= CLASS_RANGE_BASE) {
        int ret;
        /* Note: canonicalization is not needed */
        ret = re_emit_range(s, cr);
        cr_free(cr);
        if (ret) return -1;
      } else {
        if (s->ignore_case) c = lre_canonicalize(c, s->is_utf16);
        if (c <= 0xffff)
          re_emit_op_u16(s, REOP_char, c);
        else
          re_emit_op_u32(s, REOP_char32, c);
      }
      if (is_backward_dir) re_emit_op(s, REOP_prev);
      break;
  }

  /* quantifier */
  if (last_atom_start >= 0) {
    c = *p;
    switch (c) {
      case '*':
        p++;
        quant_min = 0;
        quant_max = INT32_MAX;
        goto quantifier;
      case '+':
        p++;
        quant_min = 1;
        quant_max = INT32_MAX;
        goto quantifier;
      case '?':
        p++;
        quant_min = 0;
        quant_max = 1;
        goto quantifier;
      case '{': {
        p1 = p;
        /* As an extension (see ES6 annex B), we accept '{' not
           followed by digits as a normal atom */
        if (!is_digit(p[1])) {
          if (s->is_utf16) goto invalid_quant_count;
          break;
        }
        p++;
        quant_min = parse_digits(&p, TRUE);
        quant_max = quant_min;
        if (*p == ',') {
          p++;
          if (is_digit(*p)) {
            quant_max = parse_digits(&p, TRUE);
            if (quant_max < quant_min) {
            invalid_quant_count: {
              return re_parse_error(s, "invalid repetition count");
            }
            }
          } else {
            quant_max = INT32_MAX; /* infinity */
          }
        }
        if (*p != '}' && !s->is_utf16) {
          /* Annex B: normal atom if invalid '{' syntax */
          p = p1;
          break;
        }
        if (re_parse_expect(s, &p, '}')) {
          return -1;
        }
      }
      quantifier:
        greedy = TRUE;
        if (*p == '?') {
          p++;
          greedy = FALSE;
        }
        if (last_atom_start < 0) {
          return re_parse_error(s, "nothing to repeat");
        }
        if (greedy) {
          int len, pos;

          if (quant_max > 0) {
            /* specific optimization for simple quantifiers */
            len = re_is_simple_quantifier(s->byte_code.buf + last_atom_start,
                                          s->byte_code.size - last_atom_start);
            if (len > 0) {
              re_emit_op(s, REOP_match);

              dbuf_insert(&s->byte_code, last_atom_start, 17);
              pos = last_atom_start;
              s->byte_code.buf[pos++] = REOP_simple_greedy_quant;
              put_u32(&s->byte_code.buf[pos],
                      s->byte_code.size - last_atom_start - 17);
              pos += 4;
              put_u32(&s->byte_code.buf[pos], quant_min);
              pos += 4;
              put_u32(&s->byte_code.buf[pos], quant_max);
              pos += 4;
              put_u32(&s->byte_code.buf[pos], len);
              pos += 4;
              goto done;
            }
          }

          add_zero_advance_check =
              (re_check_advance(s->byte_code.buf + last_atom_start,
                                s->byte_code.size - last_atom_start) == 0);
        } else {
          add_zero_advance_check = FALSE;
        }

        {
          int len, pos;
          len = s->byte_code.size - last_atom_start;
          if (quant_min == 0) {
            /* need to reset the capture in case the atom is
               not executed */
            if (last_capture_count != s->capture_count) {
              dbuf_insert(&s->byte_code, last_atom_start, 3);
              s->byte_code.buf[last_atom_start++] = REOP_save_reset;
              s->byte_code.buf[last_atom_start++] = last_capture_count;
              s->byte_code.buf[last_atom_start++] = s->capture_count - 1;
            }
            if (quant_max == 0) {
              s->byte_code.size = last_atom_start;
            } else if (quant_max == 1) {
              dbuf_insert(&s->byte_code, last_atom_start, 5);
              s->byte_code.buf[last_atom_start] =
                  REOP_split_goto_first + greedy;
              put_u32(s->byte_code.buf + last_atom_start + 1, len);
            } else if (quant_max == INT32_MAX) {
              dbuf_insert(&s->byte_code, last_atom_start,
                          5 + add_zero_advance_check);
              s->byte_code.buf[last_atom_start] =
                  REOP_split_goto_first + greedy;
              put_u32(s->byte_code.buf + last_atom_start + 1,
                      len + 5 + add_zero_advance_check);
              if (add_zero_advance_check) {
                /* avoid infinite loop by stoping the
                   recursion if no advance was made in the
                   atom (only works if the atom has no
                   side effect) */
                s->byte_code.buf[last_atom_start + 1 + 4] = REOP_push_char_pos;
                re_emit_goto(s, REOP_bne_char_pos, last_atom_start);
              } else {
                re_emit_goto(s, REOP_goto, last_atom_start);
              }
            } else {
              dbuf_insert(&s->byte_code, last_atom_start, 10);
              pos = last_atom_start;
              s->byte_code.buf[pos++] = REOP_push_i32;
              put_u32(s->byte_code.buf + pos, quant_max);
              pos += 4;
              s->byte_code.buf[pos++] = REOP_split_goto_first + greedy;
              put_u32(s->byte_code.buf + pos, len + 5);
              re_emit_goto(s, REOP_loop, last_atom_start + 5);
              re_emit_op(s, REOP_drop);
            }
          } else if (quant_min == 1 && quant_max == INT32_MAX &&
                     !add_zero_advance_check) {
            re_emit_goto(s, REOP_split_next_first - greedy, last_atom_start);
          } else {
            if (quant_min == 1) {
              /* nothing to add */
            } else {
              dbuf_insert(&s->byte_code, last_atom_start, 5);
              s->byte_code.buf[last_atom_start] = REOP_push_i32;
              put_u32(s->byte_code.buf + last_atom_start + 1, quant_min);
              last_atom_start += 5;
              re_emit_goto(s, REOP_loop, last_atom_start);
              re_emit_op(s, REOP_drop);
            }
            if (quant_max == INT32_MAX) {
              pos = s->byte_code.size;
              re_emit_op_u32(s, REOP_split_goto_first + greedy,
                             len + 5 + add_zero_advance_check);
              if (add_zero_advance_check) re_emit_op(s, REOP_push_char_pos);
              /* copy the atom */
              dbuf_put_self(&s->byte_code, last_atom_start, len);
              if (add_zero_advance_check)
                re_emit_goto(s, REOP_bne_char_pos, pos);
              else
                re_emit_goto(s, REOP_goto, pos);
            } else if (quant_max > quant_min) {
              re_emit_op_u32(s, REOP_push_i32, quant_max - quant_min);
              pos = s->byte_code.size;
              re_emit_op_u32(s, REOP_split_goto_first + greedy, len + 5);
              /* copy the atom */
              dbuf_put_self(&s->byte_code, last_atom_start, len);

              re_emit_goto(s, REOP_loop, pos);
              re_emit_op(s, REOP_drop);
            }
          }
          last_atom_start = -1;
        }
        break;
      default:
        break;
    }
  }
done:
  s->buf_ptr = p;
  return 0;
}

static int re_parse_alternative(REParseState *s, BOOL is_backward_dir) {
  const uint8_t *p;
  int ret;
  size_t start, term_start, end, term_size;

  start = s->byte_code.size;
  for (;;) {
    p = s->buf_ptr;
    if (p >= s->buf_end) break;
    if (*p == '|' || *p == ')') break;
    term_start = s->byte_code.size;
    ret = re_parse_term(s, is_backward_dir);
    if (ret) return ret;
    if (is_backward_dir) {
      /* reverse the order of the terms (XXX: inefficient, but
         speed is not really critical here) */
      end = s->byte_code.size;
      term_size = end - term_start;
      if (dbuf_realloc(&s->byte_code, end + term_size)) return -1;
      memmove(s->byte_code.buf + start + term_size, s->byte_code.buf + start,
              end - start);
      memcpy(s->byte_code.buf + start, s->byte_code.buf + end, term_size);
    }
  }
  return 0;
}

static int re_parse_disjunction(REParseState *s, BOOL is_backward_dir) {
  int start, len, pos;

  start = s->byte_code.size;
  if (re_parse_alternative(s, is_backward_dir)) return -1;
  while (*s->buf_ptr == '|') {
    s->buf_ptr++;

    len = s->byte_code.size - start;

    /* insert a split before the first alternative */
    dbuf_insert(&s->byte_code, start, 5);
    s->byte_code.buf[start] = REOP_split_next_first;
    put_u32(s->byte_code.buf + start + 1, len + 5);

    pos = re_emit_op_u32(s, REOP_goto, 0);

    if (re_parse_alternative(s, is_backward_dir)) return -1;

    /* patch the goto */
    len = s->byte_code.size - (pos + 4);
    put_u32(s->byte_code.buf + pos, len);
  }
  return 0;
}

/* the control flow is recursive so the analysis can be linear */
static int compute_stack_size(const uint8_t *bc_buf, int bc_buf_len) {
  int stack_size, stack_size_max, pos, opcode, len;
  uint32_t val;

  stack_size = 0;
  stack_size_max = 0;
  bc_buf += RE_HEADER_LEN;
  bc_buf_len -= RE_HEADER_LEN;
  pos = 0;
  while (pos < bc_buf_len) {
    opcode = bc_buf[pos];
    len = reopcode_info[opcode].size;
    assert(opcode < REOP_COUNT);
    assert((pos + len) <= bc_buf_len);
    switch (opcode) {
      case REOP_push_i32:
      case REOP_push_char_pos:
        stack_size++;
        if (stack_size > stack_size_max) {
          if (stack_size > STACK_SIZE_MAX) return -1;
          stack_size_max = stack_size;
        }
        break;
      case REOP_drop:
      case REOP_bne_char_pos:
        assert(stack_size > 0);
        stack_size--;
        break;
      case REOP_range:
        val = get_u16(bc_buf + pos + 1);
        len += val * 4;
        break;
      case REOP_range32:
        val = get_u16(bc_buf + pos + 1);
        len += val * 8;
        break;
    }
    pos += len;
  }
  return stack_size_max;
}

/* 'buf' must be a zero terminated UTF-8 string of length buf_len.
   Return NULL if error and allocate an error message in *perror_msg,
   otherwise the compiled bytecode and its length in plen.
*/
uint8_t *Compile(int *plen, char *error_msg, int error_msg_size,
                 const char *buf, size_t buf_len, int re_flags, void *opaque) {
  REParseState s_s, *s = &s_s;
  int stack_size;
  BOOL is_sticky;

  memset(s, 0, sizeof(*s));
  s->mem_opaque = opaque;
  s->buf_ptr = (const uint8_t *)buf;
  s->buf_end = s->buf_ptr + buf_len;
  s->buf_start = s->buf_ptr;
  s->re_flags = re_flags;
  s->is_utf16 = ((re_flags & LRE_FLAG_UTF16) != 0);
  is_sticky = ((re_flags & LRE_FLAG_STICKY) != 0);
  s->ignore_case = ((re_flags & LRE_FLAG_IGNORECASE) != 0);
  s->dotall = ((re_flags & LRE_FLAG_DOTALL) != 0);
  s->capture_count = 1;
  s->total_capture_count = -1;
  s->has_named_captures = -1;

  dbuf_init2(&s->byte_code, opaque, lre_realloc_addjudge);
  dbuf_init2(&s->group_names, opaque, lre_realloc_addjudge);

  dbuf_putc(&s->byte_code, re_flags); /* first element is the flags */
  dbuf_putc(&s->byte_code, 0);    /* second element is the number of captures */
  dbuf_putc(&s->byte_code, 0);    /* stack size */
  dbuf_put_u32(&s->byte_code, 0); /* bytecode length */

  if (!is_sticky) {
    /* iterate thru all positions (about the same as .*?( ... ) )
       .  We do it without an explicit loop so that lock step
       thread execution will be possible in an optimized
       implementation */
    re_emit_op_u32(s, REOP_split_goto_first, 1 + 5);
    re_emit_op(s, REOP_any);
    re_emit_op_u32(s, REOP_goto, -(5 + 1 + 5));
  }
  re_emit_op_u8(s, REOP_save_start, 0);

  if (re_parse_disjunction(s, FALSE)) {
  error:
    dbuf_free(&s->byte_code);
    dbuf_free(&s->group_names);
    pstrcpy(error_msg, error_msg_size, s->u.error_msg);
    *plen = 0;
    return NULL;
  }

  re_emit_op_u8(s, REOP_save_end, 0);

  re_emit_op(s, REOP_match);

  if (*s->buf_ptr != '\0') {
    re_parse_error(s, "extraneous characters at the end");
    goto error;
  }

  if (dbuf_error(&s->byte_code)) {
    re_parse_error(s, "out of memory");
    goto error;
  }

  stack_size = compute_stack_size(s->byte_code.buf, s->byte_code.size);
  if (stack_size < 0) {
    re_parse_error(s, "too many imbricated quantifiers");
    goto error;
  }

  s->byte_code.buf[RE_HEADER_CAPTURE_COUNT] = s->capture_count;
  s->byte_code.buf[RE_HEADER_STACK_SIZE] = stack_size;
  put_u32(s->byte_code.buf + 3, s->byte_code.size - RE_HEADER_LEN);

  /* add the named groups if needed */
  if (s->group_names.size > static_cast<size_t>(s->capture_count - 1)) {
    dbuf_put(&s->byte_code, s->group_names.buf, s->group_names.size);
    s->byte_code.buf[RE_HEADER_FLAGS] |= LRE_FLAG_NAMED_GROUPS;
  }
  dbuf_free(&s->group_names);

  error_msg[0] = '\0';
  *plen = s->byte_code.size;
  return s->byte_code.buf;
}

}  // namespace legacy_regexp
}  // namespace lynx
