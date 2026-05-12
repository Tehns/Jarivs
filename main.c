#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"

struct Response {
    char *data;
    size_t size;
};

static size_t callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    struct Response *r = (struct Response *)userp;
    r->data = realloc(r->data, r->size + total + 1);
    memcpy(r->data + r->size, contents, total);
    r->size += total;
    r->data[r->size] = 0;
    return total;
}

void ask_gemini(const char *prompt) {
    CURL *curl;
    CURLcode res;
    struct Response response = {0};
    response.data = malloc(1);
    response.size = 0;
    char json[2048];
    snprintf(json, sizeof(json),
        "{\"contents\":[{\"parts\":[{\"text\":\"%s\"}]}]}",
        prompt);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "content-type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=YOUR_API_KEY_HERE");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        char *start = strstr(response.data, "\"text\": \"");
        if (start) {
            start += 9;
            char *end = strstr(start, "\"");
            if (end) {
                char command[256];
                strncpy(command, start, end - start);
                command[end - start] = 0;
                *end = 0;
                printf(BLUE "Jarvis suggests: %s\n" RESET, command);
                printf(CYAN "Run it? (y/n) > " RESET);
                char choice[4];
                fgets(choice, sizeof(choice), stdin);
                if (choice[0] == 'y') {
                    system(command);
                }
            }
        } else {
            printf("Gemini: %s\n", response.data);
        }
        free(response.data);
    }
}

void startup_animation() {
    char *frames[] = {"J", "JA", "JAR", "JARV", "JARVI", "JARVIS"};
    for (int i = 0; i < 6; i++) {
        printf("\r" GREEN "%s" RESET, frames[i]);
        fflush(stdout);
        usleep(150000);
    }
    printf("\n");
}

void chat_gemini(const char *input) {
    char prompt[1024];
    snprintf(prompt, sizeof(prompt),
        "You are Jarvis, a helpful terminal assistant. Reply in plain text only, no markdown, no asterisks, no headers. Keep it short. User says: %s",
        input);

    // same as ask_gemini but no "run it?" prompt
    CURL *curl;
    struct Response response = {0};
    response.data = malloc(1);
    response.size = 0;
    char json[2048];
    snprintf(json, sizeof(json),
        "{\"contents\":[{\"parts\":[{\"text\":\"%s\"}]}]}",
        prompt);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "content-type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=YOUR_API_KEY_HERE");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        char *start = strstr(response.data, "\"text\": \"");
        if (start) {
            start += 9;
            char *end = strstr(start, "\"");
            if (end) *end = 0;
            printf(GREEN "Jarvis: %s\n" RESET, start);
        }
        free(response.data);
    }
}

void show_sysinfo() {

    FILE *f = fopen("/proc/meminfo", "r");
    long total = 0, available = 0;
    char key[64];
    long value;
    char unit[16];
    while (fscanf(f, "%s %ld %s", key, &value, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) total = value;
        if (strcmp(key, "MemAvailable:") == 0) available = value;
    }
    fclose(f);
    long used = (total - available) / 1024;
    long total_mb = total / 1024;
    printf(CYAN "RAM: %ldMB / %ldMB\n" RESET, used, total_mb);

    FILE *temp_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temp_file) {
        int temp;
        fscanf(temp_file, "%d", &temp);
        fclose(temp_file);
        printf(CYAN "CPU Temp: %d°C\n" RESET, temp / 1000);

    FILE *df = popen("df -h / | tail -1", "r");
    if (df) {
        char disk[256];
        fgets(disk, sizeof(disk), df);
        pclose(df);
        char dev[64], size[16], used_d[16], avail[16], use[16], mount[64];
        sscanf(disk, "%s %s %s %s %s %s", dev, size, used_d, avail, use, mount);
        printf(CYAN "Disk: %s used / %s total (%s)\n" RESET, used_d, size, use);
}
}
}

void show_weather() {
    CURL *curl;
    struct Response response = {0};
    response.data = malloc(1);
    response.size = 0;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://wttr.in/Kharkiv?format=%25l:+%25C+%25t+Humidity:+%25h+Wind:+%25w");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        printf(CYAN "Weather: %s\n" RESET, response.data);
        free(response.data);
    }
}

int main() {
    startup_animation();

    show_sysinfo();

    srand(time(NULL));

    FILE *file = fopen("/home/teemo/.bash_history", "r");
    int i = 0;
    char line[256];
    char history[100][256];
    while (fgets(line, sizeof(line), file) != NULL) {
        strcpy(history[i % 100], line);
        i++;}
    for (int j = 0; j < 10; j++) {
    int slot = (i + j) % 10;
    printf("%d: %s", j, history[slot]);
}

    while (1) {
    char input[256];
    printf(CYAN "What can I do for you? > " RESET);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    if (strcmp(input, "weather") == 0) {
    show_weather();
    continue;
}

    if (strcmp(input, "close") == 0) {
        char *goodbyes[] = {
        "Have a nice day! \n",
        "See you later! \n",
        "Goodbye! Stay safe! \n",
        "Cya! Don't forget to touch grass \n",
        "Take care! \n",
        "Farewell! \n",
        "Until next time! \n",
        "Goodbye! \n",
        "See you next time! \n",
        "Shutting down... \n"
};
    printf(GREEN "%s" RESET, goodbyes[rand() % 10]);
    break;
}

    if (strcmp(input, "talk") == 0) {
    printf(GREEN "Let's talk! (type 'back' to return)\n" RESET);
    while (1) {
        char talk_input[256];
        printf(CYAN "You > " RESET);
        fgets(talk_input, sizeof(talk_input), stdin);
        talk_input[strcspn(talk_input, "\n")] = 0;
        if (strcmp(talk_input, "back") == 0) {
            printf(GREEN "Back to command mode!\n" RESET);
            break;
        }
        chat_gemini(talk_input);
    }
    continue;
}

    if (strcmp(input, "help") == 0) {
    printf(GREEN "Commands:\n" RESET);
    printf(YELLOW "  weather" RESET " - show current weather\n");
    printf(YELLOW "  talk" RESET "    - chat with Jarvis\n");
    printf(YELLOW "  close" RESET "   - exit Jarvis\n");
    printf(YELLOW "  help" RESET "    - show this menu\n");
    continue;
}

    char shown[100][256];
    int shown_count = 0;
    for (int j = 0; j < 10; j++) {
        if (strstr(history[j], input) != NULL) {
            int already_shown = 0;
            for (int k = 0; k < shown_count; k++) {
                if (strstr(shown[k], history[j]) != NULL) {
                    already_shown = 1;
                }
            }
            if (already_shown == 0) {
                printf(YELLOW "suggestion: %s" RESET, history[j]);
                strcpy(shown[shown_count], history[j]);
                shown_count++;
        }
    }
    }


        char context[20000] = "My recent bash commands: ";
    for (int j = 0; j < 10; j++) {
        strcat(context, history[j]);
}

    char full_prompt[24000];
    snprintf(full_prompt, sizeof(full_prompt),
        "%s\nSuggest ONE terminal command for: %s\nReply with ONLY the command.",
        context, input);
    ask_gemini(full_prompt);
}
    return 0;
}
