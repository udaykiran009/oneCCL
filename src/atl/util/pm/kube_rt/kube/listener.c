#include <arpa/inet.h>

#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>

#include "kube_def.h"
#include "listener.h"

#define SEMAPHORE_NAME "/my_named_semaphore"

static int sock_sender;
static int count_listners;
static int sock_listner;
static sem_t* sem_glob;
static struct sockaddr_in* serv_addresses = NULL;
static char run_v2_template[RUN_TEMPLATE_SIZE];
static int listner_pid = -1;

int is_new_up(int count)
{
    int res = 0;

    sem_getvalue(sem_glob, &res);

    if (count == -1 || res < count)
        count = res;

    while (count > 0)
    {
        sem_wait(sem_glob);
    	count--;
    }

    sem_getvalue(sem_glob, &res);
    return res;
}

int collect_sock_addr()
{
    FILE *fp;
    int i = 0;
    char run_str[RUN_REQUEST_SIZE];
    char request_addesses[REQUEST_POSTFIX_SIZE];
    char request_addesses_count[REQUEST_POSTFIX_SIZE];
    char grep_listner[REQUEST_POSTFIX_SIZE];
    char grep_listner_count[REQUEST_POSTFIX_SIZE];
    char except_me[REQUEST_POSTFIX_SIZE];
    char sock_addr_str[MAX_KVS_VAL_LENGTH];
    char count_listners_str[INT_STR_SIZE];

    count_listners = 0;

    SET_STR(grep_listner_count, REQUEST_POSTFIX_SIZE, GREP_COUNT_TEMPLATE, KVS_LISTNER);

    SET_STR(grep_listner, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, KVS_LISTNER);
    SET_STR(except_me, REQUEST_POSTFIX_SIZE, GREP_EXCEPT_TEMPLATE, KVS_HOSTNAME);

    SET_STR(request_addesses, REQUEST_POSTFIX_SIZE, CONCAT_THREE_COMMAND_TEMPLATE, grep_listner, except_me, GET_VAL);
    SET_STR(request_addesses_count, REQUEST_POSTFIX_SIZE, CONCAT_THREE_COMMAND_TEMPLATE, grep_listner, except_me, grep_listner_count);

    /*get count listners(except me)*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, request_addesses_count);

    memset(count_listners_str, NULL_CHAR, INT_STR_SIZE);

    fp = popen(run_str, READ_ONLY);
    fgets(count_listners_str, sizeof(count_listners_str)-1, fp);
    pclose(fp);

    count_listners = atoi(count_listners_str);

    if (count_listners == 0)
    	return 0;

    if ((sock_sender = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    if (serv_addresses != NULL)
    {
        free(serv_addresses);
    }

    serv_addresses = (struct sockaddr_in*)malloc((count_listners)*sizeof(struct sockaddr_in));

    /*get listner addresses*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, request_addesses);

    fp = popen(run_str, READ_ONLY);

    while ((fgets(sock_addr_str, sizeof(sock_addr_str)-1, fp) != NULL) &&
           (i < count_listners))
    {
        char* point_to_port = strstr(sock_addr_str, "_");
        point_to_port[0] = NULL_CHAR;
        point_to_port++;
        serv_addresses[i].sin_port = atoi(point_to_port);
        serv_addresses[i].sin_family = AF_INET;

        if(inet_pton(AF_INET, sock_addr_str, &(serv_addresses[i].sin_addr))<=0)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }
        i++;
    }
    pclose(fp);
    return 0;
}

void send_notification(int sig)
{
    int i;
    char message[INT_STR_SIZE];

    SET_STR(message, INT_STR_SIZE, "%s", "Update!");
    for (i = 0; i < count_listners; ++i)
    {
        sendto(sock_sender, message, strlen(message), MSG_DONTWAIT,
               (const struct sockaddr *) &(serv_addresses[i]), sizeof(serv_addresses[i]));
    }
}

void clean_listener(int sig)
{
    FILE *fp;
    char patch[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_LISTNER, KVS_HOSTNAME);

    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

    /*remove listner*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);
    
    fp = popen(run_str, READ_ONLY);
    pclose(fp);
    close(sock_listner);

    exit(0);
}

int create_listner(char* _run_v2_template, int length)
{
    socklen_t len;
    char recv_buf[INT_STR_SIZE];
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);
    char addr_for_kvs[REQUEST_POSTFIX_SIZE];
    listner_pid = fork();

    strncpy(run_v2_template, _run_v2_template, length);
    
    if ( (sem_glob = sem_open(SEMAPHORE_NAME, O_CREAT, 0777, 0)) == SEM_FAILED ) {
        perror("sem_open");
        return 1;
    }

    switch (listner_pid)
    {
    case -1:
        return 1;
    
    case 0:
    {
        FILE* fp;
        char patch[REQUEST_POSTFIX_SIZE];
        char run_str[RUN_REQUEST_SIZE];
        char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
        struct sigaction act;
        int parent_pid = getppid();
        int sem_res;
        sem_t *sem;

        if ((sock_listner = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
            return 1;

        memset(&act, 0, sizeof(act));
        act.sa_handler = &clean_listener;
        act.sa_flags = 0;
        sigaction(SIGTERM, &act, 0);
        sigaction(SIGINT, &act, 0);

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = 0;

        if (bind(sock_listner,(const struct sockaddr*)&addr, sizeof(addr)) < 0)
            return 1;

        getsockname(sock_listner, (struct sockaddr *)&addr, (socklen_t*)&addr_len);

        SET_STR(addr_for_kvs, REQUEST_POSTFIX_SIZE, KVS_NAME_TEMPLATE_I, CHECKER_IP, (size_t)addr.sin_port);
        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_LISTNER, KVS_HOSTNAME);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, addr_for_kvs);

        /*set listner address*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

        fp = popen(run_str, READ_ONLY);
        pclose(fp);

        if ( (sem = sem_open(SEMAPHORE_NAME, 0)) == SEM_FAILED )
        {
            perror("sem_open");
            return 1;
        }

        while (1)
        {
            recvfrom(sock_listner, (char*) recv_buf, INT_STR_SIZE, MSG_WAITALL, (struct sockaddr*)&addr, &len);
            sem_getvalue(sem, &sem_res);

            if (sem_res == 0)
            {
                kill(parent_pid, SIGUSR1);
            }
            sem_post(sem);
        }
        sem_close(sem);
        exit(0);
    }
    }
    return 0;
}

void delete_listner()
{
    sem_close(sem_glob);

    if (listner_pid != -1)
        kill(listner_pid, SIGTERM);
}
