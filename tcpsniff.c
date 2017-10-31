#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "sniff.h"
#include "luabind.h"

static void pkt_handler(void *ud,
						const struct pcap_pkthdr *pkthdr,
						const struct ip *iphdr,
						const struct tcphdr *tcphdr,
						const struct tcpopt *tcpopt,
						const u_char *payload,
						size_t payload_size);

static void l_message(const char *pname, const char *msg)
{
	if (pname)
		lua_writestringerror("%s: ", pname);
}

static int report(lua_State *L, int status)
{
	if (status != LUA_OK)
	{
		const char *msg = lua_tostring(L, -1);
		l_message(NULL, msg);
		lua_pop(L, 1); /* remove message */
	}
	return status;
}

static void
sniffexit(struct vars *input, struct vars *output)
{
	tcpsniff_exit();
}

static void loadconf(lua_State *L, char **interface, char **exp)
{
	struct vars *args = lbind_args(L);
	struct vars *result = lbind_call(L, "tcpsniff_interface", args);
	if (result == NULL)
	{
		fprintf(stderr, "Invalid config.lua, fail to call tcpsniff_interface\n");
		exit(1);
	}
	lbind_clear(args);
	if (lbind_type(result, 0) != LT_STRING)
	{
		fprintf(stderr, "Invalid return value of tcpsniff_interface in config.lua\n");
		exit(1);
	}

	*interface = strdup(lbind_tostring(result, 0));

	result = lbind_call(L, "tcpsniff_expression", args);
	if (result == NULL)
	{
		fprintf(stderr, "Invalid config.lua, fail to call tcpsniff_expression\n");
		exit(1);
	}
	lbind_clear(args);
	if (lbind_type(result, 0) != LT_STRING)
	{
		fprintf(stderr, "Invalid return value of tcpsniff_expression in config.lua\n");
		exit(1);
	}
	*exp = strdup(lbind_tostring(result, 0));
}

static int pmain(lua_State *L)
{
	// int argc = (int)lua_tointeger(L, 1);
	// char **argv = (char **)lua_touserdata(L, 2);

	lbind_register(L, "tcpsniff_stop", sniffexit);
	if (lbind_dofile(L, "tcpsniff.lua") != LUA_OK)
		exit(1);

	char *interface;
	char *exp;
	loadconf(L, &interface, &exp);
	struct tcpsniff_opt sniff_opt = {
		.snaplen = 65535,
		.pkt_cnt_limit = 0,
		.timeout_limit = 10,
		.device = interface,
		.filter_exp = exp,
		.ud = L};

	if (!tcpsniff(&sniff_opt, pkt_handler))
	{
		exit(1);
	}

	lua_pushboolean(L, 1); /* signal no errors */
	return 1;
}

// http://www.lua.org/source/5.3/lua.c.html
int main(int argc, char **argv)
{
	lua_State *L = luaL_newstate(); /* create state */
	if (L == NULL)
	{
		l_message(argv[0], "cannot create state: not enough memory");
		return 1;
	}
	luaL_openlibs(L);

	int status, result;
	lua_pushcfunction(L, &pmain);   /* to call 'pmain' in protected mode */
	lua_pushinteger(L, argc);		/* 1st argument */
	lua_pushlightuserdata(L, argv); /* 2nd argument */
	// pmain -> tcpsniff 会阻塞, 所以 pkt_handler 回调的 ud 即 lua_State *L, 尚未被释放
	status = lua_pcall(L, 2, 1, 0); /* do the call */
	result = lua_toboolean(L, -1);  /* get result */
	report(L, status);
	lua_close(L);
	return (result && status == LUA_OK) ? 0 : 1;
}

static void pkt_handler(void *ud,
						const struct pcap_pkthdr *pkthdr,
						const struct ip *iphdr,
						const struct tcphdr *tcphdr,
						const struct tcpopt *tcpopt,
						const u_char *payload,
						size_t payload_size)
{
	lua_State *L = (lua_State *)ud;

	struct vars *args = lbind_args(L);

	lbind_openmap(args);
	lbind_pushstring(args, "caplen");
	lbind_pushinteger(args, pkthdr->caplen);
	lbind_pushstring(args, "len");
	lbind_pushinteger(args, pkthdr->len);
	lbind_pushstring(args, "ts");
	lbind_pushreal(args, (double)(pkthdr->ts.tv_sec) + (double)(pkthdr->ts.tv_usec) / 1000000.0);
	lbind_close(args);

	lbind_openmap(args);
	lbind_pushstring(args, "hdrlen");
	lbind_pushinteger(args, iphdr->ip_hl);
	lbind_pushstring(args, "ver");
	lbind_pushinteger(args, iphdr->ip_v);
	lbind_pushstring(args, "serv");
	lbind_pushinteger(args, iphdr->ip_tos);
	lbind_pushstring(args, "len");
	lbind_pushinteger(args, ntohs(iphdr->ip_len));
	lbind_pushstring(args, "id");
	lbind_pushinteger(args, ntohs(iphdr->ip_id));
	lbind_pushstring(args, "off");
	lbind_pushinteger(args, ntohs(iphdr->ip_off));
	lbind_pushstring(args, "ttl");
	lbind_pushinteger(args, iphdr->ip_ttl);
	lbind_pushstring(args, "protocol");
	lbind_pushinteger(args, iphdr->ip_p);
	lbind_pushstring(args, "sum");
	lbind_pushinteger(args, ntohs(iphdr->ip_sum));
	lbind_pushstring(args, "dst");
	lbind_pushinteger(args, ntohl(iphdr->ip_dst.s_addr));
	lbind_pushstring(args, "src");
	lbind_pushinteger(args, ntohl(iphdr->ip_src.s_addr));
	lbind_close(args);

	lbind_openmap(args);
	lbind_pushstring(args, "sport");
	lbind_pushinteger(args, ntohs(tcphdr->th_sport));
	lbind_pushstring(args, "dport");
	lbind_pushinteger(args,  ntohs(tcphdr->th_dport));
	lbind_pushstring(args, "seq");
	lbind_pushinteger(args,  (unsigned long)ntohl(tcphdr->th_seq));
	lbind_pushstring(args, "ack");
	lbind_pushinteger(args,  (unsigned long)ntohl(tcphdr->th_ack));
	lbind_pushstring(args, "off");
	lbind_pushinteger(args,  tcphdr->th_off);
	lbind_pushstring(args, "flags");
	lbind_pushinteger(args,  tcphdr->th_flags);
	lbind_pushstring(args, "win");
	lbind_pushinteger(args, ntohs(tcphdr->th_win));
	lbind_pushstring(args, "sum");
	lbind_pushinteger(args,  ntohs(tcphdr->th_sum));
	lbind_pushstring(args, "urp");
	lbind_pushinteger(args,  ntohs(tcphdr->th_urp));
	lbind_close(args);

	// // tcp_opt 已经转过字节序~
	lbind_openmap(args);
	lbind_pushstring(args, "rcv_tsval");
	lbind_pushinteger(args, tcpopt->rcv_tsval);
	lbind_pushstring(args, "rcv_tsecr");
	lbind_pushinteger(args, tcpopt->rcv_tsecr);
	lbind_pushstring(args, "sack_ok");
	lbind_pushinteger(args, tcpopt->sack_ok);
	lbind_pushstring(args, "snd_wscale");
	lbind_pushinteger(args, tcpopt->snd_wscale);
	lbind_pushstring(args, "mss_clamp");
	lbind_pushinteger(args, tcpopt->mss_clamp);
	lbind_close(args);

	lbind_pushlstring(args, (const char *)payload, payload_size);
	lbind_call(L, "tcpsniff_onPacket", args);
	lbind_clear(args);

	// fixme retrieve return value
}
