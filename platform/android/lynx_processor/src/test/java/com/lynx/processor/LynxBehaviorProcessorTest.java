// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor;

import static com.google.testing.compile.Compiler.javac;
import static org.junit.Assert.assertFalse;
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
              + "  public Behavior(String name, boolean flatten, boolean createAsync, boolean "
              + "needProcessDirection, boolean supportFragmentLayerRender) {}\n"
              + "  public com.lynx.tasm.behavior.ui.LynxUI createUI(LynxContext context) { return "
              + "null; }\n"
              + "  public com.lynx.tasm.behavior.render.IRendererHost "
              + "createPlatformRendererHost(LynxContext context) { return null; }\n"
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

  private static final JavaFileObject I_RENDERER_HOST_STUB =
      JavaFileObjects.forSourceString("com.lynx.tasm.behavior.render.IRendererHost",
          "package com.lynx.tasm.behavior.render;\n"
              + "public interface IRendererHost {}\n");

  private static final JavaFileObject KEEP_STUB =
      JavaFileObjects.forSourceString("androidx.annotation.Keep",
          "package androidx.annotation;\n"
              + "import java.lang.annotation.Retention;\n"
              + "import java.lang.annotation.RetentionPolicy;\n"
              + "@Retention(RetentionPolicy.CLASS)\n"
              + "public @interface Keep {}\n");

  private static JavaFileObject[] getCommonStubs() {
    return new JavaFileObject[] {BEHAVIOR_STUB, LYNX_CONTEXT_STUB, LYNX_UI_STUB, SHADOW_NODE_STUB,
        I_RENDERER_HOST_STUB, KEEP_STUB};
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
  public void testBehaviorWithoutFragmentLayerRender() throws IOException {
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
    assertTrue("Should use 4-arg Behavior constructor",
        source.contains("new Behavior(\"test\", false, false, false)"));
    assertFalse("Should not contain createPlatformRendererHost",
        source.contains("createPlatformRendererHost"));
  }

  @Test
  public void testLynxAutolinkElementAnnotation() throws IOException {
    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestElement",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxAutolinkElement;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxAutolinkElement(name = \"test-element\", isCreateAsync = true)\n"
            + "public class TestElement extends LynxUI {\n"
            + "  public TestElement(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertTrue("Should use LynxAutolinkElement name",
        source.contains("new Behavior(\"test-element\", false, true, false)"));
  }

  @Test
  public void testBehaviorWithFragmentLayerRender() throws IOException {
    JavaFileObject rendererHostClass = JavaFileObjects.forSourceString("com.test.TestRendererHost",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.render.IRendererHost;\n"
            + "public class TestRendererHost implements IRendererHost {\n"
            + "  public TestRendererHost(LynxContext context) {}\n"
            + "}\n");

    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestUI",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxBehavior;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxBehavior(tagName = \"test\", supportFragmentLayerRender = true, "
            + "fragmentLayerRendererHost = TestRendererHost.class)\n"
            + "public class TestUI extends LynxUI {\n"
            + "  public TestUI(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), rendererHostClass, testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertTrue("Should use 5-arg Behavior constructor with true",
        source.contains("new Behavior(\"test\", false, false, false, true)"));
    assertTrue(
        "Should contain createPlatformRendererHost", source.contains("createPlatformRendererHost"));
    assertTrue(
        "Should instantiate TestRendererHost", source.contains("new TestRendererHost(context)"));
  }

  @Test
  public void testBehaviorWithFragmentLayerRenderButNoRendererHost() throws IOException {
    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestUI",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxBehavior;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxBehavior(tagName = \"test\", supportFragmentLayerRender = true)\n"
            + "public class TestUI extends LynxUI {\n"
            + "  public TestUI(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertTrue("Should use 5-arg Behavior constructor with true",
        source.contains("new Behavior(\"test\", false, false, false, true)"));
    assertFalse("Should not contain createPlatformRendererHost when no host is set",
        source.contains("createPlatformRendererHost"));
  }

  @Test
  public void testBehaviorWithDefaultRendererHostSentinel() throws IOException {
    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestUI",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxBehavior;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.render.IRendererHost;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxBehavior(tagName = \"test\", supportFragmentLayerRender = true, "
            + "fragmentLayerRendererHost = IRendererHost.class)\n"
            + "public class TestUI extends LynxUI {\n"
            + "  public TestUI(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertFalse("Should not instantiate the default IRendererHost sentinel",
        source.contains("createPlatformRendererHost"));
  }

  @Test
  public void testAutolinkElementWithDefaultRendererHostSentinel() throws IOException {
    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestElement",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxAutolinkElement;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.render.IRendererHost;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxAutolinkElement(name = \"test\", supportFragmentLayerRender = true, "
            + "fragmentLayerRendererHost = IRendererHost.class)\n"
            + "public class TestElement extends LynxUI {\n"
            + "  public TestElement(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertFalse("Should not instantiate the default IRendererHost sentinel",
        source.contains("createPlatformRendererHost"));
  }

  @Test
  public void testBehaviorWithAsyncAndDirection() throws IOException {
    JavaFileObject rendererHostClass = JavaFileObjects.forSourceString("com.test.TestRendererHost",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.render.IRendererHost;\n"
            + "public class TestRendererHost implements IRendererHost {\n"
            + "  public TestRendererHost(LynxContext context) {}\n"
            + "}\n");

    JavaFileObject testClass = JavaFileObjects.forSourceString("com.test.TestUI",
        "package com.test;\n"
            + "import com.lynx.tasm.behavior.LynxBehavior;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxBehavior(tagName = \"test\", isCreateAsync = true, needProcessDirection = "
            + "true, supportFragmentLayerRender = true, fragmentLayerRendererHost = "
            + "TestRendererHost.class)\n"
            + "public class TestUI extends LynxUI {\n"
            + "  public TestUI(LynxContext context) { super(context); }\n"
            + "}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxBehaviorProcessor())
                                  .compile(merge(getCommonStubs(), rendererHostClass, testClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.test.BehaviorGenerator");
    assertTrue("Should use correct Behavior constructor with all flags true",
        source.contains("new Behavior(\"test\", false, true, true, true)"));
    assertTrue(
        "Should contain createPlatformRendererHost", source.contains("createPlatformRendererHost"));
  }

  private static JavaFileObject[] merge(JavaFileObject[] base, JavaFileObject... extras) {
    JavaFileObject[] result = new JavaFileObject[base.length + extras.length];
    System.arraycopy(base, 0, result, 0, base.length);
    System.arraycopy(extras, 0, result, base.length, extras.length);
    return result;
  }
}
