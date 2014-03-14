---
layout: post
title: lua字节码的格式--源码分析
tags: [lua,字节码,源码]
---

lua字节码的格式
====

#文件头#
    *lua5.1*字节码文件头的长度为12字节，在我的环境里(Win7 64位，VS下编译为Win32应用)如下： 

    {% highlight sh linenos %}
    1b4c 7561 5100 0104 0404 0800
    {% endhighlight %}

    其中第1-4字节为："\033Lua"；第5字节标识lua的版本号，lua5.1为 0x51；第6字节为官方中保留，lua5.1中为 0x0；  
    第7字节标识字节序，little-endian为0x01，big-endian为0x00；  
    第8字节为sizeof(int)；第9字节为sizeof(size_t)；第10字节为sizeof(Instruction)，Instruction为lua内的指令类型，在32位以上的机器上为unsigned int；第11字节为sizeof(lua_Number)，lua_Number即为double;  
    第12字节是判断lua_Number类型起否有效，一般为 0x00; 


    *lua5.2*字节码文件头的长度为18字节，在我的环境里(Win7 64位，VS下编译为Win32应用)如下： 

    {% highlight sh linenos %}
    1b4c 7561 5200 0104 0404 0800 1993 0d0a 1a0a
    {% endhighlight %}

    其中第1-12字节与lua5.1意义相同，第5字节在lua5.2中为 0x52；  
    第13-18字节是为了捕获字节码的转换错误而设置的，其值为 "\x19\x93\r\n\x1a\n"；  


    PS:lua在判断字节序时使用的方法如下：  

    {% highlight cpp linenos %}
    void luaU_header (char* h)
    {
        int x=1;
        //...
        *h++=(char)*(char*)&x;             /* endianness */
        //...
    }
    {% endhighlight %}

    在little-endian时，*(char*)&x值为0x01；big-endian时，*(char*)&x值为 0x00；  