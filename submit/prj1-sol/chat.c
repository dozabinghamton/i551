#include <ctype.h>
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#include "chat.h"
#include "errnum.h"
// #define DO_TRACE
#include <stdbool.h>
#include <trace.h>

ChatMsgNode *head = NULL;

// TODO: define chat ADT

// This function takes pointer of a chatMsg stucture and returns pointer of newly created ChatMsgNode
// We are allocating memory using malloc of size specified
//Based on the condition if node  is null we are sending message suitable.

//Else Case proceeds normally
// We will link it to newNode and sets it next to null as we normally  do in Linked List

ChatMsgNode *create_chat_msg_node(ChatMsg *chat_msg) {

  ChatMsgNode *node = malloc(sizeof(ChatMsgNode));
  if (node == NULL) {
    perror("Failed to allocate memory for chat message node");
    exit(EXIT_FAILURE);
  }
  node->chat_msg = chat_msg;
  node->next = NULL;
  return node;
}


// Function to add a chat message to the linked list
// Adding the msg into linked List making it as first Node of our list
// Same we are allocating the memory using malloc
//then checking if newNode is Null we are cleaning the memory

//Else
// Intializing the new node and then set it to head;
void add_chat_msg(ChatMsg *msg) {

  ChatMsgNode *new_node = malloc(sizeof(ChatMsgNode));
  if (new_node == NULL) {
      free_chat_message(msg); // Clean up allocated ChatMsg
      return;
  }

  new_node->chat_msg = msg;
  new_node->next = head;
  head = new_node;

}


// Function to safely copy a string with dynamic memory allocation
//Creating a copied string of the string that came.
// If string coming is null, then we will throw error.
// allocation of +1 in string size is done because of Null terminator.

char *copy_string(const char *str, ErrNum *err) {
  if (str == NULL) {
    *err = MEM_ERR;
    return NULL;
  }
  size_t length = strlen(str) + 1;  // +1 for null terminator
  char *copy = malloc(length);
  if (copy == NULL) {
    *err = MEM_ERR;
    return NULL;
  }
  strcpy(copy, str);
  *err = NO_ERR;
  return copy;
}

// Function to create a chat message

// This the method reponsbile for creating chat messages.
//Memory Allocation for the chat structure is done
//Error handling is also done as per the condition
ChatMsg *create_chat_message(const char *user, const char *room,
                             const char *message, char **topics,
                             size_t num_topics, ErrNum *err) {
  ChatMsg *chat_msg = malloc(sizeof(ChatMsg));
  if (chat_msg == NULL) {
    *err = MEM_ERR;
    return NULL;
  }

  // Copy user, room, and message strings
  
  chat_msg->user = copy_string(user, err);
  if (*err != NO_ERR) {
    free(chat_msg);  // Free the previously allocated memory
    return NULL;
  }
 

 // Room copy
  chat_msg->room = copy_string(room, err);
  if (*err != NO_ERR) {
    free(chat_msg->user);  // Free previously allocated memory
    free(chat_msg);
    return NULL;
  }

//Message copy
  chat_msg->message = copy_string(message, err);
  if (*err != NO_ERR) {
    free(chat_msg->room);  // Free previously allocated memory
    free(chat_msg->user);
    free(chat_msg);
    return NULL;
  }

  // Allocate memory for topics array
  // Allocating the memory for array of pointers

  chat_msg->topics = malloc(num_topics * sizeof(char *));
  if (!chat_msg->topics) {
    *err = MEM_ERR;
    free(chat_msg->message);  
    free(chat_msg->room);
    free(chat_msg->user);
    free(chat_msg);
    return NULL;
  }

  // Copy each topic string
  chat_msg->num_topics = num_topics;
  for (size_t i = 0; i < num_topics; i++) {
    chat_msg->topics[i] = copy_string(topics[i], err);
    if (*err != NO_ERR) {
      // Free previously allocated topic strings
      for (size_t j = 0; j < i; j++) {
        free(chat_msg->topics[j]);
      }
      free(chat_msg->topics);
      free(chat_msg->message);
      free(chat_msg->room);
      free(chat_msg->user);
      free(chat_msg);
      return NULL;
    }
  }

  *err = NO_ERR;
  return chat_msg;
}

// Function to free a chat message

// Kind of Helper function to clear the chat msgs
//Free memory of user, room and strings

void free_chat_message(ChatMsg *chat_msg) {
  if (chat_msg==NULL)
  {
     return;
  } 

  
  free(chat_msg->user);
  free(chat_msg->room);
  free(chat_msg->message);

  // Free each topic
  for (size_t i = 0; i < chat_msg->num_topics; i++) {
    free(chat_msg->topics[i]);
  }

  // Free topics array
  free(chat_msg->topics);

  // Free the ChatMsg structure itself
  free(chat_msg);
}

// Function to diplay chat message based on room and topics
void display_chat_messages(size_t count, char *room, char **topics, size_t num_topics, FILE *err) {
  // Traverse the list and push messages onto the stack
  ChatMsgNode *current = head;
  size_t current_count = 0;
  bool found = false;
  while (current != NULL && current_count < count) {
    ChatMsg *chat_msg = current->chat_msg;
    if (strcmp(room, current->chat_msg->room) == 0 && message_matches_topics(chat_msg, topics, num_topics)) {
      // Print message details
      fprintf(err, "%s %s ", chat_msg->user, chat_msg->room);

      for (size_t i = 0; i < chat_msg->num_topics; i++) {
        //printf("%s", chat_msg->topics[i]);
        fprintf(err, "%s", chat_msg->topics[i]);

        if(i + 1 < chat_msg->num_topics) {
            fprintf(err, " ");
        }
      }
      fprintf(err, "\n");
      fprintf(err, "%s", chat_msg->message);

      found = true;
      current_count++;
    }
    current = current->next;
  }
  if(found == false){
      if(!is_valid_room(room)){
        fprintf(err, "BAD_ROOM\n");
        }
  }
  if(found == false && num_topics > 0){
     if(!is_valid_topics(topics, num_topics)){
        fprintf(err, "BAD_TOPIC\n");
    }
  }

}

//Checking if a room exists in linkedList

bool is_valid_room(char *room) {
ChatMsgNode *current = head;
  while (current != NULL) {
    if (strcmp(room, current->chat_msg->room) == 0) {
      return true;
    }
    current = current->next;
  }
  return false;
}
// Same checking if topics are valid

bool is_valid_topics(char **topics, size_t num_topics){
    if (num_topics == 0) 
    {
        return true;

     }
          // No topics to match, always true

      // Iterate through each topic in the chat message

      for (size_t i = 0; i < num_topics; i++) {
        bool found = false;
        ChatMsgNode *current = head;
          while (current != NULL) {
          ChatMsg *chat_msg = current->chat_msg;
            for (size_t j = 0; j < chat_msg->num_topics; j++) {
              if (strcmp(chat_msg->topics[j], topics[i]) == 0) {
                found = true;
                break;
              }
            }
            if(found == true){
                break;
            }
            current = current->next;
        }

        if (!found) return false;  // If any topic is not found, return false
      }
  return true;
  
}



void free_chats() {
  ChatMsgNode *current = head;
  while (current != NULL) {
    ChatMsgNode *next = current->next;
    free(current->chat_msg->user);
    free(current->chat_msg->room);
    free(current->chat_msg->message);
    for (size_t i = 0; i < current->chat_msg->num_topics; i++) {
      free(current->chat_msg->topics[i]);
    }
    free(current->chat_msg->topics);
    free(current->chat_msg);
    free(current);
    current = next;
  }
}

bool message_matches_topics(ChatMsg *chat_msg, char **topics,
                            size_t num_topics) {
  if (num_topics == 0) 
  {
    return true; 
  }
     // No topics to match, always true

  // Iterate through each topic in the chat message
  for (size_t i = 0; i < num_topics; i++) {
    bool found = false;
    for (size_t j = 0; j < chat_msg->num_topics; j++) {
      if (strcmp(chat_msg->topics[j], topics[i]) == 0) {
        found = true;
        break;
      }
    }
    if (!found) 
    {
        return false; 
     }
         // If any topic is not found, return false
  }
  return true;  // All topics are matched
}
