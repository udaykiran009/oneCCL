#ifndef LISTENER_H_INCLUDED
#define LISTENER_H_INCLUDED

#include "helper.hpp"

class pmi_listener {
public:
    kvs_status_t send_notification(int sig, std::shared_ptr<helper> h);

    void set_applied_count(int count);

    kvs_status_t run_listener(std::shared_ptr<helper> h);

private:
    kvs_status_t collect_sock_addr(std::shared_ptr<helper> h);
    kvs_status_t clean_listener(std::shared_ptr<helper> h);
};
#endif
