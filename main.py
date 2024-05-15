# Prompt for user to enter numbers
x = int(input("Enter a number: "))
y = int(input("Enter a second number: "))

# Declaring the operations
sum = x + y
difference = x - y
product = x * y
quotient = x / y

print("1: Addition, 2: Subtraction, 3: Multiplication, 4: Division")

# Selecting an operation
choice = int(input("Enter your operation (1-4): "))

if choice == 1:
    print("The sum is:", sum)
elif choice == 2:
    print("The difference is:", difference)
elif choice == 3:
    print("The product is:", product)
elif choice == 4:
    print("The quotient is:", quotient)
else:
    print("Invalid operation")
