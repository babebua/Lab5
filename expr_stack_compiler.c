#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static FILE *f;
FILE *fp;
static int ch;
static unsigned int val;
enum {
    var,
    plus,
    minus,
    times,
    divide,
    mod,
    lparen,
    rparen,
    number,
    eof,
    illegal
};

typedef struct NodeDesc *Node;
typedef struct NodeDesc {
    char kind;        // plus, minus, times, divide, number
    int val;          // number: value
    Node left, right; // plus, minus, times, divide: children
} NodeDesc;

static void SInit(char *filename) {
    ch = EOF;
    f = fopen(filename, "r+t");
    if (f != NULL)
        ch = getc(f);
}
static void Number() {
    val = 0;
    while (('0' <= ch) && (ch <= '9')) {
        val = val * 10 + ch - '0';
        ch = getc(f);
    }
}
static int SGet() {
    register int sym;
    while ((ch != EOF) && (ch <= ' '))
        ch = getc(f);
    switch (ch) {
    case EOF:
        sym = eof;
        break;
    case 'x':
        sym = var;
        ch = getc(f);
        break;
    case '+':
        sym = plus;
        ch = getc(f);
        break;
    case '-':
        sym = minus;
        ch = getc(f);
        break;
    case '*':
        sym = times;
        ch = getc(f);
        break;
    case '/':
        sym = divide;
        ch = getc(f);
        break;
    case '%':
        sym = mod;
        ch = getc(f);
        break;
    case '(':
        sym = lparen;
        ch = getc(f);
        break;
    case ')':
        sym = rparen;
        ch = getc(f);
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        sym = number;
        Number();
        break;
    default:
        sym = illegal;
    }
    return sym;
}
static int sym;
static Node Expr();
static Node Factor() {
    register Node result;
    assert((sym == var) || (sym == number) || (sym == lparen));
    if (sym == number || sym == var) {
        result = malloc(sizeof(NodeDesc));
        result->kind = sym;
        result->val = val;
        result->left = NULL;
        result->right = NULL;
        sym = SGet();
    } else {
        sym = SGet();
        result = Expr();
        assert(sym == rparen);
        sym = SGet();
    }
    return result;
}
static Node Term() {
    register Node root, result;
    root = Factor();
    while ((sym == times) || (sym == divide) || (sym == mod)) {
        result = malloc(sizeof(NodeDesc));
        result->kind = sym;
        result->left = root;
        sym = SGet();
        result->right = Factor();
        root = result;
    }
    return root;
}
static Node Expr() {
    register Node root, result;
    root = Term();
    while ((sym == plus) || (sym == minus)) {
        result = malloc(sizeof(NodeDesc));
        result->kind = sym;
        result->left = root;
        sym = SGet();
        result->right = Term();
        root = result;
    }
    return root;
}

void GenerateStacksCode(Node root, int position) {
    if (root != NULL) {
        if (root->kind == number && position == -1) {
            printf("acc << %d\n", root->val);
            printf("push acc\n");
            return;
        } else if (root->kind == number && position == 1) {
            printf("acc << %d\n", root->val);
            return;
        }
        GenerateStacksCode(root->left, -1);
        GenerateStacksCode(root->right, 1);
        switch (root->kind) {
        case plus:
            printf("acc << acc + tos\n");
            printf("pop\n");
            if (position == -1) {
                printf("push acc\n");
            }
            return;
        case minus:
            printf("acc << acc - tos\n");
            printf("pop\n");
            if (position == -1) {
                printf("push acc\n");
            }
            return;
        case times:
            printf("acc << acc * tos\n");
            printf("pop\n");
            if (position == -1) {
                printf("push acc\n");
            }
            return;
        case divide:
            printf("acc << acc / tos\n");
            printf("pop\n");
            if (position == -1) {
                printf("push acc\n");
            }
            return;
        case mod:
            printf("acc << acc %% tos\n");
            printf("pop\n");
            if (position == -1) {
                printf("push acc\n");
            }
            return;
        }
    }
}
int main(int argc, char *argv[]) {
    fp = fopen("program.asm", "w");
    fprintf(fp, ".text # text section \n");
    fprintf(fp, ".globl main # call main by SPIM \n");
    fprintf(fp, "main:\n");
    register Node result, diffr, simplifyr;
    if (argc == 2) {
        SInit(argv[1]);
        sym = SGet();
        result = Expr();
        assert(sym == eof);
        GenerateStacksCode(result, 1);
    } else
        printf("usage: expreval <filename>\n");
    fprintf(fp, "addi $sp, $sp, 4\n");
    fprintf(fp, "li $v0, 1\n");
    fprintf(fp, "syscall\n");
    fprintf(fp, "end:\n");
    fprintf(fp, "li $v0, 10 # system call 10 for exit\n");
    fprintf(fp, "syscall # we are out of here.\n");
    return 0;
}