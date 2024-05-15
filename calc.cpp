// basic calculator in c++
#include <iostream>
using namespace std;
int x, y;
int sum;
int difference;
int product;
int quotient;
int choice;
//declaring the functions
void function1()
{

    sum = x + y;
    cout << x << "+" << y << "=" << sum;
}
void function2()

    difference = x - y;
    cout << x << "-" << y << "=" <<difference;
}
void function3()
{
    product = x * y;
    cout << x << "*" << y << "=" <<product;
}
void function4()
{
    quotient = x / y;
    if ( y == 0)
    {
        cout << "SYNTAX ERROR";
    }
    else
    {
        cout << x << " / " << y << " = " << quotient;
    }
}

//prompt for user to enter the numbers
int main()
{
    cout <<"enter a first number: ";
    cin >>x;
    cout <<"enter your operation: ";
    cin >>choice;
    cout <<"enter a second number: ";
    cin >>y;
    //selecting the functions




   if(choice == "+"){

    function1();
    }
    else if(choice == "-"){
        function2();
    }
    else if(choice == "*"){
        function3();
    }
    else if(choice == "/"){
        function4();
    }
    else{
        cout <<"unrecognized function";
    };




    return 0;
}
