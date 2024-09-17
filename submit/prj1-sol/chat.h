#ifndef CHAT_H_
#define CHAT_H_

#include "errnum.h"
#include "msgargs.h"

#include <stdbool.h>
#include <stddef.h>

//TODO: define API for chat ADT

//ash start

// Structure for holding a chat message
typedef struct ChatMsg {
    char *user;      // pointer to user string
    char *room;      // pointer to room string
    char **topics;   // pointer to an array of topic strings
    char *message;   // pointer to the message string
    size_t num_topics; // number of topics
} ChatMsg;

// Node of a linked list to hold chat messages
typedef struct ChatMsgNode {
    ChatMsg *chat_msg;      // Pointer to the ChatMsg
    struct ChatMsgNode *next; // Pointer to the next node
} ChatMsgNode;

// Function prototypes

// Function to create a chat message
ChatMsg* create_chat_message(const char *user, const char *room, const char *message, char **topics, size_t num_topics, ErrNum *err);

// Function to free a chat message
void free_chat_message(ChatMsg *chat_msg);

// Function to copy a string safely
char* copy_string(const char *source, ErrNum *err);

// Function to display chat message for debugging purposes
void display_chat_messages(size_t count, char *room, char **topics, size_t num_topics, FILE *err);

void add_chat_msg(ChatMsg *msg);

ChatMsgNode* create_chat_msg_node(ChatMsg *chat_msg);

void free_chats(void);

bool message_matches_topics(ChatMsg *chat_msg, char **topics, size_t num_topics);

bool is_valid_room(char *room);

bool is_valid_topics(char **topics, size_t num_topics);


//ash end

#endif //#ifndef CHAT_H_