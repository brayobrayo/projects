x = input("Enter a number: ")
y = input("Enter a second number: ")

# Corrected variable names
sum = int(x) + int(y)
difference = int(x) - int(y)
product = int(x) * int(y)
quotient = int(x) / int(y)
print("1:addition, 2:subtraction, 3:multiplication, 4:division")
choice = input("Enter your operation (1-4): ")

if choice == 1:
    print("Sum:", sum)
elif choice == 2:
    print("Difference:", difference)
elif choice == 3:
    print("Product:", product)
elif choice == 4:
    print("Quotient:", quotient)
else:
    print("Invalid operation")
