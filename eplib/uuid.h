#ifndef UUID_H
#define UUID_H

#define UUID_DEFAULT "00FF00FF-0000-0000-0000-00FF00FF00FF"
#define UUID_STR_LEN sizeof(UUID_DEFAULT) // 37 = 36 (uuid) + 1 (\0)

char* get_uuid();

#endif /* UUID_H */
