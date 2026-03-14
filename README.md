# fml - Dual Pane File Manager

A tiny, portable, text-mode dual-pane file manager written in C.

## Features

- **Dual independent panels** - each maintains its own directory
- **File operations**: Copy (F5), Move (F6), Delete (F8), Create dir (F7)
- **File preview**: Text files show content; binaries show hex dump
- **Quick search**: `/pattern` to filter current directory
- **Selection**: `*` toggle, `+pattern` select, `\pattern` deselect

## Building

```bash
make
```

## Usage

```
fml [directory]
```

## Key Bindings

| Key | Action |
|-----|--------|
|Up/Down/k/j | Move cursor |
|Enter | Enter directory |
|Backspace | Go to parent |
|Tab | Switch panels |
|F5 | Copy selected |
|F6 | Move/rename |
|F7 | Create directory |
|F8/Del | Delete |
|F3 | Toggle full preview |
|/ | Quick search |
|* | Toggle selection |
|q/F10 | Quit |

## Requirements

- C11 compiler
- ncurses library

## License

MIT# fml
