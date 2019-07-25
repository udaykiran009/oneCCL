#ifndef KVS_KEEPER_H_INCLUDED
#define KVS_KEEPER_H_INCLUDED

void put_key(const char kvsname[], const char kvs_key[], const char kvs_val[]);

size_t cut_head(char* kvsname, char* kvs_key, char* kvs_val);

size_t get_kvs_list_size(void);

#endif
