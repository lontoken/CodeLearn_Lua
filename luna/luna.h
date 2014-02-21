/*
 * Lenny Palozzi - lenny.palozzi@home.com
 */
extern "C" {
#include <lua.h>
}

template <class T>
class Luna
{
 public:
   /* member function map */
   struct RegType { 
      const char* name; 
      const int(T::*mfunc)(lua_State*);
   };      
   /* register class T */
   static void Register(lua_State* L) {
      lua_pushcfunction(L, &Luna<T>::constructor);
      lua_setglobal(L, T::className);
      
      if (otag == 0) {
	otag = lua_newtag(L);
	lua_pushcfunction(L, &Luna<T>::gc_obj);
	lua_settagmethod(L, otag, "gc"); /* tm to release objects */
      }
   }
 private:
   static int otag; /* object tag */
   
   /* member function dispatcher */
   static int thunk(lua_State* L) {
      /* stack = closure(-1), [args...], 'self' table(1) */
      int i = static_cast<int>(lua_tonumber(L,-1));
      lua_pushnumber(L, 0); /* userdata object at index 0 */
      lua_gettable(L, 1);
      T* obj = static_cast<T*>(lua_touserdata(L,-1));
      lua_pop(L, 2); /* pop closure value and obj */
      return (obj->*(T::Register[i].mfunc))(L);
   }
   
   /* constructs T objects */
   static int constructor(lua_State* L) {
      T* obj= new T(L); /* new T */
      /* user is expected to remove any values from stack */
      
      lua_newtable(L); /* new table object */
      lua_pushnumber(L, 0); /* userdata obj at index 0 */
      lua_pushusertag(L, obj, otag); /* have gc call tm */
      lua_settable(L, -3);
      
      /* register the member functions */
      for (int i=0; T::Register[i].name; i++) {
	 lua_pushstring(L, T::Register[i].name);
	 lua_pushnumber(L, i);
	 lua_pushcclosure(L, &Luna<T>::thunk, 1);
	 lua_settable(L, -3);
      }
      return 1; /* return the table object */
   }

   /* releases objects */
   static int gc_obj(lua_State* L) {
      T* obj = static_cast<T*>(lua_touserdata(L, -1));
      delete obj;
      return 0;
   }
 protected: 
   Luna(); /* hide default constructor */
};
template <class T>
int Luna<T>::otag = 0;
