#include "string_helper.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <regex.h>

#include "esp_log.h"

static const char TAG[] = "String";

bool is_valid_ip(const char *str) {
    regex_t regex;
    int reti;

    reti = regcomp(&regex, "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$", REG_EXTENDED);
    if (reti) {
        ESP_LOGI(TAG, "Cannot compile regular expression");
        return false;
    }

    reti = regexec(&regex, str, 0, NULL, 0);

    if (!reti) {
        regfree(&regex);
        return true;
    } else if (reti == REG_NOMATCH) {
        regfree(&regex);
        return false;
    } else {
        char error_message[100];
        regerror(reti, &regex, error_message, sizeof(error_message));
        ESP_LOGI(TAG, "Regular expression error: %s\n", error_message);
        regfree(&regex);
        return false;
    }
}

const char *find_ip(char *str) {
    char* token;
    char* rest = str;

    while ((token = strtok_r(rest, " ", &rest))) {
        if (is_valid_ip(token)) {
            return token;
        }
    }

    return NULL;
}

void remove_newline(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0'; // Erstat newline-tegn med null-terminator
            break; // Stop ved første forekomst af newline-tegn
        }
    }
}
