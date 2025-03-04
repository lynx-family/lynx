# Lynx Code Coverage

## Cpp Coverage
The code coverage of Lynx C++ is statistically measured through compiler instrumentation based on [LLVM](https://clang.llvm.org/docs/UsersManual.html#profiling-with-instrumentation) , and corresponding tools are used to generate the coverage reports.  

### How to generate code coverage? 
1. First, run the unit tests.
```
cd repo_root_dir
tools/rtf/rtf native-ut run --names lynx
```
2. You can find coverage reports in various formats in out/coverage.
- html : ```out/coverage/html```
- json : ```out/coverage/json```
- lcov : ```out/coverage/lcov```


## Android Coverage

Android unit tests use the [JaCoCo plugin](https://www.eclemma.org/jacoco/index.html) to perform runtime coverage data statistics, and use the corresponding [JaCoCo CLI](https://www.jacoco.org/jacoco/trunk/doc/cli.html) tool to generate coverage reports.  

### How to generate code coverage? 
1. First, run the unit tests.
```
cd repo_root_dir
tools/rtf/rtf android-ut run --name lynx
```
2. You can find coverage reports in various formats in out/coverage.
- xml : ```out/coverage/xml```
- html : ```out/coverage/html```

