# AGENTS.md — CS Server Manager

## Project Overview

**CS Server Manager** is a **Qt6/C++23 desktop GUI** (Linux) for managing Counter-Strike: Condition Zero dedicated servers. It drives the HLDS (Half-Life Dedicated Server) binary via `QProcess`, scans the game installation for available maps, and provides a clean interface for configuring bots, server settings, and monitoring server state.

The app has **no direct network or server logic** — all server operations are performed by spawning and communicating with the `hlds_run` (or `hlds`) binary as a child process.

## Planned Architecture

```
MainWindow (QStackedWidget or QTabWidget)
  ├── pages/
  │     ├── ServerPage       — start/stop server, port, maxplayers, hostname
  │     ├── MapPage          — scan game dir for .bsp files, current/next map
  │     ├── BotPage          — bot count, difficulty, auto-fill, team balance
  │     └── ConsolePage      — live HLDS stdout feed, command input
  └── ServerManager          — owns the HLDS QProcess, emits state signals
        └── MapScanner       — finds .bsp files in the game's maps/ directory
```

**Key data flows:**
- UI → `ServerManager` → `hlds_run` subprocess → signals back to UI
- `ServerManager` reads stdout from HLDS and parses status updates
- `MapScanner` reads the filesystem to enumerate installed maps
- `AppConfig` persists user preferences to `~/.config/CSServerManager/app.json`

## Build & Run

```bash
# Configure (from src/)
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
# Build
cmake --build cmake-build-debug
# Run
./cmake-build-debug/cs_server_manager
```

## Flatpak Sandboxing

All `QProcess` spawning **must** go through `buildHostCommand()` (`cli/flatpakUtils.h`):

```cpp
auto [prog, args] = buildHostCommand("hlds_run", {"-game", "czero", "+port", "27015"});
process->start(prog, args);
```

This transparently wraps commands with `flatpak-spawn --host` when inside a Flatpak sandbox (detected via `$FLATPAK_ID`). Never call `QProcess::start("hlds_run", ...)` directly.

## Key Files

| File | Purpose |
|---|---|
| `appConfig.h/cpp` | App preferences → `~/.config/CSServerManager/app.json` |
| `themeManager.h/cpp` | Applies palette + QSS stylesheet; dark/light/system themes |
| `main.cpp` | App init, version from `version.json`, theme apply, main window |
| `cli/flatpakUtils.h` | `buildHostCommand()` for Flatpak-safe subprocess spawning |
| `cli/platformUtils.h` | Detects Flatpak / AppImage runtime environment |
| `style_dark.qss` | Dark theme styles (CS orange accent `#e87832`) |
| `style_light.qss` | Light theme styles (darker orange `#c96820`) |

## Conventions

- **C++23**, strict conformance (`-extensions OFF`), `#pragma once` everywhere
- **No `.ui` files** — all layouts built programmatically in constructors
- **Singletons** via `static T& instance()`: `AppConfig`
- **Logging**: use `DBG_APP(msg)`, `DBG_CLI(msg)`, `DBG_SETTINGS(msg)` macros (stdout, tagged+timestamped). Never use `qDebug()`.
- **Versioning**: single source of truth is `src/version.json` (key: `app_version`); read at runtime via embedded resource `:/version.json`
- **Themes**: dark and light palettes set in `themeManager.cpp`; all widget-specific styles go in `style_dark.qss` / `style_light.qss` respectively — never hardcode style strings in widget code
- **Language**: American English only — variable names, comments, and UI strings (`color` not `colour`, `canceled` not `cancelled`)

### Code Style

- **No if-init syntax**: do not use `if (init; condition)` — declare the variable on a separate line before the `if`:
  ```cpp
  // OK
  QHBoxLayout* hl = qobject_cast<QHBoxLayout*>(layout());
  if (hl != nullptr) { ... }

  // Not OK
  if (auto* hl = qobject_cast<QHBoxLayout*>(layout()); hl != nullptr) { ... }
  ```
- **Constant placement**: declare `constexpr` constants above the function or class that uses them, not inside function bodies. For `.cpp` files use an anonymous namespace; for class-scope constants use `static constexpr` members.
- **Magic numbers**: never use numeric literals inline — define named constants using `constexpr` (or `static constexpr` at class scope) with `UPPER_SNAKE_CASE` names:
  ```cpp
  // OK
  constexpr int PANEL_WIDTH = 280;
  m_panel->setFixedWidth(PANEL_WIDTH);

  // Not OK
  m_panel->setFixedWidth(280);
  ```
  String literals, boolean literals, enumerators, and the literal `0` when used as a neutral zero are exempt.

- **Brace style**: GNU/Allman — opening brace on its own line for functions, classes, and control structures
- **`auto`**: avoid for simple/obvious types; use explicit types (`int count = 0;`, `QString name = ...`). `auto` is fine where the type is verbose or deduced from a template.
- **Loop bodies**: ALL loops (`for`, `while`) must use curly braces — no single-line unbraced loops
- **No `do`/`while` loops**: use a `while` loop instead
- **`switch` case bodies**: `case` labels indented one level inside `switch`; body starts on the next line after the label:
  ```cpp
  switch (state)
  {
      case ServerState::Running:
          handleRunning();
          break;

      default:
          break;
  }
  ```
- **Boolean negation**: use `== false` instead of `!` in conditions. Prefer `== true` when it improves clarity.
- **Pointer null checks**: always use `== nullptr` or `!= nullptr` explicitly — never rely on implicit pointer-to-bool conversion.
- **Condition bodies**: `if`/`else` bodies must use curly braces **unless** the body is a bare `return;`, `return true;`/`return false;`, `break;`, or `continue;` (same line, no braces). Everything else uses braces:
  ```cpp
  // OK — bare return / break / continue
  if (ok == false) return;
  if (done) break;

  // Not OK — must use braces
  if (x) doSomething();
  if (x) return m_value;
  ```

## Testing

Tests live in `src/tests/` and use **Qt Test** (`QtTest/QtTest`). Each test file maps to one logical unit — the naming convention is `tst_<unit>.cpp`.

**When to write tests:** write a test for any class/function that has pure or near-pure logic — parsers, config helpers, map scanners, utility functions. Do **not** try to test `QWidget` subclasses or `ServerManager` (subprocess-dependent).

**How to register a new test:**

1. Create `src/tests/tst_<unit>.cpp`.
2. Add it to `src/tests/CMakeLists.txt` using the existing `add_qt_test` macro:
   ```cmake
   add_qt_test(tst_myunit
       tst_myunit.cpp
       ../myunit.h
       ../myunit.cpp
   )
   ```

**Test structure** — one `QObject` subclass per file, `QTEST_MAIN` at the bottom:

```cpp
#include <QtTest/QtTest>
#include "myunit.h"

class TstMyUnit : public QObject
{
    Q_OBJECT

private slots:
    void methodName_condition_expectedResult()
    {
        QCOMPARE(MyUnit::doThing("input"), QStringLiteral("expected"));
    }
};

QTEST_MAIN(TstMyUnit)
#include "tst_myunit.moc"
```

**Naming:** `methodName_condition_expectedResult` (e.g. `scanMaps_emptyDir_returnsEmptyList`).
