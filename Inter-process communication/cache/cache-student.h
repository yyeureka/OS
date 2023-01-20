/*
 *  This file is for use by students to define anything they wish.  It is used by the proxy cache implementation
 */
#ifndef __CACHE_STUDENT_H__
#define __CACHE_STUDENT_H__

#include "steque.h"
#include <sys/mman.h>
#include <mqueue.h>
#include <semaphore.h>

#define MIN(x, y)  (((x) < (y)) ? (x) : (y))

#define MSGSIZE 1024
#define PATHSIZE 100
#define SHMNAMESIZE 5
#define SEMNAMESIZE SHMNAMESIZE + 1 //20
#define MQNAME "/msgname"

typedef struct {
    unsigned int nsegments;
    ssize_t segsize;
    steque_t *segment_q;
    mqd_t msgid;
} shm_pool_t;

typedef struct {
    ssize_t segsize;
    char path[PATHSIZE];
    char shm_name[SHMNAMESIZE];
    char sem_name_r[SEMNAMESIZE];
    char sem_name_w[SEMNAMESIZE];
} msg_t;

extern pthread_mutex_t queue_lock;
extern pthread_cond_t dequeue_phase;
extern shm_pool_t shm_pool;

void init_threads(int numthreads);
void cleanup_threads(int numthreads);
void *handle_request(void *worker_id);
void init_ipcs(unsigned int nsegments, size_t segsize);
void cleanup_ipcs();

#endif // __CACHE_STUDENT_H__