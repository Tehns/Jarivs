# JARVIS — AI Terminal Assistant

> A lightweight, AI-powered terminal assistant for Linux. Written in pure C. Zero bloat.

![Platform](https://img.shields.io/badge/platform-Linux-blue)
![Language](https://img.shields.io/badge/language-C99-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
![Version](https://img.shields.io/badge/version-2.1.0-cyan)

---

## The killer feature

You run a command. It fails. Instead of googling the error:

```bash
make 2>&1 | jarvis explain
```

Jarvis reads the output, finds the root cause, and tells you exactly what to fix. Works with any tool.

```
── Jarvis is reading the error ────────────────

What went wrong: the linker cannot find -lssl because OpenSSL is not installed.
The exact fix: sudo pacman -S openssl  (or: sudo apt install libssl-dev)
Why it happened: your Makefile links against OpenSSL but the library isn't on this system.
```

---

## Features

| Command | What it does |
|---|---|
| `make 2>&1 \| jarvis explain` | Read piped error output and explain it |
| `jarvis alias` | Scan your history, suggest ready-to-paste aliases |
| `jarvis "find big files"` | One-shot AI command suggestion |
| `jarvis` | Interactive mode |
| `jarvis talk` | Full chat with Gemini, rolling context |
| `jarvis weather [city]` | Current weather for any city |
| `jarvis sysinfo` | RAM, CPU temp, disk, uptime |
| `jarvis --version` | Print version |

**Works with bash, zsh, and fish** — auto-detects your history file.

---

## Install

**Dependencies:** `gcc`, `libcurl`

```bash
# Arch Linux
sudo pacman -S curl gcc

# Debian/Ubuntu
sudo apt install gcc libcurl4-openssl-dev

# Fedora
sudo dnf install gcc libcurl-devel
```

```bash
git clone https://github.com/Tehns/Jarvis.git
cd Jarvis
make
sudo make install
```

On first run, Jarvis creates `~/.config/jarvis/config`. Add your Gemini API key:

```ini
api_key=YOUR_GEMINI_API_KEY
city=Kyiv
history_path=/home/you/.bash_history   # auto-detected, usually fine
```

Get a **free** API key at [aistudio.google.com](https://aistudio.google.com).

---

## Usage

### Explain errors (pipe mode)

```bash
make 2>&1 | jarvis explain
cargo build 2>&1 | jarvis explain
gcc foo.c 2>&1 | jarvis explain
npm install 2>&1 | jarvis explain
systemctl start nginx 2>&1 | jarvis explain
```

### Alias suggestions (local, no API needed)

```bash
jarvis alias
```

```
── Alias suggestions ──────────────────────────
   Based on your 187 most recent unique commands.

  alias git_rebase='git rebase -i HEAD~3'    # used 14x
  alias dk_up='docker-compose up -d'         # used 11x
  alias py_venv='source .venv/bin/activate'  # used 8x

  Paste into your ~/.bashrc or ~/.zshrc, then run:
    source ~/.bashrc
```

### One-shot query

```bash
jarvis "compress this folder"
# Jarvis suggests: tar -czf folder.tar.gz folder/
# Run it? (y/n) >
```

### Interactive mode

```
JARVIS  v2.1.0
────────────────────────────────────────────
  Shell history: /home/you/.zsh_history (187 commands)

── System Info ─────────────────────────────
  RAM  : 4231MB / 16384MB (25%)
  CPU  : 52°C
  Disk : 42G used / 120G total (36%)
  Up   : 2d 4h 31m

jarvis > how do I kill a process on port 8080
── Asking Gemini ──────────────────────────────
  Jarvis suggests: fuser -k 8080/tcp
  Run it? (y/n) >
```

---

## Build

```bash
make            # build ./jarvis
make install    # install to /usr/local/bin
make uninstall  # remove
make clean      # remove binary
```

---

## Config reference

Location: `~/.config/jarvis/config`

| Key | Description | Default |
|---|---|---|
| `api_key` | Google Gemini API key | *(required)* |
| `city` | Default city for weather | `Kharkiv` |
| `history_path` | Path to shell history | *(auto-detected)* |

---

## How it works

- **Pure C99**, single `main.c`, only depends on `libcurl`
- History is **deduplicated** before sending to Gemini — no wasted tokens
- **Explain mode** feeds the last 3000 chars of stderr (the relevant part) to Gemini with a structured prompt
- **Alias miner** runs entirely locally — no API call, instant results
- Config lives in `~/.config/jarvis/config`, nothing hardcoded

---

## License

MIT
