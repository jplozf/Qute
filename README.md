# Qute

**Qute** is a lightweight, tabbed text and code editor written in C++17 with Qt 6 Widgets.
It focuses on the everyday needs of a developer's editor: syntax highlighting, a function outline,
smart indentation, a `tail -f`-style follow mode, favorites, Git integration, and a system tray icon.

> Author: [jplozf](https://github.com/jplozf)

## Features

- **Tabbed editing** with close buttons, a "modified" marker (`*`) on unsaved tabs, and a tab context menu
  (close, close others, close all, copy file path, add to favorites).
- **Syntax highlighting** for Plain Text, C / C++, Python, Go, Bash, JavaScript, Java, HTML and XML.
  The language is detected from the file name and can be changed from the **Language** menu.
- **Function outline**: a drop-down in the toolbar lists the functions of the current file and jumps to the one you select.
- **Find / Replace** dialog and **Go to line**.
- **Smart indentation**: a tab/space policy per language, automatic detection of the indentation used by a file,
  and one-click conversion between tabs and spaces.
- **Follow mode** (like `tail -f`) to watch a file as it grows, for example a log file.
- **New file from template** (File menu).
- **Favorites** menu to keep your most-used files (and followed files) at hand, plus an **Open Recent** list.
- **Git menu** for tracking the files you edit.
- **Tools menu** for extra utilities.
- **Themes, font and zoom**: choose your editor font and theme in the preferences, and zoom with `Ctrl++` / `Ctrl+-`.
- **Session restore**: open files, their language, follow mode, window geometry and preferences are restored on the next launch.
- **System tray**: optionally minimize Qute to the tray instead of quitting when you close the window.
- **Printing** of the current document.
- **Command-line friendly**: pass one or more files as arguments to open them at startup.
- **Update check** performed shortly after launch.
- **Status bar** showing the file path, size, modified flag, read/write state, position in the file (%),
  indentation policy and cursor line/column.

## Requirements

- A C++17 compiler (GCC, Clang or MSVC)
- [CMake](https://cmake.org/) 3.16 or newer
- [Qt 6](https://www.qt.io/) with the **Widgets**, **PrintSupport** and **Network** modules
- Git (optional): used at build time to embed the commit count and hash in the version string

On Debian / Ubuntu, for example:

```bash
sudo apt install build-essential cmake git qt6-base-dev
```

## Build

```bash
git clone https://github.com/jplozf/Qute.git
cd Qute
cmake -S . -B build
cmake --build build
```

If CMake cannot find Qt 6, point it to your installation:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
```

## Usage

```bash
./build/Qute                       # start with the previous session
./build/Qute file1.cpp notes.txt   # open specific files
```

The version shown in the toolbar has the form `vX.<commit count>-<commit hash>`.
Double-click it to open the *About* box.

### Keyboard shortcuts

Standard platform shortcuts are used (`Ctrl` becomes `Cmd` on macOS).

| Action                  | Shortcut                      |
|-------------------------|-------------------------------|
| New file                | `Ctrl+N`                      |
| Open file               | `Ctrl+O`                      |
| Save / Save As          | `Ctrl+S` / `Ctrl+Shift+S`     |
| Print                   | `Ctrl+P`                      |
| Close tab               | `Ctrl+W`                      |
| Quit                    | `Ctrl+Q`                      |
| Cut / Copy / Paste      | `Ctrl+X` / `Ctrl+C` / `Ctrl+V`|
| Find / Replace          | `Ctrl+F`                      |
| Go to line              | `Ctrl+G`                      |
| Zoom in / out           | `Ctrl++` / `Ctrl+-`           |
| Next / previous tab     | `Ctrl+PageDown` / `Ctrl+PageUp` |
| Toggle full screen      | `F11`                         |

## Project layout

| File                                   | Role                                              |
|----------------------------------------|---------------------------------------------------|
| `main.cpp`                             | Application entry point                           |
| `MainWindow.cpp/.h`                    | Main window, menus, tabs, status bar, tray        |
| `CodeEditor.cpp/.h`                    | Editor widget (line numbers, indentation, zoom)   |
| `SyntaxHighlighter.cpp/.h`             | Syntax highlighting rules and themes              |
| `FindReplaceDialog.cpp/.h`             | Find / Replace dialog                             |
| `SettingsDialog.cpp/.h`                | Preferences dialog                                |
| `constants.cpp/.h`                     | Application-wide constants                        |
| `Qute.qrc`, `icons/`                   | Qt resources and icons                            |
| `CMakeLists.txt`                       | Build configuration                               |

## Contributing

Bug reports and pull requests are welcome. Please open an issue to discuss larger changes first.

## License

This project is distributed under the [MIT License](License.md).
