// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_STREAM_H_
#define CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_STREAM_H_

#include <vector>

#include "base/include/auto_reset.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"

namespace lynx {
namespace css {

namespace detail {

template <typename...>
bool IsTokenTypeOneOf(CSSParserTokenType t) {
  return false;
}

template <CSSParserTokenType Head, CSSParserTokenType... Tail>
bool IsTokenTypeOneOf(CSSParserTokenType t) {
  return t == Head || IsTokenTypeOneOf<Tail...>(t);
}

}  // namespace detail

// A streaming interface to CSSTokenizer that tokenizes on demand.
// Abstractly, the stream ends at either EOF or the beginning/end of a block.
// To consume a block, a BlockGuard must be created first to ensure that
// we finish consuming a block even if there was an error.
//
// Methods prefixed with "Unchecked" can only be called after calls to Peek(),
// EnsureLookAhead(), or AtEnd() with no subsequent modifications to the stream
// such as a consume.
class CSSParserTokenStream {
 public:
  // Instantiate this to start reading from a block. When the guard is out of
  // scope, the rest of the block is consumed.
  class BlockGuard {
   public:
    explicit BlockGuard(CSSParserTokenStream& stream) : stream_(stream) {
      const CSSParserToken next = stream.ConsumeInternal();
      DCHECK_EQ(next.GetBlockType(), CSSParserToken::kBlockStart);
    }

    ~BlockGuard() {
      stream_.EnsureLookAhead();
      stream_.UncheckedSkipToEndOfBlock();
    }

   private:
    CSSParserTokenStream& stream_;
  };

  static constexpr uint64_t FlagForTokenType(CSSParserTokenType token_type) {
    return 1ull << static_cast<uint64_t>(token_type);
  }

  // The specified token type will be treated as kEOF while the Boundary is on
  // the stack.
  class Boundary {
   public:
    Boundary(CSSParserTokenStream& stream, CSSParserTokenType boundary_type)
        : auto_reset_(&stream.boundaries_,
                      stream.boundaries_ | FlagForTokenType(boundary_type)) {}
    ~Boundary() = default;

   private:
    lynx::base::AutoReset<uint64_t> auto_reset_;
  };

  // We found that this value works well empirically by printing out the
  // maximum buffer size for a few top alexa websites. It should be slightly
  // above the expected number of tokens in the prelude of an at rule and
  // the number of tokens in a declaration.
  // TODO(crbug.com/661854): Can we streamify at rule parsing so that this is
  // only needed for declarations which are easier to think about?
  static constexpr size_t InitialBufferSize() { return 128; }

  explicit CSSParserTokenStream(CSSTokenizer& tokenizer)
      : tokenizer_(tokenizer), next_(kEOFToken) {
    buffer_.reserve(InitialBufferSize());
  }

  CSSParserTokenStream(CSSParserTokenStream&&) = default;
  CSSParserTokenStream(const CSSParserTokenStream&) = delete;
  CSSParserTokenStream& operator=(const CSSParserTokenStream&) = delete;

  inline void EnsureLookAhead() {
    if (!HasLookAhead()) {
      has_look_ahead_ = true;
      next_ = tokenizer_.TokenizeSingle();
    }
  }

  // Forcibly read a lookahead token.
  inline void LookAhead() {
    DCHECK(!HasLookAhead());
    next_ = tokenizer_.TokenizeSingle();
    has_look_ahead_ = true;
  }

  inline bool HasLookAhead() const { return has_look_ahead_; }

  inline const CSSParserToken& Peek() {
    EnsureLookAhead();
    return next_;
  }

  inline const CSSParserToken& UncheckedPeek() const {
    DCHECK(HasLookAhead());
    return next_;
  }

  inline const CSSParserToken& Consume() {
    EnsureLookAhead();
    return UncheckedConsume();
  }

  const CSSParserToken& UncheckedConsume() {
    DCHECK(HasLookAhead());
    DCHECK(next_.GetBlockType() != CSSParserToken::kBlockStart);
    DCHECK(next_.GetBlockType() != CSSParserToken::kBlockEnd);
    has_look_ahead_ = false;
    offset_ = tokenizer_.Offset();
    return next_;
  }

  inline bool AtEnd() {
    EnsureLookAhead();
    return UncheckedAtEnd();
  }

  inline bool UncheckedAtEnd() const {
    DCHECK(HasLookAhead());
    return (boundaries_ & FlagForTokenType(next_.GetType())) ||
           next_.GetBlockType() == CSSParserToken::kBlockEnd;
  }

  // Get the index of the character in the original string to be consumed next.
  size_t Offset() const { return offset_; }

  // Get the index of the starting character of the look-ahead token.
  size_t LookAheadOffset() const {
    DCHECK(HasLookAhead());
    return tokenizer_.PreviousOffset();
  }

  // Returns a view on a range of characters in the original string.
  std::u16string StringRangeAt(size_t start, size_t length) const;

  void ConsumeWhitespace();
  CSSParserToken ConsumeIncludingWhitespace();
  void UncheckedConsumeComponentValue();

  // Either consumes a comment token and returns true, or peeks at the next
  // token and return false.
  bool ConsumeCommentOrNothing();

  // Invalidates any ranges created by previous calls to
  // ConsumeUntilPeekedTypeIs()
  template <CSSParserTokenType... Types>
  CSSParserTokenRange ConsumeUntilPeekedTypeIs() {
    EnsureLookAhead();

    buffer_.clear();
    while (!UncheckedAtEnd() &&
           !detail::IsTokenTypeOneOf<Types...>(UncheckedPeek().GetType())) {
      ConsumeTokenOrBlockAndAppendToBuffer();
    }

    return CSSParserTokenRange(buffer_);
  }

 private:
  inline void ConsumeTokenOrBlockAndAppendToBuffer() {
    // Have to use internal consume/peek in here because they can read past
    // start/end of blocks
    unsigned nesting_level = 0;
    do {
      const CSSParserToken& token = UncheckedConsumeInternal();
      buffer_.push_back(token);

      if (token.GetBlockType() == CSSParserToken::kBlockStart)
        nesting_level++;
      else if (token.GetBlockType() == CSSParserToken::kBlockEnd)
        nesting_level--;
    } while (!PeekInternal().IsEOF() && nesting_level);
  }

  const CSSParserToken& PeekInternal() {
    EnsureLookAhead();
    return UncheckedPeekInternal();
  }

  const CSSParserToken& UncheckedPeekInternal() const {
    DCHECK(HasLookAhead());
    return next_;
  }

  const CSSParserToken& ConsumeInternal() {
    EnsureLookAhead();
    return UncheckedConsumeInternal();
  }

  const CSSParserToken& UncheckedConsumeInternal() {
    DCHECK(HasLookAhead());
    has_look_ahead_ = false;
    offset_ = tokenizer_.Offset();
    return next_;
  }

  void UncheckedSkipToEndOfBlock();

  std::vector<CSSParserToken> buffer_;
  CSSTokenizer& tokenizer_;
  CSSParserToken next_;
  size_t offset_ = 0;
  bool has_look_ahead_ = false;
  uint64_t boundaries_ = FlagForTokenType(kEOFToken);
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_STREAM_H_
