--
-- Luna Account example
--
local a = Account{balance=100}
a:deposit(50.00)
a:withdraw(25.00)
print("Account balance = $"..a:balance())

local e = Euclid()
print("Euclid gcd = "..e:gcd(10000,2))
