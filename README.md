# JARVIS — AI Terminal Assistant

> A lightweight, AI-powered terminal assistant for Linux. Written in pure C.

![Platform](https://img.shields.io/badge/platform-Linux-blue)
![Language](https://img.shields.io/badge/language-C-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

- **AI command suggestions** — describe what you want, get the right command back
- **Shell history awareness** — Gemini sees your recent commands for context-aware answers
- **Multi-shell support** — auto-detects bash, zsh, fish history
- **Talk mode** — full chat with Gemini, with rolling conversation context
- **Weather** — `weather [city]` with any city, no hardcoding
- **System info** — RAM, CPU temp, disk usage, uptime on startup
- **Config file** — API key and preferences stored in `~/.config/jarvis/config`
- **Zero bloat** — single `main.c`, only depends on `libcurl`

---

## Install

**Dependencies:** `gcc`, `libcurl`

```bash
# Arch Linux
sudo pacman -S curl gcc

# Debian/Ubuntu
sudo apt install gcc libcurl4-openssl-dev
```

**Build & install:**

```bash
git clone https://github.com/Tehns/Jarvis.git
cd Jarvis
make
sudo make install
```

**First run:**

```bash
jarvis
```

On first run, Jarvis creates `~/.config/jarvis/config`. Edit it and add your Gemini API key:

```ini
api_key=YOUR_GEMINI_API_KEY
city=Kyiv
history_path=/home/you/.bash_history
```

Get a free API key at [aistudio.google.com](https://aistudio.google.com).

---

## Usage

```
jarvis > <anything>       ask Gemini for a terminal command
jarvis > weather          show weather for your configured city
jarvis > weather London   show weather for any city
jarvis > sysinfo          show RAM, CPU, disk, uptime
jarvis > talk             chat mode with Gemini
jarvis > help             show all commands
jarvis > close            exit
```

### Example

```
jarvis > find large files taking up space
── History matches ────────────────────────
  → du -sh /*
── Asking Gemini ──────────────────────────
  Jarvis suggests: find / -type f -size +100M -exec ls -lh {} \;
  Run it? (y/n) > y
```

---

## Config

Config file location: `~/.config/jarvis/config`

| Key            | Description                          | Default            |
|----------------|--------------------------------------|--------------------|
| `api_key`      | Google Gemini API key                | *(required)*       |
| `city`         | Default city for weather             | `Kharkiv`          |
| `history_path` | Path to your shell history file      | *(auto-detected)*  |

---

## Build from source

```bash
make          # build
make install  # install to /usr/local/bin
make uninstall # remove
make clean    # remove binary
```

---

## Dependencies

- `libcurl` — HTTP requests to Gemini and wttr.in
- `gcc` — C99 or later

No other runtime dependencies.

---

## License

MIT — do whatever you want with it.
