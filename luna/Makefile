all:
	g++ -g account.cc -llua -llualib -o account
	g++ account2.cc -llua -llualib -o account2
	./account account.lua
	./account2 account2.lua
