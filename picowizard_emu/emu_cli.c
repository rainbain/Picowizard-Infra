
/* Picowizard Emulator Client
 *
 * Command line interface
 *
 * Author: Samuel Fitzsimons (rainbain)
 * File: emu_cli.c
 * Date: 2026
 * License: GPLv2 (See included license file)
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include "system.h"

#define PROGRAM_NAME "emu_cli"

static void print_usage(FILE *stream)
{
    fprintf(stream, "usage: " PROGRAM_NAME " [-h] --isa ISA input\n");
}

static void print_help(void)
{
    print_usage(stdout);

    printf(
        "\n"
        "Picowizard Infrastructure Emulator\n"
        "\n"
        "positional arguments:\n"
        "  input       ROM file to load\n"
        "\n"
        "options:\n"
        "  --isa ISA   ISA version (e.g. 1.2 or 1.2p)\n"
        "  -h, --help  show this help message and exit\n"
    );
}

static void print_error(const char *message)
{
    fprintf(stderr, PROGRAM_NAME ": error: %s\n", message);
}

/*
 * Decode an ISA version:
 *   1.2  -> version = 120, plus = false
 *   1.2p -> version = 120, plus = true
 *
 * This currently accepts a single-digit major and minor version.
 */
static bool parse_isa(const char *text, int *version, bool *plus)
{
    if (text == NULL || text[0] < '0' || text[0] > '9' ||
        text[1] != '.' ||
        text[2] < '0' || text[2] > '9') {
        return false;
    }

    int decoded_version = (text[0] - '0') * 100
                        + (text[2] - '0') * 10;

    bool decoded_plus;

    if (text[3] == '\0') {
        decoded_plus = false;
    } else if (text[3] == 'p' && text[4] == '\0') {
        decoded_plus = true;
    } else {
        return false;
    }

    *version = decoded_version;
    *plus = decoded_plus;

    return true;
}

/*
 * Read an entire ROM file into a heap-allocated buffer.
 * The caller is responsible for freeing the buffer.
 */
static uint8_t *load_rom(const char *path, size_t *len)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        perror(path);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror(path);
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);

    if (file_size < 0) {
        perror(path);
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        perror(path);
        fclose(file);
        return NULL;
    }

    size_t size = (size_t)file_size;
    uint8_t *rom = malloc(size > 0 ? size : 1);

    if (rom == NULL) {
        fprintf(stderr, PROGRAM_NAME ": error: out of memory loading ROM\n");
        fclose(file);
        return NULL;
    }

    if (size > 0 && fread(rom, 1, size, file) != size) {
        if (ferror(file)) {
            perror(path);
        } else {
            fprintf(stderr, PROGRAM_NAME ": error: incomplete ROM read\n");
        }

        free(rom);
        fclose(file);
        return NULL;
    }

    if (fclose(file) != 0) {
        perror(path);
        free(rom);
        return NULL;
    }

    *len = size;
    return rom;
}

int main(int argc, char *argv[])
{
    static const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"isa",  required_argument, NULL, 'i'},
        {NULL, 0, NULL, 0}
    };

    int isa_version = 0;
    bool isa_plus = false;
    bool isa_provided = false;

    int opt;

    while ((opt = getopt_long(argc, argv, "hi:", long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            print_help();
            return EXIT_SUCCESS;

        case 'i':
            if (!parse_isa(optarg, &isa_version, &isa_plus)) {
                print_error("invalid ISA version; expected FORMAT like 1.2 or 1.2p");
                print_usage(stderr);
                return EXIT_FAILURE;
            }

            isa_provided = true;
            break;

        case '?':
        default:
            print_error("unrecognized arguments");
            print_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (!isa_provided) {
        print_error("the following arguments are required: --isa");
        print_usage(stderr);
        return EXIT_FAILURE;
    }

    if (optind >= argc) {
        print_error("the following arguments are required: input");
        print_usage(stderr);
        return EXIT_FAILURE;
    }

    if (optind + 1 < argc) {
        print_error("unrecognized arguments");
        print_usage(stderr);
        return EXIT_FAILURE;
    }

    const char *input_file = argv[optind];

    printf("Running ISA version %03d %s\n", isa_version, isa_plus ? "PLUS": "");

    // Initialize the emulator.
    if (system_initialize(isa_plus) < 0) {
        print_error("failed to initialize emulator");
        return EXIT_FAILURE;
    }

    // Load the ROM.
    size_t rom_len = 0;
    uint8_t *rom = load_rom(input_file, &rom_len);

    if (rom == NULL) {
        return EXIT_FAILURE;
    }

    // Write the ROM into emulator memory.
    system_write_rom(rom, rom_len);

    free(rom);

    // Run until the emulator returns.
    system_run();

    return EXIT_SUCCESS;
}
