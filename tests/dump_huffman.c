
#include "textlink.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Usage:
 * dump_huffman <input_file> <byte|char|s16> <output_file>
 *
 * Call the original C huff_encode() and save its encoded bytes.
 * This is a benchmark analysis tool, not a network sender.
 */

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <input> <byte|char|s16> <output>\n",
                argv[0]);
        return 1;
    }

    tl_sym_t sym;

    if (strcmp(argv[2], "byte") == 0) {
        sym = SYM_BYTE;
    } else if (strcmp(argv[2], "char") == 0) {
        sym = SYM_CHAR;
    } else if (strcmp(argv[2], "s16") == 0) {
        sym = SYM_S16;
    } else {
        fprintf(stderr, "Unknown symbol mode\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Cannot open input");
        return 1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return 1;
    }

    long size = ftell(fp);
    if (size < 0 || (unsigned long)size > TL_MAX_FILE) {
        fprintf(stderr, "Invalid input size\n");
        fclose(fp);
        return 1;
    }

    rewind(fp);

    size_t input_len = (size_t)size;
    uint8_t *input = malloc(input_len ? input_len : 1);

    if (!input) {
        fclose(fp);
        return 1;
    }

    if (fread(input, 1, input_len, fp) != input_len) {
        fprintf(stderr, "Failed to read input\n");
        free(input);
        fclose(fp);
        return 1;
    }

    fclose(fp);

    uint8_t *encoded = NULL;
    size_t encoded_len = 0;

    int rc = huff_encode(
        input, input_len, sym, &encoded, &encoded_len
    );

    free(input);

    if (rc != TL_OK) {
        fprintf(stderr, "huff_encode failed: %d\n", rc);
        free(encoded);
        return 1;
    }

    FILE *out = fopen(argv[3], "wb");

    if (!out) {
        perror("Cannot open output");
        free(encoded);
        return 1;
    }

    if (fwrite(encoded, 1, encoded_len, out) != encoded_len) {
        fprintf(stderr, "Failed to write output\n");
        fclose(out);
        free(encoded);
        return 1;
    }

    if (fclose(out) != 0) {
        free(encoded);
        return 1;
    }

    printf("Input bytes:   %zu\n", input_len);
    printf("Encoded bytes: %zu\n", encoded_len);
    printf("Symbol mode:   %s\n", argv[2]);
    printf("Output file:   %s\n", argv[3]);

    free(encoded);
    return 0;
}
