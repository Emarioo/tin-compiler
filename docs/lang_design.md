Extended Backus-Naur form
```c
program ::= { struct | function | global_var | constant | include } *

global_var ::= "global" declaration
constant ::= "const" declaration

include ::= "include" "\"" filename "\""
filename ::= any characters except?

struct ::= "struct" id "{" members "}"
members ::= { id ":" type "," } *

function ::= "fun" id "(" parameters ")" [ ":" type ] "{" statements "}"
parameters ::= id ":" type { "," id ":" type "," } *

statements ::= { while | if | return | function_call | constant | declaration | assignment } *

declaration ::= id ":" type [ "=" expression ] ";"
assignment ::= id "=" expression ";"

loop_statements ::= statements | { "break" | "continue" } *

return ::= "return" [ expression ] ";"
while ::= "while" expression "{" loop_statements "}"
if ::= "if" expression "{" statements "}" [ else ]
else ::= "else" [ if | "{" statements "}" ]

function_call ::= id "(" [ arguments ] ")"
arguments ::= expression { "," expression } *

expression ::= comparison { ( "||" | "&&" ) expression } *
comparison ::= arithmetic_low { ( "==" | "<" | ">" | "<=" | ">=" ) comparison } *
arithmetic_low ::= arithmetic_high { ( "+" | "-" ) arithmetic_low } *
arithmetic_high ::= unary_operation { ( "*" | "/" ) arithmetic_high } *
unary_operation ::= "!" unary_operation | "*" unary_operation | "&" id | "sizeof" type | index_operation
index_operation ::= value { "[" expression "]" } *
value ::= "(" expression ")" | id | number | function_call | string

number ::= [0-9]*
string ::= any characters in quotes?
id ::= [a-zA-Z][a-zA-Z0-9]*
type ::= id [ "*" ] *
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

include <main.txt>
include <stdio.txt>

```

Types in the language: int, string
Handled as semantics, not as syntax