```c
program := { struct | function } *

struct := "struct" id "{" members "}"

members := { id type "," } *

function := "fun" id "(" parameters ")" type "{" statements "}"

parameters := { id type "," } *

statements := { while | if | function_call | declaration } *

declaration := id type "=" expression ";"

while := "while" expression "{" statements "}"
if := "if" expression "{" statements "}" [ else ]
else := "else" "{" statements "}"

function_call := id "(" [ arguments ] ")"
arguments := expression { "," expression } *

expression := term { "+" term } *
term :=  factor { "*" term } *
factor := "(" expression ")" | operand
operand := id | number | function_call

// 1 + 3 * ( 3 - hey(3 + 5, 2))

```


```c++

fun name(name int, name int) int {

}
fun name() int

int name(int arg, int arg2) {
    int b = 0;

    a = 3;
    a = 3 + 4;

    name(23,23)

    while true { }
    if true { } else 
    break
    continue
    return 
}

struct KOkea {
    int* name;
    int** name;
    name : int,
    name int
    name int;
}

```