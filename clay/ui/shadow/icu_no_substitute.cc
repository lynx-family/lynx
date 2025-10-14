// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/icu_substitute.h"

#if defined(ENABLE_SKITY) && !defined(OS_IOS) && !defined(OS_WIN)
#include "clay/ui/component/text/tttext_headers.h"
#else
#include <unicode/ubidi.h>
#include <unicode/uscript.h>
#endif

namespace clay {

namespace icu_substitute {

#if defined(ENABLE_SKITY) && !defined(OS_IOS) && !defined(OS_WIN)
#define UScriptGetScript tttext::ICUWrapper::uscriptGetScript
#define UBidiOpenSized tttext::ICUWrapper::ubidiOpenSized
#define UBidiSetPara tttext::ICUWrapper::ubidiSetPara
#define UBidiGetDirection tttext::ICUWrapper::ubidiGetDirection
#define UBidiClose tttext::ICUWrapper::ubidiClose
#else
#define UScriptGetScript uscript_getScript
#define UBidiOpenSized ubidi_openSized
#define UBidiSetPara ubidi_setPara
#define UBidiGetDirection ubidi_getDirection
#define UBidiClose ubidi_close
#endif

bool IsCJKCharacter(uint32_t unicode) {
  UErrorCode status = U_ZERO_ERROR;
  UScriptCode script = UScriptGetScript(unicode, &status);
  return (script == USCRIPT_HAN || script == USCRIPT_HIRAGANA ||
          script == USCRIPT_KATAKANA);
}

bool IsRTLCharacter(const std::u16string& text) {
  UErrorCode status = U_ZERO_ERROR;
  UBiDi* bidi = UBidiOpenSized(text.length(), 0, &status);
  // There are differences between uchar types on the Windows platform and uchar
  // types on other platforms.
  const UChar* text_ptr = reinterpret_cast<const UChar*>(text.data());
  UBidiSetPara(bidi, text_ptr, text.length(), UBIDI_DEFAULT_LTR, nullptr,
               &status);
  UBiDiDirection direction = UBidiGetDirection(bidi);

  UBidiClose(bidi);

  return direction == UBIDI_RTL;
}

}  // namespace icu_substitute

}  // namespace clay
