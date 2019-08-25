/*
 * fpc.c
 *
 *  Created on: Apr 13, 2018
 *      Author: menright
 *  Parsing from goaway project.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <stddef.h> // official home of ptrdiff_t

// parameters

#define BUFSIZE (1024*1024*100)     // max input size

// data structures

#define NEW(T)    ((T *)ass(malloc(sizeof(T))))
#define NEWA(T,N) ((T *)ass(malloc((N)*sizeof(T))))

void *ass(void *p) {assert(p != NULL); return p;}

typedef struct list {
    void *data;
    struct list *next;
} list_t;

list_t *cons(void *x, list_t *xs)
{
    list_t *r = NEW(list_t);
    r->data = x;
    r->next = xs;
    return r;
}
void* head(list_t* xs) {
	// The top was put there with 'cons'.
	// The usual thing would be head : tail
	return xs->data;
}
list_t* tail(list_t* xs) {
	// The top was put there with 'cons'.
	// The usual thing would be head : tail
	return xs->next;
}

size_t length(const list_t *xs)
{
    size_t n=0;
    for(; xs; xs=xs->next) n++;
    return n;
}

// input language

struct expr;

typedef enum {
    EXPR_VAR,
    EXPR_APP,
    EXPR_STR,
    EXPR_NUM,
	EXPR_OPER,
	EXPR_LET
} exprtag_t;

typedef const char *expr_var_t;

typedef struct {
    const struct expr *fun;
    const struct expr *arg;
} expr_app_t;

typedef char *expr_str_t;

typedef int expr_num_t;

typedef int expr_oper_t;

typedef struct expr expr_t;
typedef struct {
	expr_var_t name;
	const expr_t* expr;
} expr_let_def;
typedef struct {
	list_t* let_defs;
	const expr_t* let_value;
} expr_let_t;
typedef struct expr {
    exprtag_t tag;
    union {
        expr_var_t var;
        expr_app_t app;
        expr_str_t str;
        expr_num_t num;
        expr_oper_t op;
        expr_let_t letx;
    };
} expr_t;

typedef struct {
    const char *name;
    const list_t *args;
    const expr_t *rhs;
} def_t;

// parser

typedef struct {
    void *data;
} result_t;

typedef enum {
	T_NAME,
	T_PUNCT,
	T_NUM,
	T_SEMI,
	T_EQUALS,
	T_LPAREN,
	T_RPAREN,
	T_LT,
	T_GT,
	T_LE,
	T_GE,
	T_OR,
	T_AND,
	T_ADD,
	T_SUB,
	T_MUL,
	T_DIV,
	T_MOD,
	T_LET,
	T_IN,
	//T_IF,
	T_EOF
} Token;
typedef struct {
	Token type;
	char* s;
	int value;
} tkn;
tkn token;
int ch;
FILE* fp;
void nextChar()
{
	ch = fgetc(fp);
}
//char* s;
void next() {
	while (true){
		while (ch == ' ' || ch=='\t' || ch=='\n')
			nextChar();
		if (ch=='#') {
			while (ch != '\n' && ch != EOF)
				nextChar();
			if (ch == '\n') {
				continue;
			} else {
				break;
			}
		}
		break;
	}
	if (feof(fp)) {
		token.type = T_EOF;
		return;
	}
	switch (ch) {
	case ';': token.type = T_SEMI; nextChar(); return;
	case '(': token.type = T_LPAREN; nextChar(); return;
	case ')': token.type = T_RPAREN; nextChar(); return;
	}
	char* t = malloc(128);
	char* s = t;
	if (isalpha(ch)) {
		while (isalnum(ch) || ch=='_' || ch=='\'')
		{
			*t++ = ch;
			nextChar();
		}
		*t++ = 0;
		free(token.s);
		token.s = strdup(s);
		token.type = T_NAME;
		if (strcmp(s, "let")==0)
			token.type = T_LET;
		else if (strcmp(s, "in")==0)
			token.type = T_IN;
		return;
	}
	if (isdigit(ch)) {
		token.value = ch - '0';
		nextChar();
		while (isdigit(ch)) {
			token.value = token.value*10 + ch-'0';
			nextChar();
		}
		token.type = T_NUM;
		return;
	}

	while (ch && !isalnum(ch) && ch!=';' &&ch !=')' && ch!='(' && ch!=' ' && ch!='\t' && ch!='\n' && ch!='#') {
		*t++ = ch;
		nextChar();
	}
	*t = 0;
	token.type = T_PUNCT;
	switch (t-s) {
	case 1: // 1-character sequence
		switch (*s){
		case '&': token.type = T_AND; break;
		case '|': token.type = T_OR; break;
		case '<': token.type = T_LT; break;
		case '>': token.type = T_GT; break;
		case '=': token.type = T_EQUALS; break;
		case '+': token.type = T_ADD; break;
		case '-': token.type = T_SUB; break;
		case '*': token.type = T_MUL; break;
		case '/': token.type = T_DIV; break;
		case '%': token.type = T_MOD; break;
		}
		break;
	case 2: // 2-character sequence
		if (strncmp(s,"<=",2)==0)
			token.type = T_LE;
		else if (strncmp(s,">=",2)==0)
			token.type = T_GE;
		break;
	}
	if (token.type == T_PUNCT) {
		free(token.s);
		token.s = strdup(s);
	}
	return;
}
const char* token_to_string(const tkn* token) {
	static char buf[50];
	switch (token->type) {
	case T_NAME: snprintf(buf, 50, "T_NAME: %s", token->s); break;
	case T_PUNCT: snprintf(buf, 50, "T_PUNCT: %s", token->s); break;
	case T_NUM: snprintf(buf, 50, "T_NUM: %d", token->value); break;
	case T_SEMI: snprintf(buf, 50, "T_SEMI"); break;
	case T_LPAREN: snprintf(buf, 50, "T_LPAREN"); break;
	case T_RPAREN: snprintf(buf, 50, "T_RPAREN"); break;
	case T_LT: snprintf(buf, 50, "T_LT"); break;
	case T_MUL: snprintf(buf, 50, "T_MUL"); break;
	case T_SUB: snprintf(buf, 50, "T_SUB"); break;
	case T_ADD: snprintf(buf, 50, "T_ADD"); break;
	case T_DIV: snprintf(buf, 50, "T_DIV"); break;
	case T_MOD: snprintf(buf, 50, "T_MOD"); break;
	case T_IN: snprintf(buf, 50, "T_IN"); break;
	case T_LET: snprintf(buf, 50, "T_LET"); break;
	default: snprintf(buf, 50, "%d", token->type); break;
	}
	return buf;
}
expr_t *parse_expr();

void pprint_expr(int col, const expr_t *e);
expr_t* mkleaf(tkn t) {
	expr_t* r = NEW(expr_t);
	switch (t.type) {
	case T_NAME: r->tag = EXPR_VAR; r->var = strdup(t.s); break;
	case T_NUM: r->tag = EXPR_NUM; r->num = t.value; break;
	default: return NULL;
	}
	return r;
}
expr_t* mkop(Token t) {
	expr_t* r = NEW(expr_t);
	r->tag = EXPR_OPER;
	r->op = t;
	return r;
}
expr_t* mkapp(expr_t* fun, expr_t* arg) {
	expr_t* r = NEW(expr_t);
	r->tag = EXPR_APP;
	r->app.fun = fun;
	r->app.arg = arg;
	return r;
}
bool atom(tkn t) {
	switch (t.type) {
	case T_NAME:
	case T_NUM:
		return true;
	default: break;
	}
	return false;
}
bool binop(Token t) {
	switch (t) {
	case T_OR:
	case T_AND:
	case T_ADD:
	case T_SUB:
	case T_MUL:
	case T_DIV:
	case T_MOD:
	case T_LT:
		return true;
	default: break;
	}
	return false;
}
bool postfix(Token t) {
	(void)t;
	return false; // don't have these yet.
}
int precedence(Token t) {
	switch (t){
	case T_OR:
		return 2;
	case T_AND:
		return 3;
	case T_LT:
		return 4;
	case T_ADD: case T_SUB:
		return 5;
	case T_DIV: case T_MOD:
		return 6;
	case T_MUL:
		return 7;
	default: break;
	}
	return 1;
}
int rightPrec(Token t) {
	switch (t){
	case T_OR:
		return 3;
	case T_AND:
		return 4;
	case T_LT:
		return 5;
	case T_ADD: case T_SUB:
		return 6;
	case T_DIV:
		return 7;
	case T_MUL:
		return 8;
	default: break;
	}
	return 1;
}
int nextPrec(Token t) {
	switch (t){
	case T_OR:
		return 1;
	case T_AND:
		return 2;
	case T_LT:
		return 3;
	case T_ADD: case T_SUB:
		return 5;
	case T_DIV:
		return 6;
	case T_MUL:
		return 7;
	default: break;
	}
	return -1;
}
expr_t* P(void);
expr_t* E(int p) {
	expr_t* t = P();
	while (atom(token) || token.type == T_LPAREN) {
		expr_t* arg = P();
		t = mkapp(t, arg);
	}
	int r = 8;
	//printf("E(%d): %s binop %d postfix %d precedence() %d, p <=precedence() %d precedence() <= r %d r=%d\n", p, token_to_string(&token), binop(token.type), postfix(token), precedence(token.type), p <= precedence(token.type), precedence(token.type)<=r, r);
	while ((binop(token.type) || postfix(token.type)) && ((p <= precedence(token.type)) && (precedence(token.type) <= r))) {
		Token b = token.type;
		next();
		if (binop(b)) {
			expr_t* t1 = E(rightPrec(b));
			//printf("RHS expr: "); pprint_expr(0, t1);
			t = mkapp(mkapp(mkop(b), t), t1);
			//pprint_expr(0, t);
		} else {
			t = mkapp(mkop(b), t);
		}
		r = nextPrec(b);
		//printf("E(%d): %s binop %d postfix %d precedence() %d, p <=precedence() %d precedence() <= r %d r=%d\n", p, token_to_string(&token), binop(token.type), postfix(token), precedence(token.type), p <= precedence(token.type), precedence(token.type)<=r, r);
	}
	//pprint_expr(0, t);
	return t;
}
expr_t* P(void) {
	if (token.type == T_SUB) { next(); expr_t* t = E(2); return mkapp(mkop(T_SUB), t); }
	else if (token.type == T_LPAREN) { next(); expr_t* t = E(0); assert(token.type==T_RPAREN); next(); return t; }
	else if (atom(token)) { expr_t* t = mkleaf(token); next(); return t; }
	else exit(1);
}
expr_t* parse_expr() {
	 return E(0);
}
list_t* parse_let_exprs()
{
	list_t* exs = NULL;
	next();
	while (token.type == T_NAME)
	{
		char* id = strdup(token.s);
		next();
		assert(token.type == T_EQUALS);
		next();
		expr_t* val = parse_expr();
		assert(token.type == T_IN || token.type == T_SEMI);
		if (token.type == T_SEMI)
			next();
		expr_let_def* def = NEW(expr_let_def);
		def->name = id;
		def->expr = val;
		exs = cons(def, exs);
	}
	assert(token.type == T_IN);
	next();
	return exs;
}
expr_t* parse_let() {
	expr_t* letx = NEW(expr_t);
	letx->tag = EXPR_LET;
	letx->letx.let_defs = parse_let_exprs();
	letx->letx.let_value = parse_expr();
	return letx;
}
list_t* parse_names(void) {
	list_t* names = NULL;
	while (token.type == T_NAME) {
		char* first = strdup(token.s);
		next();
		list_t* rest = parse_names();
		names = cons(first, rest);
	}
	return names;
}
result_t *parse_def(void) {
	result_t *r = NULL;
	result_t *rname = NULL;
	list_t* rargs = NULL;
	expr_t *rrhs = NULL;

	if (token.type != T_NAME)
		return NULL;
	rname = NEW(result_t);
	rname->data = strdup(token.s);

	next();

	rargs = parse_names();

	assert(token.type == T_EQUALS);
	next();

	if (token.type == T_LET)
		rrhs = parse_let();
	else
		rrhs = parse_expr();

	if (!rrhs)
		return NULL;
	assert(token.type == T_SEMI || token.type == T_EOF);
	next();

	r = NEW(result_t);
	def_t *def = NEW(def_t);
	def->name = rname->data;
	def->args = rargs;
	def->rhs = rrhs;
	r->data = def;

	return r;
}

result_t *parse_defs()
{
    result_t *r =  NULL;
    result_t *rdef =  NULL;
    list_t *defs = NULL;
    list_t *last = NULL;

    for (rdef = parse_def(); rdef!=NULL; rdef=parse_def()) {
        if(last) {
            last = last->next = cons(rdef->data, NULL);
        } else {
            last = defs = cons(rdef->data, NULL);
        }
    }

    r = NEW(result_t);
    r->data = defs;

    return r;
}

// debug output

void indent(int col)
{
    int i;
    for(i=0; i<col; i++) putchar(' ');
}

void pprint_expr(int col, const expr_t *e)
{
	const list_t* p;
    switch(e->tag) {
    case EXPR_VAR:
        printf("VAR %s\n", e->var);
        break;
    case EXPR_APP:
        printf("APP ");
        pprint_expr(col+4, e->app.fun);
        indent(col+4);
        pprint_expr(col+4, e->app.arg);
        break;
    case EXPR_STR:
        printf("STR %s\n", e->str);
        break;
    case EXPR_NUM:
        printf("NUM %d\n", e->num);
        break;
	case EXPR_OPER: {
		tkn t;
		t.type = e->op;
		printf("OPER %s\n", token_to_string(&t));
		break;
	}
	case EXPR_LET: {
		printf("LET\n");
		for (p = e->letx.let_defs; p; p=p->next)
		{
			expr_let_def* def = p->data;
			indent(col);
			if (def)
			{
				printf("%s=",def->name);
				pprint_expr(col+4,def->expr);
			}
			else
				printf("-\n");

		}
		indent(col);
		printf("IN ");
		//indent(col+4);
		pprint_expr(col+4, e->letx.let_value);
		break;
	}
    default:
        printf("?%d?\n", e->tag);
        break;
    }
}

void pprint_def(int col, const def_t *def)
{
    const list_t *p = def->args;

    printf("%s", def->name);
    for(; p; p=p->next) printf(" %s", (char*)p->data);
    printf(" :=\n");
    indent(col+2);
    pprint_expr(col+2, def->rhs);
}

void pprint_defs(int col, const list_t *defs)
{
    for(; defs; defs=defs->next)
        pprint_def(col, defs->data);
}

typedef enum InstructionType {
	Halt, Take, Push, Enter, EnterT, Op, CheckMarkers, Move, PutChar
} InstructionType;
const char* insToString(InstructionType ins) {
	switch (ins) {
	case Halt: return "Halt";
	case Take: return "Take";
	case Push: return "Push";
	case Enter: return "Enter";
	case EnterT: return "EnterT";
	case Op: return "Op";
	case CheckMarkers: return "CheckMarkers";
	case Move: return "Move";
	case PutChar: return "PutChar";
	}
	return "inxxx";
}
typedef enum AddressModeMode {
	Arg, Label, Super, Num, List, Marker
} AddressModeMode;
const char* amToString(AddressModeMode am) {
	switch (am) {
	case Arg: return "Arg";
	case Label: return "Label";
	case Super: return "Super";
	case Num: return "Num";
	case List: return "List";
	case Marker: return "Marker";
	}
	return "amxxx";
}
typedef enum OpType {
	Add, Sub, Mul, Div, Mod, Lt
} OpType;
const char* opToString(OpType op) {
	switch (op) {
	case Add: return "Add";
	case Sub: return "Sub";
	case Mul: return "Mul";
	case Div: return "Div";
	case Mod: return "Mod";
	case Lt: return "Lt";
	}
	return "opxxx";
}
typedef struct CodeArray CodeArray;
typedef struct {
	AddressModeMode mode;
	union {
		ptrdiff_t address;
		CodeArray* code;
		unsigned arg;
	} params;
} AddressMode;
typedef struct Instruction {
	InstructionType ins;//ins:3;//unsigned ins:3;
	union {
		AddressMode addr;
		OpType op;
	};
	unsigned take;
	unsigned frameSize;
} Instruction;
const char* instructionToString(Instruction* ins) {
	static char text[50];
	text[0] = 0;
	switch (ins->ins) {
	case Halt: return insToString(ins->ins);
	case CheckMarkers: snprintf(text, 50, "%s %d", insToString(ins->ins), ins->take); break;
	case Take: snprintf(text, 50, "%s %d %d", insToString(ins->ins), ins->take, ins->frameSize); break;
	case Push:
	case Enter:
	case EnterT:
		switch (ins->addr.mode) {
		case Arg: snprintf(text, 50, "%s %s %d", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.arg); break;
		case Label: snprintf(text, 50, "%s %s %ld", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.address); break;
		case Super: snprintf(text, 50, "%s %s %ld", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.address); break;
		case Num: snprintf(text, 50, "%s %s %ld", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.address); break;
		case List: snprintf(text, 50, "%s %s %ld", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.address); break;
		case Marker: snprintf(text, 50, "%s %s %d", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.arg); break;
		}
		break;
	case Op: snprintf(text, 50, "%s %s", insToString(ins->ins), opToString(ins->op)); break;
	case PutChar: snprintf(text, 50, "%s", insToString(ins->ins)); break;
	case Move:
		switch (ins->addr.mode) {
		case Arg: snprintf(text, 50, "%s %s %d %d", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.arg, ins->take); break;
		case Label: snprintf(text, 50, "%s %s %ld %d", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.address, ins->take); break;
		case Super: snprintf(text, 50, "%s %s %ld %d", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.address, ins->take); break;
		case Num: snprintf(text, 50, "%s %s %ld %d", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.address, ins->take); break;
		case List: snprintf(text, 50, "%s %s %ld %d", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.address, ins->take); break;
		case Marker: snprintf(text, 50, "%s %s %d %d", insToString(ins->ins), amToString(ins->addr.mode), ins->addr.params.arg, ins->take); break;
		}
		break;
	}
	return text;
}
typedef struct CodeArray {
	Instruction* code;
	unsigned code_size;
	unsigned code_capy;
} CodeArray;
static
void addInstruction(CodeArray* code, Instruction n) {
	if (code->code_size+2 > code->code_capy) {
		code->code_capy = 2+(code->code_capy*11)/10;
		code->code = realloc(code->code, code->code_capy*sizeof(Instruction));
	}
	code->code[code->code_size++] = n;
}
static
void append(CodeArray* code, CodeArray* target) {
	// Amenable to an obvious optimization in the future.
	for (unsigned i=0; i<target->code_size; ++i) {
		addInstruction(code, target->code[i]);
	}
}
static
void haltInstruction(CodeArray* code) {
	Instruction n;
	n.ins = Halt;
	addInstruction(code, n);
}
static
void checkMarkersInstruction(CodeArray* code, unsigned take) {
	Instruction n;
	n.ins = CheckMarkers;
	n.take = take;
	addInstruction(code, n);
}
static
void takeInstruction(CodeArray* code, unsigned take, unsigned frame) {
	Instruction n;
	n.ins = Take;
	n.take = take;
	n.frameSize = frame;
	addInstruction(code, n);
}
static
void pushInstruction(CodeArray* code, AddressMode mode) {
	Instruction n;
	n.ins = Push;
	n.addr = mode;
	//n.addr.address = mode.params.address;
	//n.addr.arg = mode.params.arg;
	addInstruction(code, n);
}
static
void pushLabelInstruction(CodeArray* code, ptrdiff_t label) {
	Instruction n;
	n.ins = Push;
	n.addr.mode = Label;
	n.addr.params.address = label;
	addInstruction(code, n);
}
static
void enterInstruction(CodeArray* code, AddressMode mode) {
	Instruction n;
	n.ins = Enter;
	n.addr = mode;
	addInstruction(code, n);
}
static
void enterArgInstruction(CodeArray* code, unsigned arg) {
	AddressMode mode;
	mode.mode = Arg;
	mode.params.arg = arg;
	enterInstruction(code, mode);
}
static
void enterNumInstruction(CodeArray* code, ptrdiff_t num) {
	Instruction n;
	n.ins = Enter;
	n.addr.mode = Num;
	n.addr.params.address = num;
	addInstruction(code, n);
}
static
void enterTInstruction(CodeArray* code, AddressMode am) {
	Instruction n;
	n.ins = EnterT;
	n.addr = am;
	addInstruction(code, n);
}
static
void opInstruction(CodeArray* code, OpType op) {
	Instruction n;
	n.ins = Op;
	n.op = op;
	addInstruction(code, n);
}
struct env_t {
	const char* name;
	int args;
	AddressMode mode;
};
static
list_t* envAddArgs(list_t* env, const list_t* args) {
	int kArg = 0;
	for (; args; args=args->next) {
		char* arg = args->data;
        struct env_t* entry = NEW(struct env_t);
        entry->name = arg;
        entry->mode.mode = Label;
        entry->mode.params.address = 2*kArg;
        kArg++;
        env = cons(entry, env);
	}
    //printf("envAddArgs "); pprint_env(env);
	return env;
}
static
list_t* envAddVar(list_t* env, const char* name, AddressMode mode) {
	struct env_t* entry = NEW(struct env_t);
	entry->name = name;
	entry->mode = mode;
	env = cons(entry, env);
    //printf("%s ", __FUNCTION__); pprint_env(env);
	return env;
}
static
list_t* envDup(list_t* env) {
	list_t* dup = NULL;
	for (; env; env = env->next) {
		void* payload = NEW(struct env_t);
		memcpy(payload, env->data, sizeof(struct env_t));
		dup = cons(payload, dup);
	}
    //printf("envDup "); pprint_env(dup);
	return dup;
}
static
bool find_mode(list_t* env, expr_var_t var, AddressMode* mode) {
    //printf("find_mode "); pprint_env(env);
	for (; env; env = env->next) {
		struct env_t* entry = env->data;
		if (strcmp(entry->name, var)==0) {
			*mode = entry->mode;
			return true;
		}
	}
	return false;
}

typedef struct {
	CodeArray* code;
	int d;
} code_regs_t;
typedef struct {
	AddressMode am;
	int d;
} LetAddressMode;
static
LetAddressMode compileAExp(const expr_t* exp, int d, list_t* env);
static
code_regs_t compileRExp(code_regs_t code, int d, const expr_t* exp, list_t* env);
static
LetAddressMode compileAExp(const expr_t* exp, int d, list_t* env) {
	LetAddressMode m;
	switch (exp->tag) {
	default: {
		//return compileAApp(exp->app, env);
		code_regs_t code;
		code.d = 0;
		code.code = NEW(CodeArray); code.code->code=0; code.code->code_capy=0; code.code->code_size=0;
		code = compileRExp(code, d, exp, env);
		m.am.mode = List;
		m.am.params.code = code.code;
		m.d = code.d;
		break;
	}
	case EXPR_NUM: {
		m.am.mode = Num;
		m.am.params.address = exp->num;
		m.d = d;
		break;
	}
	case EXPR_VAR: {
		m.d = d;
		if (!find_mode(env, exp->var, &m.am)) {
			m.am.mode = Super;
			m.am.params.address = 9999;
		}
		break;
	}
	}
	printf("%s: mode %s\n", __FUNCTION__, amToString(m.am.mode));
	return m;
}
typedef struct {
	const char* name;
	ptrdiff_t ref;
} forward_t;
list_t* forwards = 0;
void fixForwardReferences(CodeArray* code, list_t* env) {
	for (list_t* forwardP = forwards; forwardP != NULL; forwardP = forwardP->next) {
		forward_t* forward = forwardP->data;
		AddressMode mode;
		if (!find_mode(env, forward->name, &mode)) {
			printf("Definition of %s still missing\n", forward->name);
		} else {
			if (code->code[forward->ref].addr.params.address == 9999) {
				printf("Found %s %ld fixing %ld\n", forward->name, mode.params.address, forward->ref);
				code->code[forward->ref].addr.params.address = mode.params.address;
			}
		}
	}
}
void adjustForwardReferences(CodeArray* code, ptrdiff_t newStart) {
	for (list_t* forwardP = forwards; forwardP != NULL; forwardP = forwardP->next) {
		forward_t* forward = forwardP->data;
		if (forward->ref < code->code_size) {
			forward->ref += newStart;
		}
	}
}
static
code_regs_t compileRVar(code_regs_t code, int d, expr_var_t var, list_t* env) {
	AddressMode mode;
	if (!find_mode(env, var, &mode)) {
		printf("Missing definition of a variable %s\n", var);
		mode.mode = Super;
		mode.params.address = 9999;
		forward_t* forward = NEW(forward_t);
		forward->name = strdup(var);
		forward->ref = code.code->code_size;
		forwards = cons(forward, forwards);
	}
	enterInstruction(code.code, mode);
	code.d = d;
	return code;
}
static
code_regs_t compileRNum(code_regs_t code, int d, expr_num_t num, list_t* env) {
	(void)env;
	enterNumInstruction(code.code, num);
	code.d = d;
	return code;
}
static
code_regs_t compileRApp(code_regs_t code, int d, expr_app_t app, list_t* env) {
	// compileR (EAp e1 e2) env = Push (compileA e2 env) : compileR e1 env
	LetAddressMode toPush = compileAExp(app.arg, d, env);
	pushInstruction(code.code, toPush.am);
	return compileRExp(code, toPush.d, app.fun, env);
}
static
code_regs_t compileROper(code_regs_t code, int d, const expr_oper_t op, list_t* env) {
	AddressMode mode;
	bool found;
	switch (op) {
	case T_ADD: found = find_mode(env, "+", &mode); break;
	case T_SUB: found = find_mode(env, "-", &mode); break;
	case T_MUL: found = find_mode(env, "*", &mode); break;
	case T_DIV: found = find_mode(env, "/", &mode); break;
	case T_MOD: found = find_mode(env, "%", &mode); break;
	case T_LT: found = find_mode(env, "<", &mode); break;
	default:
		puts("Unhandled operator in compileROper"); exit(1); break;
	}
	if (!found) {
		printf("Missing definition of an operator\n");
		mode.mode = Super;
		mode.params.address = 9999;
	}
	enterInstruction(code.code, mode);
	code.d = d;
	return code;
}
static
code_regs_t compileRLet(code_regs_t code, int d, const expr_let_t exp, list_t* env) {
	//LetAddressMode modes[length(exp.let_defs)];
	int vars = length(exp.let_defs);
	list_t* def = exp.let_defs;
	int slot = 0;
	list_t* innerEnv = envDup(env);
	for (; def; def = def->next)
	{
		expr_let_def* let_st = def->data;
		LetAddressMode am = compileAExp(let_st->expr, d+vars, env);
		printf("Slot %d address mode %s\n", slot, amToString(am.am.mode));
		Instruction inst;
		inst.ins = Move;
		inst.addr = am.am;
		inst.take = slot+vars;
		innerEnv = envAddVar(innerEnv, let_st->name, inst.addr);
		printf("Slot %d address mode %s\n", slot, amToString(inst.addr.mode));
		printf("Slot %d instruction %s\n", slot, instructionToString(&inst));

		addInstruction(code.code, inst);
		slot++;
	}

	printf("Compiling inner expression at %u\n", code.code->code_size);
	compileRExp(code, d+vars, exp.let_value, innerEnv);
	code.d = d+vars;
    //haltInstruction(code.code);
	return code;
}
static
code_regs_t compileRExp(code_regs_t code, int d, const expr_t* exp, list_t* env) {
	switch (exp->tag)
	{
	case EXPR_APP: {
		return compileRApp(code, d, exp->app, env);
	}
	case EXPR_NUM: {
		return compileRNum(code, d, exp->num, env);
	}
	case EXPR_VAR: {
		return compileRVar(code, d, exp->var, env);
	}
	case EXPR_OPER: {
		return compileROper(code, d, exp->op, env);
	}
	case EXPR_STR: {
		puts("EXPR_STR in COMPILE_R_EXP");
		exit(1);
	}
	case EXPR_LET: {
		puts("EXPR_LET in COMPILE_R_EXP");
		return compileRLet(code, d, exp->letx, env);
		exit(1);
	}
	}
	return code;
}
static
unsigned countLocalsExp(const expr_t* exp)
{
	switch (exp->tag)
	{
	case EXPR_APP: {
		return 0;// FIXME compileRApp(code, d, exp->app, env);
	}
	case EXPR_NUM: {
		return 0;
	}
	case EXPR_VAR: {
		return 0;
	}
	case EXPR_OPER: {
		return 0;
	}
	case EXPR_STR: {
		return 0;
	}
	case EXPR_LET: {
		unsigned vars = length(exp->letx.let_defs) + countLocalsExp(exp->letx.let_value);
		return vars;
	}
	}
	return 9999;
}
static
unsigned countLocals(def_t* def)
{
	return countLocalsExp(def->rhs);
}
static
CodeArray* compileDef(CodeArray* code, def_t* def, list_t* env) {
	//def->args;
	//def->name;
	//def->rhs; // expr_t
	unsigned nArgs = length(def->args);
	unsigned nLocals = countLocals(def);
	int takeAddr = -1;
	if (nArgs + nLocals) {
		if (nArgs) {
			for (unsigned i=0; i<nArgs-1; ++i) {
				pushLabelInstruction(code, 2*(nArgs-i));
			}
		}
		checkMarkersInstruction(code, nArgs);
		takeAddr = code->code_size;
		takeInstruction(code, nArgs, nArgs+nLocals);
	}
	list_t* new_env = envDup(env);
	new_env = envAddArgs(new_env, def->args);
	code_regs_t cr; cr.code = code; cr.d = 0;
	code_regs_t cr2 = compileRExp(cr, 0, def->rhs, new_env);
	printf("%s args %d original locals %d locals now %d\n", def->name, nArgs, nLocals, cr2.d);
	code->code[takeAddr].frameSize = nArgs + cr2.d;
	for (unsigned i=0; i<code->code_size; ++i) {
		switch (code->code[i].ins) {
		default:
			break;
		case Enter:
		case Push:
		case Move:
			if (code->code[i].addr.mode == List) {
				printf("Found a List at %d appending it at %d\n", i, code->code_size);
				// Capture the code that needs to be appended to the linear sequence
				CodeArray* target = code->code[i].addr.params.code;
				adjustForwardReferences(target, code->code_size);
				// Convert the current instruction to a jump to the appended code.
				code->code[i].addr.mode = Label;
				code->code[i].addr.params.address = code->code_size;
				//fixForwardReferences(target, new_env);
				append(code, target);
			}
			break;
		}
	}
	//code = append(code, code);
	return code;
}
enum {
	SELF = -1,
};
static ptrdiff_t MARKER_PC = -2;

struct Frame;
struct Closure {
	ptrdiff_t pc;
	union {
	struct Frame* framePtr; //struct Closure* frame;
	ptrdiff_t value;
	};
};
struct Frame {
	int frameElements;
	struct Closure* closures;
};
list_t* stack;
struct Closure state;
char* pcToString(ptrdiff_t pc)
{
	static char text[50];
	if (pc >= 0)
	{
		sprintf(text, "%ld", pc);
	}
	else if (pc == -1)
	{
		sprintf(text, "SELF");
	}
	else if (pc <= MARKER_PC)
	{
		sprintf(text, "MARKER-%ld", MARKER_PC-pc);
	}
	return text;
}
void showSomeFrameElts(struct Frame* frame, int elts) {
	if (frame) {
		for (int i=0; i<elts; ++i) {
			printf("%s|%p ", pcToString(frame->closures[i].pc), frame->closures[i].framePtr);
		}
		puts("");
	}
}
void showSomeFrame(struct Frame* frame) {
	showSomeFrameElts(frame, frame->frameElements);
}
void showMainFrameElts(int elts) {
	if (state.pc == SELF)
	{
		printf("%s|%lx\n", pcToString(state.pc), state.value);
	}
	else
	{
		showSomeFrameElts(state.framePtr, elts);
	}
}
void showMainFrame() {
	if (state.pc != SELF && state.framePtr)
	showMainFrameElts(state.framePtr->frameElements);
}
void printStack()
{
	for (list_t* sp = stack; sp; sp = sp->next) {
		struct Closure* closure = sp->data;
		if (closure->pc == -1)
			printf(" %s|%ld", pcToString(closure->pc), closure->value);
		else
			printf(" %s|%p", pcToString(closure->pc), closure->framePtr);
	}
	printf("%s", "\n");
}
struct Frame* NEWFRAME(int n) {
	struct Frame* nf = NEW(struct Frame);
	nf->frameElements = n;
	nf->closures = NEWA(struct Closure, n);
	return nf;
}
void stepTake(const Instruction* ins) {
	state.framePtr = NEWFRAME(ins->frameSize);
	for (unsigned i=0; i<ins->take; ++i) {
		state.framePtr->closures[i] = *(struct Closure*)head(stack);
		stack = tail(stack);
	}
	printf("New Frame: F:");
	showMainFrameElts(ins->frameSize);
}
void stepPush(const Instruction* ins) {
	struct Closure* toPush = NULL;
	switch (ins->addr.mode) {
	case Num:
		toPush = NEW(struct Closure);
		toPush->value = ins->addr.params.address;
		toPush->pc = SELF;
		break;
	case Arg:
		if (state.framePtr == 0)
		{
			assert(!"Frame should not be NULL when pushing arg");
		}
		toPush = NEW(struct Closure);
		*toPush = state.framePtr->closures[ins->addr.params.arg];
		break;
	case Label:
		if (state.framePtr == 0)
		{
			//assert(!"Frame should not be NULL when pushing label");
			// frame can be zero if main just returns an expression with no
			// lazy supercombinator called.
			// A heuristic could be, if a Take has ever been done,
			// there should be a frame available for a push-label.
			// Prior to a take, there would never be a frame.
		}
		toPush = NEW(struct Closure);
		toPush->pc = ins->addr.params.address;
		toPush->framePtr = state.framePtr;
		break;
	case Super:
		toPush = NEW(struct Closure);
		toPush->pc = ins->addr.params.address;
		toPush->framePtr = NULL;
		break;
	case Marker:
		if (state.framePtr == 0)
		{
			assert(!"Frame should not be NULL when pushing marker");
		}
		toPush = NEW(struct Closure);
		toPush->pc = MARKER_PC - ins->addr.params.arg;
		toPush->framePtr = state.framePtr;
		break;
	default:
		puts("Invalid push instruction"); exit(1); break;
	}
	stack = cons(toPush, stack);
}
void stepEnter(const Instruction* ins) {
	struct Closure* closure;
	//struct Closure* toPush;
	struct Closure* arg;
	switch (ins->addr.mode) {
	case Num:
		state.pc = SELF;
		state.value = ins->addr.params.address;
		printf("F:"); showMainFrame();
		break;
	case Arg:
		printf("Entering Arg %u: %s\n", ins->addr.params.arg, pcToString(state.framePtr->closures[ins->addr.params.arg].pc));
		printf("F:"); showMainFrame();
		arg = &state.framePtr->closures[ins->addr.params.arg];
		if (arg->pc == SELF)
		{
			closure = head(stack);
			stack = tail(stack);
			if (closure->pc <= MARKER_PC)
			{
				int slot = MARKER_PC - closure->pc;
				// Put SELF,value in the slot of the frame that the closure has.
				closure->framePtr->closures[slot].pc = arg->pc;
				closure->framePtr->closures[slot].value = arg->value;
				state.pc = closure->framePtr->closures[slot].pc;
				state.framePtr = closure->framePtr->closures[slot].framePtr;
			}
			else
			{
				//toPush = NEW(struct Closure);
				//toPush->value = ins->addr.params.address;
				//toPush->pc = SELF;
				//state.pc = SELF;
				//state.value = ins->addr.params.address;
				stack = cons(arg, stack);
				state.pc = closure->pc;
				state.framePtr = closure->framePtr;
				printf("F:"); showMainFrame();
			}
		}
		else
		{
			if (arg->framePtr == NULL)
				assert("Entering arg whose frame is NULL");
			state = state.framePtr->closures[ins->addr.params.arg];
			if (state.pc >=0 && state.framePtr != 0) {
				printf("Enter w/New Frame: ");
				printf("F:"); showMainFrame();
			}
		}
		break;
	case Label:
		state.pc = ins->addr.params.address;
		// Entering a label directly does not change the frame pointer.
		printf("F:"); showMainFrame();
		break;
	case Super:
		state.pc = ins->addr.params.address;
		// Entering a super directly does not create a frame. Take does.
		break;
	default:
		puts("Invalid enter instruction"); exit(1); break;
	}
}
void stepEnterT(const Instruction* ins) {
	// Pop the top value from stack. If it is 'true' then enter whatever the address is.
	struct Closure* condC = head(stack); stack=tail(stack);
	if (condC->value)
		stepEnter(ins);
}
void stepOp(const Instruction* ins) {
	struct Closure* leftC = head(stack); stack=tail(stack);
	struct Closure* rightC= head(stack); stack=tail(stack);
	state.pc = -1;
	switch (ins->op) {
	case Add: state.value = leftC->value + rightC->value; break;
	case Mul: state.value = leftC->value * rightC->value; break;
	case Sub: state.value = leftC->value - rightC->value; break;
	case Div: state.value = leftC->value / rightC->value; break;
	case Mod: state.value = leftC->value % rightC->value; break;
	case Lt:  state.value = leftC->value < rightC->value; break;
	}
}
void stepPutChar(const Instruction* ins) {
	(void)ins;
	struct Closure* leftC = head(stack); stack=tail(stack);
	putchar(leftC->value);
	state.value = leftC->value;
	state.pc = SELF;
}
void stepCheckMarkers(const Instruction* ins) {
	list_t* checkStack = stack;
	unsigned iStack=0;
	while (checkStack != 0 && iStack < ins->take) {
		struct Closure* check = head(checkStack); checkStack=tail(checkStack);
		if (check->pc <= MARKER_PC) {
			ptrdiff_t slot = MARKER_PC - check->pc;
			struct Frame* newFramePtr = NEWFRAME(iStack);
			list_t* ws = stack;
			for (unsigned i=0; i<iStack; ++i) {
				newFramePtr->closures[i] = *(struct Closure*)head(ws); ws = tail(ws);
			}
			check->framePtr->closures[slot].framePtr = newFramePtr;
			check->framePtr->closures[slot].pc = state.pc - iStack;
			ws = stack;
			struct Closure* reloc = NEWA(struct Closure, iStack);
			for (unsigned r=0; r<iStack; ++r) {
				reloc[r] = *(struct Closure*)ws->data;
				ws = ws->next;
			}
			stack = ws->next;
			printf("%d: Closures and marker popped S:", __LINE__); printStack();
			for (unsigned r=iStack; r>0; r--) {
				stack = cons(&reloc[r-1], stack);
				printf("%d: Removing marker S:", __LINE__); printStack();
			}
			printf("%d: Removing marker S:", __LINE__); printStack();
			state.pc--;
			return;
		}
		iStack++;
	}
}
void stepMove(Instruction* ins) {
	// This is very much stepPush but with a slightly
	// different way of disposing of 'toPush'
	struct Closure* toMove = NULL;
	switch (ins->addr.mode) {
	case Num:
		toMove = NEW(struct Closure);
		toMove->value = ins->addr.params.address;
		toMove->pc = SELF;
		break;
	case Arg:
		if (state.framePtr == 0)
		{
			assert(!"Frame should not be NULL when pushing arg");
		}
		toMove = NEW(struct Closure);
		*toMove = state.framePtr->closures[ins->addr.params.arg];
		break;
	case Label:
		if (state.framePtr == 0)
		{
			//assert(!"Frame should not be NULL when pushing label");
			// frame can be zero if main just returns an expression with no
			// lazy supercombinator called.
			// A heuristic could be, if a Take has ever been done,
			// there should be a frame available for a push-label.
			// Prior to a take, there would never be a frame.
		}
		toMove = NEW(struct Closure);
		toMove->pc = ins->addr.params.address;
		toMove->framePtr = state.framePtr;
		break;
	case Super:
		toMove = NEW(struct Closure);
		toMove->pc = ins->addr.params.address;
		toMove->framePtr = NULL;
		break;
	case Marker:
		if (state.framePtr == 0)
		{
			assert(!"Frame should not be NULL when pushing marker");
		}
		toMove = NEW(struct Closure);
		toMove->pc = MARKER_PC - ins->addr.params.arg;
		toMove->framePtr = state.framePtr;
		break;
	default:
		puts("Invalid push instruction"); exit(1); break;
	}
	state.framePtr->closures[ins->take] = *toMove;
}
void step(CodeArray* code) {
	//if (state.pc < 0)
	//	return; // We don't run SELF or MARKER yet.
	if (code->code[state.pc].ins == Halt)
		return;
	ptrdiff_t old_pc = state.pc;
	state.pc ++;

	switch (code->code[old_pc].ins) {
	case CheckMarkers:
		stepCheckMarkers(&code->code[old_pc]);
		break;
	case Take:
		stepTake(&code->code[old_pc]);
		break;
	case Push:
		stepPush(&code->code[old_pc]);
		break;
	case Enter:
		stepEnter(&code->code[old_pc]);
		break;
	case EnterT:
		stepEnterT(&code->code[old_pc]);
		break;
	case Op:
		stepOp(&code->code[old_pc]);
		break;
	case Move:
		stepMove(&code->code[old_pc]);
		break;
	case PutChar:
		stepPutChar(&code->code[old_pc]);
		break;
	default:
		break;
	}
	if (state.pc == SELF) {
		struct Closure* mark = head(stack);
		while (mark->pc <= MARKER_PC) {
			struct Frame* fr = mark->framePtr;
			int slot = MARKER_PC - mark->pc;
			fr->closures[slot] = state;
			stack = tail(stack);
			mark = head(stack);
		}
		struct Closure* top = head(stack);
		stack = tail(stack);
		struct Closure* n = NEW(struct Closure);
		n->pc = state.pc;
		n->value = state.value;
		stack = cons(n, stack);
		state = *top;
	}
}
void addOpCombinator(CodeArray* code, OpType op, const char* name, list_t** env) {
	unsigned res = code->code_size;
	takeInstruction(code, 2, 2);
	pushLabelInstruction(code, res+3);
	enterArgInstruction(code, 1);
	pushLabelInstruction(code, res+5);
	enterArgInstruction(code, 0);
	opInstruction(code, op);
	struct env_t* entry = NEW(struct env_t);
	entry->args = 2;
	entry->mode.mode = Super;
	entry->mode.params.address = res;
	entry->name = name;
	*env = cons(entry, *env);
}
void addCcCombinator(CodeArray* code, InstructionType ins, const char* name, list_t** env) {
	unsigned res = code->code_size;
	takeInstruction(code, 1, 1);
	pushLabelInstruction(code, res+3);
	//AddressMode arg0;
	//arg0.mode = Arg;
	//arg0.params.arg = 0;
	//pushInstruction(code, arg0);
	enterArgInstruction(code, 0);
	Instruction instr;
	instr.ins = ins;
	addInstruction(code, instr);//opInstruction(code, op);
	struct env_t* entry = NEW(struct env_t);
	entry->args = 1;
	entry->mode.mode = Super;
	entry->mode.params.address = res;
	entry->name = name;
	*env = cons(entry, *env);
}
void addCondCombinator(CodeArray* code, list_t** env) {
	unsigned res = code->code_size;
	takeInstruction(code, 3, 3);
	pushLabelInstruction(code,res+3);
	enterArgInstruction(code, 0);
	AddressMode am;
	am.mode = Arg;
	am.params.arg = 1;
	enterTInstruction(code, am);
	enterArgInstruction(code, 2);
	struct env_t* entry = NEW(struct env_t);
	entry->args = 3;
	entry->mode.mode = Super;
	entry->mode.params.address = res;
	entry->name = "if";
	*env = cons(entry, *env);
}
void addMarkers(CodeArray* code, list_t* defs) {
	list_t* pdef;
	unsigned max_args = 3;
	for (pdef = defs; pdef; pdef = pdef->next) {
		def_t* def = pdef->data;

		unsigned def_args = length(def->args);
		if (def_args > max_args)
			max_args = def_args;
	}
	AddressMode marker;
	marker.mode = Marker;
	for (unsigned arg = 0; arg < max_args; ++arg) {
		marker.params.arg = arg;
		pushInstruction(code, marker);
		enterArgInstruction(code, arg);
	}
}
int main(int argc, char** argv)
{
    result_t *r;

    // Initialize input
    if (argc>1)
    {
    	fp = fopen(argv[1],"rt");
		if (fp == NULL)
			return -1;
    }
    else {
		fp = stdin;
    }

    nextChar();
	next();
    r = parse_defs();

    if(r == NULL) {
        fprintf(stderr, "no parse\n");
        return 1;
    }

    list_t *defs = r->data;
    pprint_defs(0, defs);
    CodeArray code; memset(&code, 0, sizeof(code));
    list_t* env = NULL;
    addMarkers(&code, defs);
    addOpCombinator(&code, Add, "+", &env);
    addOpCombinator(&code, Mul, "*", &env);
    addOpCombinator(&code, Sub, "-", &env);
    addOpCombinator(&code, Div, "/", &env);
    addOpCombinator(&code, Mod, "%", &env);
    addOpCombinator(&code, Lt,  "<", &env);
    addCcCombinator(&code, PutChar,  "putChar", &env);
    addCondCombinator(&code, &env);
    list_t* pdef;
    for (pdef = defs; pdef; pdef=pdef->next) {
        def_t* def = pdef->data;
		struct env_t* entry = NEW(struct env_t);
		entry->name = def->name;
		entry->args = length(def->args);
		entry->mode.mode = Super;
		// If there are no arguments, the entry point should be
		// the first instruction we will emit.
		// If there are arguments, the entry point should be
		// the check instruction, which is after n-1 push-labels.
		if (entry->args == 0)
			entry->mode.params.address = code.code_size;
		else
			entry->mode.params.address = code.code_size + (entry->args-1);
		env = cons(entry, env);
		compileDef(&code, def, env);
    }
    fixForwardReferences(&code, env); // Go back and fix the forward references
    for (pdef = defs; pdef; pdef=pdef->next) {
        def_t* def = pdef->data;
        printf("%s args %zu locals %u\n", def->name, length(def->args), countLocals(def));
    }
    list_t* penv;
    for (penv = env; penv; penv=penv->next) {
        struct env_t* entry = penv->data;
        ptrdiff_t addr = entry->mode.params.address;
        printf("%s = %ld\n", entry->name, addr);
    }
    unsigned haltat = code.code_size;
    haltInstruction(&code);
    for (unsigned i=0; i<code.code_size; ++i) {
		printf("%d: %s\n", i, instructionToString(&code.code[i]));
    }
    AddressMode start_am;
    if (!find_mode(env, "main", &start_am))
    {
		puts("main not found");
		return 1;
    }
    state.pc = start_am.params.address;
    struct Closure* ret = NEW(struct Closure);
    ret->pc = haltat;
    ret->framePtr = NULL;
	stack = cons(ret, stack);
	while (state.pc >= 0 && code.code[state.pc].ins != Halt) {
		printf("%s: %s\n", pcToString(state.pc), instructionToString(&code.code[state.pc]));
		printf("S:");printStack();
		step(&code);
	}
	if (state.pc < 0)
		printf("Error: PC is Non-executable: %s\n", pcToString(state.pc));
	else
		printf("%s: %s\n", pcToString(state.pc), instructionToString(&code.code[state.pc]));
	printf("F:"); showMainFrame();
	printf("S:"); printStack();


	return 0;
}
