#include "lex.h"
#define TBLSIZE 65535
#ifndef __PARSER__
#define __PARSER__


typedef enum {MISPAREN, NOTNUMID, NOTFOUND, RUNOUT} ErrorType;

typedef struct {
    char name[MAXLEN];
    int val;
} Symbol;
Symbol table[TBLSIZE];


typedef struct _Node {
    char lexeme[MAXLEN];
    TokenSet data;
    int val;
    struct _Node *left, *right;
} BTNode;


extern int getval(void);
extern int setval(char *str, int val);
extern BTNode* makeNode(TokenSet tok, const char *lexe);
extern void freeTree(BTNode *root);
extern BTNode* factor(void);
extern BTNode* term(void);
extern BTNode* expr(void);
extern void statement(void);
extern void error(ErrorType errorNum);
#endif // __PARSER__
