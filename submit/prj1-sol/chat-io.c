#include "chat-io.h"

#include "chat.h"
#include "errnum.h"

#include <errors.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


/** This function should read commands from `in` and write successful
 *  responses to `out` and errors to `err` as per the following spec.
 *  It should flush prompt on `out` before each read from `in`.
 *
 *  A word is defined to be a maximal sequence of characters without
 *  whitespace.  We have the following types of words:
 *
 *    USER:  A word starting with a '@' character.
 *    TOPIC: A word starting with a '#' character.
 *    ROOM:  A word starting with a letter.
 *    COUNT: A non-negative integer.
 *
 *  Comparison of all the above words must be case-insensitive.
 *
 *  In the following description, we use regex-style notation:
 *
 *     X?:  Denotes an optional X.
 *     X*:  Denotes 0-or-more occurrences of X.
 *
 *  Errors are of two kinds:
 *
 *    *User Error* caused by incorrect user input.  This should result
 *    in a single line error message being printed on `err` starting
 *    with a specific error code (defined below) followed by a single
 *    ": " followed by an informational message; the informational
 *    message is not defined by this spec but can be any text giving
 *    the details of the error.  The function should continue reading
 *    the next input after outputting the user error.
 *
 *    *System Error* caused by a system error like a memory allocation
 *    failure, an I/O error or an internal error (like an assertion
 *    failure).  The program should terminate (with a non-zero status)
 *    after printing a suitable message on `err` (`stderr` if it is
 *    an assertion failure).
 *
 *  There are two kinds of commands:
 *
 *  1) An ADD command consists of the following sequence of words
 *  starting with the "+" word:
 *
 *    + USER ROOM TOPIC*
 *
 *  followed by *one*-or-more lines constituting a MESSAGE terminated
 *  by a line containing only a '.'.  Note that the terminating line
 *  is not included in the MESSAGE.
 *
 *  A sucessful ADD command will add MESSAGE to chat-room ROOM on
 *  behalf of USER associated with the specified TOPIC's (if any).  It
 *  will succeed silently and will not produce any output on out or
 *  err.
 *
 *  An incorrect ADD command should output a single line on `err`
 *  giving the first error detected from the following possiblities:
 *
 *    BAD_USER:  USER not specified or USER does not start with '@'
 *    BAD_ROOM:  ROOM not specified or ROOM does not start with a
 *               alphabetic character.
 *    BAD_TOPIC: TOPIC does not start with a '#'.
 *    NO_MSG:    message is missing
 *
 *  It is not an error for an ADD command to specify duplicate TOPIC's.
 *
 *  2) A QUERY command consists of a single line starting with the "?"
 *  word:
 *
 *    ? ROOM COUNT? TOPIC*
 *
 *   followed by a terminating line containing only a '.'.
 *
 *   If specified, COUNT must specify a positive integer.  If not
 *   specified, it should default to 1.
 *
 *   A successful QUERY must output (on `out`) up to the last COUNT
 *   messages from chat-room ROOM which match all of the specified
 *   TOPIC's in a LIFO order.
 *
 *   Each matching message must be preceded by a line containing
 *
 *      USER ROOM TOPIC*
 *
 *   where USER is the user who posted the message, ROOM is the
 *   queried ROOM and TOPIC's are all the *distinct* topics associated
 *   with the message in the same order as when added. In case there
 *   are duplicate topics, only the first occurrence of that topic is
 *   shown.  The USER, ROOM and TOPICs must be output in lower-case.  The
 *   message contents must be output with the same whitespace content
 *   as when input.
 *
 *   If there are no messages matching the specified QUERY, then no
 *   output should be produced. If there are fewer than COUNT matching
 *   messages, then those messages should be output without any error.
 *
 *  An incorrect QUERY command should output a single line on `err`
 *  giving the first error detected from the following possiblities:
 *
 *    BAD_ROOM:  ROOM not specified or ROOM does not start with a
 *               alphabetic character.
 *               or room has not been specified in any added message.
 *    BAD_COUNT: COUNT is not a positive integer.
 *    BAD_TOPIC: A TOPIC does not start with a '#" or it
 *               has not been specified in any added message.
 *
 *  If the command is not a ADD or QUERY command then an error with
 *  code BAD_COMMAND should be output on `err`.
 *
 *  A response from either kind of command must always be followed by
 *  a single empty line on `out`.
 *
 *  There should be no hard-coded limits: specifically no limits
 *  beyond available memory on the size of a message, the number of
 *  messages or the number of TOPIC's associated with a message.
 *
 *  Under normal circumstances, the function should return only when
 *  EOF is detected on `in`.  All allocated memory must have been
 *  deallocated before returning.
 *
 *  When a system error is detected, the function should terminate the
 *  program with a non-zero exit status after outputting a suitable
 *  message on err (or stderr for an assertion failure).  Memory need
 *  not be cleaned up.
 *
 */


 #define MAX_USER_LENGTH 100
 #define MAX_ROOM_LENGTH 100
 #define MAX_TOPIC_LENGTH 100
 #define MAX_MESSAGE_LENGTH 1024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_STRING_LENGTH 100

// Function to copy a string into newly allocated memory
char* allocate_and_copy_string(const char *str) {
    if (str == NULL) return NULL;
    size_t length = strlen(str) + 1; // +1 for the null terminator
    char *copy = malloc(length);
    if (copy == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    strcpy(copy, str);
    return copy;
}

char* concatenate_message(char *message, const char *line) {
    size_t current_length = message ? strlen(message) : 0;
    size_t line_length = strlen(line);
    char *new_message = realloc(message, current_length + line_length + 2); // +2 for newline and null terminator

    if (new_message == NULL) {
        return NULL; // Memory allocation failed
    }

    if (message) {
        strcat(new_message, line);
    } else {
        strcpy(new_message, line);
    }
//    strcat(new_message, "\n"); // Append newline

    return new_message;
}

void trim_whitespace(char *str) {
    if (str == NULL) {
        return; // Handle null pointer
    }

    // Trim leading whitespace
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // If the string is all whitespace
    if (*start == '\0') {
        *str = '\0'; // Empty string
        return;
    }

    // Trim trailing whitespace
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }

    // Null-terminate the trimmed string
    *(end + 1) = '\0';

    // Move trimmed string to the beginning
    memmove(str, start, strlen(start) + 1);
}


void chat_io(const char *prompt, FILE *in, FILE *out, FILE *err) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    ErrNum errnum = NO_ERR;

    while ((read = getline(&line, &len, in)) != -1) {
        // Strip newline character
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        // Determine command type
        trim_whitespace(line);
        if (line[0] == '+') {
            // Handle ADD command
            char *token = strtok(line + 1, " ");  // Skip the leading '+'
            char *user = NULL;
            char *room = NULL;
            char **topics = NULL;
            size_t num_topics = 0;
            char *message = NULL;

            // Parses user
            if (token && token[0] == '@') {
                user = allocate_and_copy_string(token);
                token = strtok(NULL, " ");
            } else {
                fprintf(err, "BAD_USER\n");
                continue;
            }
            //parses room
            if (token && isalpha(token[0])) {
                room = allocate_and_copy_string(token);
                token = strtok(NULL, " ");
            } else {
                fprintf(err, "BAD_ROOM\n");
                free(user);
                continue;
            }

            // // Parses topics
            if(token && token[0] == '#') {
                //Multiple topics
                while (token && token[0] == '#') {
                    if (num_topics == 0) {
                        topics = (char **)malloc(sizeof(char *));
                    } else {
                        topics = (char **)realloc(topics, (num_topics + 1) * sizeof(char *)); //multiple topics
                    }
                    if (topics == NULL) {
                        perror("Failed to allocate memory for topics");
                        free(user);
                        free(room);
                        continue;
                    }
                    topics[num_topics] = allocate_and_copy_string(token);
                    to_lowercase(topics[num_topics]);
                    num_topics++;
                    token = strtok(NULL, " ");
                }
            } else {
                fprintf(err, "BAD_TOPIC\n");
                free(user);
                free(room);
                continue;
            }

            // Collect message lines until 'period'
            while ((read = getline(&line, &len, in)) != -1) {
                // Check for end of message input
                if (line[0] == '.') {
                    break;  // End of message input
                }
                message = concatenate_message(message, line);
                if (message == NULL) {
                    fprintf(err, "NO_MSG\n");
                    free(user);
                    free(room);
                    for (size_t i = 0; i < num_topics; i++) {
                        free(topics[i]);
                    }
                    free(topics);
                    free(line);
                    return;
                }
            }

            // Create and store chat message
            if (user && room && message) {
                ChatMsg *chat_msg = create_chat_message(user, room, message, topics, num_topics, &errnum);
                if (errnum == NO_ERR) {
                    // Example: Store or handle the chat message
                    add_chat_msg(chat_msg);
                } else {
                    fprintf(err, "Error creating chat message: %s\n", errnum_to_string(errnum));
                }
            } else {
                fprintf(err, "NO_MSG\n");
            }

            // Free topics array
            for (size_t i = 0; i < num_topics; i++) {
                free(topics[i]);
            }
            free(topics);
            free(user);
            free(room);

        } else if (line[0] == '?') {
            // Handle QUERY command
            char *token = strtok(line + 2, " ");  // Skip the leading '?'
            char *room = NULL;
            size_t count = 1;  // Default count
            char **topics = NULL;
            size_t num_topics = 0;

            // Parse QUERY command components: room
            if (token && isalpha(token[0])) {
                room = allocate_and_copy_string(token);
                token = strtok(NULL, " ");
            } else {
                fprintf(err, "BAD_ROOM\n");
                continue;
            }

            if (token && isdigit(token[0])) {
                count = (size_t)atoi(allocate_and_copy_string(token));
                if(count < 0) {
                    fprintf(err, "BAD_COUNT\n");
                    free(room);
                    continue;
                }
                token = strtok(NULL, " ");
            }

            // Collect topics
            if(token && token[0] == '#') {
                while (token && token[0] == '#') {
                    if (num_topics == 0) {
                        topics = (char **)malloc(sizeof(char *));
                    } else {
                        topics = (char **)realloc(topics, (num_topics + 1) * sizeof(char *));
                    }
                    if (topics == NULL) {
                        perror("Failed to allocate memory for topics");
                        free(room);
                        continue;
                    }
                    topics[num_topics] = allocate_and_copy_string(token);
                    to_lowercase(topics[num_topics]);
                    num_topics++;
                    token = strtok(NULL, " ");
                }
            }  else if(token) {
                fprintf(err, "BAD_TOPIC\n");
                free(room);
                continue;
            }

            // Perform query and display results
            display_chat_messages(count, room, topics, num_topics, err);

            // Free topics array
            for (size_t i = 0; i < num_topics; i++) {
                free(topics[i]);
            }
            free(topics);
            free(room);
        } else if (line[0] != '.') {
               fprintf(err, "BAD_COMMAND\n");
        }
    }

    // Free the line buffer
    free(line);
}

void to_lowercase(char *str) {
    if (str == NULL) {
        return; // Handle null pointer
    }

    for (size_t i = 0; str[i] != '\0'; i++) {
        str[i] = tolower((unsigned char) str[i]);
    }
}

//#define NO_CHAT_IO_MAIN to allow an alternate main()
#ifndef NO_CHAT_IO_MAIN

#include <unistd.h>

int main(int argc, const char *argv[]) {
  bool isInteractive = isatty(fileno(stdin));
  const char *prompt = isInteractive ? "> " : "";
  FILE *err = stderr;
  chat_io(prompt, stdin, stdout, err);
}

#endif //#ifndef NO_CHAT_IO_MAIN