# AGENTS Guide for QuarkEngine

This file serves as the entry point for AI agents and human contributors working on QuarkEngine. It provides a high-level map of the project and points to detailed documentation in the `docs/` folder.

## Quick Links

| Topic                        | File                              | Description |
|-----------------------------|-----------------------------------|-----------|
| **Architecture**            | [docs/architecture-overview.md](docs/architecture-overview.md) | Overall layering, runtime flow, and rendering boundaries |
| **Code Conventions**        | [docs/code-conventions.md](docs/code-conventions.md) | Include ordering, nullability rules, memory usage, logging, etc. |
| **Build & Run Commands**    | [docs/commands.md](docs/commands.md) | How to configure, build, run, and test the project |
| **Design Principles**       | [docs/principles.md](docs/principles.md) | Core philosophy and safe ways to extend the engine |
| **Current Status**          | [docs/status.md](docs/status.md) | What has been completed recently (especially the cstdlib migration) |
| **Roadmap**                 | [docs/roadmap.md](docs/roadmap.md) | Medium and long-term direction |
| **Open Tasks**              | [docs/todo.md](docs/todo.md) | Concrete things that still need work |

## Project Snapshot

- `quark/` builds `quark` as a shared library; `testbed/` builds the host application.
- Heavy dependency on the sibling `cstdlib` project for all low-level primitives.
- Public API lives under `quark/include/quark/`.
- Engine internals are under `quark/src/`.
- Entry point is inverted (app provides `init_application`, engine provides `main`).
- Rendering is strictly Vulkan 1.4 with no backend abstraction.

For any detailed question about how the engine works or how to contribute, start with the relevant file in the `docs/` directory listed above.
