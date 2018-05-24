#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

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

#define MAXLEN 256

#define TBLSIZE 65535

typedef enum { UNKNOWN,
               END,
               INT,
               ID,
               ADDSUB,
               MULDIV,
               ASSIGN,
               LPAREN,
               RPAREN } TokenSet;

typedef struct
{
    char name[MAXLEN];
    int val;
} Symbol;
Symbol table[TBLSIZE];
int sbcount = 0;

typedef struct _Node
{
    char lexeme[MAXLEN];
    TokenSet token;
    int val;
    int weight;
    struct _Node *left, *right;
} BTNode;

void statement(void);
BTNode *expr(void);
BTNode *term(void);
BTNode *factor(void);
int getval(void);
int setval(char *, int);
void error(void);

/* Lexical-related function */
int match(TokenSet token);
void advance(void);
char *getLexeme(void);
static TokenSet getToken(void);
static TokenSet lookahead = UNKNOWN;
static char lexeme[MAXLEN];

/* create a node without any child */
BTNode *makeNode(TokenSet tok, const char *lexe)
{
    BTNode *node = (BTNode *)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->token = tok;
    node->val = 0;
    node->weight = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void constructTable()
{
    for(int i=0; i<3; i++) {
        table[i].name[0] = 'x'+i;
        table[i].name[1] = '\0';
        table[i].val = 1;
    }
    sbcount = 3;
}

/* clean a tree */
void freeTree(BTNode *root)
{
    if (root != NULL)
    {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

/* print a tree by pre-order */
void printPrefix(BTNode *root)
{
    if (root != NULL)
    {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}

void printInfix(BTNode *root)
{
    if(root->left){
        if(root->left->token==ADDSUB && root->token==MULDIV) {
            printf("(");
            printInfix(root->left);
            printf(")");
        }
        else printInfix(root->left);
    }
    if(root->token==INT) printf("%d ", root->val);
    else printf("%s ", root->lexeme);
    if(root->right) {
        if(root->right->token==ADDSUB) {
            printf("( ");
            printInfix(root->right);
            printf(") ");
        }
        else printInfix(root->right);
    }
}

/* traverse the syntax tree by pre-order
   and evaluate the underlying expression */

int start;

/* Registers */
int reg[11] = {0}; //8 register + 3 memory;
int state[11] = {0}; //0:=available, 1:=temporary data, 2:=preserved, 3:preserved and using
int count[3] = {0};
int primary = 0;
TokenSet last_token = -1;

void toINT(BTNode *root)
{
    root->token = INT;
    root->weight = 0;
    strcpy(root->lexeme, "\0");
    //if(root->left) free(root->left);
    //if(root->right) free(root->right);
    root->left = root->right = NULL;
}

BTNode* simulate(BTNode *root)
{
	if (root != NULL) {
		switch (root->token) {
        case ID:
            count[root->lexeme[0]-'x']++;
            break;
        case ASSIGN:
            error();
            break;
		case ADDSUB:
		case MULDIV:
            /* TODO */
            if(root->left && root->right){
                root->left = simulate(root->left);
                root->right = simulate(root->right);
                if(root->left->token==INT && root->right->token==INT) {
                    if (root->lexeme[0]!='+' && root->lexeme[0]=='-' && root->lexeme[0]=='*' && root->lexeme[0]=='/')  error();
                }
                
            }
            else error();
            if(root->left && root->right) {
                root->weight = root->left->weight + root->right->weight;
                if(root->left->token==INT||root->left->token==ID) root->weight = root->right->weight + 1;
            }
            else if(root->token == INT) root->weight = 0;
            else error();
            break;
		}
	}
	return root;
}

int evaluateTree(BTNode *root)
{
    int retval = 0, lv, rv;
    int regId = 0, regId_left, regId_right;
    if (root != NULL)
    {
        switch (root->token)
        {
        case ID:
            regId = root->lexeme[0] - 'x'; //x: regId = 0,y: regId = 1,z: regId = 2,
            if (last_token == ASSIGN) {
                for(int i=0; i<3; i++) {
                    if(count[i] && state[i]==0) {
                        printf("MOV r%d [%d]\n", i, i*4);
                        state[i] = 2;
                    }
                }
                if(count[primary]==0) state[regId] = 0;
            }
            else {
                retval = root->val;
                count[regId]--;
                if(state[regId]==0){
                    printf("MOV r%d [%d]\n", regId, regId*4);
                    state[regId] = 3;
                }
                else if(state[regId]==1) {
                    int memId = regId*4;
                    for(regId=0; state[regId]>0; regId++);
                    if(regId>=8) {
                        for(regId=0; state[regId]!=2; regId++);
                        printf("MOV [%d] r%d\n", regId*4, regId);
                    }
                    printf("MOV r%d [%d]\n", regId, memId);
                    state[regId] = 1;
                }
                else if(state[regId]==2) {
                    state[regId] = 3;
                }
                if(regId == primary && count[primary]==0) state[regId] = 1;
            }
            last_token = root->token;
            break;
        case INT:
            retval = root->val;
            if(state[primary]==0) regId = primary;
            else {
                for(regId=0; state[regId]>0; regId++);
                if(regId>=8) for(regId=0; state[regId]!=2; regId++);
            }
            printf("MOV r%d %d\n", regId, root->val);
            state[regId] = 1;
            last_token = root->token;
            break;
        case ASSIGN:
        case ADDSUB:
        case MULDIV:
            /* TODO */
            last_token = root->token;
            if ((strcmp(root->lexeme, "+")==0||strcmp(root->lexeme, "*")==0) && root->right->weight > root->left->weight) {
                regId_right = evaluateTree(root->right);
                regId_left = evaluateTree(root->left);
            }
            else {
                regId_left = evaluateTree(root->left);
                regId_right = evaluateTree(root->right);
            }
            lv = reg[regId_left];
            rv = reg[regId_right];
            regId = regId_left;
            if (strcmp(root->lexeme, "+") == 0)
            {
                retval = lv + rv;
                if((state[regId_left]==3 || regId_right==primary) && state[regId_right]==1) {
                    printf("ADD r%d r%d\n", regId_right, regId_left);
                    if(state[regId_left]==3) state[regId_left] = 2;
                    if(state[regId_left]==1) state[regId_left] = 0;
                    regId = regId_right;
                }
                else {
                    if(state[regId_left]==3) {
                        for(regId=0; state[regId]>0; regId++);
                        if(regId<8) {
                            printf("MOV r%d r%d\n", regId, regId_left);
                            state[regId_left] = 2;
                        }
                        else {
                            regId = regId_left;
                            printf("MOV [%d] r%d\n", regId*4, regId);
                        }
                        state[regId] = 1;
                    }
                    printf("ADD r%d r%d\n", regId, regId_right);
                    if(state[regId_right]==3) state[regId_right] = 2;
                    if(state[regId_right]==1) state[regId_right] = 0;
                }
            }
            else if (strcmp(root->lexeme, "-") == 0)
            {
                retval = lv - rv;
                if(state[regId_left]==3) {
                    for(regId=0; state[regId]>0; regId++);
                    if(regId<8) {
                        printf("MOV r%d r%d\n", regId, regId_left);
                        state[regId_left] = 2;
                    }
                    else {
                        regId = regId_left;
                        printf("MOV [%d] r%d\n", regId*4, regId);
                    }
                    state[regId] = 1;
                }
                printf("SUB r%d r%d\n", regId, regId_right);
                if(state[regId_right]==3) state[regId_right] = 2;
                if(state[regId_right]==1) state[regId_right] = 0;
            }
            else if (strcmp(root->lexeme, "*") == 0)
            {
                retval = lv * rv;
                if((state[regId_left]==3 || regId_right==primary) && state[regId_right]==1) {
                    printf("MUL r%d r%d\n", regId_right, regId_left);
                    if(state[regId_left]==3) state[regId_left] = 2;
                    if(state[regId_left]==1) state[regId_left] = 0;
                    regId = regId_right;
                }
                else {
                    if(state[regId_left]==3) {
                        for(regId=0; state[regId]>0; regId++);
                        if(regId<8) {
                            printf("MOV r%d r%d\n", regId, regId_left);
                            state[regId_left] = 2;
                        }
                        else {
                            regId = regId_left;
                            printf("MOV [%d] r%d\n", regId*4, regId);
                        }
                        state[regId] = 1;
                    }
                    printf("MUL r%d r%d\n", regId, regId_right);
                    if(state[regId_right]==3) state[regId_right] = 2;
                    if(state[regId_right]==1) state[regId_right] = 0;
                }
            }
            else if (strcmp(root->lexeme, "/") == 0)
            {
                if(rv==0) error();
                else {
                    retval = lv / rv;
                    if(state[regId_left]==3) {
                        for(regId=0; state[regId]>0; regId++);
                        if(regId<8) {
                            printf("MOV r%d r%d\n", regId, regId_left);
                            state[regId_left] = 2;
                        }
                        else {
                            regId = regId_left;
                            printf("MOV [%d] r%d\n", regId*4, regId);
                        }
                        state[regId] = 1;
                    }
                    printf("DIV r%d r%d\n", regId, regId_right);
                    if(state[regId_right]==3) state[regId_right] = 2;
                    if(state[regId_right]==1) state[regId_right] = 0;
                }
            }
            else if (strcmp(root->lexeme, "=") == 0)
            {
                retval = setval(root->left->lexeme, rv);
                if(regId_right!=regId_left) {
                    printf("MOV r%d r%d\n", regId_left, regId_right);
                }
                state[regId_left] = 2;
                if(state[regId_right]==3) state[regId_right] = 2;
                if(state[regId_right]==1) state[regId_right] = 0;
            }
            if (root->lexeme[0] != '=') state[regId] = 1;
            break;
        default:
            retval = 0;
        }
    }

    reg[regId] = retval;
    return regId;
}

int getval(void) //return a value from INT / ID
{
    int i, found, retval = 0;

    if (match(INT))
    {                               //lookahead == INT
        retval = atoi(getLexeme()); //set retval = int
    }
    else if (match(ID))
    { //lookahead ==ID
        i = 0;
        found = 0;
        retval = 0;
        while (i < sbcount && !found)
        { //find the variable in table and set retval
            if (strcmp(getLexeme(), table[i].name) == 0)
            {
                retval = table[i].val;
                found = 1;
                break;
            }
            else
            {
                i++;
            }
        }
        if (!found)
        { //add new variable to table, return initial value = 0
            if (sbcount < TBLSIZE)
            {
                strcpy(table[sbcount].name, getLexeme());
                table[sbcount].val = 1;
                sbcount++;
            }
            else
            {
                error();
            }
        }
    }
    return retval;
}

int setval(char *str, int val) //set variable in table to val
{
    int i, retval = 0;
    i = 0;
    while (i < sbcount)
    {
        if (strcmp(str, table[i].name) == 0)
        {
            table[i].val = val;
            retval = val;
            break;
        }
        else
        {
            i++;
        }
    }
    return retval;
}

//  expr        := term expr_tail
//  expr_tail   := ADDSUB term expr_tail | NIL
BTNode *expr(void)
{
    BTNode *retp, *left;
    retp = left = term();
    while (match(ADDSUB))
    { // tail recursion => while
        retp = makeNode(ADDSUB, getLexeme());
        advance();
        retp->right = term();
        retp->left = left;
        left = retp;
    }
    return retp;
}

//  term        := factor term_tail
//  term_tail := MULDIV factor term_tail | NIL
BTNode *term(void)
{
    BTNode *retp, *left;
    retp = left = factor();
    while (match(MULDIV))
    { // tail recursion => while
        retp = makeNode(MULDIV, getLexeme());
        advance();
        retp->right = factor();
        retp->left = left;
        left = retp;
    }
    return retp;
}

BTNode *factor(void)
{
    BTNode *retp = NULL;
    char tmpstr[MAXLEN];

    if (match(ID))
    {
        start = 0;
        BTNode *left = makeNode(ID, getLexeme());
        left->val = getval();
        strcpy(tmpstr, getLexeme());
        advance();
        if (match(ASSIGN))
        {
            retp = makeNode(ASSIGN, getLexeme());
            advance();
            retp->right = expr();
            retp->left = left;
        }
        else
        {
            retp = left;
        }
    }
    else if (start) error();
    else if (match(INT))
    {
        retp = makeNode(INT, getLexeme());
        retp->val = getval();
        advance();
    }
    else if (match(ADDSUB))
    {
        strcpy(tmpstr, getLexeme());
        advance();
        if (match(ID) || match(INT))
        {
            retp = makeNode(ADDSUB, tmpstr);
            if (match(ID))
                retp->right = makeNode(ID, getLexeme());
            else
                retp->right = makeNode(INT, getLexeme());
            retp->right->val = getval();
            retp->left = makeNode(INT, "0");
            retp->left->val = 0;
            advance();
        } else {
            error();
        }
    }
    else if (match(LPAREN))
    {
        advance();
        retp = expr();
        if (match(RPAREN))
        {
            advance();
        } else {
            error();
        }
    } else {
        error();
    }
    return retp;
}

void endProccess(void)
{
    for(int regId=0; regId<3; regId++){
        if(state[regId]!=2) printf("MOV r%d [%d]\n", regId, regId*4);
    }
    printf("EXIT 0\n");
    exit(0);
}

void error(void)
{
    /* TODO:
     *
     * Error-Handler,
     * You should deal with the error that happened in calculator
     * An example is x = 5 / 0, which is divide zero error.
     * You should call error() when any error occurs
     *
     */

    printf("EXIT 1\n");
    exit(0);
}

void statement(void)
{
    BTNode *retp;
    start = 1;

    if (match(END))
    {
        advance();
    }
    else
    {
        retp = expr();
        if (match(END))
        {
            if(retp->token!=ASSIGN || retp->left->token != ID || retp->left->left || retp->left->right) error();
            primary = retp->left->lexeme[0] - 'x';
            //printPrefix(retp);
            //printf("\n");
            retp->right = simulate(retp->right);
            for(int i=0; i<3; i++) count[i] = 0;
            retp->right = simulate(retp->right);
            evaluateTree(retp);
            freeTree(retp);
            advance();
        }
    }
}

TokenSet getToken(void)
{
    int i;
    char c;

    while ((c = fgetc(stdin)) == ' ' || c == '\t')
        ;

    if (isdigit(c))
    {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN)
        {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    }
    else if (c == '+' || c == '-')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return ADDSUB;
    }
    else if (c == '*' || c == '/')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    }
    else if (c == '\n')
    {
        lexeme[0] = '\0';
        return END;
    }
    else if (c == '=')
    {
        strcpy(lexeme, "=");
        return ASSIGN;
    }
    else if (c == '(')
    {
        strcpy(lexeme, "(");
        return LPAREN;
    }
    else if (c == ')')
    {
        strcpy(lexeme, ")");
        return RPAREN;
    }
    else if (c=='x'||c=='y'||c=='z')
    {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        /*while (isalpha(c) || isdigit(c) || c == '_')
        {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }*/
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    }
    else if (c==EOF) endProccess();
    else
    {
        error();
        //return UNKNOWN;
    }
}

void advance(void)
{
    lookahead = getToken();
}

int match(TokenSet token)
{
    if (lookahead == UNKNOWN)
        advance();
    return token == lookahead;
}

char *getLexeme(void)
{
    return lexeme;
}

int main()
{
    char c;
    constructTable();
    while (1)
    {
        statement();
    }
    return 0;
}
