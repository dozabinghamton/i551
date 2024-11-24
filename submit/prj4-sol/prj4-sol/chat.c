#include "chat.h"
#include "server.h"

#include <chat-cmd.h>
#include <errors.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>


//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

#define SERVER 0
#define REQUEST 1
#define RESPONSE 2
#define N_SEM 3

struct _Chat {
    pid_t serverPid;      // PID of the server process
    size_t shmSize;       // Size of the shared memory
    Shm *sharedMem;       // Pointer to the shared memory  
  //TODO: fill out declaration
};

//TODO: add private functions

/** Return a new Chat object which creates a server process which uses
 *  the sqlite database located at dbPath.  All commands must be sent
 *  by this client process to the server and handled by the server
 *  using the database. All IPC must use shared memory of size shmSize
 *  with POSIX semaphores used for synchronization.  The returned
 *  object should encapsulate all the state needed to implement the
 *  following API.
 *
 *  The client process must use `out` for writing success output for
 *  commands where each output must start with a line containing "ok".
 *
 *  The client process must use `err` for writing error message
 *  lines. Each line must start with "err ERR_CODE: " where ERR_CODE is
 *  as in your previous project for user errors.  System errors
 *  can result in unclean program termination.
 *
 *  [Note that since a `ChatCmd` is guaranteed to be syntactically
 *  valid, the only user errors which the program will need to detect
 *  will be `BAD_ROOM`/`BAD_TOPIC` for unknown room or topic.  This
 *  will have to be done by the server which will then return an error
 *  response to the client for output.]
 *
 *  The server should not use the `in` or `out` streams.  It may use
 *  `stderr` for "logging", but all such logging *must* be turned off
 *  before submission.
 *
 *  If errors are encountered, then this function should return NULL.
 */

// /** Helper to send data through shared memory */
// static void send_data(Shm *sharedMem, const void *data, size_t nData) {
//     sem_wait(&sharedMem->sems[SERVER]); // Wait for shared memory to be available
//     memcpy(sharedMem->buf, data, nData); // Copy data into the buffer
//     sem_post(&sharedMem->sems[REQUEST]); // Notify server that data is available
// }

// /** Helper to receive data through shared memory */
// static void receive_data(Shm *sharedMem, void *data, size_t nData) {
//     sem_wait(&sharedMem->sems[RESPONSE]); // Wait for server response
//     memcpy(data, sharedMem->buf, nData);  // Copy data from buffer
//     sem_post(&sharedMem->sems[SERVER]);  // Notify shared memory is available
// }

// Helper function to initialize shared memory and semaphores
static Shm *init_shared_memory(size_t shmSize, FILE *err) {
    Shm *sharedMem = mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sharedMem == MAP_FAILED) {
        fprintf(err, ERROR "SYS_ERR: mmap failed\n");
        return NULL;
    }

    sharedMem->shmSize = shmSize - sizeof(Shm);

    if (sem_init(&sharedMem->clientSem, 1, 0) < 0 ||
        sem_init(&sharedMem->serverSem, 1, 1) < 0) {
        fprintf(err, ERROR "SYS_ERR: sem_init failed\n");
        munmap(sharedMem, shmSize);
        return NULL;
    }

    return sharedMem;
}

Chat *make_chat(const char *dbPath, size_t shmSize, FILE *out, FILE *err) {
    if (shmSize < MIN_SHM_SIZE) {
        fprintf(err, ERROR "SYS_ERR: Shared memory size too small\n");
        return NULL;
    }

    Shm *sharedMem = init_shared_memory(shmSize, err);
    if (!sharedMem) return NULL;

    pid_t serverPid = fork();
    if (serverPid < 0) {
        fprintf(err, ERROR "SYS_ERR: fork failed\n");
        munmap(sharedMem, shmSize);
        return NULL;
    }

    if (serverPid == 0) {
        // Server process
        do_server(dbPath, sharedMem);
        exit(EXIT_SUCCESS); // Ensure server exits cleanly
    }

    // Client process
    Chat *chat = malloc(sizeof(Chat));
    if (!chat) {
        fprintf(err, ERROR "SYS_ERR: malloc failed\n");
        munmap(sharedMem, shmSize);
        return NULL;
    }

    chat->serverPid = serverPid;
    chat->sharedMem = sharedMem;
    chat->shmSize = shmSize;
    return chat;
}

// Chat *
// make_chat(const char *dbPath, size_t shmSize, FILE *out, FILE *err)
// {
//   // Allocate shared memory
//   Shm *sharedMem = mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//   if (sharedMem == MAP_FAILED) {
//       errorf(err, ERROR "SYS_ERR: mmap failed");
//       return NULL;
//   }
//   // Initialize shared memory structure
//   sharedMem->shmSize = shmSize;
//   sharedMem->bufSize = shmSize - offsetof(Shm, buf);
//   sem_init(&sharedMem->sems[SERVER], 1, 1);
//   sem_init(&sharedMem->sems[REQUEST], 1, 0);
//   sem_init(&sharedMem->sems[RESPONSE], 1, 0);

//   // Fork the server process
//   pid_t serverPid = fork();
  
//   if (serverPid < 0) {
//       errorf(err, ERROR "SYS_ERR: fork failed");
//       munmap(sharedMem, shmSize);
//       return NULL;
//   }

//   if (serverPid == 0) {
//       // Server process
//       do_server(dbPath, sharedMem);
//       exit(EXIT_SUCCESS);  // Ensure server exits cleanly
//   }

//   // Client process
//   Chat *chat = malloc(sizeof(Chat));
//   if (!chat) {
//       errorf(err, ERROR "SYS_ERR: malloc failed");
//       munmap(sharedMem, shmSize);
//       return NULL;
//   }

//   *chat = (Chat){
//       .serverPid = serverPid,
//       .shmSize = shmSize,
//       .sharedMem = sharedMem,
//   };

//   return chat;  
//   //TODO
//   //return NULL;
// }

/** free all resources like memory used by chat.  All resources must
 *  be freed even after user errors have been detected.  It is okay if
 *  resources are not freed after system errors.
 */

void free_chat(Chat *chat) {
    if (!chat) return;

    sem_destroy(&chat->sharedMem->clientSem);
    sem_destroy(&chat->sharedMem->serverSem);
    munmap(chat->sharedMem, chat->shmSize);
    free(chat);
}

// void
// free_chat(Chat *chat)
// {
//   //TODO
//   if (!chat) return;

//   // Cleanup semaphores
//   sem_destroy(&chat->sharedMem->sems[SERVER]);
//   sem_destroy(&chat->sharedMem->sems[REQUEST]);
//   sem_destroy(&chat->sharedMem->sems[RESPONSE]);

//   // Unmap shared memory
//   munmap(chat->sharedMem, chat->shmSize);

//   // Free chat object
//   free(chat);  
// }


/** perform cmd using chat, with the client writing response to chat's
 *  out/err streams.  It can be assumed that cmd is free of user
 *  errors except for unknown room/topic for QUERY commands.
 *
 *  If the command is an END_CMD command, then ensure that the server
 *  process has terminated before returning.
 */

void do_chat_cmd(Chat *chat, const ChatCmd *cmd) {
    Shm *sharedMem = chat->sharedMem;

    // Serialize the command into shared memory
    sem_wait(&sharedMem->serverSem); // Wait for the server to be ready
    serialize_chat_cmd(cmd, sharedMem); // Store the command in shared memory
    sem_post(&sharedMem->clientSem); // Signal the server that the command is ready

    // Wait for the server's response
    sem_wait(&sharedMem->serverSem); // Wait for the server's response
    printf("%s", sharedMem->buf); // Read the response from shared memory
    sem_post(&sharedMem->clientSem); // Signal the server that the buffer is free
}

// void
// do_chat_cmd(Chat *chat, const ChatCmd *cmd)
// {
//   Shm *sharedMem = chat->sharedMem;
//   // Serialize the ChatCmd into shared memory
//   serialize_chat_cmd(cmd, sharedMem);

//   // Notify the server that a command is ready
//   sem_post(&sharedMem->sems[REQUEST]);

//   // Wait for the server's response
//   sem_wait(&sharedMem->sems[RESPONSE]);

//   // Read the response
//   char response[sharedMem->bufSize];
//   memcpy(response, sharedMem->buf, sharedMem->bufSize);

//   printf(" sharedMem->bufSize ");
//   // Print response
//   if (strncmp(response, "err", 3) == 0) {
//       fprintf(stderr, "%s\n", response);
//   } else {
//       fprintf(stdout, "%s\n", response);
//   }
// }

/** return server's PID */
pid_t chat_server_pid(const Chat *chat) {
  //TODO
  return chat->serverPid;
}
