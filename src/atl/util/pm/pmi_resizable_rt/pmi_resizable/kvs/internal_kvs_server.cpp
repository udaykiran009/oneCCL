#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <list>
#include <map>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <memory>

#include "common/log/log.hpp"
#include "internal_kvs_server.hpp"

#define MAX_CLIENT_COUNT 300

class server {
public:
    server() = default;
    void run(void*);
    bool check_finalize(fd_set read_fds);
    void make_client_request(int& socket);
    void try_to_connect_new(fd_set read_fds);

private:
    struct clients_info {
        int socket;
        bool in_barrier;
        clients_info(int sock, bool barrier) : socket(sock), in_barrier(barrier) {}
    };
    struct proc_info {
        std::string rank;
        size_t rank_count;
        std::string thread_id;
    };
    struct socket_info {
        int socket;
        std::string proc_id;
        proc_info process_info;
    };
    struct comm_info {
        size_t global_size = 0;
        size_t local_size = 0;
        std::list<socket_info> sockets;
        std::map<std::string, std::list<proc_info>> processes;
    };
    struct barrier_info {
        size_t global_size = 0;
        size_t local_size = 0;
        std::list<std::shared_ptr<clients_info>> clients;
    };
    kvs_request_t request{};
    size_t count{};
    size_t client_count = 0;
    int sockets[MAX_CLIENT_COUNT]{};
    std::map<std::string, barrier_info> barriers;
    std::map<std::string, comm_info> communicators;
    int server_control_sock{};
    int ret = 0;
    std::mutex server_memory_mutex;
    std::map<std::string, std::map<std::string, std::string>> requests;
    int server_listen_sock; /* used on server side to handle new incoming connect requests from clients */
    const int free_socket = -1;
};

void server::try_to_connect_new(fd_set read_fds) {
    int i = 0;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

    if (FD_ISSET(server_listen_sock, &read_fds)) {
        int new_socket;
        socklen_t peer_addr_size = sizeof(addr);
        if ((new_socket = accept(
                 server_listen_sock, (struct sockaddr*)&addr, (socklen_t*)&peer_addr_size)) < 0) {
            perror("server: server_listen_sock accept");
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < MAX_CLIENT_COUNT; i++) {
            if (sockets[i] == free_socket) {
                sockets[i] = new_socket;
                break;
            }
        }
        if (i >= MAX_CLIENT_COUNT) {
            printf("server: no free sockets\n");
            exit(EXIT_FAILURE);
        }
    }
}

void server::make_client_request(int& socket) {
    DO_RW_OP_1(
        read, socket, &request, sizeof(kvs_request_t), ret, "server: get command from client");
    if (ret == 0) {
        close(socket);
        socket = free_socket;
        client_count--;
        return;
    }

    switch (request.mode) {
        case AM_CONNECT: {
            client_count++;
            break;
        }
        case AM_PUT: {
            auto& req = requests[request.name];
            req[request.key] = request.val;
            break;
        }
        case AM_REMOVE: {
            requests[request.name].erase(request.key);
            break;
        }
        case AM_GET_VAL: {
            count = 0;
            auto it_name = requests.find(request.name);
            if (it_name != requests.end()) {
                auto it_key = it_name->second.find(request.key);
                if (it_key != it_name->second.end()) {
                    count = 1;
                    DO_RW_OP(write,
                             socket,
                             &count,
                             sizeof(size_t),
                             server_memory_mutex,
                             "server: get_value write size");

                    kvs_str_copy(request.val, it_key->second.c_str(), MAX_KVS_VAL_LENGTH);

                    DO_RW_OP(write,
                             socket,
                             &request,
                             sizeof(kvs_request_t),
                             server_memory_mutex,
                             "server: get_value write data");
                    break;
                }
            }
            DO_RW_OP(write,
                     socket,
                     &count,
                     sizeof(size_t),
                     server_memory_mutex,
                     "server: get_value write size");
            break;
        }
        case AM_GET_COUNT: {
            count = 0;
            auto it = requests.find(request.name);
            if (it != requests.end()) {
                count = it->second.size();
            }
            DO_RW_OP(
                write, socket, &count, sizeof(size_t), server_memory_mutex, "server: get_count");
            break;
        }
        case AM_GET_REPLICA: {
            char* replica_size_str = getenv(CCL_WORLD_SIZE_ENV);
            count = (replica_size_str != nullptr) ? safe_strtol(replica_size_str, nullptr, 10)
                                                  : client_count;
            DO_RW_OP(
                write, socket, &count, sizeof(size_t), server_memory_mutex, "server: get_replica");
            break;
        }
        case AM_GET_KEYS_VALUES: {
            size_t j = 0;
            kvs_request_t* answers = nullptr;
            count = 0;
            auto it_name = requests.find(request.name);
            if (it_name != requests.end()) {
                count = it_name->second.size();
            }

            DO_RW_OP(write,
                     socket,
                     &count,
                     sizeof(size_t),
                     server_memory_mutex,
                     "server: get_keys_values write size");
            if (count == 0)
                break;

            answers = (kvs_request_t*)calloc(count, sizeof(kvs_request_t));
            if (answers == nullptr) {
                printf("Memory allocation failed\n");
                break;
            }
            for (auto it_keys : it_name->second) {
                kvs_str_copy(answers[j].name, request.name, MAX_KVS_NAME_LENGTH);
                kvs_str_copy(answers[j].key, it_keys.first.c_str(), MAX_KVS_KEY_LENGTH);
                kvs_str_copy(answers[j].val, it_keys.second.c_str(), MAX_KVS_VAL_LENGTH);
                j++;
            }

            DO_RW_OP(write,
                     socket,
                     answers,
                     sizeof(kvs_request_t) * count,
                     server_memory_mutex,
                     "server: get_keys_values write data");

            free(answers);
            break;
        }
        case AM_BARRIER: {
            auto& barrier_list = barriers[request.name];
            auto& clients = barrier_list.clients;
            auto client_it = std::find_if(std::begin(clients),
                                          std::end(clients),
                                          [&](std::shared_ptr<server::clients_info> v) {
                                              return v->socket == socket;
                                          });
            if (client_it == clients.end()) {
                // TODO: Look deeper to fix this error
                printf("Server error: Unregister Barrier request!");
                exit(1);
            }
            auto client_inf = client_it->get();
            client_inf->in_barrier = true;

            if (barrier_list.global_size == barrier_list.local_size) {
                bool is_barrier_stop = true;
                for (const auto& client : clients) {
                    if (!client->in_barrier) {
                        is_barrier_stop = false;
                        break;
                    }
                }
                if (is_barrier_stop) {
                    int is_done = 1;
                    for (const auto& client : clients) {
                        client->in_barrier = false;
                        DO_RW_OP(write,
                                 client->socket,
                                 &is_done,
                                 sizeof(is_done),
                                 server_memory_mutex,
                                 "server: barrier");
                    }
                }
            }
            break;
        }
        case AM_BARRIER_REGISTER: {
            char* glob_size = request.val;
            char* local_size = strstr(glob_size, "_");
            auto& barrier = barriers[request.name];
            if (local_size == nullptr) {
                barrier.local_size++;
            }
            else {
                local_size[0] = '\0';
                local_size++;
                barrier.local_size += safe_strtol(local_size, nullptr, 10);
            }
            barrier.global_size = safe_strtol(glob_size, nullptr, 10);

            barrier.clients.push_back(
                std::shared_ptr<clients_info>(new clients_info(socket, false)));
            break;
        }
        case AM_SET_SIZE: {
            char* glob_size = request.val;
            communicators[request.key].global_size = safe_strtol(glob_size, nullptr, 10);

            break;
        }
        case AM_INTERNAL_REGISTER: {
            auto& communicator = communicators[request.key];
            char* rank_count_str = request.val;
            char* rank = strstr(rank_count_str, "_");
            rank[0] = '\0';
            rank++;
            char* proc_id = strstr(rank, "_");
            proc_id[0] = '\0';
            proc_id++;
            char* thread_id = strstr(proc_id, "_");
            thread_id[0] = '\0';
            thread_id++;
            size_t rank_count = safe_strtol(rank_count_str, nullptr, 10);
            communicators[request.key].local_size += rank_count;
            socket_info sock_info{ socket, proc_id, { rank, rank_count, thread_id } };
            communicator.processes[proc_id].push_back(sock_info.process_info);
            communicator.sockets.push_back(sock_info);
            if (communicator.local_size == communicator.global_size) {
                std::string proc_count_str = std::to_string(communicator.processes.size());
                for (auto& it : communicator.processes) {
                    it.second.sort([](proc_info a, proc_info b) {
                        return a.rank < b.rank;
                    });
                }
                for (auto& it : communicator.sockets) {
                    std::string thread_num;
                    int i = 0;
                    size_t proc_rank_count = 0;
                    auto process_info = communicator.processes[it.proc_id];
                    std::string threads_count = std::to_string(process_info.size());
                    for (auto& proc_info_it : process_info) {
                        if (it.process_info.rank == proc_info_it.rank) {
                            thread_num = std::to_string(i);
                            break;
                        }
                        i++;
                    }
                    for (auto& proc_info_it : process_info) {
                        proc_rank_count += proc_info_it.rank_count;
                    }
                    memset(request.val, 0, MAX_KVS_VAL_LENGTH);
                    /*return string: %PROC_COUNT%_%RANK_NUM%_%PROCESS_RANK_COUNT%_%THREADS_COUNT%_%THREAD_NUM% */
                    snprintf(request.val,
                             MAX_KVS_VAL_LENGTH,
                             "%s_%s_%s_%s_%s",
                             proc_count_str.c_str(),
                             it.process_info.rank.c_str(),
                             std::to_string(proc_rank_count).c_str(),
                             threads_count.c_str(),
                             thread_num.c_str());
                    DO_RW_OP(write,
                             it.socket,
                             &request,
                             sizeof(kvs_request_t),
                             server_memory_mutex,
                             "server: register write data");
                }
            }
            break;
        }
        default: {
            if (request.name[0] == '\0')
                return;
            printf("server: unknown request mode - %d.\n", request.mode);
            exit(EXIT_FAILURE);
        }
    }
}

bool server::check_finalize(fd_set read_fds) {
    bool to_finalize = false;
    if (FD_ISSET(server_control_sock, &read_fds)) {
        DO_RW_OP_1(read,
                   server_control_sock,
                   &request,
                   sizeof(kvs_request_t),
                   ret,
                   "server: get control msg from client");
        if (ret == 0) {
            close(server_control_sock);
            server_control_sock = free_socket;
        }
        if (request.mode != AM_FINALIZE) {
            printf("server: invalid access mode for local socket\n");
            exit(EXIT_FAILURE);
        }
        to_finalize = true;
    }
    return to_finalize;
}

void server::run(void* args) {
    int max_sd, sd, i;
    bool should_stop = false;
    fd_set read_fds;
    int so_reuse = 1;
    struct sockaddr_in addr;
    server_listen_sock = ((server_args_t*)args)->sock_listener;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

#ifdef SO_REUSEPORT
    setsockopt(server_listen_sock, SOL_SOCKET, SO_REUSEPORT, &so_reuse, sizeof(so_reuse));
#else
    setsockopt(server_listen_sock, SOL_SOCKET, SO_REUSEADDR, &so_reuse, sizeof(so_reuse));
#endif

    if (listen(server_listen_sock, MAX_CLIENT_COUNT) < 0) {
        LOG_ERROR("server: server_listen_sock listen");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < MAX_CLIENT_COUNT; i++) {
        sockets[i] = free_socket;
    }

    if ((server_control_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server: server_control_sock init");
        exit(EXIT_FAILURE);
    }

    while (connect(server_control_sock,
                   (struct sockaddr*)(((server_args_t*)args)->args),
                   sizeof(addr)) < 0) {
    }
    while (!should_stop || client_count > 1) {
        FD_ZERO(&read_fds);
        FD_SET(server_listen_sock, &read_fds);
        FD_SET(server_control_sock, &read_fds);
        max_sd = server_listen_sock;

        for (i = 0; i < MAX_CLIENT_COUNT; i++) {
            sd = sockets[i];

            if (sd != free_socket)
                FD_SET(sd, &read_fds);

            if (sd > max_sd)
                max_sd = sd;
        }

        if (server_control_sock > max_sd)
            max_sd = server_control_sock;

        if (select(max_sd + 1, &read_fds, nullptr, nullptr, nullptr) < 0) {
            if (errno != EINTR) {
                perror("server: select");
                exit(EXIT_FAILURE);
            }
            else {
                /* restart select */
                continue;
            }
        }

        for (i = 0; i < MAX_CLIENT_COUNT; i++) {
            sd = sockets[i];
            if (sd == free_socket)
                continue;

            if (FD_ISSET(sd, &read_fds)) {
                make_client_request(sockets[i]);
            }
        }
        try_to_connect_new(read_fds);
        if (!should_stop) {
            should_stop = check_finalize(read_fds);
        }
    }

    if (server_control_sock) {
        DO_RW_OP_1(write,
                   server_control_sock,
                   &should_stop,
                   sizeof(should_stop),
                   ret,
                   "server: send control msg to client");
    }

    close(server_control_sock);
    server_control_sock = free_socket;

    for (i = 0; i < MAX_CLIENT_COUNT; i++) {
        if (sockets[i] != free_socket) {
            close(sockets[i]);
            sockets[i] = free_socket;
        }
    }

    close(server_listen_sock);
    server_listen_sock = free_socket;
}

void* kvs_server_init(void* args) {
    server s;

    s.run(args);

    return nullptr;
}