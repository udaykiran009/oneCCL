#pragma once

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

struct {
    bool show_root_device_only{};
    ssize_t show_device_with_idx{ -1 };
} options;

void parse_options(int argc, char* argv[]) {
    struct option long_options[] {
        { "show_root_device_only", no_argument, nullptr, 'r' },
            { "show_device_with_idx", required_argument, nullptr, 'i' }, {
            nullptr, 0, nullptr, 0
        }
    };

    const char* const short_options = "ri:";

    int ch;
    while ((ch = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (ch) {
            case 'r': options.show_root_device_only = true; break;
            case 'i': options.show_device_with_idx = std::atoi(optarg); break;
            default:
                std::cout << "unknown option" << std::endl;
                abort();
                break;
        }
    }
}
