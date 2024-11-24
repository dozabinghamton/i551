#include "server.h"
#include "common.h"

#include <errors.h>
#include <chat-db.h>

// Uncomment next line to turn on tracing; use TRACE() with printf-style args
// #define DO_TRACE
#include <trace.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Iterator function for query results
static int query_result_iterator(const ChatInfo *chatInfo, void *context) {
    Shm *sharedMem = (Shm *)context;
    char *response = sharedMem->buf;

    // Calculate the remaining buffer size
    size_t len = strlen(response);
    size_t remainingSize = sharedMem->bufSize - len - 1; // Reserve space for null terminator
    if (remainingSize <= 0) {
        // Buffer is full; stop processing further results
        return 1; // Returning non-zero stops the iteration
    }

    // Format the chat info
    int written = snprintf(response + len, remainingSize,
                           "%s %s %s [",
                           chatInfo->user, chatInfo->room, chatInfo->message);

    if (written < 0 || (size_t)written >= remainingSize) {
        // Output was truncated or an error occurred
        return 1;
    }

    len += written;
    remainingSize = sharedMem->bufSize - len - 1;

    // Append topics
    for (size_t i = 0; i < chatInfo->nTopics; i++) {
        if (remainingSize <= 0) {
            return 1;
        }

        written = snprintf(response + len, remainingSize, "%s%s",
                           chatInfo->topics[i],
                           (i + 1 < chatInfo->nTopics) ? ", " : "");
        if (written < 0 || (size_t)written >= remainingSize) {
            return 1;
        }

        len += written;
        remainingSize = sharedMem->bufSize - len - 1;
    }

    // Close the topics array
    if (remainingSize > 0) {
        written = snprintf(response + len, remainingSize, "]\n");
        if (written < 0 || (size_t)written >= remainingSize) {
            return 1;
        }
    } else {
        return 1;
    }

    return 0; // Continue processing further results
}

void do_server(const char *dbPath, Shm *sharedMem) {
    ChatDb *chatDb = NULL;
    MakeChatDbResult result;

    if (make_chat_db(dbPath, &result) != 0) {
        fprintf(stderr, ERROR "SYS_ERR: Failed to open database\n");
        exit(EXIT_FAILURE);
    }

    chatDb = result.chatDb;

    while (true) {
        sem_wait(&sharedMem->clientSem); // Wait for the client command
        memset(sharedMem->buf, 0, sharedMem->bufSize); // Clear shared memory buffer

        // Deserialize the command from shared memory
        ChatCmd cmd;
        deserialize_chat_cmd(sharedMem, &cmd); // Extract the command
        snprintf(sharedMem->buf, sharedMem->shmSize, OKAY);

        // Handle END_CMD
        if (cmd.type == END_CMD) {
            snprintf(sharedMem->buf, sharedMem->shmSize, OKAY);
            sem_post(&sharedMem->serverSem); // Notify client
            break;
        }

        // Process the command
        if (cmd.type == QUERY_CMD) {
            const char *room = cmd.query.room; // Room name
            size_t nTopics = cmd.query.nTopics; // Number of topics
            const char **topics = cmd.query.topics; // Topics array
            size_t count = cmd.query.count; // Number of results to fetch

            // Use the iterator function to process query results
            int err = query_chat_db(chatDb, room, nTopics, topics, count,
                                    query_result_iterator, sharedMem);

            if (err != 0) {
                size_t len = strlen(sharedMem->buf);
                snprintf(sharedMem->buf + len, sharedMem->bufSize - len,
                         ERROR "BAD_QUERY: %s\n", error_chat_db(chatDb));
            } else {
                size_t len = strlen(sharedMem->buf);
                snprintf(sharedMem->buf + len, sharedMem->bufSize - len, OKAY);
            }

            // Signal the client that the response is ready
            sem_post(&sharedMem->serverSem);
        } else if (cmd.type == ADD_CMD) {
            const char *user = cmd.add.user;
            const char *room = cmd.add.room;
            const char *message = cmd.add.message;
            size_t nTopics = cmd.add.nTopics;
            const char **topics = cmd.add.topics;

            int errCode = add_chat_db(chatDb, user, room, nTopics, topics, message);
            if (errCode == 0) {
                snprintf(sharedMem->buf, sharedMem->shmSize, OKAY);
            } else {
                snprintf(sharedMem->buf, sharedMem->shmSize, ERROR "SYS_ERR: %s\n", error_chat_db(chatDb));
            }

            // Log only the values (without additional strings)
           // printf("%s %s %s %zu\n", user, room, message, nTopics);
        } else {
            snprintf(sharedMem->buf, sharedMem->shmSize, ERROR "SYS_ERR: Unknown command\n");
        }

        // Signal the client that the response is ready
        sem_post(&sharedMem->serverSem);
    }

    free_chat_db(chatDb);
    exit(EXIT_SUCCESS);
}