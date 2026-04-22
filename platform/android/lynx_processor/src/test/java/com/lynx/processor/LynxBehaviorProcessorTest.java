// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor;

import static com.google.testing.compile.Compiler.javac;
import static org.junit.Assert.assertTrue;

import com.google.testing.compile.Compilation;
import com.google.testing.compile.JavaFileObjects;
import java.io.IOException;
import javax.tools.JavaFileObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class LynxBehaviorProcessorTest {
  private static final JavaFileObject BEHAVIOR_STUB =
      JavaFileObjects.forSourceString("com.lynx.tasm.behavior.Behavior",
          "package com.lynx.tasm.behavior;\n"
              + "public class Behavior {\n"
              + "  public Behavior(String name, boolean flatten, boolean createAsync, boolean "
              + "needProcessDirection) {}\n"
              + "  public com.lynx.tasm.behavior.ui.LynxUI createUI(LynxContext context) { return "
              + "null; }\n"
              + "  public com.lynx.tasm.behavior.shadow.ShadowNode createShadowNode() { return "
              + "null; }\n"
              + "}\n");

  private static final JavaFileObject LYNX_CONTEXT_STUB =
      JavaFileObjects.forSourceString("com.lynx.tasm.behavior.LynxContext",
          "package com.lynx.tasm.behavior;\n"
              + "public class LynxContext {}\n");

  private static final JavaFileObject LYNX_UI_STUB =
      JavaFileObjects.forSourceString("com.lynx.tasm.behavior.ui.LynxUI",
          "package com.lynx.tasm.behavior.ui;\n"
              + "import com.lynx.tasm.behavior.LynxContext;\n"
              + "public class LynxUI {\n"
              + "  public LynxUI(LynxContext context) {}\n"
              + "}\n");

  private static final JavaFileObject SHADOW_NODE_STUB =
      JavaFileObjects.forSourceString("com.lynx.tasm.behavior.shadow.ShadowNode",
          "package com.lynx.tasm.behavior.shadow;\n"
              + "public class ShadowNode {}\n");

  private static final JavaFileObject KEEP_STUB =
      JavaFileObjects.forSourceString("androidx.annotation.Keep",
          "package androidx.annotation;\n"
              + "import java.lang.annotation.Retention;\n"
              + "import java.lang.annotation.RetentionPolicy;\n"
              + "@Retention(RetentionPolicy.CLASS)\n"
              + "public @interface Keep {}\n");

  private static JavaFileObject[] getCommonStubs() {
    return new JavaFileObject[] {
        BEHAVIOR_STUB, LYNX_CONTEXT_STUB, LYNX_UI_STUB, SHADOW_NODE_STUB, KEEP_STUB};
  }

  private static String getGeneratedSource(Compilation compilation, String qualifiedName)
      throws IOException {
    JavaFileObject generated = compilation.generatedSourceFile(qualifiedName).orElseThrow(() -> {
      StringBuilder sb = new StringBuilder(
          "Expected generated file not found: " + qualifiedName + "\nGenerated files:\n");
      for (JavaFileObject file : compilation.generatedSourceFiles()) {
        sb.append("  ").append(file.getName()).append("\n");
      }
      return new AssertionError(sb.toString());
    });
    return generated.getCharContent(true).toString();
  }

  @Test
  public void testBehaviorDefaultFlags() throws IOException {
    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestUI",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxBehavior;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxBehavior(tagName = \"test\")\n"
            + "public class TestUI extends LynxUI {\n"
            + "  public TestUI(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertTrue("Should use 4-arg Behavior constructor with all flags false",
        source.contains("new Behavior(\"test\", false, false, false)"));
  }

  @Test
  public void testBehaviorWithAsyncAndDirection() throws IOException {
    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestUI",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxBehavior;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxBehavior(tagName = \"test\", isCreateAsync = true, needProcessDirection = "
            + "true)\n"
            + "public class TestUI extends LynxUI {\n"
            + "  public TestUI(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertTrue("Should use correct Behavior constructor with async and direction true",
        source.contains("new Behavior(\"test\", false, true, true)"));
  }

  @Test
  public void testBehaviorWithShadowNode() throws IOException {
    JavaFileObject shadowNodeClass = JavaFileObjects.forSourceString("com.test.TestShadowNode",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxShadowNode;\n"
            + "@LynxShadowNode(tagName = \"test\")\n"
            + "public class TestShadowNode extends com.lynx.tasm.behavior.shadow.ShadowNode {}\n");

    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestUI",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxBehavior;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxBehavior(tagName = \"test\")\n"
            + "public class TestUI extends LynxUI {\n"
            + "  public TestUI(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), shadowNodeClass, testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertTrue("Should contain createShadowNode", source.contains("createShadowNode"));
  }

  private static JavaFileObject[] merge(JavaFileObject[] base, JavaFileObject... extras) {
    JavaFileObject[] result = new JavaFileObject[base.length + extras.length];
    System.arraycopy(base, 0, result, 0, base.length);
    System.arraycopy(extras, 0, result, base.length, extras.length);
    return result;
  }
}
