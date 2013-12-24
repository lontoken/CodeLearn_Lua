#lua的栈--源码分析
lua中有两种栈:数据栈和调用栈.

##栈的定义和结构
lua中的数据可以分为两类:值类型和引用类型,值类型可以被任意复制,而引用类型共享一份数据,复制时只是复制其引用,并由GC负责维护其生命期.lua使用一个unine Value来保存数据.  

    union Value {  
        GCObject *gc;    /* collectable objects */  
        void *p;         /* light userdata */  
        int b;           /* booleans */  
        lua_CFunction f; /* light C functions */  
        numfield         /* numbers */  
    };  

引用类型用一个指针GCObject *gc来间接引用,而其它值类型都直接保存在联合中.  

在lua_State中栈的保存如下:  

    struct lua_State {  
        StkId top;          /* first free slot in the stack */
        StkId stack_last;   /* last free slot in the stack */  
        StkId stack;        /* stack base */  
        int stacksize;
        CallInfo base_ci;  /* CallInfo for first level (C calling Lua) */
        ...
    }  

StkId的定义:  

    typedef TValue *StkId;  
    typedef struct lua_TValue TValue;  
    struct lua_TValue {  
        TValuefields;  
    };  

    #define TValuefields    Value value_; int tt_  

在Windows VC下:  

    #define TValuefields  \  
    union { struct { Value v__; int tt__; } i; double d__; } u  

这里使用了繁杂的宏定义,TValuefields和numfield是为了应用一个被称为NaN Trick的技巧.  

lua初始化堆栈:  

    static void stack_init (lua_State *L1, lua_State *L) {
        int i; CallInfo *ci;
        /* initialize stack array */
        L1->stack = luaM_newvector(L, BASIC_STACK_SIZE, TValue);        //BASIC_STACK_SIZE为40
        L1->stacksize = BASIC_STACK_SIZE;
        for (i = 0; i < BASIC_STACK_SIZE; i++)
            setnilvalue(L1->stack + i);  /* erase new stack */          //将tt_字段设置为0
        L1->top = L1->stack;
        L1->stack_last = L1->stack + L1->stacksize - EXTRA_STACK;       //EXTRA_STACK为5
        /* initialize first ci */
        ci = &L1->base_ci;
        ci->next = ci->previous = NULL;
        ci->callstatus = 0;
        ci->func = L1->top;
        setnilvalue(L1->top++);  /* 'function' entry for this 'ci' */
        ci->top = L1->top + LUA_MINSTACK;
        L1->ci = ci;
    }

初始化之后,lua_State栈的情况:  


lua供C使用的栈相关API是不检查数据栈越界的,因为通常编写C扩展都能把数据栈空间的使用控制在BASIC_STACK_SIZE以内,或是显式扩展.对每次数据栈访问都强制做越界检查是非常低效的.  
数据栈不够用时,可以使用luaD_reallocstack\luaD_growstack函数扩展,每次至少分配比原来大一倍的空间.  

    /* some space for error handling */  
    #define ERRORSTACKSIZE  (LUAI_MAXSTACK + 200)  

    void luaD_reallocstack (lua_State *L, int newsize) {        
        TValue *oldstack = L->stack;        
        int lim = L->stacksize;     
        lua_assert(newsize <= LUAI_MAXSTACK || newsize == ERRORSTACKSIZE);  
        lua_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK);  
        luaM_reallocvector(L, L->stack, L->stacksize, newsize, TValue);
        for (; lim < newsize; lim++)   
            setnilvalue(L->stack + lim); /* erase new segment */  
        L->stacksize = newsize;  
        L->stack_last = L->stack + newsize - EXTRA_STACK;  
        correctstack(L, oldstack);  
    }  

    void luaD_growstack (lua_State *L, int n) {  
        int size = L->stacksize;  
        if (size > LUAI_MAXSTACK)  /* error after extra size? */  
            luaD_throw(L, LUA_ERRERR);  
        else {  
            int needed = cast_int(L->top - L->stack) + n + EXTRA_STACK;  
            int newsize = 2 * size;  
            if (newsize > LUAI_MAXSTACK) newsize = LUAI_MAXSTACK;  
            if (newsize < needed) newsize = needed;  
            if (newsize > LUAI_MAXSTACK) {  /* stack overflow? */  
                luaD_reallocstack(L, ERRORSTACKSIZE);  
                luaG_runerror(L, "stack overflow");  
            }  
            else   
                luaD_reallocstack(L, newsize);  
        }  
    }  

数据栈扩展的过程,伴随着数据拷贝,这些数据都是可能直接值复制的,所有不需要在扩展之后修正其中的指针.但有此外部对数据栈的引用需要修正为正确的新地址.这些需要修正的位置包括upvalue以及执行对数据栈的引用.此过程由correctstack函数实现.  

    static void correctstack (lua_State *L, TValue *oldstack) {  
        CallInfo *ci;  
        GCObject *up;  
        L->top = (L->top - oldstack) + L->stack;  
        for (up = L->openupval; up != NULL; up = up->gch.next)  
            gco2uv(up)->v = (gco2uv(up)->v - oldstack) + L->stack;  
        for (ci = L->ci; ci != NULL; ci = ci->previous) {  
            ci->top = (ci->top - oldstack) + L->stack;  
        ci->func = (ci->func - oldstack) + L->stack;  
        if (isLua(ci))  
            ci->u.l.base = (ci->u.l.base - oldstack) + L->stack;  
        }  
    }  

##栈的使用
入栈的函数或宏:  

    lua_pushnil\lua_pushnumber\lua_pushinteger\lua_pushunsigned\lua_pushlstring\lua_pushstring\lua_pushvfstring\lua_pushfstring\lua_pushcclosure\lua_pushboolean\lua_pushlightuserdata\lua_pushthread\lua_newtable\lua_register\lua_pushcfunction;  
    lua_getglobal\lua_gettable\lua_getfield\lua_rawget\lua_rawgeti\lua_rawgetp\lua_createtable\lua_newuserdata\lua_getmetatable\lua_getuservalue  

lua_pushnumber定义如下:  

    LUA_API void lua_pushinteger (lua_State *L, lua_Integer n) {  
        lua_lock(L);  
        //VS下展开之后:TValue *io_=(L->top); ((io_)->u.d__)=(n); ((void)0);  
        setnvalue(L->top, cast_num(n));  
        //VS下展开之后:L->top++; ((void)0);  
        api_incr_top(L);  
        lua_unlock(L);  
    }  

lua_newtable的定义如下:  

    #define lua_newtable(L)     lua_createtable(L, 0, 0)  
    LUA_API void lua_createtable (lua_State *L, int narray, int nrec) {     
        Table *t;   
        lua_lock(L);  
        luaC_checkGC(L);  
        t = luaH_new(L);  
        //VC下展开之后:TValue *io=(L->top); ((io)->u.i.v__).gc=((GCObject *)((t)));  
        //(((io)->u.i.tt__) = (0x7FF7A500 | (((5) | (1 << 6))))); ((void)0);  
        sethvalue(L, L->top, t); 
        //VC下展开之后:L->top++; ((void)0);  
        api_incr_top(L);   
        if (narray > 0 || nrec > 0)  
        luaH_resize(L, t, narray, nrec);  
        lua_unlock(L);  
    }  

示例代码如下:  

    lua_State* L = luaL_newstate();
    lua_pushnumber(L, 1);
    lua_newtable(L);

lua_pushinteger(L, 1)之后,栈的情况:  

lua_newtable(L)之后,栈的情况:  

出栈的函数或宏:  
    lua_setglobal\lua_settable\lua_setfield\lua_rawset\lua_rawseti\lua_rawsetp\lua_setmetatable\lua_setuservalue  
从栈中获取数据的函数或宏:  
    luaL_checklstring\luaL_checknumber\luaL_checkinteger\luaL_checkunsigned  
lua_settable的定义如下:  

    LUA_API void lua_settable (lua_State *L, int idx) {  
        StkId t;  
        lua_lock(L);  
        api_checknelems(L, 2);  
        t = index2addr(L, idx);  
        luaV_settable(L, t, L->top - 2, L->top - 1);  
        L->top -= 2;  /* pop index and value */  
        lua_unlock(L);  
    }  

luaL_checknumber的定义如下:  

    LUALIB_API lua_Number luaL_checknumber (lua_State *L, int narg) {  
        int isnum;  
        lua_Number d = lua_tonumberx(L, narg, &isnum);  
        if (!isnum)  
            tag_error(L, narg, LUA_TNUMBER);  
        return d;  
    }  

示例代码如下(lua库的luaopen_base函数,用于注册):  
    
    #define lua_pushglobaltable(L)  \
        lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS)

    LUALIB_API void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
        luaL_checkversion(L);
        luaL_checkstack(L, nup, "too many upvalues");
        for (; l->name != NULL; l++) {  /* fill the table with given functions */
            int i;
            for (i = 0; i < nup; i++)  /* copy upvalues to the top */
                lua_pushvalue(L, -nup);
            lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
            lua_setfield(L, -(nup + 2), l->name);
        }
        lua_pop(L, nup);  /* remove upvalues */
    }

    LUAMOD_API int luaopen_base (lua_State *L) {  
        /* set global _G */  
        lua_pushglobaltable(L);  
        lua_pushglobaltable(L);  
        lua_setfield(L, -2, "_G");       
        /* open lib into global table */  
        luaL_setfuncs(L, base_funcs, 0);  
        lua_pushliteral(L, LUA_VERSION);  
        lua_setfield(L, -2, "_VERSION");  /* set global _VERSION */  
        return 1;  
    }  
    
luaopen_base函数设置全局注册表的"_G"字段,base_funcs指定的函数列表,"_VERSION"字段.  
第一个lua_pushglobaltable(L)之后:  

第二个lua_pushglobaltable(L)之后:  

lua_setfield(L, -2, "_G")之后:  

luaL_setfuncs(L, base_funcs, 0)之后:  

lua_pushliteral(L, LUA_VERSION)之后:  

lua_setfield(L, -2, "_VERSION")



##参考资源
*   lua源码分析<http://www.codingnow.com/temp/readinglua.pdf>
*   lua源程序<http://www.lua.org/download.html>