/*
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
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/tcp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <zlib.h>

#include "atop.h"
#include "json.h"

#define INBUF_SIZE	4096
#define URL_LEN		1024

static int listenfd = -1;
static int clifd = -1;

static char *http_200 = "HTTP/1.1 200 OK\r\n";
static char *http_404 = "HTTP/1.1 404 Not Found\r\n";
static char *http_generic = "Server: atop\r\n"
"Content-Type: %s; charset=utf-8\r\n"
"Content-Length: %d\r\n\r\n";

static char *http_generic_encode = "Server: atop\r\n"
"Content-Encoding: deflate\r\n"
"Content-Type: %s; charset=utf-8\r\n"
"Content-Length: %d\r\n\r\n";

static char *http_content_type_html = "text/html";
static char *http_content_type_css = "text/css";
static char *http_content_type_javascript = "application/javascript";

static void http_prepare_response()
{
	/* try to send response data in a timeout */
	int onoff = 1;
	setsockopt(clifd, IPPROTO_TCP, TCP_NODELAY, &onoff, sizeof(onoff));

	fcntl(clifd, F_SETFL, fcntl(clifd, F_GETFL) & ~O_NONBLOCK);

	struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
	setsockopt(clifd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

}

static void http_response_200(char *buf, size_t len, int encoding, char* content_type)
{
	struct iovec iovs[3], *iov;

	http_prepare_response();

	/* 1, http code */
	iov = &iovs[0];
	iov->iov_base = http_200;
	iov->iov_len = strlen(http_200);

	/* 2, http generic content */
	iov = &iovs[1];
	char content[128] = {0};
	int content_length = 0;
	if (encoding)
	    content_length = sprintf(content, http_generic_encode, content_type, len);
	else
	    content_length = sprintf(content, http_generic, content_type, len);
	iov->iov_base = content;
	iov->iov_len = content_length;

	/* 3, sample data record */
	iov = &iovs[2];
	iov->iov_base = buf;
	iov->iov_len = len;

	writev(clifd, iovs, sizeof(iovs) / sizeof(iovs[0]));

	close(clifd);
}

static void http_show_samp_done(struct output *op)
{
	char *compbuf = malloc(op->ob.offset);
	unsigned long complen = op->ob.offset;

	if (compress((Bytef *)compbuf, &complen, (Bytef *)op->ob.buf, op->ob.offset) != Z_OK){
		http_prepare_response();
		write(clifd, http_404, strlen(http_404));
		exit(0);
	}

	http_response_200(compbuf, complen, 1, http_content_type_html);
	/* we usually response a single record */
	exit(0);
}

static int http_arg_long(char *req, char *needle, long *l)
{
	char *s = strstr(req, needle);
	if (!s)
		return -1;

	s += strlen(needle);
	if (*s != '=')
		return -1;

	*l = atol(++s);

	return 0;
}

static int http_arg_str(char *req, char *needle, char *p, int len)
{
        char *s = strstr(req, needle);
        if (!s)
                return -1;

        s += strlen(needle);
        if (*s++ != '=')
                return -1;

        char *e = strchr(s, '&');
        if (!e)
                e = s + strlen(s);

	if (e - s > len)
		return -1;

        memcpy(p, s, e - s);
        p[e - s] = '\0';
	while ((p = strstr(p, "%2C")) != NULL)
	{
		*(p++) = ',';
		memmove(p, p + 2, strlen(p));
	}

        return 0;
}

static void http_showsamp(char *req)
{
	time_t timestamp = 0;
	char lables[1024];

	if (http_arg_long(req, "timestamp", &timestamp) < 0)
		return;

	if (http_arg_str(req, "lables", lables, sizeof(lables)) < 0)
		return;

	pid_t pid = fork();
	if (pid != 0)
		return;

	jsondef(lables);
	vis.show_samp = jsonout;
	vis.op.output_type = OUTPUT_BUF;
	vis.op.done = http_show_samp_done;

	char timestr[10] = {0};
	convtime(timestamp, timestr);
	timestr[5] = '\0'; /* truncate time string from HH:MM:SS to HH:MM */
	getbranchtime(timestr, &begintime);
	begintime += timestamp % 60;

	struct tm *tt = localtime(&timestamp);
	sprintf(rawname, "%04d%02d%02d", tt->tm_year + 1900, tt->tm_mon + 1, tt->tm_mday);

	rawread();
}

/* Import a binary file */
#define IMPORT_BIN(sect, file, sym) asm (	\
	".section " #sect "\n"			\
	".global " #sym "\n"			\
	".global " #sym "_end\n"		\
	#sym ":\n"				\
	".incbin \"" file "\"\n"		\
	#sym "_end:\n"				\
	".section \".text\"\n")

/* Build favicon.ico into atop binary */
IMPORT_BIN(".rodata", "http/favicon.ico", favicon);
extern char favicon[], favicon_end[];

static void http_favicon()
{
	http_response_200(favicon, favicon_end - favicon, 0, http_content_type_html);
}

/* Build index.html into atop binary */
IMPORT_BIN(".rodata", "http/index.html", http_index_html);
extern char http_index_html[], http_index_html_end[];

static void http_index()
{
	http_response_200(http_index_html, http_index_html_end - http_index_html, 0, http_content_type_html);
}

/* Build atop.js into atop binary */
IMPORT_BIN(".rodata", "http/js/atop.js", http_js);
extern char http_js[], http_js_end[];

static void http_get_js()
{
	http_response_200(http_js, http_js_end - http_js, 0, http_content_type_javascript);
}

/* Build atop.css into atop binary */
IMPORT_BIN(".rodata", "http/css/atop.css", http_css);
extern char http_css[], http_css_end[];

static void http_get_css()
{
	http_response_200(http_css, http_css_end - http_css, 0, http_content_type_css);
}

/* Build template.html into atop binary */
IMPORT_BIN(".rodata", "http/template/html/generic.html", generic_html_template);
extern char generic_html_template[], generic_html_template_end[];

IMPORT_BIN(".rodata", "http/template/html/memory.html", memory_html_template);
extern char memory_html_template[], memory_html_template_end[];

IMPORT_BIN(".rodata", "http/template/html/disk.html", disk_html_template);
extern char disk_html_template[], disk_html_template_end[];

IMPORT_BIN(".rodata", "http/template/html/command_line.html", command_line_html_template);
extern char command_line_html_template[], command_line_html_template_end[];

static void http_get_template(char *req)
{
	char template_type[256];
	if (http_arg_str(req, "type", template_type, sizeof(template_type)) < 0)
		return;

	if (!strcmp(template_type, "generic")) {
		http_response_200(generic_html_template, generic_html_template_end - generic_html_template, 0, http_content_type_html);
	} else if (!strcmp(template_type, "memory")) {
		http_response_200(memory_html_template, memory_html_template_end - memory_html_template, 0, http_content_type_html);
	} else if (!strcmp(template_type, "disk")) {
		http_response_200(disk_html_template, disk_html_template_end - disk_html_template, 0, http_content_type_html);
	} else if (!strcmp(template_type, "command_line")) {
		http_response_200(command_line_html_template, command_line_html_template_end - command_line_html_template, 0, http_content_type_html);
	} else {
		http_prepare_response();
		write(clifd, http_404, strlen(http_404));
	}
}

static void http_ping()
{
	char *pong = "pong\r\n";

	http_response_200(pong, strlen(pong), 0, http_content_type_html);
}

static void http_process_request(char *req)
{
	char location[URL_LEN] = {0};
	char *c;

	c = strchr(req, '?');
	if (c)
		memcpy(location, req, c - req);
	else
		memcpy(location, req, strlen(req));

	if (strlen(location) == 0)
	{
		http_index();
		return;
	}

	if (!strcmp(location, "ping"))
		http_ping();
	else if (!strcmp(location, "favicon.ico"))
		http_favicon();
	else if (!strcmp(location, "showsamp"))
		http_showsamp(req);
	else if (!strcmp(location, "index.html"))
		http_index();
	else if (!strcmp(location, "js/atop.js"))
		http_get_js();
	else if (!strcmp(location, "css/atop.css"))
		http_get_css();
	else if (!strcmp(location, "template"))
		http_get_template(req);
	else
	{
		http_prepare_response();
		write(clifd, http_404, strlen(http_404));
	}
}

static time_t httpd_now_ms()
{
	struct timeval now;

	gettimeofday(&now, NULL);

	return now.tv_sec * 1000 + now.tv_usec / 1000;
}

static void httpd_handle_request()
{
	char inbuf[INBUF_SIZE] = {0};
	int inbytes = 0;
	char httpreq[URL_LEN] = {0};
	time_t timeout = httpd_now_ms() + 100;

	fcntl(clifd, F_SETFL, fcntl(clifd, F_GETFL) | O_NONBLOCK);

	for ( ; ; )
	{
		time_t now = httpd_now_ms();
		if (now >= timeout)
			goto closefd;

		struct pollfd pfd = {.fd = clifd, .events = POLLIN, .revents = 0};
		poll(&pfd, 1, timeout - now);

		int ret = read(clifd, inbuf + inbytes, sizeof(inbuf) - inbytes);
		if (ret < 0)
		{
			if (errno != EAGAIN)
				goto closefd;
			continue;
		}

		inbytes += ret;
		if ((inbytes >= 4) && strstr(inbuf, "\r\n\r\n"))
			break;

		/* buf is full, but we can not search the ent of HTTP header */
		if (inbytes == sizeof(inbuf))
			goto closefd;
	}

	/* support GET request only */
	if (strncmp("GET ", inbuf, 4))
		goto closefd;

	/* support HTTP 1.1 request only */
	char *httpver = strstr(inbuf, "HTTP/1.1");
	if (!httpver)
		goto closefd;

	/* Ex, GET /hello HTTP/1.1 */
	if ((httpver - inbuf > URL_LEN + 6) || (httpver - inbuf < 6))
		goto closefd;

	memcpy(httpreq, inbuf + 5, httpver - inbuf - 6);
	http_process_request(httpreq);

closefd:
	close(clifd);
}

static void *httpd_routine(void *arg)
{
	struct sockaddr_in cliaddr;
	socklen_t addrlen = sizeof(cliaddr);

	while (1) {
		clifd = accept(listenfd, (struct sockaddr *)&cliaddr, &addrlen);
		if (clifd < 0)
			continue;

		httpd_handle_request(clifd);
	}

	return NULL;
}

static void httpd_sigchld_handler(int sig)
{
	int wstatus;
	wait(&wstatus);
}

/* only work for rawwrite mode. return 0 on success, otherwise return errno(> 0) */
int httpd(int httpport)
{
	struct sockaddr_in listenaddr;
	struct sigaction sigact = {0};

	sigact.sa_handler = httpd_sigchld_handler;
	sigaction(SIGCHLD, &sigact, NULL);

	signal(SIGPIPE, SIG_IGN);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1)
		return errno;

	int reuse = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	listenaddr.sin_port = htons(httpport);
	if (bind(listenfd, (struct sockaddr *)&listenaddr, sizeof(listenaddr)))
		return errno;

	/* to make atop easy to handle, just process one by one */
	if (listen(listenfd, 1))
		return errno;

	pthread_t thread;
	pthread_create(&thread, NULL, httpd_routine, NULL);

	return 0;
}
