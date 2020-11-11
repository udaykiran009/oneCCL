#pragma once
#define private public
#define protected public
#include <sys/types.h>
#include <unistd.h>

#include "common_fixture.hpp"

#undef protected
#undef private

class default_fixture : public common_fixture {
protected:
    default_fixture() : common_fixture("") {}

    virtual ~default_fixture() {}
    void SetUp() override {
        create_global_platform();
    }
};


class ipc_handles_fixture : public common_fixture {
public:
    int wait_for_child_fork(int pid) {
        return 0;
    }

protected:
    ipc_handles_fixture(): common_fixture(get_global_device_indices()) {
    }

    virtual ~ipc_handles_fixture() override {}

    bool is_child() const {
        return !pid;
    }

    inline void wait_phase(unsigned char phase) {

        int ret = writeToSocket(communication_socket, &phase, sizeof(phase));
        if(ret != 0) {
            std::cerr << "Cannot writeToSocket on phase: " << (int)phase << ", error: " << strerror(errno) << std::endl;;
            abort();
        }

        unsigned char get_phase = phase + 1;
        ret = readFromSocket(communication_socket, &get_phase, sizeof(get_phase));
        if(ret != 0) {
            std::cerr << "Cannot readFromSocket on phase: " << (int)phase << ", error: " << strerror(errno) << std::endl;
            abort();
        }

        if(phase != get_phase) {
            std::cerr << "wait_phase phases mismatch! wait phase: " << (int)phase << ", got phase: " << (int)get_phase << std::endl;;
            abort();
        }
    }

    void SetUp() override {

        if(global_affinities.size() != 2)
        {
            std::cerr << __FUNCTION__ << " - not enough devices in L0_CLUSTER_AFFINITY_MASK."
                      << "Two devices required at least" << std::endl;
            abort();
        }
        if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockets) < 0) {
            std::cerr << __FUNCTION__ << " - cannot create sockets pairt before fork: "
                      << strerror(errno) << std::endl;
            abort();
        }

        pid_pair[TO_CHILD] = getpid();
        pid_pair[TO_PARENT] = getpid();

        pid = fork();
        if (pid < 0) {
            std::cerr << __FUNCTION__ << " - cannot fork() process: "
                      << strerror(errno) << std::endl;
            abort();
        }
        else if (pid == 0) //child
        {
            close(sockets[TO_CHILD]);
            local_affinity = global_affinities[TO_CHILD];

            pid_pair[TO_CHILD] = getpid();
            my_pid = &pid_pair[TO_CHILD];
            other_pid = &pid_pair[TO_PARENT];

            communication_socket = sockets[TO_PARENT];

            // create local to child process driver & devices
            create_global_platform();
            create_local_platform();
        }
        else {
            close(sockets[TO_PARENT]);
            local_affinity = global_affinities[TO_PARENT];

            pid_pair[TO_CHILD] = pid;

            my_pid = &pid_pair[TO_PARENT];
            other_pid = &pid_pair[TO_CHILD];

            communication_socket = sockets[TO_CHILD];

            // create local to parent process driver & devices
            create_global_platform();
            create_local_platform();
        }
    }

    void TearDown() override {
        //TODO waitpid for parent
        if (pid) {
            std::cout << "PID: " << *my_pid << " wait child: " << *other_pid << std::endl;
            int status = 0;
            int ret = waitpid(pid, &status, 0);
            UT_ASSERT(ret == pid, "Waitpid return fails");
        }
        else {
            std::cout << "PID: " << *my_pid << " exit" << std::endl;
            quick_exit(0);
        }
    }

    int pid = 0;
    enum { TO_CHILD, TO_PARENT };

    int pid_pair[2];
    int* other_pid = nullptr;
    int* my_pid = nullptr;

    int sockets[2];
    int communication_socket = 0;
};
