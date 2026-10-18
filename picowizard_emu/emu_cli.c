
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#define PROGRAM_NAME "emu_cli"

static void print_help(void)
{
    printf(
        "usage: " PROGRAM_NAME " [-h] input\n"
        "\n"
        "Picowizard Infrastructure Emulator\n"
        "\n"
        "positional arguments:\n"
        "  input       Input file to process\n"
        "\n"
        "options:\n"
        "  -h, --help  show this help message and exit\n"
    );
}

static void print_error(const char *message)
{
    fprintf(stderr, PROGRAM_NAME ": error: %s\n", message);
}

int main(int argc, char *argv[])
{
    static const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;

    while ((opt = getopt_long(argc, argv, "h", long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            print_help();
            return EXIT_SUCCESS;

        case '?':
        default:
            print_error("unrecognized arguments");
            fprintf(stderr, "usage: " PROGRAM_NAME " [-h] input\n");
            return EXIT_FAILURE;
        }
    }
    if (optind >= argc) {
        print_error("the following arguments are required: input");
        fprintf(stderr, "usage: " PROGRAM_NAME " [-h] input\n");
        return EXIT_FAILURE;
    }

    if (optind + 1 < argc) {
        print_error("unrecognized arguments");
        fprintf(stderr, "usage: " PROGRAM_NAME " [-h] input\n");
        return EXIT_FAILURE;
    }

    const char *input_file = argv[optind];

    printf("Input file: %s\n", input_file);

    return EXIT_SUCCESS;
}
