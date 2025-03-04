/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * <p>
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
package com.lynx.tasm.behavior.shadow.text;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import java.text.DecimalFormat;

/**
 * {@link ShadowNode} class for pure raw text node (aka {@code textContent} in terms of DOM).
 * Raw text node can only have simple string value without any attributes, properties or state.
 */
public class RawTextShadowNode extends ShadowNode {
  private @Nullable String mText = null;
  private boolean mIsPseudo = false;

  public RawTextShadowNode() {}

  @LynxProp(name = PropsConstants.TEXT)
  public void setText(@Nullable Dynamic text) {
    mText = TextHelper.convertRawTextValue(text);

    markDirty();
  }

  @LynxProp(name = "pseudo")
  public void setPsuedo(@Nullable boolean isPseudo) {
    mIsPseudo = isPseudo;
  }

  public @Nullable String getText() {
    return mText;
  }

  @Override
  public boolean isVirtual() {
    return true;
  }

  @Override
  public String toString() {
    return getTagName() + " [text: " + mText + "]";
  }

  public boolean isPseudo() {
    return mIsPseudo;
  }
}
