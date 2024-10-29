#include "utils.h"

#include <chat-cmd.h>
#include <chat-db.h>
#include <errors.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/** Invoked with two arguments:
 *
 *    SERVER_DIR: the path to the directory in which the server should
 *    run and where all FIFOs will be created.
 *
 *    DBFILE_PATH: path to the sqlite file.  This must be relative to
 *    SERVER_DIR.
 *
 *  The server may use `stderr` for "logging", but all such logging
 *  *must* be turned off before submission.
 */
int
main(int argc, const char *argv[])
{
  //TODO
  return 0;
}
