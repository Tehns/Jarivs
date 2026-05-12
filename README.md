# JARVIS - AI Terminal Assistant

A terminal-based AI assistant for Linux, written in C.

## Features
- Real-time bash history tracking
- AI-powered command suggestions using Google Gemini
- Pattern matching from command history
- System info on startup (RAM, CPU temp, disk)
- Weather for any city
- Talk mode — chat with Gemini directly
- Run suggested commands directly
- Startup animation and colors

## Dependencies
- libcurl
- gcc

## Build
\```bash
gcc main.c -o jarvis -lcurl
\```

## Install
\```bash
sudo cp jarvis /usr/local/bin/jarvis
\```

## Usage
```bash
jarvis
```

### Commands
- `weather [city]` — show weather
- `talk` — chat mode with Gemini
- `help` — show all commands
- `close` — exit

## Note
You need a Google Gemini API key. Replace `YOUR_KEY` in main.c with your key from aistudio.google.com
