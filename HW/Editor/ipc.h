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

#ifndef _IPC_H_INCLUDED /* [ */
#define _IPC_H_INCLUDED

#define HWE_PORT        "hwedit"        /* Name of the message port */
#define HWE_TCP         "22763"         /* TCP addr to use if name not found */
#define HWE_PORT_AUX    "hwedit1"       /* Name of aux message port */
#define HWE_TCP_AUX     "22764"         /* TCP addr to use if name not found */

#define COMMAND_DONE    "<<<DONE>>>\n"
#define COMMAND_INVALID "<<<INVALID>>>\n"
#define COMMAND_BADARGS "<<<BADARGS>>>\n"
#define COMMAND_FAILED  "<<<FAILED>>>\n"
#define COMMAND_EXIT    "<<<EXIT>>>\n"

#if !defined(WIN32) /* [ */

#  define IPC_INVALID_SOCK -1

#else /* ] [ */

#  include <windows.h>
#  define IPC_INVALID_SOCK INVALID_SOCKET

#endif /* ] */

typedef int IPC_SOCK;

/* Maximum size message */
#define BSIZ    8192

int PrepareForIPC( void );
IPC_SOCK InitIPC( char *Host, char *Service );
int IPCServer( IPC_SOCK Sock, int Wait, IPC_SOCK *Socks, int NumSocks );
void TermIPC( IPC_SOCK Sock/*, int Final*/ );
int GetMsg( IPC_SOCK Sock, char msg[BSIZ] );
int PutMsg( char msg[BSIZ], IPC_SOCK Sock );
int IPCWait( IPC_SOCK Sock, long Wait );

#endif /* ] _IPC_H_INCLUDED */
