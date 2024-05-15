#include <stdio.h>

int x;
int y;

void Function1() {
    int sum = x + y;
    printf("%d\n", sum);
}

void Function2() {
    int difference = x - y;
    printf("%d\n", difference);
}

void Function3() {
    int product = x * y;
    printf("%d\n", product);
}

void Function4() {
    if (y != 0) {
        int quotient = x / y;
        printf("%d\n", quotient);
    } else {
        printf("Error: Division by zero\n");
    }
}

int main() {
    // Prompt for the user to enter numbers
    printf("Enter the first number: ");
    scanf("%d", &x);
    printf("Enter the second number: ");
    scanf("%d", &y);

    int choice;
    printf("Select an operator:\n");
    printf("1. Addition\n");
    printf("2. Subtraction\n");
    printf("3. Multiplication\n");
    printf("4. Division\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            Function1();
            break;
        case 2:
            Function2();
            break;
        case 3:
            Function3();
            break;
        case 4:
            Function4();
            break;
        default:
            printf("Invalid choice\n");
            break;
    }

    return 0;
}

