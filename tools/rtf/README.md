# RTF (Reusable Testing Framework)

RTF is a testing management framework for transparent testing management, simplifying the testing configuration process, and enabling quick integration for new projects.

Related resources:
 * User Guide in [RTF Tool User Guide](reference to lynx official website)


## What RTF is for

RTF is currently used as the test management tool for Lynx and related projects. 
Some strengths of RTF are:

 * Provide template files for centralized management of test cases, using corresponding 
   template files to manage and configure test cases, enabling users to focus solely on 
   the template files for the entire testing system without needing to concern themselves 
   with the underlying operation of the testing tools.

 * Support coexistence of different build environments, and for test cases with different 
   build parameters, support isolated configuration by the builder.

 * The testing template supports dynamic parameters. The template exposes dynamic capabilities 
   through the [options](./core/options/README.md) object, allowing template configuration to be determined at runtime.

 * Support parallel execution of cases to reduce testing time.


## Quick start

- Print help message
```commandline
rtf -h
```

- Init project
> For each repository, you must use the rtf init plugin (integrated by default) 
to initialize the repository first. After successful initialization, you will 
find a .rtf folder in the initialization directory.
```commandline
rtf init -h
rtf init project --path .
```

- Add Plugin
> You can configure plugins in the .rtf/config file.
```python
# .rtf/config

plugins = ["NativeUT", "AndroidUT", "CoverageChecker"]
```
> After configuring the plugins, you can reuse the 'help' command to discover the newly added plugins.
```commandline
rtf -h
```
>  The current list of supported plugins is as follows.

| plugin          | Details                                                            |
|-----------------|--------------------------------------------------------------------|
| NativeUT        | [NativeUTPlugin](./plugins/README.md#nativeutplugin)               |
| AndroidUT       | [AndroidUTPlugin](./plugins/README.md#androidutplugin)             |
| CoverageChecker | [CoverageCheckerPlugin](./plugins/README.md#coveragecheckerplugin) |

