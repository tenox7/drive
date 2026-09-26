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

/* Internal hoverware mutex stuff */

#if !defined(HW_MUTEX_H_INCLUDED) /* [ */

#define HW_MUTEX_H_INCLUDED 1

#if defined(WIN32) /* [ */

#undef ERROR    // Stupid Windows.  What a silly thing to have defined.

extern HANDLE __hwMutex;
extern DWORD __hwTlsSlot;

#define HW_DECLARE_MUTEX(mutex)         HANDLE mutex
#define HW_INITIALIZE_MUTEX(mutex)      mutex = CreateMutex(NULL, FALSE, NULL)
#define HW_DESTROY_MUTEX(mutex)         CloseHandle(mutex)
#define HW_LOCK_MUTEX(mutex)            WaitForSingleObject(mutex, INFINITE)
#define HW_UNLOCK_MUTEX(mutex)          ReleaseMutex(mutex)

#define HW_SET_CURR_DISP(disp)          TlsSetValue(__hwTlsSlot, disp)
#define HW_GET_CURR_DISP()              TlsGetValue(__hwTlsSlot)

#else /* ] [ */

#include <pthread.h>
extern pthread_mutex_t __hwMutex;
extern pthread_key_t __hwTlsSlot;

#define HW_DECLARE_MUTEX(mutex)         pthread_mutex_t mutex
#define HW_INITIALIZE_MUTEX(mutex)      pthread_mutex_init(&mutex, NULL)
#define HW_DESTROY_MUTEX(mutex)         pthread_mutex_destroy(&mutex)
#define HW_LOCK_MUTEX(mutex)            pthread_mutex_lock(&mutex)
#define HW_UNLOCK_MUTEX(mutex)          pthread_mutex_unlock(&mutex)

#define HW_SET_CURR_DISP(disp)          pthread_setspecific(__hwTlsSlot, disp)
#define HW_GET_CURR_DISP()              pthread_getspecific(__hwTlsSlot)

#endif /* ] */

#define HW_DECLARE_GLOBAL_MUTEX         HW_DECLARE_MUTEX(__hwMutex)
#define HW_INITIALIZE_GLOBAL_MUTEX()    HW_INITIALIZE_MUTEX(__hwMutex)
#define HW_GLOBAL_LOCK()                HW_LOCK_MUTEX(__hwMutex)
#define HW_GLOBAL_UNLOCK()              HW_UNLOCK_MUTEX(__hwMutex)

/* Macros to help object hash creation code */
#define HW_INSERT(a,b,c)        if(!hwInsert(a,b,c)) goto ERROR

#define HW_HASH_SETUP(t,n)  \
    switch(__hwAllocTabLock(&t,n)) { \
    case 1 :

#define HW_HASH_CLEANUP \
        HW_GLOBAL_UNLOCK(); \
        break; \
    case 0 : \
        break; \
    ERROR : \
        HW_GLOBAL_UNLOCK(); \
    default : \
        return 0; \
    }

#endif /* ] */
