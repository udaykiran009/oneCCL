#pragma once

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

struct {
    bool show_root_device_only{};
    ssize_t show_device_with_idx{ -1 };
} options;

void print_help(std::string bin_name) {
    std::string str;
    str += "Usage: " + bin_name + " [options]\n\n";
    str += "Options:\n";
    str += "   -r | --show-root-device-only   Hide subdevices\n";
    str += "   -i | --show-device-with-idx    Show device with specific root device index\n";
    std::cout << str << std::endl;
}

void parse_options(int argc, char* argv[]) {
    struct option long_options[] {
        { "help", no_argument, nullptr, 'h' },
            { "show-device-with-idx", required_argument, nullptr, 'i' },
            { "show-root-device-only", no_argument, nullptr, 'r' }, {
            nullptr, 0, nullptr, 0
        }
    };

    const char* short_options = "hi:r";

    int ch;
    while ((ch = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (ch) {
            case 'h':
                print_help(argv[0]);
                exit(0);
                break;
            case 'i': options.show_device_with_idx = std::atoi(optarg); break;
            case 'r': options.show_root_device_only = true; break;
            default:
                std::cout << "unknown option" << std::endl;
                print_help(argv[0]);
                abort();
                break;
        }
    }
}
