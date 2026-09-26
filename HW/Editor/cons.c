/* (c) Copyright Hewlett-Packard Company 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ipc.h"

int main(int argc, char **argv)
{
    char msg[BSIZ];
    char hostname[256];
    int sock;
    int done;
    int i, n;
    char *s;

    gethostname(hostname, sizeof(hostname));
    if (argc > 1) {
        (void)strcpy(hostname, argv[1]);
    }

    if (!PrepareForIPC()) exit(1);

    sock = InitIPC(hostname, HWE_PORT);
    if (sock < 0) sock = InitIPC(hostname, HWE_TCP);
    if (sock < 0) sock = InitIPC(hostname, HWE_PORT_AUX);
    if (sock < 0) sock = InitIPC(hostname, HWE_TCP_AUX);
    if (sock < 0) exit(1);

    while (!feof(stdin)) {
        *msg = 0;
        fgets(msg, BSIZ, stdin);

        /* Kill the newline */
        n = strlen(msg);
        if (n && (msg[n-1] == '\n')) {
            msg[n-1] = 0;
        }

        if (PutMsg(msg, sock) < 0) {
            exit(1);
        }
        do {
            done = 0;
            n = IPCWait(sock, 500);
            if (n < 0) {
                exit(1);
            }
            n = GetMsg(sock, msg);
            if (n < 0) {
                exit(1);
            }
            s = msg;
            while (n > 0) {
                i = strlen(s);
                if (strcmp(s, COMMAND_DONE) == 0) {
                    done = 1;
                }
                else if (strcmp(s, COMMAND_INVALID) == 0) {
                    fputs(s, stdout);
                    done = 1;
                }
                else if (strcmp(s, COMMAND_BADARGS) == 0) {
                    fputs(s, stdout);
                    done = 1;
                }
                else if (strcmp(s, COMMAND_FAILED) == 0) {
                    fputs(s, stdout);
                    done = 1;
                }
                else if (strcmp(s, COMMAND_EXIT) == 0) {
                    exit(0);
                }
                else {
                    fputs(s, stdout);
                }
                s += i + 1;
                n -= i + 1;
            }
        } while (!done);
    }
    return 0;
}

/*** EOF cons.c ***/
