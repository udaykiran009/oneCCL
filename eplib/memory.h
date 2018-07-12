#ifndef _MEMORY_H_
#define _MEMORY_H_

void memory_get_limits();
void memory_init(void);
int memory_register(void*, size_t, const char*, int);
void memory_set_cqueue(int, void*);
void* memory_get_cqueue(int);
void memory_set_quant_params(int, void*);
void* memory_get_quant_params(int);
void memory_get_client_shm_base(int);
void* memory_translate_clientaddr(void*);
int memory_is_shmem(void*, int*);
void* memory_malloc(size_t);
void* memory_realloc(void*, size_t);
void* memory_calloc(size_t, size_t);
void* memory_memalign(size_t, size_t);
void memory_free(void*);
void memory_release(int);
void memory_unlink(void);
void memory_finalize(void);

#endif /* _MEMORY_H_ */
