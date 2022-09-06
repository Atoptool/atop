/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This program concatenates several raw logfiles into one output stream,
** to be stored as new file or to be passed via a pipe to atop/atopsar directly.
** ==========================================================================
** Author:      zhenwei pi
** E-mail:      pizhenwei@bytedance.com
** Initial:     Jul 2022
** --------------------------------------------------------------------------
** Copyright (C) 2022 zhenwei pi
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2, or (at your option) any
** later version.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
** --------------------------------------------------------------------------
*/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "atop.h"

#define OUTBUF_DEF_SIZE (1024 * 1024)

static void output_buf(struct output *op, char *buf, int size)
{
	if (!op->ob.buf)
	{
		op->ob.size = OUTBUF_DEF_SIZE;
		op->ob.buf = calloc(1, op->ob.size);
	}

	/* no enought buf, grow it */
	if (op->ob.size - op->ob.offset < size)
	{
		op->ob.size *= 2;
		op->ob.buf = realloc(op->ob.buf, op->ob.size);
	}

	memcpy(op->ob.buf + op->ob.offset, buf, size);
	op->ob.offset += size;
}

void output_samp(struct output *op, char *buf, int size)
{
	switch (op->output_type)
	{
		case OUTPUT_STDOUT:
		printf("%s", buf);
		break;

		case OUTPUT_FD:
		write(op->fd, buf, size);
		break;

		case OUTPUT_BUF:
		output_buf(op, buf, size);
		break;

		default:
		fprintf(stderr, "Error, unknown output type\n");
		exit(EINVAL);
	}
}

void output_samp_done(struct output *op)
{
	if (op->done)
		op->done(op);

	if (op->output_type == OUTPUT_BUF)
	{
		memset(op->ob.buf, 0x00, op->ob.offset);
		op->ob.offset = 0;
	}
}
