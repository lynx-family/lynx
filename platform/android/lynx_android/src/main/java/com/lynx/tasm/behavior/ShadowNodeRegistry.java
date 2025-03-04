/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.tasm.behavior;

import android.util.SparseArray;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.common.SingleThreadAsserter;

/**
 * Simple container class to keep track of {@link ShadowNode}s associated with a particular
 * UIManagerModule instance.
 */
public class ShadowNodeRegistry {
  private final SparseArray<ShadowNode> mShadowNodeList;

  public ShadowNodeRegistry() {
    mShadowNodeList = new SparseArray<>();
  }

  public void addNode(ShadowNode node) {
    mShadowNodeList.put(node.getSignature(), node);
  }

  public ShadowNode removeNode(int signature) {
    ShadowNode removed = mShadowNodeList.get(signature);
    mShadowNodeList.remove(signature);
    return removed;
  }

  public ShadowNode getNode(int tag) {
    return mShadowNodeList.get(tag);
  }

  public SparseArray<ShadowNode> getAllNodes() {
    return mShadowNodeList;
  }
}
