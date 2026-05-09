// Copyright 2026 The Lynx Authors. All rights reserved.
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
public class LynxExtensionProcessorTest {
  private static final JavaFileObject KEEP_STUB =
      JavaFileObjects.forSourceString("androidx.annotation.Keep",
          "package androidx.annotation;\n"
              + "import java.lang.annotation.Retention;\n"
              + "import java.lang.annotation.RetentionPolicy;\n"
              + "@Retention(RetentionPolicy.CLASS)\n"
              + "public @interface Keep {}\n");

  private static final JavaFileObject PROVIDER_STUB =
      JavaFileObjects.forSourceString("com.lynx.tasm.extension.LynxExtensionProvider",
          "package com.lynx.tasm.extension;\n"
              + "public interface LynxExtensionProvider {\n"
              + "  void register(LynxExtensionRegistry registry);\n"
              + "}\n");

  private static final JavaFileObject REGISTRY_STUB =
      JavaFileObjects.forSourceString("com.lynx.tasm.extension.LynxExtensionRegistry",
          "package com.lynx.tasm.extension;\n"
              + "import java.util.List;\n"
              + "import com.lynx.jsbridge.LynxModule;\n"
              + "import com.lynx.tasm.behavior.Behavior;\n"
              + "import com.lynx.tasm.service.IServiceProvider;\n"
              + "public class LynxExtensionRegistry {\n"
              + "  public void addBehaviors(List<Behavior> behaviors) {}\n"
              + "  public void registerModule(String name, Class<? extends LynxModule> cls) {}\n"
              + "  public void registerService(Class<? extends IServiceProvider> cls) {}\n"
              + "}\n");

  private static final JavaFileObject LYNX_MODULE_STUB =
      JavaFileObjects.forSourceString("com.lynx.jsbridge.LynxModule",
          "package com.lynx.jsbridge;\n"
              + "public class LynxModule {}\n");

  private static final JavaFileObject SERVICE_PROVIDER_STUB =
      JavaFileObjects.forSourceString("com.lynx.tasm.service.IServiceProvider",
          "package com.lynx.tasm.service;\n"
              + "public interface IServiceProvider {}\n");

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

  @Test
  public void testMixedExtensionProviderGeneration() throws IOException {
    JavaFileObject element = JavaFileObjects.forSourceString("com.ext.Button",
        "package com.ext;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.LynxAutolinkElement;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxAutolinkElement(name = \"button\")\n"
            + "public class Button extends LynxUI {\n"
            + "  public Button(LynxContext context) { super(context); }\n"
            + "}\n");
    JavaFileObject module = JavaFileObjects.forSourceString("com.ext.StorageModule",
        "package com.ext;\n"
            + "import com.lynx.jsbridge.LynxModule;\n"
            + "import com.lynx.jsbridge.LynxAutolinkNativeModule;\n"
            + "@LynxAutolinkNativeModule(name = \"NativeLocalStorage\")\n"
            + "public class StorageModule extends LynxModule {}\n");
    JavaFileObject service = JavaFileObjects.forSourceString("com.ext.LogService",
        "package com.ext;\n"
            + "import com.lynx.tasm.service.IServiceProvider;\n"
            + "import com.lynx.tasm.service.LynxAutolinkService;\n"
            + "@LynxAutolinkService\n"
            + "public class LogService implements IServiceProvider {}\n");

    Compilation compilation =
        javac()
            .withOptions("-A"
                + "lynx.extension.packageName=com.generated.ext")
            .withProcessors(new LynxBehaviorProcessor(), new LynxExtensionProcessor())
            .compile(merge(getCommonStubs(), element, module, service));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);

    String source = getGeneratedSource(compilation, "com.generated.ext.LynxExtensionProviderImpl");
    assertTrue(source.contains("registry.addBehaviors(BehaviorGenerator.getBehaviors())"));
    assertTrue(
        source.contains("registry.registerModule(\"NativeLocalStorage\", StorageModule.class)"));
    assertTrue(source.contains("registry.registerService(LogService.class)"));
  }

  @Test
  public void testEmptyExtensionDoesNotForceProcessorRun() {
    JavaFileObject emptyClass = JavaFileObjects.forSourceString("com.ext.Empty",
        "package com.ext;\n"
            + "public class Empty {}\n");

    Compilation compilation = javac()
                                  .withOptions("-A"
                                      + "lynx.extension.packageName=com.ext")
                                  .withProcessors(new LynxExtensionProcessor())
                                  .compile(merge(getCommonStubs(), emptyClass));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);
    assertFalse(compilation.generatedSourceFile("com.ext.LynxExtensionProviderImpl").isPresent());
  }

  @Test
  public void testExtensionProviderRequiresPackageNameOption() {
    JavaFileObject element = JavaFileObjects.forSourceString("com.ext.Button",
        "package com.ext;\n"
            + "import com.lynx.tasm.behavior.LynxContext;\n"
            + "import com.lynx.tasm.behavior.LynxAutolinkElement;\n"
            + "import com.lynx.tasm.behavior.ui.LynxUI;\n"
            + "@LynxAutolinkElement(name = \"button\")\n"
            + "public class Button extends LynxUI {\n"
            + "  public Button(LynxContext context) { super(context); }\n"
            + "}\n");
    JavaFileObject module = JavaFileObjects.forSourceString("com.ext.StorageModule",
        "package com.ext;\n"
            + "import com.lynx.jsbridge.LynxModule;\n"
            + "import com.lynx.jsbridge.LynxAutolinkNativeModule;\n"
            + "@LynxAutolinkNativeModule(name = \"NativeLocalStorage\")\n"
            + "public class StorageModule extends LynxModule {}\n");
    JavaFileObject service = JavaFileObjects.forSourceString("com.ext.LogService",
        "package com.ext;\n"
            + "import com.lynx.tasm.service.IServiceProvider;\n"
            + "import com.lynx.tasm.service.LynxAutolinkService;\n"
            + "@LynxAutolinkService\n"
            + "public class LogService implements IServiceProvider {}\n");

    Compilation compilation = javac()
                                  .withProcessors(new LynxExtensionProcessor())
                                  .compile(merge(getCommonStubs(), element, module, service));

    assertTrue("Compilation should succeed", compilation.status() == Compilation.Status.SUCCESS);
    assertFalse(compilation.generatedSourceFile("com.ext.LynxExtensionProviderImpl").isPresent());
  }

  @Test
  public void testDoesNotClaimAllAnnotationTypes() {
    assertFalse(new LynxExtensionProcessor().getSupportedAnnotationTypes().contains("*"));
  }

  private static JavaFileObject[] getCommonStubs() {
    return new JavaFileObject[] {KEEP_STUB, PROVIDER_STUB, REGISTRY_STUB, LYNX_MODULE_STUB,
        SERVICE_PROVIDER_STUB, BEHAVIOR_STUB, LYNX_CONTEXT_STUB, LYNX_UI_STUB, SHADOW_NODE_STUB,
        I_RENDERER_HOST_STUB};
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

  private static JavaFileObject[] merge(JavaFileObject[] base, JavaFileObject... extras) {
    JavaFileObject[] result = new JavaFileObject[base.length + extras.length];
    System.arraycopy(base, 0, result, 0, base.length);
    System.arraycopy(extras, 0, result, base.length, extras.length);
    return result;
  }
}
