#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

struct matches {
    int count;
    int capacity;
    char **instances;
};

static char *strip_copy(const char *line, size_t len) {
    size_t begin = 0;
    while (begin < len && isspace((unsigned char)line[begin])) {
        begin++;
    }
    while (len > begin && isspace((unsigned char)line[len - 1])) {
        len--;
    }

    char *copy = malloc(len - begin + 1);
    if (copy == NULL) {
        perror("malloc");
        exit(1);
    }
    memcpy(copy, line + begin, len - begin);
    copy[len - begin] = '\0';
    return copy;
}

static void add_instance(struct matches *m, const char *line, size_t len) {
    if (m->count == m->capacity) {
        m->capacity = m->capacity ? m->capacity * 2 : 16;
        m->instances = realloc(m->instances, m->capacity * sizeof(char *));
        if (m->instances == NULL) {
            perror("realloc");
            exit(1);
        }
    }
    m->instances[m->count] = strip_copy(line, len);
}

static void scan_line(const char *line, size_t len, const char *target,
                      size_t target_len, bool keep_lines, struct matches *m) {
    const char *p = line;
    const char *stop = line + len;

    while ((size_t)(stop - p) >= target_len) {
        p = memchr(p, target[0], stop - p - target_len + 1);
        if (p == NULL) {
            break;
        }
        if (memcmp(p, target, target_len) == 0) {
            if (keep_lines) {
                add_instance(m, line, len);
            }
            m->count++;
            p += target_len;
        } else {
            p++;
        }
    }
}

static struct matches search_file(char *filename, char *target, bool keep_lines) {
    struct matches m = {0, 0, NULL};
    size_t target_len = strlen(target);
    if (target_len == 0) {
        return m;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        return m;
    }

    char *line = NULL;
    size_t line_cap = 0;
    ssize_t len;
    while ((len = getline(&line, &line_cap, fp)) != -1) {
        scan_line(line, len, target, target_len, keep_lines, &m);
    }

    free(line);
    fclose(fp);
    return m;
}

int search_count(char *filename, char *target) {
    return search_file(filename, target, false).count;
}

struct count_result search_instance(char *filename,char *target){
    struct matches m = search_file(filename, target, true);
    struct count_result res = {m.count, m.instances};
    return res;
}
