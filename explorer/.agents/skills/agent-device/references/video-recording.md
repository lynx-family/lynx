# Video Recording

Capture device automation sessions as video for debugging, documentation, or verification

## iOS Simulator

Use `agent-device record` commands (wrapper around simctl):

```bash
# Start recording
agent-device record start ./recordings/ios.mov

# Perform actions
agent-device open App
agent-device snapshot -i
agent-device click @e3
agent-device close

# Stop recording
agent-device record stop
```

`record` is iOS simulator-only.

## Android Emulator/Device

Use `agent-device record` commands (wrapper around adb):

```bash
# Start recording
agent-device record start ./recordings/android.mp4

# Perform actions
agent-device open App
agent-device snapshot -i
agent-device click @e3
agent-device close

# Stop recording
agent-device record stop
```
