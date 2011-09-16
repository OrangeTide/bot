#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include "ini.h"
#include "net.h"
#include "xcalc.h"

#define _mkstr_(x) #x
#define _mkstr(x) _mkstr_(x)

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

#define pr_err(...) do { \
	if (opt.verbose >= 0) \
		fprintf(stderr, "ERROR:" __VA_ARGS__); \
	} while(0)

#define pr_info(...) do { \
	if (opt.verbose > 0) \
		fprintf(stderr, __VA_ARGS__); \
	} while(0)

#define pr_dbg(...) do { \
	if (opt.verbose > 1) \
		fprintf(stderr, "DEBUG:" __VA_ARGS__); \
	} while(0)

static struct {
	char host[64];
	int port;
	char nick[32];
	char realname[128];
	char autojoin[256];
	char quitmessage[256];
	char commandprefix[64];
	int verbose;
	char unknowncommand[128];
} opt;

static char inbuf[1024];
static size_t inbuf_ofs;
static size_t inbuf_max = sizeof(inbuf);
static sig_atomic_t keep_going;
static sig_atomic_t restart;
static time_t start_time;

static int bot_option(const char *name, const char *value)
{
	const struct {
		char *name;
		char *type;
		void *ptr;
	} options[] = {
		{ "host", "%63s", &opt.host },
		{ "port", "%d", &opt.port },
		{ "realname", "%127[^\n]", &opt.realname },
		{ "nick", "%31s", &opt.nick },
		{ "autojoin", "%255s", &opt.autojoin },
		{ "quitmessage", "%255[^\n]", &opt.quitmessage },
		{ "commandprefix", "%63s", &opt.commandprefix },
		{ "verbose", "%d", &opt.verbose },
		{ "unknowncommand", "%127[^\n]", &opt.unknowncommand },
	};
	unsigned i;
	for (i = 0; i < ARRAY_SIZE(options); i++) {
		if (!strcasecmp(name, options[i].name)) {
			if (sscanf(value, options[i].type, options[i].ptr) != 1)
				return -1;
			return 0;
		}
	}
	return -1;
}

static int bot_ini(const char *section, const char *name, const char *value)
{
	if (!section || !*section) {
		return 0;
	} else if (!strcmp(section, "bot")) {
		return bot_option(name, value);
	} else {
		pr_err("unknown section '%s'\n", section);
	}
	return 0;
}

/* block until entire buffer is sent */
static int irc_write(int fd, const char *buf, size_t len)
{
	int rem;
	pr_dbg("OUT>>>%.*s<<<\n", (int)len, buf);
	while (len > 0) {
		rem = write(fd, buf, len);
		if (rem < 0) {
			perror("write()");
			return -1;
		}
		len -= rem;
		buf += rem;
	}
	return 0;
}

static int irc_printf(int fd, const char *fmt, ...)
{
	static char buf[1024];
	int cnt;
	va_list ap;

	va_start(ap, fmt);
	cnt = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (cnt >= (int)sizeof(buf)) {
		pr_err("overflow!\n");
		return -1;
	}

	if (irc_write(fd, buf, cnt))
		return -1;

	return 0;
}

static int irc_user(int fd, const char *host, const char *nick)
{
	return irc_printf(fd, "USER %s %s localhost :%s\r\n",
		nick, host, opt.realname);
}

static int irc_nick_set(int fd, const char *nick)
{
	return irc_printf(fd, "NICK %s\r\n", nick);
}

static int irc_join(int fd, const char *channel, const char *key)
{
	if (key && *key) {
		return irc_printf(fd, "JOIN %s :%s\r\n", channel, key);
	} else {
		return irc_printf(fd, "JOIN %s\r\n", channel);
	}
}

static int irc_quit(int fd, const char *quitmsg)
{
	return irc_printf(fd, "QUIT :%s\r\n", quitmsg);
}

static int irc_generic_msg(int fd, const char *type,
	const char *to, const char *fmt, va_list ap)
{
	char buf[512];
	int bufmax = sizeof buf;
	int cnt;

	cnt = snprintf(buf, sizeof(buf), "%s %s :", type, to);
	if (cnt >= bufmax) {
		pr_err("overflow!\n");
		return -1;
	}

	cnt += vsnprintf(buf + cnt, bufmax - cnt, fmt, ap);
	if (cnt >= bufmax) {
		pr_err("overflow!\n");
		return -1;
	}

	cnt += snprintf(buf + cnt, bufmax - cnt, "\r\n");
	if (cnt >= bufmax) {
		pr_err("overflow!\n");
		return -1;
	}

	return irc_write(fd, buf, cnt);
}

static int irc_privmsg(int fd, const char *to, const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = irc_generic_msg(fd, "PRIVMSG", to, fmt, ap);
	va_end(ap);
	return ret;
}

static int irc_notice(int fd, const char *to, const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = irc_generic_msg(fd, "NOTICE", to, fmt, ap);
	va_end(ap);
	return ret;
}

static int irc_connect(int *fd)
{
	int e;
	char addr[128];

	snprintf(addr, sizeof(addr), "%s/%u", opt.host, opt.port);
	pr_info("CONNECTING %s ...\n", addr);
	e = socket_connect_by_name(fd, SOCK_STREAM, addr);
	if (e) {
		return -1;
	}
	return 0;
}

static const char *get_nick(const char *prefix)
{
	static char nick[32];
	if (prefix) {
		char *e = strchr(prefix, '!');
		if (e)
			*e = 0;
		snprintf(nick, sizeof(nick), "%.*s", (int)(e - prefix), prefix);
	} else {
		*nick = 0;
	}
	return nick;
}

/* return - non-zero on match */
static int match_commandprefix(const char *privmsg, const char **endptr)
{
	char *e;
	// TODO: use regexec for prefix
	for (e = opt.commandprefix; *e; e++, privmsg++) {
		if (*privmsg != *e) {
			*endptr = privmsg;
			return 0;
		}
	}
	*endptr = privmsg;
	return 1;
}

static int on_user_quit(_unused int fd,
	const char *from, _unused const char *to,
	_unused char *msg)
{
	const char *nick = get_nick(from); // WARNING: called twice

	pr_info("quit requested by %s\n", nick);
	keep_going = 0;
	return 0;
}

static int on_user_restart(_unused int fd, const char *from,
	_unused const char *to, _unused char *msg)
{
	const char *nick = get_nick(from); // WARNING: called twice

	pr_info("restart requested by %s\n", nick);
	keep_going = 0;
	restart = 1;
	return 0;
}

static int on_user_echo(int fd, const char *from, const char *to,
	char *msg)
{
	const char *nick = get_nick(from); // WARNING: called twice
	if (!msg)
		msg = "<nothing>";
	return irc_privmsg(fd, to, "%s said '%s'", nick, msg);
}

static int on_user_calc(int fd, const char *from, const char *to,
	char *msg)
{
	const char *nick = get_nick(from); // WARNING: called twice
	int ret;
	struct num *num;

	if (!msg)
		return irc_notice(fd, to, "calc error");

	num = xcalc(msg);
	if (!num)
		return irc_notice(fd, to, "%s's result is an error!", nick);

	ret = irc_notice(fd, to, "%s's result is %s", nick, num_string(num));
	num_free(num);
	return ret;
}

static int on_user_hexcalc(int fd, const char *from, const char *to,
	char *msg)
{
	const char *nick = get_nick(from); // WARNING: called twice
	int ret;
	struct num *num;

	if (!msg)
		return irc_notice(fd, to, "calc error");

	num = xcalc(msg);
	if (!num)
		return irc_notice(fd, to, "%s's result is an error!", nick);

	ret = irc_notice(fd, to, "%s's result is %s", nick, num_hexstring(num));
	num_free(num);
	return ret;
}

static int on_user_help(int fd, _unused const char *from, const char *to,
	_unused char *msg)
{
	return irc_notice(fd, to,
		"HELP: quit, restart, echo, calc, hexcalc, time, uptime, help");
}

static int on_user_time(int fd, _unused const char *from, const char *to,
	_unused char *msg)
{
	char buf[64];
	struct tm tm;
	time_t t;
	time(&t);
	gmtime_r(&t, &tm);
	strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S +0000", &tm);
	return irc_notice(fd, to, "the time is %s", buf);
}

static int on_user_uptime(int fd, _unused const char *from, const char *to,
	_unused char *msg)
{
	time_t now;
	double uptime;
	unsigned days, hours, min, sec;
	div_t r;
	time(&now);
	uptime = difftime(now, start_time);
	r = div(uptime, 86400);
	days = r.quot;
	hours = r.rem / 3600;
	r.rem %= 3600;
	min = r.rem / 60;
	sec = r.rem % 60;
	if (days)
		return irc_notice(fd, to, "I've been awake for %u days, %u hours, %u minutes and %u seconds.",
			days, hours, min, sec);
	else if (hours)
		return irc_notice(fd, to, "I've been awake for %u hours, %u minutes and %u seconds.",
			hours, min, sec);
	else if (min)
		return irc_notice(fd, to, "I've been awake for %u minutes and %u seconds.",
			min, sec);
	else
		return irc_notice(fd, to, "I've been awake for %u seconds.", sec);
}

static void unknowncommand(int fd, const char *to, const char *cmd)
{
	size_t cmd_len = strlen(cmd);
	char buf[sizeof(opt.unknowncommand) + cmd_len];
	const char *s = opt.unknowncommand;
	unsigned i;

	i = 0;
	s = opt.unknowncommand;
	while (*s && i < sizeof(buf) - 1) {
		if (*s != '%') {
			buf[i++] = *s++;
		} else {
			memcpy(buf + i, cmd, cmd_len);
			i += cmd_len;
			s++;
		}
	}
	buf[i] = 0;

	irc_notice(fd, to, "%s", buf);
}

static int dispatch_user_command(int fd, const char *from, const char *to,
	const char *cmd, char *msg)
{
	const struct {
		char *name;
		int (*func)(int fd, const char *from, const char *to,
			char *msg);
	} user_cmds[] = {
		{ "quit", on_user_quit },
		{ "restart", on_user_restart },
		{ "echo", on_user_echo },
		{ "calc", on_user_calc },
		{ "hexcalc", on_user_hexcalc },
		{ "help", on_user_help },
		{ "time", on_user_time },
		{ "uptime", on_user_uptime },
	};
	unsigned i;

	if (!cmd || !*cmd)
		return 0; /* ignore */

	if (!to || (*to != '#' && *to != '&'))
		to = get_nick(from);

	for (i = 0; i < ARRAY_SIZE(user_cmds); i++) {
		if (!strcasecmp(cmd, user_cmds[i].name))
			return user_cmds[i].func(fd, from, to, msg);
	}

	unknowncommand(fd, to, cmd);

	return 0;
}

/* get the first word
 * msg_in - modified to break up string
 * msg_out - points to second word
 * return - pointer to first word (msg_in)
 */
static const char *first_word(char *msg_in, char **msg_out)
{
	while (isspace(*msg_in))
		msg_in++;
	*msg_out = strchr(msg_in, ' ');
	if (!*msg_out)
		return msg_in;
	**msg_out = 0;
	do {
		++*msg_out;
	} while (isspace(**msg_out));
	return msg_in;
}

static int on_privmsg(int fd, const char *prefix, int argc, const char **argv)
{
	const char *endptr;

	if (argc < 2)
		return 0; /* ignored */

	if (match_commandprefix(argv[2], &endptr)) {
		assert(endptr != NULL);
		const char *cmd;
		char *buf = strdup(endptr);
		char *msg;
		int ret;

		cmd = first_word(buf, &msg);
		ret = dispatch_user_command(fd, prefix, argv[1], cmd, msg);

		free(buf);
		return ret;
	}

	return 0;
}

static int on_ping(int fd, _unused const char *prefix,
	int argc, const char **argv)
{
	if (argc < 2) {
		pr_err("malformed PING message\n");
		return 0; /* ignore */
	}
	return irc_printf(fd, "PONG %s\r\n", argv[1]);
}

static int dispatch_message(int fd, const char *prefix,
	int argc, const char **argv)
{
	const struct {
		char *name;
		int (*func)(int fd, const char *prefix,
			int argc, const char **argv);
	} cmds[] = {
		{ "PING", on_ping },
		{ "PRIVMSG", on_privmsg },
	};
	unsigned i;

	pr_dbg("%s():argc=%d\n", __func__, argc);
	for (i = 0; (int)i < argc; i++) {
		pr_dbg("argv[%d]=%s*\n", i, argv[i]);
	}
	if (argc < 1)
		return 0; /* ignore */

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		if (!strcasecmp(argv[0], cmds[i].name))
			return cmds[i].func(fd, prefix, argc, argv);
	}

	return 0; /* ignored */
}

static int decode_message(char *msg, const char **prefix,
	int *argc, const char **argv, int argmax)
{
	int i;
	char *e;
	unsigned ofs;

	assert(prefix != NULL);
	assert(argc != NULL);
	assert(argv != NULL);

	pr_info("%s\n", msg);

	if (*msg == ':') {
		*prefix = msg + 1;
		msg = strchr(msg, ' ');
		if (!msg) {
			argc = 0;
			pr_err("malformed message from \"%s\"\n", *prefix);
			return -1;
		}
	} else {
		*prefix = NULL;
	}

	while (isspace(*msg))
		msg++;

	for (i = 0, ofs = 0; i < argmax; ) {
		if (msg[ofs] == ':') {
			argv[i++] = msg + ofs + 1;
			break;
		} else {
			argv[i++] = msg + ofs;
		}
		e = strchr(msg + ofs, ' ');
		if (!e)
			break;
		ofs = e - msg;
		msg[ofs++] = 0;
	}
	*argc = i;

	return 0;
}

/* walk buffer, decode messages and dispatch them */
static int process_messages(int fd)
{
	unsigned i;
	int ret = 0;
	const char *prefix;
	const char *argv[10];
	int argc;

	for (i = 0; i < inbuf_ofs; ) {
		char *p = memchr(inbuf + i, '\n', inbuf_ofs - i);
		char *m = inbuf + i;
		if (!p)
			break;
		*p = 0;
		if (p > m && *(p - 1) == '\r')
			*(p - 1) = 0;
		i = p - inbuf + 1;
		if (decode_message(m, &prefix, &argc, argv, ARRAY_SIZE(argv)) ||
			dispatch_message(fd, prefix, argc, argv)) {
			ret = -1;
			break;
		}
	}
	assert(inbuf_ofs >= i);
	inbuf_ofs -= i;
	if (inbuf_ofs)
		memmove(inbuf, inbuf + i, inbuf_ofs);
	return ret;
}

static int fill_buffer(int fd)
{
	ssize_t cnt;
	cnt = read(fd, inbuf + inbuf_ofs, inbuf_max - inbuf_ofs);
	if (cnt <= 0)
		return -1;
	pr_dbg("IN>>>%.*s<<<\n", (int)cnt, inbuf + inbuf_ofs);
	inbuf_ofs += cnt;
	return 0;
}

static int bot_loop(int fd)
{
	while (keep_going) {
		if (fill_buffer(fd)) {
			return -1;
		}
		if (process_messages(fd)) {
			return -1;
		}
	}
	return 0;
}

static int bot_start(void)
{
	int fd;
	int ret;

	ret = irc_connect(&fd);
	if (ret) {
		pr_err("unable to connect!\n");
		return ret;
	}

	time(&start_time);

	irc_user(fd, opt.host, opt.nick);

	irc_nick_set(fd, opt.nick);

	irc_join(fd, opt.autojoin, 0); /* TODO: walk through CSV list */

	ret = bot_loop(fd);

	pr_info("Terminating connection ...\n");

	irc_quit(fd, !ret ? opt.quitmessage : "Fatal Error!");

	sleep(5);

	close(fd);
	return ret;
}

static void stop_going(_unused int s)
{
	// dprintf(2, "\nCaught signal %d\n", s);
	keep_going = 0;
	return;
}

int main(_unused int argc, char **argv)
{
	int e;
	int ret;

	opt.port = 6667;
	e = ini_load("bot.ini", bot_ini);
	if (e) {
		pr_err("%s:could not load\n", "bot.ini");
		return 1;
	}
	if (!*opt.unknowncommand)
		strcpy(opt.unknowncommand, "unknown command '%'");
	keep_going = 1;
	/*
	signal(SIGTERM, stop_going);
	signal(SIGINT, stop_going);
	*/
	ret = bot_start();

	if (!ret && restart) {
		execv(argv[0], argv);
		perror(argv[0]);
		return 1;
	}

	if (ret)
		return 1;

	return 0;
}
