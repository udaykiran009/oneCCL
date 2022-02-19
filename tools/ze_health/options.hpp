#pragma once

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

struct {
    bool show_all = true;
    bool show_links_only = false;
    bool show_mem_only = false;
    bool show_firmware_only = false;
} options;

void print_help(std::string bin_name) {
    std::string str;
    str += "Usage: " + bin_name + " [options]\n\n";
    str += "Options:\n";
    str += "   -l | --links   Show links\n";
    str += "   -m | --memory    Show memory\n";
    str += "   -f | --firmware    Show firmware\n";
    std::cout << str << std::endl;
}

void parse_options(int argc, char* argv[]) {
    struct option long_options[] {
        { "help", no_argument, nullptr, 'h' }, { "links", no_argument, nullptr, 'l' },
            { "memory", no_argument, nullptr, 'm' }, { "firmware", no_argument, nullptr, 'f' }, {
            nullptr, 0, nullptr, 0
        }
    };

    const char* short_options = "hlmf";

    int ch;
    while ((ch = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (ch) {
            case 'h':
                print_help(argv[0]);
                exit(0);
                break;
            case 'l':
                options.show_all = false;
                options.show_links_only = true;
                break;
            case 'm':
                options.show_all = false;
                options.show_mem_only = true;
                break;
            case 'f':
                options.show_all = false;
                options.show_firmware_only = true;
                break;
            default:
                std::cout << "unknown option" << std::endl;
                print_help(argv[0]);
                abort();
                break;
        }
    }
}
