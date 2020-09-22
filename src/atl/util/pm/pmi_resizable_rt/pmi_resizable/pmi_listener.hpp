#ifndef LISTENER_H_INCLUDED
#define LISTENER_H_INCLUDED

#include "helper.h"

class pmi_listener {
public:
    void send_notification(int sig, std::shared_ptr<helper> h);

    void set_applied_count(int count);

    int run_listener(std::shared_ptr<helper> h);

private:
    int collect_sock_addr(std::shared_ptr<helper> h);
    void clean_listener(std::shared_ptr<helper> h);
};
#endif
