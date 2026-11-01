/*
** ATOP - System & Process Monitor
**
** Shared base type definitions used across multiple top-level headers.
** Keeping these in a single low-level header avoids that high-level
** headers (photoproc.h, photosyst.h, ...) have to include each other
** and prevents scattered/repeated type declarations.
** ==========================================================================
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2, or (at your option) any
** later version.
*/

#ifndef __ATOPTYPES__
#define __ATOPTYPES__

typedef long long count_t; // type for counters (perfmetrics)

struct tstat;
struct devtstat;
struct sstat;
struct cgchainer;
struct netpertask;
struct pergpu;

#endif
