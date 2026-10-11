#include "engine.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <count|instance> <input_file> <target_word>\n",
                argv[0]);
        return 1;
    }

    char *mode = argv[1];
    char *filename = argv[2];
    char *target = argv[3];

    // The engines report a missing file as zero matches, so check here.
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return 1;
    }
    fclose(fp);

    if (strcmp(mode, "count") == 0) {
        int count = search_count(filename, target);
        printf("Found: %d of %s in %s\n", count, target, filename);
    } else if (strcmp(mode, "instance") == 0) {
        struct count_result res = search_instance(filename, target);
        printf("Found: %d of %s in %s\n", res.count, target, filename);
        for (int i = 0; i < res.count; i++) {
            printf("res.instances[%d]: %s\n", i, res.instances[i]);
        }
    } else {
        fprintf(stderr, "Unknown mode '%s': expected count or instance\n", mode);
        return 1;
    }

    return 0;
}
