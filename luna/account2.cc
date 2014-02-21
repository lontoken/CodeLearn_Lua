#include "luna.h"
extern "C" {
#include <lualib.h>
}

class Account
{
 public:   
   Account(double balance) 
     : m_balance(balance) {} 
   void deposit(double amount) {
      m_balance += amount;  
   }
   void withdraw(double amount) {
      m_balance -= amount;
   }
   double balance(void) {
      return m_balance;  
   }
 private:
   double m_balance;
};


class Account_Proxy
{
 public:
   static const char className[];
   static const Luna<Account_Proxy>::RegType Register[];
   
   Account_Proxy(lua_State* L) {
      /* constructor table at top of stack */
      lua_pushstring(L, "balance");
      lua_gettable(L, -2);
      double amount = lua_tonumber(L, -1);
      m_Account = new Account(amount);
      lua_pop(L, 2); /* pop constructor table and balance */
   }

   ~Account_Proxy() {
      delete m_Account;
   }
   int deposit(lua_State* L) {
      m_Account->deposit(lua_tonumber(L, -1));
      lua_pop(L, 1);
      return 0;  
   }
   int withdraw(lua_State* L) {
      m_Account->withdraw(lua_tonumber(L, -1));
      lua_pop(L, 1);
      return 0;
   }
   int balance(lua_State* L) {
      lua_pushnumber(L, m_Account->balance());
      return 1;  
   }
 private:
   Account* m_Account;
};

const char Account_Proxy::className[] = "Account";
const Luna<Account_Proxy>::RegType Account_Proxy::Register[] = {
     {"deposit",  &Account_Proxy::deposit},
     {"withdraw", &Account_Proxy::withdraw},
     {"balance",  &Account_Proxy::balance},
     {0}
};

int main(int argc, char* argv[])
{
   lua_State* L = lua_open(0);
   lua_baselibopen(L);
   
   Luna<Account_Proxy>::Register(L);
   lua_dofile(L, argv[1]);
   
   lua_close(L);
   return 0;
}
