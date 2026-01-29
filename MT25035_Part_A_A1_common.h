#ifndef COMMON_H
#define COMMON_H

#define NUM_FIELDS 8

typedef struct {
    char *fields[NUM_FIELDS];
    int field_size;
} message_t;

#endif
