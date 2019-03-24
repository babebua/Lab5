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
enum { center, right, left };
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

void GenerateMIPSCode(Node root, char position) {
    if (root != NULL) {
        if (root->kind == number && position == left) {
            fprintf(fp, "li $a0 %d\n", root->val);
            fprintf(fp, "sw $a0 ,0($sp)\n");
            fprintf(fp, "addi $sp ,$sp, -4\n");
            return;
        } else if (root->kind == number && position == right) {
            fprintf(fp, "li $a0 %d\n", root->val);
            return;
        }
        GenerateMIPSCode(root->left, left);
        GenerateMIPSCode(root->right, right);
        switch (root->kind) {
        case plus:
            fprintf(fp, "lw $t1, 4($sp)\n");
            fprintf(fp, "add $a0, $t1, $a0\n");
            fprintf(fp, "addi $sp, $sp, 4\n");
            if (position == left) {
                fprintf(fp, "sw $a0, 0($sp)\n");
                fprintf(fp, "addi $sp, $sp, -4\n");
            }
            return;
        case minus:
            fprintf(fp, "lw $t1, 4($sp)\n");
            fprintf(fp, "sub $a0, $t1, $a0\n");
            fprintf(fp, "addi $sp, $sp, 4\n");
            if (position == left) {
                fprintf(fp, "sw $a0, 0($sp)\n");
                fprintf(fp, "addi $sp, $sp, -4\n");
            }
            return;
        case times:
            fprintf(fp, "lw $t1, 4($sp)\n");
            fprintf(fp, "mul $a0, $t1, $a0\n");
            fprintf(fp, "addi $sp, $sp, 4\n");
            if (position == left) {
                fprintf(fp, "sw $a0, 0($sp)\n");
                fprintf(fp, "addi $sp, $sp, -4\n");
            }
            return;
        case divide:
            fprintf(fp, "lw $t1, 4($sp)\n");
            fprintf(fp, "div $t1, $a0\n");
            fprintf(fp, "mflo $a0\n");
            if (position == left) {
                fprintf(fp, "sw $a0, 0($sp)\n");
                fprintf(fp, "addi $sp, $sp, -4\n");
            }
            return;
        case mod:
            fprintf(fp, "lw $t1, 4($sp)\n");
            fprintf(fp, "div $t1, $a0\n");
            fprintf(fp, "mfhi $a0\n");
            fprintf(fp, "addi $sp, $sp, 4\n");
            if (position == left) {
                fprintf(fp, "sw $a0, 0($sp)\n");
                fprintf(fp, "addi $sp, $sp, -4\n");
            }
            return;
        }
    }
}
int main(int argc, char *argv[]) {
    fp = fopen("expression.asm", "w");
    fprintf(fp, ".text # text section \n");
    fprintf(fp, ".globl main # call main by SPIM \n");
    fprintf(fp, "main:\n");
    register Node result;
    if (argc == 2) {
        SInit(argv[1]);
        sym = SGet();
        result = Expr();
        assert(sym == eof);
        GenerateMIPSCode(result, right);
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