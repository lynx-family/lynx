
# Shell

## Table of Contents
1. [Overview](#overview)
2. [Architectural Diagram](#architectural-diagram)
3. [LynxActor](#lynxactor)
4. [Life Cycle](#life-cycle)
5. [Cross-Thread Communication in the Native Layer](#cross-thread-communication-in-the-native-layer)
6. [Accessing Layers](#accessing-layers)
   - [Platform Layer](#platform-layer)
   - [Background Threads](#background-threads)

---

## Overview
The "shell" module is located in `lynx/core/shell` and implements the
multi-threaded architecture of Lynx. It manages the native-layer lifecycle,
bridges interactions between the native layer and the platform layer, and
handles cross-thread communication in a unified manner.

---

## Architectural Diagram

The diagram below illustrates the hierarchical relationship of the shell module within Lynx.

```plaintext

+---------------------------------------------------------------------------+
|                             LynxView (Platform Layer)                     |
+--------------------------------------^------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------+
|                         LynxShell (Shell Layer)                           |
|                (Manages lifecycle, cross-thread comm, etc.)               |
+----------+---------------+------------------+-------------------+---------+
           |               |                  |                   |
           v               v                  v                   v
+----------------+  +---------------+  +----------------+  +----------------+
|  NativeFacade  |  |  LynxEngine   |  | LayoutContext  |  |  LynxRuntime   |
| (Platform-     |  | (Tasm-thread  |  | (Layout-thread |  | (JS-thread     |
|  thread Actor) |  |     Actor)    |  |     Actor)     |  |     Actor)     |
+------^---------+  +------^--------+  +------^---------+  +------^---------+
                           |                  |                   |
                           v                  v                   v
                    +---------------+  +----------------+  +-----------------+
                    |  TasmMediator |  | LayoutMediator |  | RuntimeMediator |
                    |               |  |                |  |                 |
                    |               |  |                |  |                 |
                    +------^--------+  +------^---------+  +------^----------+
```

All inter-thread communication occurs via delegates or message passing. **Direct state sharing or synchronous access should be restricted.**

---

## LynxActor

**LynxActor** is the core abstraction in LynxShell’s multi-threaded design. Each actor:

- **Runs on its own thread**: Avoids shared state and reduces concurrency issues.
- **Manages its own lifecycle**: Creates, initializes, and destroys its owned objects.
- **Communicates via messages**: Uses delegates or asynchronous mechanisms to interact with other actors, ensuring loose coupling.

By separating functionality into distinct **LynxActor** units, the shell module achieves clearer boundaries, better scalability, and improved maintainability.

---

## Life Cycle

Ensure that your module’s lifecycle is **managed by a LynxActor**.
This means that your module needs to be **a compositional part of LynxActor**.

### Key Guidelines
- Use a **stack object** or **`std::unique_ptr`** in the parent object for ownership.
- **Avoid using `std::shared_ptr`** for lifecycle management.

**Thread-to-Actor Correspondences**:
- **TASM thread** → **LynxEngine**
- **Layout thread** → **LayoutContext**
- **JS thread** → **LynxRuntime**
- **Platform thread** → **NativeFacade**

---

## Cross-Thread Communication in the Native Layer

All cross-thread communication in the native layer is achieved through LynxActor. The caller does not need to worry about lifecycle details—just use `LynxActor::Act`, which takes a closure and ensures proper execution on the corresponding thread.

### Example

Define a Delegate for cross-thread communication:

```cpp
class YourModule {
 public:
  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;

    virtual void CallFunction() = 0;
  };
};
```

Let the Delegate inherit from `LynxEngine::Delegate` and implement the method in `TasmMediator`:

```cpp
class LynxEngine {
 public:
  class Delegate : public YourModule::Delegate {        
  };
};

class TasmMediator : public LynxEngine::Delegate {
 public:
  void CallFunction() override;
};
```

Access the desired LynxActor within `TasmMediator`:

```cpp
void TasmMediator::CallFunction() {
  runtime_actor_->Act(
      [](auto& runtime) {
        runtime->CallFunction();
      });
}
```

---

## Accessing Layers

### Platform Layer

Ensure LynxShell is accessed on the **platform UI thread**. Access from background threads may cause crashes. For platform-layer access to the native layer on background threads, refer to the next section.

To call `CallFunction` through the platform layer on the TASM thread:

```cpp
void LynxShell::CallFunction() {
  engine_actor_->Act(
      [](auto& engine) {
        engine->CallFunction();
      });
}
```

---

### Background Threads

When the native layer needs to be accessed from a background thread, use proxies like `LynxEngineProxy` or `JSProxy`.

For example:

```cpp
class LynxEngineProxy {
 public:
  virtual void CallFunction() = 0;
};
```

Implement the method in `LynxEngineProxyImpl`:

```cpp
class LynxEngineProxyImpl : public LynxEngineProxy {
 public:
  void CallFunction() override {
    engine_actor_->Act(
        [](auto& engine) {
          engine->CallFunction();
        });
  }
};
```
