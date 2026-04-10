# fml - Dual Pane File Manager

A tiny, portable, text-mode dual-pane file manager written in C. Inspired by classic file managers like Midnight Commander and Norton Commander.

## Features

- **Dual independent panels** - Each panel maintains its own directory, allowing easy file management between two locations
- **File operations**: Copy (F5), Move/Rename (F6), Delete (F8), Create directory (F7) with batch operations on selected files
- **File preview**: Text files show content with syntax highlighting by file type; binaries show hex dump
- **Archive support**: Browse inside zip, tar, tar.gz, tar.bz2, tar.xz, 7z, and rar archives
- **Quick search**: Press `/` to filter files by pattern in the current directory
- **Advanced selection**: Toggle individual files with `Insert` or `*`, select/deselect by pattern with `+` and `\`
- **Sorting**: Sort by name, size, date, or extension with `s`; reverse order with `S`
- **Hidden files**: Toggle visibility with `.`
- **Shell access**: Spawn a shell in the current directory with `!` or `Ctrl+S`
- **Clipboard**: Copy filename (`Ctrl+C`) or full path (`Ctrl+Y`) to clipboard
- **Smart cursor**: Cursor position is preserved when navigating up/down directories, refreshing after file operations, and toggling hidden files
- **Page navigation**: Page Up/Page Down/Home/End with proper scroll tracking
- **Cross-filesystem moves**: Files are automatically copied and deleted when moving across filesystems

## Building

### Dependencies

- C11 compiler (gcc, clang, etc.)
- ncurses library

### Linux/macOS

```bash
make
```

### Debian/Ubuntu

```bash
sudo apt-get install build-essential libncurses5-dev libncursesw5-dev
make
```

### Fedora/RHEL

```bash
sudo dnf install gcc ncurses-devel
make
```

### macOS

```bash
# Using Homebrew
brew install ncurses
# Or use Xcode command line tools (includes ncurses)
make
```

### Windows (WSL or MSYS2)

```bash
# WSL
sudo apt install build-essential libncurses5-dev
make

# MSYS2
pacman -S mingw-w64-x86_64-ncurses
make
```

## Usage

```
fml [directory]
```

If no directory is specified, fml starts in the current working directory. You can also specify a starting directory for the left panel.

## Interface

```
+--------------------------------+--------------------------------+
| Left Panel                     | Right Panel                    |
| /path/to/directory             | /other/path                    |
|--------------------------------|--------------------------------|
| file1.txt              1.2K    | document.pdf          256K      |
| file2.log                512B  | image.png             512K      |
| directory/             <DIR>  | archive.zip           1.2M      |
|                                |                                |
|                                |                                |
+--------------------------------+--------------------------------+
| 1 files, 0 sel, 1K | Sort:name | F1Help F3View F5Copy F6Move   |
+--------------------------------+--------------------------------+
```

The bottom toolbar shows available function key actions.

## Key Bindings

### Navigation

| Key | Action |
|-----|--------|
| `Up/Down/k/j` | Move cursor up/down |
| `PgUp/PgDn` | Scroll up/down one page |
| `Home/End` | Jump to first/last entry |
| `Enter/l` | Enter directory or open file |
| `Backspace/h` | Go to parent directory (restores cursor to previous directory) |
| `Tab` | Switch between left and right panels |

### Sorting & View

| Key | Action |
|-----|--------|
| `s` | Cycle sort mode (name → size → date → ext → name...) |
| `S` | Reverse sort order |
| `.` | Toggle hidden files |
| `F3` | Toggle full-screen file preview |

### File Operations

| Key | Action |
|-----|--------|
| `F5` | Copy selected files (or current file) to the other panel; extracts archives |
| `F6` | Move selected files to other panel, or rename current file |
| `F7` | Create new directory |
| `F8/Del` | Delete selected files (or current file) |

### Selection

| Key | Action |
|-----|--------|
| `Insert/*` | Toggle selection on current item |
| `+` | Select files matching pattern |
| `\` | Deselect files matching pattern |

### Shell & Clipboard

| Key | Action |
|-----|--------|
| `!` | Spawn shell in current directory |
| `Ctrl+S` | Spawn shell in current directory |
| `Ctrl+C` | Copy filename to clipboard |
| `Ctrl+Y` | Copy full path to clipboard |

### Other

| Key | Action |
|-----|--------|
| `/` | Quick search/filter |
| `F1` | Show help |
| `q/F9/F10` | Quit |

## Keyboard Shortcuts Summary

The bottom toolbar displays: `F1Help F3View F5Copy F6Move F7MkDir F8Del F9Quit`

## Configuration

fml has no configuration files. All settings are controlled via keyboard shortcuts.

## Troubleshooting

### Terminal too small
fml requires a minimum terminal size of 80x24 characters. If your terminal is smaller, resize it or use a different terminal application.

### Colors not displaying properly
Ensure your terminal supports 256 colors and has appropriate color schemes configured. Try setting `TERM=xterm-256color`.

### Archive contents not showing
Make sure you have the necessary archive utilities installed:
- zip/unzip for .zip files
- tar for .tar files  
- gzip/gunzip for .gz files
- bzip2/bunzip2 for .bz2 files
- xz/unxz for .xz files
- 7z for .7z files
- unrar for .rar files

## License

MIT License
