#ifndef COMMON_H_
#define COMMON_H_

#include <chat-cmd.h>

#include <semaphore.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// declarations common between server and client

#define ERROR "err "
#define OKAY "ok\n"


enum { MIN_SHM_SIZE = 1024 };


//TODO: add declarations useful to both server and client
// Semaphore indices
#define SERVER 0    // Semaphore for controlling shared memory access
#define REQUEST 1   // Semaphore indicating a client request
#define RESPONSE 2  // Semaphore indicating a server response

#define N_SEM 3     // Total number of semaphores

// typedef struct _Shm {
//     size_t shmSize;     // Total size of shared memory
//     size_t bufSize;     // Usable size of the buffer
//     sem_t sems[3];      // Semaphores for synchronization
//     char buf[];         // Flexible array member for data transfer
// } Shm;
typedef struct {
    size_t shmSize;     // Total shared memory size
    sem_t clientSem;    // Semaphore to signal client activity
    sem_t serverSem;    // Semaphore to signal server activity
    size_t bufSize;     // Usable size of the buffer    
    char buf[];      // Flexible buffer for communication
} Shm;

#ifndef SEM_TRACE
#define SEM_TRACE 1
#endif

#if SEM_TRACE
#define SEM_VALUE(prg, state, sem, posixName) \
  do { \
    int sval; \
    if (sem_getvalue(sem, &sval) < 0) { \
      fatal("cannot get value for semaphore %s:", posixName); \
    } \
    fprintf(stderr, "%s: %s value of semaphore %s is %d\n", \
            prg, state, posixName, sval);                   \
  } while (0)
#else
#define SEM_VALUE(prg, state, sem, posixName) do { } while (0)
#endif


// /** Initialize a POSIX unnamed semaphore */
// void init_semaphore(sem_t *sem, int pshared, unsigned int value);

// /** Wait on a semaphore */
// void wait_semaphore(const char *name, sem_t *sem);

// /** Post (signal) a semaphore */
// void post_semaphore(const char *name, sem_t *sem);

// /** Destroy a POSIX unnamed semaphore */
// void destroy_semaphore(sem_t *sem);

// /** Clear shared memory */
// void clear_shared_memory(void *shm, size_t size);

// // Serialize a ChatCmd into shared memory
// void serialize_chat_cmd(const ChatCmd *cmd, Shm *sharedMem);

// // Deserialize a ChatCmd from shared memory
// void deserialize_chat_cmd(const Shm *sharedMem, ChatCmd *cmd);

/** Print an error message and terminate */
//void fatal(const char *format, ...);



// Function declarations common to client and server
void clear_shared_memory(Shm *sharedMem);
void serialize_chat_cmd(const ChatCmd *cmd, Shm *sharedMem);
void deserialize_chat_cmd(const Shm *sharedMem, ChatCmd *cmd);


#endif //#ifndef COMMON_H_
