/* Header for poll(2) emulation
   Contributed by Paolo Bonzini.

   Copyright 2001, 2002, 2003, 2007, 2009, 2010 Free Software Foundation, Inc.

   This file is part of gnulib.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _DEMO_POLL_H
#define _DEMO_POLL_H

#include <CommonAPI/MainLoopContext.h>
#include <CommonAPI/pollStructures.h>

struct DemoPollFd : public COMMONAPI_POLLFD {
	bool isWsaEvent;
	DemoPollFd() = default;
	DemoPollFd(const COMMONAPI_POLLFD& internalPollFd) : COMMONAPI_POLLFD(internalPollFd) {	}
};


typedef unsigned long nfds_t;

extern int poll(DemoPollFd *pfd, nfds_t nfd, int timeout);

/* Define INFTIM only if doing so conforms to POSIX.  */
#if !defined (_POSIX_C_SOURCE) && !defined (_XOPEN_SOURCE)
#define INFTIM (-1)
#endif

#endif /* _DEMO_POLL_H */
