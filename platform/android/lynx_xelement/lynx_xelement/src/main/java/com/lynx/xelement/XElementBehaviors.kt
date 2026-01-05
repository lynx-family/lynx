
// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement

import androidx.annotation.Keep
import com.lynx.tasm.behavior.Behavior
import com.lynx.tasm.behavior.BehaviorBundle

@Keep
class XElementBehaviors : BehaviorBundle {
  open override fun create(): List<Behavior> {
    synchronized(XElementBehaviors::class.java) {
      if (sCacheBehaviors != null && sCacheBehaviors!!.isNotEmpty()) {
        return ArrayList(sCacheBehaviors)
      }
      val list: MutableList<Behavior> = ArrayList()
      for (s in GENERATOR_FILE_NAME_SETS) {
        list.addAll(getModuleBehaviors(s)!!)
      }
      sCacheBehaviors = list
      return ArrayList(list)
    }
  }

  private fun getModuleBehaviors(generatorFilePackageName: String): List<Behavior>? {
    var result: List<Behavior> = ArrayList()
    try {
      val cls = Class.forName("$generatorFilePackageName.BehaviorGenerator")
      val method = cls.getMethod("getBehaviors")
      result = method.invoke(null) as List<Behavior>
    } catch (t: Throwable) {
      // ignore, otherwise will report a lot of invalid logs
    }
    return result
  }

  companion object {

    @Volatile
    private var sCacheBehaviors: List<Behavior>? = null
    val GENERATOR_FILE_NAME_SETS: HashSet<String> = object : HashSet<String>() {
      init {
        add("com.lynx.xelement")
        add("com.lynx.xelement.svg")
      }
    }
  }
}



