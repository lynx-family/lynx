# AI Context & Guidelines

This repository contains a dedicated directory structure for AI agents to better understand the project's specific domain context, architectural decisions, and coding conventions.

## 🤖 For AI Agents

When working in this repository, please consult the contents of the **[`agents/`](./agents)** directory. This folder serves as a knowledge base containing:

- **Domain Knowledge**: Specific business logic and terminology used in this project.
- **Architecture Overview**: High-level design patterns and structural decisions.
- **Coding Standards**: Project-specific conventions that supplement general best practices.

**Action**: Before proposing significant changes or architectural updates, read the relevant documents in `agents/` to align with the project's established patterns.

For Harmony XElement work, also consult [`agents/harmony_xelement.md`](./agents/harmony_xelement.md), especially the native C++ registration direction for `markdown`.

For Explorer Node-API addon work, first read [`explorer/docs/lynx-node-api.md`](./explorer/docs/lynx-node-api.md) for the capability model, SDK prerequisites, sample loader behavior, and page-facing API. For platform packaging/build details, also read the relevant platform guide:

- Android: [`explorer/android/lynx-napi-addon.md`](./explorer/android/lynx-napi-addon.md)
- iOS: [`explorer/darwin/ios/lynx-napi-addon.md`](./explorer/darwin/ios/lynx-napi-addon.md)
- Harmony: [`explorer/harmony/lynx-napi-addon.md`](./explorer/harmony/lynx-napi-addon.md)
- macOS: [`explorer/darwin/macos/lynx-napi-addon.md`](./explorer/darwin/macos/lynx-napi-addon.md)
- Windows: [`explorer/windows/lynx-napi-addon.md`](./explorer/windows/lynx-napi-addon.md)
