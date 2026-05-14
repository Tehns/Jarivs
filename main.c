/*
 *   ██╗ █████╗ ██████╗ ██╗   ██╗██╗███████╗
 *   ██║██╔══██╗██╔══██╗██║   ██║██║██╔════╝
 *   ██║███████║██████╔╝██║   ██║██║███████╗
 *██  ██║██╔══██║██╔══██╗╚██╗ ██╔╝██║╚════██║
 *╚█████╔╝██║  ██║██║  ██║ ╚████╔╝ ██║███████║
 * ╚════╝ ╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚═╝╚══════╝
 *
 *  AI-powered terminal assistant for Linux
 *  https://github.com/Tehns/Jarvis
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <curl/curl.h>

/* ─── ANSI colors ─────────────────────────────────────────────────────────── */
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define RESET   "\033[0m"

/* ─── Constants ───────────────────────────────────────────────────────────── */
#define VERSION         "2.0.0"
#define MAX_HISTORY     200
#define MAX_LINE        512
#define MAX_PATH        1024
#define MAX_CONFIG_VAL  256
#define GEMINI_URL      "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=%s"

/* ─── Config ──────────────────────────────────────────────────────────────── */
typedef struct {
    char api_key[MAX_CONFIG_VAL];
    char city[MAX_CONFIG_VAL];
    char history_path[MAX_PATH];
} Config;

/* ─── HTTP response buffer ────────────────────────────────────────────────── */
typedef struct {
    char  *data;
    size_t size;
} Response;

/* ══════════════════════════════════════════════════════════════════════════════
 * HTTP
 * ══════════════════════════════════════════════════════════════════════════════ */

static size_t http_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    Response *r  = (Response *)userp;
    char *ptr    = realloc(r->data, r->size + total + 1);
    if (!ptr) return 0;
    r->data = ptr;
    memcpy(r->data + r->size, contents, total);
    r->size += total;
    r->data[r->size] = '\0';
    return total;
}

static Response *http_post(const char *url, const char *json_body) {
    Response *r = malloc(sizeof(Response));
    r->data = malloc(1);
    r->size = 0;

    CURL *curl = curl_easy_init();
    if (!curl) { free(r->data); free(r); return NULL; }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,           url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     r);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       15L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        printf(RED "Network error: %s\n" RESET, curl_easy_strerror(res));
        free(r->data); free(r);
        return NULL;
    }
    return r;
}

static Response *http_get(const char *url) {
    Response *r = malloc(sizeof(Response));
    r->data = malloc(1);
    r->size = 0;

    CURL *curl = curl_easy_init();
    if (!curl) { free(r->data); free(r); return NULL; }

    curl_easy_setopt(curl, CURLOPT_URL,           url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     r);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        printf(RED "Network error: %s\n" RESET, curl_easy_strerror(res));
        free(r->data); free(r);
        return NULL;
    }
    return r;
}

/* ══════════════════════════════════════════════════════════════════════════════
 * CONFIG
 * ══════════════════════════════════════════════════════════════════════════════ */

static void config_path(char *buf, size_t len) {
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    snprintf(buf, len, "%s/.config/jarvis/config", home);
}

static void config_ensure_dir(void) {
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s/.config/jarvis", home);
    mkdir(dir, 0700);  /* no-op if exists */
}

/* Auto-detect the user's shell history file */
static void detect_history_path(char *buf, size_t len) {
    const char *home = getenv("HOME");
    if (!home) home = "/root";

    /* Check $HISTFILE first */
    const char *histfile = getenv("HISTFILE");
    if (histfile && access(histfile, R_OK) == 0) {
        strncpy(buf, histfile, len - 1);
        return;
    }

    /* Try shell-specific defaults */
    const char *shell = getenv("SHELL");
    char candidates[4][MAX_PATH];
    snprintf(candidates[0], MAX_PATH, "%s/.zsh_history",  home);
    snprintf(candidates[1], MAX_PATH, "%s/.bash_history", home);
    snprintf(candidates[2], MAX_PATH, "%s/.local/share/fish/fish_history", home);
    snprintf(candidates[3], MAX_PATH, "%s/.history",      home);

    /* Prefer shell that matches $SHELL */
    if (shell) {
        if (strstr(shell, "zsh")  && access(candidates[0], R_OK) == 0) { strncpy(buf, candidates[0], len-1); return; }
        if (strstr(shell, "bash") && access(candidates[1], R_OK) == 0) { strncpy(buf, candidates[1], len-1); return; }
        if (strstr(shell, "fish") && access(candidates[2], R_OK) == 0) { strncpy(buf, candidates[2], len-1); return; }
    }

    /* Fallback: first readable */
    for (int i = 0; i < 4; i++) {
        if (access(candidates[i], R_OK) == 0) {
            strncpy(buf, candidates[i], len - 1);
            return;
        }
    }

    strncpy(buf, candidates[1], len - 1); /* last resort: bash */
}

static int config_load(Config *cfg) {
    char path[MAX_PATH];
    config_path(path, sizeof(path));

    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[MAX_PATH];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        char key[64], val[MAX_CONFIG_VAL];
        if (sscanf(line, "%63[^=]=%255[^\n]", key, val) == 2) {
            if (strcmp(key, "api_key")      == 0) strncpy(cfg->api_key,      val, MAX_CONFIG_VAL - 1);
            if (strcmp(key, "city")         == 0) strncpy(cfg->city,         val, MAX_CONFIG_VAL - 1);
            if (strcmp(key, "history_path") == 0) strncpy(cfg->history_path, val, MAX_PATH       - 1);
        }
    }
    fclose(f);
    return 1;
}

static void config_create_default(Config *cfg) {
    config_ensure_dir();
    char path[MAX_PATH];
    config_path(path, sizeof(path));

    char hist[MAX_PATH] = {0};
    detect_history_path(hist, sizeof(hist));

    FILE *f = fopen(path, "w");
    if (!f) {
        printf(RED "Could not create config at %s\n" RESET, path);
        return;
    }
    fprintf(f,
        "# Jarvis configuration\n"
        "# Get your Gemini API key at: https://aistudio.google.com\n"
        "\n"
        "api_key=YOUR_GEMINI_API_KEY\n"
        "city=Kharkiv\n"
        "history_path=%s\n",
        hist);
    fclose(f);

    printf(YELLOW BOLD "First run! Config created at: %s\n" RESET, path);
    printf(YELLOW "Edit it and add your Gemini API key to get started.\n" RESET);
    printf(DIM    "  Get a free key at: https://aistudio.google.com\n" RESET);
}

static int config_init(Config *cfg) {
    memset(cfg, 0, sizeof(Config));

    /* Set defaults */
    strcpy(cfg->city, "Kharkiv");
    detect_history_path(cfg->history_path, sizeof(cfg->history_path));

    if (!config_load(cfg)) {
        config_create_default(cfg);
        return 0;
    }

    if (strcmp(cfg->api_key, "YOUR_GEMINI_API_KEY") == 0 || cfg->api_key[0] == '\0') {
        char path[MAX_PATH];
        config_path(path, sizeof(path));
        printf(RED "No API key set. Edit %s and add your Gemini key.\n" RESET, path);
        printf(DIM "  Get a free key at: https://aistudio.google.com\n" RESET);
        return 0;
    }

    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════════
 * HISTORY
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    char  cmds[MAX_HISTORY][MAX_LINE];
    int   count;
} History;

/* Strip fish history format prefix: "- cmd: actual command" */
static void strip_fish(char *line) {
    if (strncmp(line, "- cmd: ", 7) == 0) {
        memmove(line, line + 7, strlen(line + 7) + 1);
    }
}

/* Strip zsh extended history format: ": 1234567890:0;actual command" */
static void strip_zsh(char *line) {
    if (line[0] == ':' && line[1] == ' ') {
        char *semi = strchr(line, ';');
        if (semi) memmove(line, semi + 1, strlen(semi + 1) + 1);
    }
}

static void history_load(History *h, const char *path) {
    h->count = 0;
    FILE *f = fopen(path, "r");
    if (!f) {
        printf(RED "Could not open history: %s\n" RESET, path);
        return;
    }

    char line[MAX_LINE];
    /* Read all lines, keep last MAX_HISTORY unique ones */
    char all[MAX_HISTORY * 4][MAX_LINE];
    int  total = 0;

    while (fgets(line, sizeof(line), f) && total < MAX_HISTORY * 4) {
        line[strcspn(line, "\n")] = '\0';
        strip_fish(line);
        strip_zsh(line);
        if (line[0] == '\0') continue;
        strncpy(all[total++], line, MAX_LINE - 1);
    }
    fclose(f);

    /* Dedup: take the last MAX_HISTORY unique commands */
    char seen[MAX_HISTORY][MAX_LINE];
    int  seen_count = 0;

    for (int i = total - 1; i >= 0 && h->count < MAX_HISTORY; i--) {
        int dup = 0;
        for (int j = 0; j < seen_count; j++) {
            if (strcmp(seen[j], all[i]) == 0) { dup = 1; break; }
        }
        if (!dup) {
            strncpy(seen[seen_count++],       all[i], MAX_LINE - 1);
            strncpy(h->cmds[h->count++], all[i], MAX_LINE - 1);
        }
    }
}

static void history_print_recent(const History *h, int n) {
    printf(DIM "── Recent commands " RESET DIM "─────────────────────────────\n" RESET);
    int start = 0;
    int end   = (n < h->count) ? n : h->count;
    for (int i = start; i < end; i++) {
        printf(DIM "  %2d  " RESET "%s\n", i + 1, h->cmds[i]);
    }
    printf("\n");
}

/* Build a compact context string from history (most recent first, deduped) */
static void history_build_context(const History *h, char *buf, size_t max) {
    size_t pos = 0;
    int    n   = (h->count < 50) ? h->count : 50; /* top 50 recent cmds */

    for (int i = 0; i < n && pos < max - 64; i++) {
        /* Sanitize for JSON: escape quotes and backslashes */
        for (int j = 0; h->cmds[i][j] && pos < max - 4; j++) {
            char c = h->cmds[i][j];
            if (c == '"' || c == '\\') buf[pos++] = '\\';
            if (c != '\r' && c != '\n') buf[pos++] = c;
        }
        buf[pos++] = '\n';
    }
    buf[pos] = '\0';
}

/* ══════════════════════════════════════════════════════════════════════════════
 * GEMINI
 * ══════════════════════════════════════════════════════════════════════════════ */

/* Extract "text" field value from Gemini JSON response */
static char *gemini_extract_text(const char *json) {
    static char result[8192];
    result[0] = '\0';

    const char *needle = "\"text\": \"";
    char *start = strstr(json, needle);
    if (!start) return NULL;
    start += strlen(needle);

    /* Unescape into result */
    int i = 0;
    while (*start && i < (int)sizeof(result) - 1) {
        if (*start == '\\' && *(start + 1)) {
            start++;
            switch (*start) {
                case 'n':  result[i++] = '\n'; break;
                case 't':  result[i++] = '\t'; break;
                case '"':  result[i++] = '"';  break;
                case '\\': result[i++] = '\\'; break;
                default:   result[i++] = *start; break;
            }
        } else if (*start == '"') {
            break;
        } else {
            result[i++] = *start;
        }
        start++;
    }
    result[i] = '\0';
    return result;
}

/* Send a prompt to Gemini, return malloc'd response text (caller frees) */
static char *gemini_ask(const Config *cfg, const char *prompt) {
    /* Build URL */
    char url[512];
    snprintf(url, sizeof(url), GEMINI_URL, cfg->api_key);

    /* Build JSON — escape the prompt */
    char *json = malloc(strlen(prompt) * 2 + 256);
    if (!json) return NULL;

    char *escaped = malloc(strlen(prompt) * 2 + 1);
    if (!escaped) { free(json); return NULL; }

    int j = 0;
    for (int i = 0; prompt[i]; i++) {
        if (prompt[i] == '"'  || prompt[i] == '\\') escaped[j++] = '\\';
        if (prompt[i] != '\r') escaped[j++] = prompt[i];
    }
    escaped[j] = '\0';

    sprintf(json, "{\"contents\":[{\"parts\":[{\"text\":\"%s\"}]}]}", escaped);
    free(escaped);

    Response *resp = http_post(url, json);
    free(json);

    if (!resp) return NULL;

    char *text = gemini_extract_text(resp->data);
    char *result = text ? strdup(text) : NULL;

    free(resp->data);
    free(resp);
    return result;
}

/* ══════════════════════════════════════════════════════════════════════════════
 * COMMANDS
 * ══════════════════════════════════════════════════════════════════════════════ */

static void cmd_suggest(const Config *cfg, const History *h, const char *query) {
    /* First: show local history matches */
    int local_hits = 0;
    for (int i = 0; i < h->count; i++) {
        if (strstr(h->cmds[i], query)) {
            if (local_hits == 0)
                printf(YELLOW "── History matches ─────────────────────────\n" RESET);
            printf(YELLOW "  → " RESET "%s\n", h->cmds[i]);
            if (++local_hits >= 5) break;
        }
    }

    /* Then: ask Gemini */
    printf(CYAN "── Asking Gemini " RESET DIM "──────────────────────────────\n" RESET);

    char context[4096];
    history_build_context(h, context, sizeof(context));

    char prompt[6000];
    snprintf(prompt, sizeof(prompt),
        "You are a Linux terminal expert. "
        "The user's recent shell history (most recent first):\n%s\n"
        "Suggest ONE shell command for: %s\n"
        "Reply with ONLY the raw command. No explanation, no markdown, no backticks.",
        context, query);

    char *answer = gemini_ask(cfg, prompt);
    if (!answer) return;

    /* Trim leading/trailing whitespace */
    char *trimmed = answer;
    while (*trimmed == ' ' || *trimmed == '\n' || *trimmed == '\t') trimmed++;
    char *end = trimmed + strlen(trimmed) - 1;
    while (end > trimmed && (*end == ' ' || *end == '\n' || *end == '\t')) *end-- = '\0';

    printf(BLUE BOLD "  Jarvis suggests: " RESET BOLD "%s\n" RESET, trimmed);
    printf(CYAN "  Run it? (y/n) > " RESET);

    char choice[4];
    if (fgets(choice, sizeof(choice), stdin) && choice[0] == 'y') {
        system(trimmed);
    }

    free(answer);
}

static void cmd_talk(const Config *cfg) {
    printf(GREEN "Talk mode — type " RESET BOLD "'back'" RESET GREEN " to return.\n\n" RESET);

    char session[16384] = {0}; /* keep conversation context */
    int  first = 1;

    while (1) {
        printf(CYAN "You > " RESET);
        char input[512];
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "back") == 0) {
            printf(GREEN "Back to command mode.\n\n" RESET);
            break;
        }
        if (input[0] == '\0') continue;

        /* Build prompt with rolling context */
        char prompt[8192];
        if (first) {
            snprintf(prompt, sizeof(prompt),
                "You are Jarvis, a sharp and helpful Linux terminal assistant. "
                "Reply in plain text only — no markdown, no asterisks, no headers. "
                "Be concise. User: %s", input);
            first = 0;
        } else {
            snprintf(prompt, sizeof(prompt),
                "You are Jarvis, a Linux terminal assistant. "
                "Conversation so far:\n%s\nUser: %s\n"
                "Reply in plain text, no markdown, be concise.",
                session, input);
        }

        char *answer = gemini_ask(cfg, prompt);
        if (!answer) continue;

        printf(GREEN "Jarvis: " RESET "%s\n\n", answer);

        /* Append to session context (capped) */
        size_t sl = strlen(session), al = strlen(answer), il = strlen(input);
        if (sl + al + il + 32 < sizeof(session) - 1) {
            snprintf(session + sl, sizeof(session) - sl,
                "User: %s\nJarvis: %s\n", input, answer);
        }

        free(answer);
    }
}

static void cmd_weather(const Config *cfg, const char *city) {
    if (!city || city[0] == '\0') city = cfg->city;

    char url[512];
    snprintf(url, sizeof(url),
        "https://wttr.in/%s?format=%%25l:+%%25C+%%25t+Humidity:+%%25h+Wind:+%%25w", city);

    Response *r = http_get(url);
    if (!r) return;

    printf(CYAN "── Weather " RESET DIM "────────────────────────────────────\n" RESET);
    printf("  %s\n\n", r->data);
    free(r->data);
    free(r);
}

static void cmd_sysinfo(void) {
    printf(CYAN "── System Info " RESET DIM "─────────────────────────────────\n" RESET);

    /* RAM */
    FILE *f = fopen("/proc/meminfo", "r");
    if (f) {
        long total = 0, available = 0;
        char key[64]; long value; char unit[16];
        while (fscanf(f, "%63s %ld %15s", key, &value, unit) >= 2) {
            if (strcmp(key, "MemTotal:")     == 0) total     = value;
            if (strcmp(key, "MemAvailable:") == 0) available = value;
        }
        fclose(f);
        long used_mb  = (total - available) / 1024;
        long total_mb = total / 1024;
        int  pct      = total ? (int)(100.0 * (total - available) / total) : 0;
        printf("  RAM  : %ldMB / %ldMB (%d%%)\n", used_mb, total_mb, pct);
    }

    /* CPU temp */
    const char *temp_paths[] = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp",
        NULL
    };
    for (int i = 0; temp_paths[i]; i++) {
        FILE *tf = fopen(temp_paths[i], "r");
        if (tf) {
            int temp = 0;
            fscanf(tf, "%d", &temp);
            fclose(tf);
            if (temp > 0) { printf("  CPU  : %d°C\n", temp / 1000); break; }
        }
    }

    /* Disk */
    FILE *df = popen("df -h / 2>/dev/null | tail -1", "r");
    if (df) {
        char dev[64], size[16], used[16], avail[16], use[16], mount[64];
        if (fscanf(df, "%63s %15s %15s %15s %15s %63s",
                   dev, size, used, avail, use, mount) == 6) {
            printf("  Disk : %s used / %s total (%s)\n", used, size, use);
        }
        pclose(df);
    }

    /* Uptime */
    FILE *up = fopen("/proc/uptime", "r");
    if (up) {
        double uptime_sec;
        fscanf(up, "%lf", &uptime_sec);
        fclose(up);
        int days  = (int)(uptime_sec / 86400);
        int hours = (int)(uptime_sec / 3600)  % 24;
        int mins  = (int)(uptime_sec / 60)    % 60;
        if (days > 0)
            printf("  Up   : %dd %dh %dm\n", days, hours, mins);
        else
            printf("  Up   : %dh %dm\n", hours, mins);
    }

    printf("\n");
}

static void cmd_help(void) {
    printf(CYAN "── Commands " RESET DIM "───────────────────────────────────\n" RESET);
    printf(YELLOW "  %-18s" RESET " ask Gemini for a command\n",   "<anything>");
    printf(YELLOW "  %-18s" RESET " show weather (default city)\n", "weather");
    printf(YELLOW "  %-18s" RESET " weather for a specific city\n", "weather [city]");
    printf(YELLOW "  %-18s" RESET " show system info\n",            "sysinfo");
    printf(YELLOW "  %-18s" RESET " chat with Jarvis\n",            "talk");
    printf(YELLOW "  %-18s" RESET " show this menu\n",              "help");
    printf(YELLOW "  %-18s" RESET " exit\n\n",                      "close / exit / q");
}

/* ══════════════════════════════════════════════════════════════════════════════
 * UI
 * ══════════════════════════════════════════════════════════════════════════════ */

static void startup_animation(void) {
    const char *frames[] = { "J","JA","JAR","JARV","JARVI","JARVIS" };
    for (int i = 0; i < 6; i++) {
        printf("\r" GREEN BOLD "%s" RESET, frames[i]);
        fflush(stdout);
        usleep(120000);
    }
    printf(DIM "  v%s\n" RESET, VERSION);
    printf(DIM "────────────────────────────────────────────\n" RESET);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * MAIN
 * ══════════════════════════════════════════════════════════════════════════════ */

int main(void) {
    startup_animation();

    /* Load config */
    Config cfg;
    if (!config_init(&cfg)) {
        printf(DIM "\nFix your config and restart Jarvis.\n" RESET);
        return 1;
    }

    /* Load history */
    History history;
    history_load(&history, cfg.history_path);

    /* Startup info */
    printf(DIM "  Shell history: %s (%d commands)\n\n" RESET,
           cfg.history_path, history.count);
    cmd_sysinfo();
    history_print_recent(&history, 10);

    /* Main loop */
    const char *farewells[] = {
        "Have a nice day!", "See you later!", "Goodbye! Stay safe!",
        "Cya! Don't forget to touch grass.", "Take care!", "Farewell!",
        "Until next time!", "Shutting down..."
    };
    srand((unsigned)time(NULL));

    while (1) {
        printf(CYAN "jarvis" RESET DIM " > " RESET);
        fflush(stdout);

        char input[512];
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';

        if (input[0] == '\0') continue;

        if (strcmp(input, "close") == 0 ||
            strcmp(input, "exit")  == 0 ||
            strcmp(input, "q")     == 0) {
            printf(GREEN "%s\n" RESET,
                   farewells[rand() % (sizeof(farewells) / sizeof(*farewells))]);
            break;
        }

        if (strcmp(input, "help") == 0) { cmd_help();    continue; }
        if (strcmp(input, "sysinfo") == 0) { cmd_sysinfo(); continue; }
        if (strcmp(input, "talk")    == 0) { cmd_talk(&cfg); continue; }

        if (strncmp(input, "weather", 7) == 0) {
            char *city = input + 7;
            while (*city == ' ') city++;
            cmd_weather(&cfg, city);
            continue;
        }

        /* Default: suggest a command */
        cmd_suggest(&cfg, &history, input);
    }

    return 0;
}
