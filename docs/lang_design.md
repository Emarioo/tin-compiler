Extended Backus-Naur form
```c
program := { struct | function | global_var } *

global_var := "global" declaration

struct := "struct" id "{" members "}"
members := { id ":" type "," } *

function := "fun" id "(" parameters ")" [ ":" type ] "{" statements "}"
parameters := id ":" type { "," id ":" type "," } *

statements := { while | if | return | function_call | declaration } *

loop_statements := statements | { "break" | "continue" } *

declaration := id ":" type [ "=" expression ] ";"

return := "return" [ expression ] ";"
while := "while" expression "{" loop_statements "}"
if := "if" expression "{" statements "}" [ else ]
else := "else" [ if | "{" statements "}" ]

function_call := id "(" [ arguments ] ")"
arguments := expression { "," expression } *

expression := term { ( "+" | "-" ) term } *
term :=  factor { ( "*" | "/" ) term } *
factor := "(" expression ")" | operand
operand := id | number | function_call

number := [0-9]*
id := [a-zA-Z][a-zA-Z0-9]*
type := id [ "*" ] *
```

Language examples
```c++

fun main() {
    print("hi")
}

fun name(name: int, name: int): int {

}
struct Yo {
    na: int;
    si: string;
    no: int;
}
fun name(arg int, arg2 int) int {
    a: int;
    b: int = 0;
    a = "sasasa";
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
    namep: string*;
    name: string,
}

```

Types in the language: int, string
Handled as semantics, not as syntax