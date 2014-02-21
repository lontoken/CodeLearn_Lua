#include "luna.h"
extern "C" {
#include <lualib.h>
}

class Account
{
 public:
   static const char className[];
   static const Luna<Account>::RegType Register[];
   
   Account(lua_State* L) {
      /* constructor table at top of stack */
      lua_pushstring(L, "balance");
      lua_gettable(L, -2);
      m_balance = lua_tonumber(L, -1);
      lua_pop(L, 2); /* pop constructor table and balance */
   }
   
   int deposit(lua_State* L) {
      m_balance += lua_tonumber(L, -1);
      lua_pop(L, 1);
      return 0;  
   }
   int withdraw(lua_State* L) {
      m_balance -= lua_tonumber(L, -1);
      lua_pop(L, 1);
      return 0;
   }
   int balance(lua_State* L) {
      lua_pushnumber(L, m_balance);
      return 1;  
   }
 protected:
   double m_balance;
};

const char Account::className[] = "Account";
const Luna<Account>::RegType Account::Register[] = {
     {"deposit",  &Account::deposit},
     {"withdraw", &Account::withdraw},
     {"balance",  &Account::balance},
     {0}
};

class Euclid {
 public:
   static const char className[];
   static const Luna<Euclid>::RegType Register[];
   
   Euclid(lua_State* L) {
   }
   
   int gcd(lua_State* L) {
      int m = static_cast<int>(lua_tonumber(L, -1));
      int n = static_cast<int>(lua_tonumber(L, -2));
      lua_pop(L, 2);
      
      /* precondition: m>0 and n>0.  Let g=gcd(m,n). */
      while( m > 0 ) {
	 /* invariant: gcd(m,n)=g */
	 if( n > m ) {
	    int t = m; m = n; n = t; /* swap */
	 }
	 /* m >= n > 0 */
	 m -= n;
      }
      
      lua_pushnumber(L, n);
      return 1;
   }
};

const char Euclid::className[] = "Euclid";
const Luna<Euclid>::RegType Euclid::Register[] = {
     {"gcd", &Euclid::gcd},
     {0}
};

int main(int argc, char* argv[])
{
   lua_State* L = lua_open(0);
   lua_baselibopen(L);
      
   Luna<Account>::Register(L);
   Luna<Euclid>::Register(L);
   
   lua_dofile(L, argv[1]);
   
   lua_close(L);
   return 0;
}
