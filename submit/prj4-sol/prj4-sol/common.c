#include "common.h"

#include <errors.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <semaphore.h>
#include <unistd.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

//TODO: define functions useful to both client and server
// void init_semaphore(sem_t *sem, int pshared, unsigned int value) {
//     if (sem_init(sem, pshared, value) < 0) {
//         fatal(ERROR "SYS_ERR: Cannot initialize semaphore");
//     }
// }

// void wait_semaphore(const char *name, sem_t *sem) {
//     TRACE("Waiting on semaphore: %s", name);
//     if (sem_wait(sem) < 0) {
//         fatal(ERROR "SYS_ERR: Failed to wait on semaphore %s", name);
//     }
//     SEM_VALUE("client/server", "after wait", sem, name);
// }

// void post_semaphore(const char *name, sem_t *sem) {
//     TRACE("Posting semaphore: %s", name);
//     if (sem_post(sem) < 0) {
//         fatal(ERROR "SYS_ERR: Failed to post semaphore %s", name);
//     }
//     SEM_VALUE("client/server", "after post", sem, name);
// }

// void destroy_semaphore(sem_t *sem) {
//     if (sem_destroy(sem) < 0) {
//         fatal(ERROR "SYS_ERR: Failed to destroy semaphore");
//     }
// }

// void clear_shared_memory(void *shm, size_t size) {
//     memset(shm, 0, size);
// }

// // void fatal(const char *format, ...) {
// //     va_list args;
// //     va_start(args, format);
// //     vfprintf(stderr, format, args);
// //     va_end(args);
// //     exit(EXIT_FAILURE);
// // }


// void serialize_chat_cmd(const ChatCmd *cmd, Shm *sharedMem) {
//     size_t offset = 0;

//     // Copy the ChatCmd structure
//     memcpy(sharedMem->buf + offset, cmd, sizeof(ChatCmd));
//     offset += sizeof(ChatCmd);

//     // Copy the user string
//     if (cmd->type == ADD_CMD) {
//         strcpy(sharedMem->buf + offset, cmd->add.user);
//         ((ChatCmd *)(sharedMem->buf))->add.user = sharedMem->buf + offset;
//         offset += strlen(cmd->add.user) + 1;

//         // Copy the room string
//         strcpy(sharedMem->buf + offset, cmd->add.room);
//         ((ChatCmd *)(sharedMem->buf))->add.room = sharedMem->buf + offset;
//         offset += strlen(cmd->add.room) + 1;

//         // Copy the message string
//         strcpy(sharedMem->buf + offset, cmd->add.message);
//         ((ChatCmd *)(sharedMem->buf))->add.message = sharedMem->buf + offset;
//         offset += strlen(cmd->add.message) + 1;

//         // Copy the topics
//         for (size_t i = 0; i < cmd->add.nTopics; i++) {
//             strcpy(sharedMem->buf + offset, cmd->add.topics[i]);
//             ((ChatCmd *)(sharedMem->buf))->add.topics[i] = sharedMem->buf + offset;
//             offset += strlen(cmd->add.topics[i]) + 1;
//         }
//     }
// }

// void deserialize_chat_cmd(const Shm *sharedMem, ChatCmd *cmd) {
//     size_t offset = 0;

//     // Copy the ChatCmd structure
//     memcpy(cmd, sharedMem->buf, sizeof(ChatCmd));
//     offset += sizeof(ChatCmd);

//     // Rebuild the user, room, and message pointers
//     if (cmd->type == ADD_CMD) {
//         cmd->add.user = sharedMem->buf + offset;
//         offset += strlen(cmd->add.user) + 1;

//         cmd->add.room = sharedMem->buf + offset;
//         offset += strlen(cmd->add.room) + 1;

//         cmd->add.message = sharedMem->buf + offset;
//         offset += strlen(cmd->add.message) + 1;

//         for (size_t i = 0; i < cmd->add.nTopics; i++) {
//             cmd->add.topics[i] = sharedMem->buf + offset;
//             offset += strlen(cmd->add.topics[i]) + 1;
//         }
//     }
// }




// Clear shared memory before writing to it
void clear_shared_memory(Shm *sharedMem) {
    memset(sharedMem->buf, 0, sharedMem->shmSize);
}

// Serialize ChatCmd into the shared memory buffer
void serialize_chat_cmd(const ChatCmd *cmd, Shm *sharedMem) {
    char *ptr = sharedMem->buf;
    size_t remaining = sharedMem->shmSize;

    // Clear shared memory buffer
    memset(ptr, 0, remaining);

    // Write the command type
    if (remaining < sizeof(cmd->type)) {
        // Not enough space
        fprintf(stderr, "Serialization Error: Not enough space for command type\n");
        return;
    }
    memcpy(ptr, &cmd->type, sizeof(cmd->type));
    ptr += sizeof(cmd->type);
    remaining -= sizeof(cmd->type);

    if (cmd->type == ADD_CMD) {
        // Serialize ADD_CMD fields
        // Serialize user
        size_t len = strlen(cmd->add.user) + 1;
        memcpy(ptr, &len, sizeof(len));
        ptr += sizeof(len);
        remaining -= sizeof(len);

        memcpy(ptr, cmd->add.user, len);
        ptr += len;
        remaining -= len;

        // Serialize room
        len = strlen(cmd->add.room) + 1;
        memcpy(ptr, &len, sizeof(len));
        ptr += sizeof(len);
        remaining -= sizeof(len);

        memcpy(ptr, cmd->add.room, len);
        ptr += len;
        remaining -= len;

        // Serialize message
        len = strlen(cmd->add.message) + 1;
        memcpy(ptr, &len, sizeof(len));
        ptr += sizeof(len);
        remaining -= sizeof(len);

        memcpy(ptr, cmd->add.message, len);
        ptr += len;
        remaining -= len;

        // Serialize nTopics
        memcpy(ptr, &cmd->add.nTopics, sizeof(cmd->add.nTopics));
        ptr += sizeof(cmd->add.nTopics);
        remaining -= sizeof(cmd->add.nTopics);

        // Serialize topics
        for (size_t i = 0; i < cmd->add.nTopics; i++) {
            len = strlen(cmd->add.topics[i]) + 1;
            memcpy(ptr, &len, sizeof(len));
            ptr += sizeof(len);
            remaining -= sizeof(len);

            memcpy(ptr, cmd->add.topics[i], len);
            ptr += len;
            remaining -= len;
        }
    } else if (cmd->type == QUERY_CMD) {
        // Serialize QUERY_CMD fields
        // Serialize room
        size_t len = strlen(cmd->query.room) + 1;
        memcpy(ptr, &len, sizeof(len));
        ptr += sizeof(len);
        remaining -= sizeof(len);

        memcpy(ptr, cmd->query.room, len);
        ptr += len;
        remaining -= len;

        // Serialize nTopics
        memcpy(ptr, &cmd->query.nTopics, sizeof(cmd->query.nTopics));
        ptr += sizeof(cmd->query.nTopics);
        remaining -= sizeof(cmd->query.nTopics);

        // Serialize topics
        for (size_t i = 0; i < cmd->query.nTopics; i++) {
            len = strlen(cmd->query.topics[i]) + 1;
            memcpy(ptr, &len, sizeof(len));
            ptr += sizeof(len);
            remaining -= sizeof(len);

            memcpy(ptr, cmd->query.topics[i], len);
            ptr += len;
            remaining -= len;
        }

        // Serialize count
        memcpy(ptr, &cmd->query.count, sizeof(cmd->query.count));
        ptr += sizeof(cmd->query.count);
        remaining -= sizeof(cmd->query.count);
    }
    // Handle other command types similarly
}


void deserialize_chat_cmd(const Shm *sharedMem, ChatCmd *cmd) {
    const char *ptr = sharedMem->buf;
    size_t remaining = sharedMem->shmSize;

    // Read command type
    if (remaining < sizeof(cmd->type)) {
        fprintf(stderr, "Deserialization Error: Not enough data for command type\n");
        return;
    }
    memcpy(&cmd->type, ptr, sizeof(cmd->type));
    ptr += sizeof(cmd->type);
    remaining -= sizeof(cmd->type);

    if (cmd->type == ADD_CMD) {
        // Deserialize ADD_CMD fields
        size_t len;

        // Deserialize user
        memcpy(&len, ptr, sizeof(len));
        ptr += sizeof(len);
        remaining -= sizeof(len);

        cmd->add.user = malloc(len);
        memcpy(cmd->add.user, ptr, len);
        ptr += len;
        remaining -= len;

        // Deserialize room
        memcpy(&len, ptr, sizeof(len));
        ptr += sizeof(len);
        remaining -= sizeof(len);

        cmd->add.room = malloc(len);
        memcpy(cmd->add.room, ptr, len);
        ptr += len;
        remaining -= len;

        // Deserialize message
        memcpy(&len, ptr, sizeof(len));
        ptr += sizeof(len);
        remaining -= sizeof(len);

        cmd->add.message = malloc(len);
        memcpy(cmd->add.message, ptr, len);
        ptr += len;
        remaining -= len;

        // Deserialize nTopics
        memcpy(&cmd->add.nTopics, ptr, sizeof(cmd->add.nTopics));
        ptr += sizeof(cmd->add.nTopics);
        remaining -= sizeof(cmd->add.nTopics);

        // Allocate topics array
        cmd->add.topics = malloc(sizeof(char *) * cmd->add.nTopics);

        // Deserialize topics
        for (size_t i = 0; i < cmd->add.nTopics; i++) {
            memcpy(&len, ptr, sizeof(len));
            ptr += sizeof(len);
            remaining -= sizeof(len);

            cmd->add.topics[i] = malloc(len);
            memcpy(cmd->add.topics[i], ptr, len);
            ptr += len;
            remaining -= len;
        }
    } else if (cmd->type == QUERY_CMD) {
        // Deserialize QUERY_CMD fields
        size_t len;

        // Deserialize room
        memcpy(&len, ptr, sizeof(len));
        ptr += sizeof(len);
        remaining -= sizeof(len);

        cmd->query.room = malloc(len);
        memcpy(cmd->query.room, ptr, len);
        ptr += len;
        remaining -= len;

        // Deserialize nTopics
        memcpy(&cmd->query.nTopics, ptr, sizeof(cmd->query.nTopics));
        ptr += sizeof(cmd->query.nTopics);
        remaining -= sizeof(cmd->query.nTopics);

        // Allocate topics array
        cmd->query.topics = malloc(sizeof(char *) * cmd->query.nTopics);

        // Deserialize topics
        for (size_t i = 0; i < cmd->query.nTopics; i++) {
            memcpy(&len, ptr, sizeof(len));
            ptr += sizeof(len);
            remaining -= sizeof(len);

            cmd->query.topics[i] = malloc(len);
            memcpy(cmd->query.topics[i], ptr, len);
            ptr += len;
            remaining -= len;
        }

        // Deserialize count
        memcpy(&cmd->query.count, ptr, sizeof(cmd->query.count));
        ptr += sizeof(cmd->query.count);
        remaining -= sizeof(cmd->query.count);
    }
    // Handle other command types similarly
}




