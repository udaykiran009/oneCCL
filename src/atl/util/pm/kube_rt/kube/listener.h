#ifndef LISTENER_H_INCLUDED
#define LISTENER_H_INCLUDED

int collect_sock_addr();

void send_notification(int sig);

int create_listner(char* _run_v2_template, int length);

int is_new_up(int count);

void delete_listner();

#endif
