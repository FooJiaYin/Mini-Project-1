#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "lex.h"
#include "parser.h"
/*
Something like Python
>> y = 2
>> z = 2
>> x = 3*y + 4/(2*z)

*/

/*
the only type: integer
everything is an expression
  statement   := END | expr END
  expr        := term expr_tail
  expr_tail   := ADDSUB term expr_tail | NIL
  term        := factor term_tail
  term_tail := MULDIV factor term_tail | NIL
  factor      := INT | ADDSUB INT | ADDSUB ID | ID ASSIGN expr | ID | LPAREN expr RPAREN
*/

int main()
{
    printf(">> ");
    while (1) {
        statement();
    }
    return 0;
}

