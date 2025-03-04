# About //lynx/base
This is a basic library for the lynx project, which contains some basic components and utility functions. The aim is to make the development of the lynx project more convenient, reduce repetitive work, and facilitate code maintenance.
# How to use?
The base library uses GN to organize the code. We can directly add the dependency of the base library to the project to use the basic functions provided by the base library.
```GN
# Add the base dependency.
source_set("lynx_project") {
    deps = [
        "//lynx/base/src:base", // "//lynx/base/src:base" hasn't include trace and log functions
    ]
    deps += [
        "//lynx/base/src:base_log", // log functions
    ]
}
``` 
# What capabilities include?
> Only some capabilities are listed. For more, please refer to the source code.
- Log utils
 
    The base library provides log utils. By using the [LOGV~LOGE](include/log/logging.h) log macro APIs, we can easily log messages. It supports log output, log filtering, log formatting, and customizable log output channels. Additionally, it provides a lightweight log stream class, which makes the log system more efficient.
- thread utils
 
    Thread utils include message loop, shared mutex, semaphore, and so on. The message loop is an event-driven thread programming model. It is used to process events and tasks for a particular thread. Its basic functionality is to accept posted tasks and run them on the associated thread.
- String utils
    
    The base provides string utils, such as converting characters to numbers, converting characters to arrays, extracting substrings, and converting between unicode32, unicode16, and unicode8.
- Trace utils
    
    The base provides an independent and shareable instrumentation tool named lynx-trace. Please refer to [trace/README.md](trace/README.md) for details.




