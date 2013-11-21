// LuaTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

int _tmain(int argc, _TCHAR* argv[])
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

    const char *buf = "local len = 10000; local tab = {};"
        "for i = 1, len do"
        "   tab[i] = 'str' .. i;"
        "   print(tab[i]);"
        "end";
	luaL_dostring(L,buf);
	
    double const *v = lua_version(L);
    printf("%f\n", *v);

	lua_close(L);

    int i;

    std::cin >> i;

	return 0;
}