# Plugins

## InitPlugin

The init plugin is used to initialize repositories and testing templates. 
It is already integrated by default in the RTF tool and does not need to be added manually.

#### help
```commandline
rtf init -h

usage: app.py init [-h] {project,template} ...
optional arguments:
  -h, --help          show this help message and exit
init_type:
  valid init commands
  {project,template}
    project           init project
    template          init template
```

#### init project
> Initialize the current repository
```commandline
rtf init project -h

usage: app.py init project [-h] [-p PATH]
optional arguments:
  -h, --help            show this help message and exit
  -p PATH, --path PATH  Specify the workspace path (default: '.')
```
> Initialize the current repository in current path.
```commandline
rtf init project --path .
```

#### init test template
```commandline
rtf init template -h

usage: app.py init template [-h] -t TEMPLATE_TYPE -n TEMPLATE_NAME

optional arguments:
  -h, --help            show this help message and exit
  -t TEMPLATE_TYPE, --type TEMPLATE_TYPE
                        Template type to initialize
  -n TEMPLATE_NAME, --name TEMPLATE_NAME
                        Template name to initialize
```
> Initialize a C++ test template named 'lynx'. After successful initialization, 
> you will find the 'native-ut-lynx.template' file in the .rtf directory.
```commandline
rtf init template --name lynx --type native-ut
```

## NativeUTPlugin

The native ut plugin is used to manage C++ unit tests, responsible for building, running, and generating coverage.

#### add native ut plugin
```python
# .rtf/config
plugins = ["NativeUT"]
```

#### help
```commandline
rtf native-ut -h

usage: app.py native-ut [-h] {run,list} ...

optional arguments:
  -h, --help  show this help message and exit

command:
  run|list

  {run,list}
    run       run targets
    list      list targets
```

#### run native ut tests
- help
```commandline
rtf native-ut run -h

usage: app.py native-ut run [-h] --names [NAMES ...] [--target TARGET] [--args ARGS]

optional arguments:
  -h, --help           show this help message and exit
  --names [NAMES ...]  Specify the template name
  --target TARGET      Specify the target name
  --args ARGS          User custom params, eg: --args="args_1=true, args_2=1"
```
- run all tests
> Run all test cases under the 'lynx' template.
```
rtf native-ut run --names lynx
```
- Run a specified test case
> Run test_a under the 'lynx' template.
```commandline
rtf native-ut run --names lynx --target test_a
```

#### Print the list of test objects.
```commandline
rtf native-ut list --names lynx
```

#### Parameterize dynamically
You can use dynamic parameters to control the behavior of the template. Refer to:[dynamic-params](../core/options/README.md)
```commandline
rtf native-ut run --names lynx --target test_a --args="a=1,b=false"
```

## AndroidUTPlugin

The android ut plugin is used to manage android unit tests, responsible for building, running, and generating coverage.

#### add android ut plugin
```python
# .rtf/config
plugins = ["AndroidUT"]
```

#### help
```commandline
rtf android-ut -h

usage: app.py android-ut [-h] {run,list} ...

optional arguments:
  -h, --help  show this help message and exit

command:
  run|list

  {run,list}
    run       run targets
    list      list targets
```

#### run android ut tests
- help
```commandline
rtf android-ut run -h

usage: app.py android-ut run [-h] --names [NAMES ...] [--target TARGET] [--args ARGS] [--rmd]

optional arguments:
  -h, --help           show this help message and exit
  --names [NAMES ...]  Specify the template name
  --target TARGET      Specify the target name
  --args ARGS          User custom params, eg: --args="args_1=true, args_2=1"
  --rmd                run test in real mobile device
```
- run all tests
> Run all android test cases under the 'lynx' template.
```
rtf android-ut run --names lynx
```
- Run a specified test case
> Run test_a under the 'lynx' template.
```commandline
rtf android-ut run --names lynx --target test_a
```

#### Simulator or real device
> By default, Android unit tests run on an emulator. You can also use a real 
> device by specifying --rmd.
```commandline
rtf android-ut run --names lynx --target test_a --rmd
```

#### Parameterize dynamically
You can use dynamic parameters to control the behavior of the template. Refer to:[dynamic-params](../core/options/README.md)
```commandline
rtf android-ut run --names lynx --target test_a --args="a=1,b=false"
```

## CoverageCheckerPlugin

The coverage detection plugin is used to identify which lines of tests have not been covered in the current changes.

#### add coverage checker plugin
```python
# .rtf/config
plugins = ["CoverageChecker"]
```

#### help
```commandline
rtf coverage-check -h

usage: app.py coverage-check [-h] --type CHECK_TYPE --inputs [INPUTS ...] [--threshold THRESHOLD] [--job JOB]

optional arguments:
  -h, --help            show this help message and exit
  --type CHECK_TYPE     check type, android 、cpp、oc etc.
  --inputs [INPUTS ...]
                        Specify the input data
  --threshold THRESHOLD
                        Input a threshold
  --job JOB

```

#### inputs type

The coverage detection requires an input coverage information file, which can be obtained during the execution phase.

| TestType  | Details                                                     |
|-----------|-------------------------------------------------------------|
| AndroidUT | [coverage.xml](../core/coverage/README.md#android-coverage) |
| NativeUT  | [coverage.lcov](../core/coverage/README.md#cpp-coverage) |

#### Threshold

The coverage threshold, 0.5 means that if the coverage is less than 50%, an error will be reported.

#### Job

Specify a task name, which will be displayed in the output coverage detection report.

#### Example

```commandline
rtf coverage-check --type android --inputs coverage.xml --threshold 0.5 --job "android-coverage-check"
```