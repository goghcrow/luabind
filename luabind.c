#include "luabind.h"

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct llstr
{
	const char *s;
	size_t sz;
};

struct lvar
{
	int type;
	union {
		lua_Integer i;
		lua_Number f;
		const char *s;
		struct llstr *ls;
		int b;
		void *p;
		// struct larray *a;
		// struct lmap *m;
		struct vars *m;
	} v;
};

struct vars
{
	int n;
	int cap;
	struct lvar *v;

	struct vars *prev;
};

struct vars *
lbind_new()
{
	struct vars *v = malloc(sizeof(*v));
	if (v == NULL)
		return NULL;
	v->n = 0;
	v->cap = 0;
	v->v = NULL;
	v->prev = NULL;
	return v;
}

void lbind_delete(struct vars *v)
{
	if (v == NULL)
		return;
	if (v->v)
	{
		free(v->v);
	}
	// fixme free prev
	free(v);
}

// 清除之后可复用
void lbind_clear(struct vars *v)
{
	v->n = 0;
}

// 获取 idx var 类型
int lbind_type(struct vars *v, int index)
{
	if (index < 0 || index >= v->n)
	{
		return LT_NONE;
	}
	return v->v[index].type;
}

int lbind_tointeger(struct vars *v, int index)
{
	if (index < 0 || index >= v->n)
	{
		return 0;
	}
	return v->v[index].v.i;
}

double
lbind_toreal(struct vars *v, int index)
{
	if (index < 0 || index >= v->n)
	{
		return 0;
	}
	return v->v[index].v.f;
}

const char *
lbind_tostring(struct vars *v, int index)
{
	if (index < 0 || index >= v->n)
	{
		return 0;
	}
	return v->v[index].v.s;
}

struct llstr *
lbind_tolstring(struct vars *v, int index)
{
	if (index < 0 || index >= v->n)
	{
		return 0;
	}
	return v->v[index].v.ls;
}

int lbind_toboolean(struct vars *v, int index)
{
	if (index < 0 || index >= v->n)
	{
		return 0;
	}
	return v->v[index].v.b;
}

void *
lbind_topointer(struct vars *v, int index)
{
	if (index < 0 || index >= v->n)
	{
		return 0;
	}
	return v->v[index].v.p;
}

static struct lvar *
newvalue(struct vars *v)
{
	if (v->n >= v->cap)
	{
		int cap = v->cap * 2;
		if (cap == 0)
		{
			cap = 16;
		}
		struct lvar *nv = malloc(cap * sizeof(*nv));
		if (nv == NULL)
			return NULL;
		memcpy(nv, v->v, v->n * sizeof(*nv));
		free(v->v);
		v->v = nv;
		v->cap = cap;
	}
	struct lvar *ret = &v->v[v->n];
	++v->n;
	return ret;
}

int lbind_openmap(struct vars *v)
{
	// fixme !!! free
	struct vars *cpy = lbind_new();
	if (cpy == NULL)
		return -1;
	memcpy(cpy, v, sizeof(*v));
	v->prev = cpy;
	v->n = 0;
	v->cap = 0;
	v->v = NULL;
	return 0;
}

int lbind_close(struct vars *v)
{
	struct vars *prev = v->prev;
	if (prev == NULL)
	{
		return -1;
	}

	// fixme !!! free
	struct vars *cpy = lbind_new();
	if (cpy == NULL)
		return -1;
	memcpy(cpy, v, sizeof(*v));
	struct lvar *m = newvalue(prev);
	if (m == NULL)
		return -1;
	m->type = LT_MAP;
	m->v.m = cpy;
	m->v.m->prev = NULL;
	memcpy(v, prev, sizeof(*v));
	// free(prev); // free 关系: lbind_openmap -> lbind_new
	return 0;
}

// fixme
int lbind_pushmap(struct vars *v, struct vars **m)
{
	struct lvar *s = newvalue(v);
	if (s == NULL)
		return -1;
	s->type = LT_MAP;
	s->v.m = lbind_new();
	if (s->v.m == NULL)
		return -1;
	*m = s->v.m;
	return 0;
}

int lbind_pushinteger(struct vars *v, int i)
{
	struct lvar *s = newvalue(v);
	if (s == NULL)
		return -1;
	s->type = LT_INTEGER;
	s->v.i = i;
	return 0;
}

int lbind_pushreal(struct vars *v, double f)
{
	struct lvar *s = newvalue(v);
	if (s == NULL)
		return -1;
	s->type = LT_REAL;
	s->v.f = f;
	return 0;
}

int lbind_pushstring(struct vars *v, const char *str)
{
	struct lvar *s = newvalue(v);
	if (s == NULL)
		return -1;
	s->type = LT_STRING;
	s->v.s = str;
	return 0;
}

int lbind_pushlstring(struct vars *v, const char *s, size_t sz)
{
	struct lvar *ls = newvalue(v);
	if (ls == NULL)
		return -1;
	ls->type = LT_LSTRING;
	// 注意 free, 这里在 lua_pushstring 完之后会 free
	// 但如果只 调用 lbind_pushlstring, 之后 lbind_clear 或者 lbind_delete 会产生内存泄漏
	// so, lbind_pushlstring 之后一定要使用~
	ls->v.ls = malloc(sizeof(*(ls->v.ls)));
	if (ls->v.ls == NULL)
		return -1;
	ls->v.ls->s = s;
	ls->v.ls->sz = sz;
	return 0;
}

int lbind_pushboolean(struct vars *v, int b)
{
	struct lvar *s = newvalue(v);
	if (s == NULL)
		return -1;
	s->type = LT_BOOLEAN;
	s->v.b = b;
	return 0;
}

int lbind_pushnil(struct vars *v)
{
	struct lvar *s = newvalue(v);
	if (s == NULL)
		return -1;
	s->type = LT_NIL;
	return 0;
}

int lbind_pushpointer(struct vars *v, void *p)
{
	struct lvar *s = newvalue(v);
	if (s == NULL)
		return -1;
	s->type = LT_POINTER;
	s->v.p = p;
	return 0;
}

static int
ldelvars(lua_State *L)
{
	// 如果给定索引处的值是一个完全用户数据， 函数返回其内存块的地址。 如果值是一个轻量用户数据， 那么就返回它表示的指针。 否则，返回 NULL 。
	// void *lua_touserdata (lua_State *L, int index);
	struct vars **box = (struct vars **)lua_touserdata(L, 1);
	if (*box)
	{
		lbind_delete(*box);
		*box = NULL;
	}
	return 0;
}

// 创建 struct vars, 转为 userdata, 并为 userdata 设置元表, 主要设置 __gc 用来调用 ldelvars
// 或者理解为 创建一个 lua 值且可以被 gc 回收的struct vars
// 意图: 安全内存管理
// https://blog.codingnow.com/2015/05/lua_c_api.html
// 在用 C 编写 Lua 的 C 扩展库时，由于 C API 有抛出异常的可能，你还需要考虑，每次当你调用 Lua API 时，下一行程序都有可能运行不到。
// 所以一旦你想临时申请一些堆内存使用，要充分考虑你在同一函数内编写的释放临时对象的代码很可能运行不到。
// 正确的方法是使用 lua_newuserdata 来申请临时内存，如果被异常打断，后续的 gc 流程会清理它们。
static struct vars *
newvarsobject(lua_State *L)
{
	// void *lua_newuserdata (lua_State *L, size_t size);
	// 这个函数分配一块指定大小的内存块， 把内存块地址作为一个完全用户数据压栈， 并返回这个地址。 宿主程序可以随意使用这块内存。
	struct vars **box = (struct vars **)lua_newuserdata(L, sizeof(*box)); // -1
	*box = NULL;
	struct vars *v = lbind_new();
	if (v == NULL)
	{
		return NULL;
	}
	*box = NULL;

	// int luaL_newmetatable (lua_State *L, const char *tname);
	// 如果注册表中已存在键 tname，返回 0 。 否则， 为用户数据的元表创建一张新表。
	// 向这张表加入 __name = tname 键值对， 并将 [tname] = new table 添加到注册表中， 返回 1 。 （__name项可用于一些错误输出函数。）
	// 这两种情况都会把最终的注册表中关联 tname 的值压栈。
	if (luaL_newmetatable(L, "luabindvars"))
	{
		// 设置元表终结器

		// void lua_pushcfunction (lua_State *L, lua_CFunction f);
		// 将一个 C 函数压栈。 这个函数接收一个 C 函数指针， 并将一个类型为 function 的 Lua 值压栈。 当这个栈顶的值被调用时，将触发对应的 C 函数。
		// 注册到 Lua 中的任何函数都必须遵循正确的协议来接收参数和返回值 （参见 lua_CFunction ）。
		// lua_pushcfunction 是作为一个宏定义出现的：
		// #define lua_pushcfunction(L,f)  lua_pushcclosure(L,f,0)
		lua_pushcfunction(L, ldelvars); // -2
		// void lua_setfield (lua_State *L, int index, const char *k);
		// 做一个等价于 t[k] = v 的操作， 这里 t 是给出的索引处的值， 而 v 是栈顶的那个值。
		// 这个函数将把这个值弹出栈。 跟在 Lua 中一样，这个函数可能触发一个 "newindex" 事件的元方法 （参见 §2.4）。
		lua_setfield(L, -2, "__gc");
	}
	// 注册表中关联 tname 的元表 -2
	// 为 struct vars **box 设置元表, 主要设置 __gc 用来回收 struct vars

	// void lua_setmetatable (lua_State *L, int index);
	// 把一张表弹出栈，并将其设为给定索引处的值的元表。
	lua_setmetatable(L, -2);

	*box = v;
	return v;
}

static void
pushvar(lua_State *L, struct lvar *v)
{
	switch (v->type)
	{
	case LT_NIL:
		lua_pushnil(L);
		break;
	case LT_INTEGER:
		lua_pushinteger(L, v->v.i);
		break;
	case LT_REAL:
		lua_pushnumber(L, v->v.f);
		break;
	case LT_STRING:
		lua_pushstring(L, v->v.s);
		break;
	case LT_LSTRING:
		// const char *lua_pushlstring (lua_State *L, const char *s, size_t len);
		// 把指针 s 指向的长度为 len 的字符串压栈。
		// Lua 对这个字符串做一个内部副本（或是复用一个副本）， 因此 s 处的内存在函数返回后，可以释放掉或是立刻重用于其它用途。
		// 字符串内可以是任意二进制数据，包括零字符。
		// 返回内部副本的指针。
		lua_pushlstring(L, v->v.ls->s, v->v.ls->sz);
		free(v->v.ls);
		break;
	case LT_BOOLEAN:
		lua_pushboolean(L, v->v.b);
		break;
	case LT_POINTER:
		lua_pushlightuserdata(L, v->v.p);
		break;
	case LT_ARRAY: //todo
	case LT_MAP:
	{
		struct vars *m = v->v.m;
		int j, n = m->n;
		if ((n % 2) != 0)
		{
			luaL_error(L, "no matched kv pairs in map");
		}
		lua_newtable(L);
		for (j = 0; j < n; j += 2)
		{
			pushvar(L, &m->v[j]);
			pushvar(L, &m->v[j + 1]);
			lua_settable(L, -3);
		}
		lbind_clear(m);
		// lbind_delete(m); // free 关系: lbind_close -> lbind_new
		break;
	}
	default:
		luaL_error(L, "unsupport type %d", v->type);
	}
}

// v -> lua stack
static int
pushargs(lua_State *L, struct vars *vars)
{
	if (vars == NULL)
		return 0;
	int n = vars->n;
	luaL_checkstack(L, n, NULL);
	int i;
	for (i = 0; i < n; i++)
	{
		struct lvar *v = &vars->v[i];
		pushvar(L, v);
	}
	return n;
}

// 这里 struct vars *v 做返回值使用
// lua stack -> v
static void
genargs(lua_State *L, struct vars *v)
{
	int n = lua_gettop(L);
	int i;
	for (i = 1; i <= n; i++)
	{
		int err = 0;
		int t = lua_type(L, i);
		switch (t)
		{
		case LUA_TNIL:
			err = lbind_pushnil(v);
			break;
		case LUA_TBOOLEAN:
			err = lbind_pushboolean(v, lua_toboolean(L, i));
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, i))
			{
				err = lbind_pushinteger(v, lua_tointeger(L, i));
			}
			else
			{
				err = lbind_pushreal(v, lua_tonumber(L, i));
			}
			break;
		case LUA_TSTRING:
			err = lbind_pushstring(v, lua_tostring(L, i));
			break;
		case LUA_TLIGHTUSERDATA:
			err = lbind_pushpointer(v, lua_touserdata(L, i));
			break;
		case LUA_TTABLE: // todo
		default:
			luaL_error(L, "unsupport type %s", lua_typename(L, t));
		}
		if (err)
		{
			luaL_error(L, "push arg %d error", i);
		}
	}
}

// 通过 p 取出对应的 struct vars, 如果不存在则创建一个新的 struct vars, 清空后返回
static struct vars *
getvars(lua_State *L, void *p)
{
	struct vars *args = NULL;
	// Lua 提供了一个 注册表， 这是一个预定义出来的表， 可以用来保存任何 C 代码想保存的 Lua 值。 这个表可以用有效伪索引 LUA_REGISTRYINDEX 来定位。
	// int lua_rawgetp (lua_State *L, int index, const void *p);
	// 把 t[k] 的值压栈， 这里的 t 是指给定索引处的表， k 是指针 p 对应的轻量用户数据。 这是一次直接访问；就是说，它不会触发元方法。
	// 返回入栈值的类型。
	lua_rawgetp(L, LUA_REGISTRYINDEX, p);
	if (lua_isuserdata(L, -1))
	{
		struct vars **box = (struct vars **)lua_touserdata(L, -1);
		args = *box;
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
		args = newvarsobject(L);
		if (args == NULL)
			luaL_error(L, "new result object failed");
		// void lua_rawsetp (lua_State *L, int index, const void *p);
		// 等价于 t[k] = v ， 这里的 t 是指给定索引处的表， k 是指针 p 对应的轻量用户数据。 而 v 是栈顶的值。
		// 这个函数会将值弹出栈。 赋值是直接的；即不会触发元方法。
		lua_rawsetp(L, LUA_REGISTRYINDEX, p);
	}
	lbind_clear(args);
	return args;
}

// 作为返回值的 struct vars
static struct vars *
resultvars(lua_State *L)
{
	// 这里用静态变量地址(整数)作存放 lua 用到 C值 的索引
	// 静态变量地址不变, 适合做索引
	static int result = 0;
	return getvars(L, &result);
}

// 作为参数的 struct vars
static struct vars *
argvars(lua_State *L, int onlyfetch)
{
	// 这里用静态变量地址(整数)作存放 lua 用到 C值 的索引
	static int args = 0;
	// 只获取
	if (onlyfetch)
	{
		lua_rawgetp(L, LUA_REGISTRYINDEX, &args);
		if (lua_isuserdata(L, -1))
		{
			struct vars **box = (struct vars **)lua_touserdata(L, -1);
			lua_pop(L, 1);
			return *box;
		}
		else
		{
			// 如果不是 userdata 则 pop 出去
			lua_pop(L, 1);
			return NULL;
		}
	}
	// 获取不到创建新的
	else
	{
		return getvars(L, &args);
	}
}

static int
lnewargs(lua_State *L)
{
	struct vars *args = argvars(L, 0);
	lua_pushlightuserdata(L, args);
	return 1;
}

static void
errorlog(lua_State *L)
{
	fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
	lua_pop(L, 1);
}

// 如果已经存在返回, 如果没有通过 pcall 创建一个 struct vars
struct vars *
lbind_args(lua_State *L)
{
	struct vars *args = argvars(L, 1); // try fetch args, never raise error
	if (args)
		return args;
	lua_pushcfunction(L, lnewargs);
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
	{
		errorlog(L);
		return NULL;
	}
	args = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return args;
}

static int
lcall(lua_State *L)
{
	const char *funcname = lua_touserdata(L, 1);
	struct vars *args = lua_touserdata(L, 2);
	lua_settop(L, 0);
	if (lua_getglobal(L, funcname) != LUA_TFUNCTION)
	{
		return luaL_error(L, "%s is not a function", funcname);
	}
	lua_call(L, pushargs(L, args), LUA_MULTRET);
	luaL_checkstack(L, LUA_MINSTACK, NULL);
	// 以下两行, 将多返回值转换为 struct args
	args = resultvars(L);
	genargs(L, args);
	// 将返回值ptr入栈
	lua_pushlightuserdata(L, args);
	return 1;
}

struct vars *
lbind_call(lua_State *L, const char *funcname, struct vars *args)
{
	lua_pushcfunction(L, lcall);
	lua_pushlightuserdata(L, (void *)funcname);
	lua_pushlightuserdata(L, args);
	int ret = lua_pcall(L, 2, 1, 0);
	if (ret == LUA_OK)
	{
		struct vars *result = lua_touserdata(L, -1);
		lua_pop(L, 1);
		return result;
	}
	else
	{
		// ignore the error message, If you  need log error message, use lua_tostring(L, -1)
		errorlog(L);
		return NULL;
	}
}

static int
lfunc(lua_State *L)
{
	lbind_function f = (lbind_function)lua_touserdata(L, lua_upvalueindex(1));
	struct vars *args = argvars(L, 0);
	struct vars *result = resultvars(L);
	genargs(L, args);
	f(args, result);
	lbind_clear(args);
	lua_settop(L, 0);
	int n = pushargs(L, result);
	lbind_clear(result);

	return n;
}

static int
lregister(lua_State *L)
{
	const char *funcname = lua_touserdata(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	if (lua_getglobal(L, "C") != LUA_TTABLE)
	{
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setglobal(L, "C");
	}
	lua_pushvalue(L, 2);
	lua_pushcclosure(L, lfunc, 1);
	lua_setfield(L, -2, funcname);

	return 0;
}

void lbind_register(lua_State *L, const char *funcname, lbind_function f)
{
	lua_pushcfunction(L, lregister);
	lua_pushlightuserdata(L, (void *)funcname);
	lua_pushlightuserdata(L, f);
	int ret = lua_pcall(L, 2, 0, 0);
	if (ret != LUA_OK)
	{
		// ignore the error message, If you  need log error message, use lua_tostring(L, -1)
		errorlog(L);
	}
}

static int
ldofile(lua_State *L)
{
	const char *filename = (const char *)lua_touserdata(L, 1);
	int ret = luaL_loadfile(L, filename);
	if (ret != LUA_OK)
	{
		// ignore the error message
		errorlog(L);
		if (ret == LUA_ERRFILE)
		{
			lua_pushinteger(L, LF_NOTFOUND);
		}
		else
		{
			lua_pushinteger(L, LF_ERRPARSE);
		}
		return 1;
	}
	ret = lua_pcall(L, 0, 0, 0);
	if (ret != LUA_OK)
	{
		// ignore the error message
		errorlog(L);
		lua_pushinteger(L, LF_ERRRUN);
		return 1;
	}
	lua_pushinteger(L, 0);
	return 1;
}

int lbind_dofile(lua_State *L, const char *filename)
{
	lua_pushcfunction(L, ldofile);
	lua_pushlightuserdata(L, (void *)filename);
	int ret = lua_pcall(L, 1, 1, 0);
	if (ret != LUA_OK)
	{
		// ignore the error message, If you need log error message, use lua_tostring(L, -1)
		errorlog(L);
		return -1;
	}
	else
	{
		int ret = lua_tointeger(L, -1);
		lua_pop(L, 1);
		return ret;
	}
}
