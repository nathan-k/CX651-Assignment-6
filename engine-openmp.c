#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <omp.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define MAX_THREADS 64

struct matches {
    int count;
    int capacity;
    char **instances;
};

struct worker_args {
    char *filename;
    char *target;
    bool keep_lines;

    long start; //start of the chunk to process
    long end; //end of the chunk to process

    struct matches result;
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

static void search_chunk(struct worker_args *args) {
    size_t target_len = strlen(args->target);

    FILE *fp = fopen(args->filename, "rb");
    if (fp == NULL) {
        return;
    }

    long pos = args->start;
    if (pos > 0) {
        fseek(fp, pos - 1, SEEK_SET);
        pos--;
        int c;
        while ((c = fgetc(fp)) != EOF) {
            pos++;
            if (c == '\n') {
                break;
            }
        }
        if (c == EOF) {
            fclose(fp);
            return;
        }
    }

    char *line = NULL;
    size_t line_cap = 0;
    ssize_t len;
    while (pos < args->end && (len = getline(&line, &line_cap, fp)) != -1) {
        pos += len;
        scan_line(line, len, args->target, target_len, args->keep_lines,
                  &args->result);
    }

    free(line);
    fclose(fp);
}

static int thread_count(long size) {
    long n = omp_get_max_threads();
    if (n < 1) {
        n = 1;
    }
    if (n > MAX_THREADS) {
        n = MAX_THREADS;
    }
    if (size > 0 && n > size) {
        n = size;
    }
    return (int)n;
}

static struct matches search_file(char *filename, char *target, bool keep_lines) {
    struct matches total = {0, 0, NULL};
    struct stat st;
    if (strlen(target) == 0 || stat(filename, &st) != 0) {
        return total;
    }

    long size = st.st_size;
    int n = thread_count(size);
    struct worker_args args[MAX_THREADS];

    #pragma omp parallel for num_threads(n)
    for (int i = 0; i < n; i++) {
        args[i] = (struct worker_args){
            .filename = filename,
            .target = target,
            .keep_lines = keep_lines,
            .start = size * i / n,
            .end = size * (i + 1) / n,
            .result = {0, 0, NULL},
        };
        search_chunk(&args[i]);
    }

    for (int i = 0; i < n; i++) {
        total.count += args[i].result.count;
    }

    if (!keep_lines || total.count == 0) {
        for (int i = 0; i < n; i++) {
            free(args[i].result.instances);
        }
        return total;
    }

    total.instances = malloc(total.count * sizeof(char *));
    if (total.instances == NULL) {
        perror("malloc");
        exit(1);
    }
    total.capacity = total.count;
    int offset = 0;
    for (int i = 0; i < n; i++) {
        if (args[i].result.count > 0) {
            memcpy(total.instances + offset, args[i].result.instances,
                   args[i].result.count * sizeof(char *));
            offset += args[i].result.count;
        }
        free(args[i].result.instances);
    }
    return total;
}

int search_count(char *filename, char *target) {
    return search_file(filename, target, false).count;
}

struct count_result search_instance(char *filename,char *target){
    struct matches m = search_file(filename, target, true);
    struct count_result res = {m.count, m.instances};
    return res;
}
