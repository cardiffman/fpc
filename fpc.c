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
} exprtag_t;

typedef char *expr_var_t;

typedef struct {
    const struct expr *fun;
    const struct expr *arg;
} expr_app_t;

typedef char *expr_str_t;

typedef int expr_num_t;

typedef int expr_oper_t;

typedef struct expr {
    exprtag_t tag;
    union {
        expr_var_t var;
        expr_app_t app;
        expr_str_t str;
        expr_num_t num;
        expr_oper_t op;
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
    const char *s;
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
	//T_IF,
	T_EOF
} Token;
typedef struct {
	Token type;
	char* s;
	int value;
} tkn;
tkn token;
char* s;
void next() {
	while (true){
		while (*s == ' ' || *s=='\t' || *s=='\n')
			s++;
		if (*s=='#') {
			s = strchr(s,'\n');
			continue;
		}
		break;
	}
	if (!*s) {
		token.type = T_EOF;
		return;
	}
	switch (*s) {
	case ';': token.type = T_SEMI; s++; return;
	case '(': token.type = T_LPAREN; s++; return;
	case ')': token.type = T_RPAREN; s++; return;
	}
	char* t =s;
	if (isalpha(*s)) {
		while (isalnum(*t) || *t=='_' || *t=='\'')
			++t;
		free(token.s);
		token.s = NEWA(char, t-s+1);
		memcpy(token.s, s, t-s);
		token.s[t-s]=0;
		token.type = T_NAME;
		s = t;
		return;
	}
	if (isdigit(*t)) {
		token.value = *t - '0';
		++t;
		while (isdigit(*t)) {
			token.value = token.value*10 + *t-'0';
			++t;
		}
		token.type = T_NUM;
		s = t;
		return;
	}

	while (*t && !isalnum(*t) && *t!=';' &&*t !=')' && *t!='(' && *t!=' ' && *t!='\t' && *t!='\n' && *t!='#') {
		++t;
	}
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
		token.s = NEWA(char, t-s+1);
		memcpy(token.s, s, t-s);
		token.s[t-s]=0;
	}
	s = t;
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
	default: snprintf(buf, 50, "%d", token->type); break;
	}
	return buf;
}
result_t *parse_expr();

void pprint_expr(int col, const expr_t *e);
result_t* op_apply(Token t, result_t* left, result_t* right) {
    expr_t* op = NEW(expr_t);
    op->tag = EXPR_OPER;
    op->op = t;
    expr_t *innera = NEW(expr_t);
    innera->tag = EXPR_APP;
    innera->app.fun = op;
    innera->app.arg = left->data;
    expr_t* outera = NEW(expr_t);
    outera->tag = EXPR_APP;
    outera->app.fun = innera;
    outera->app.arg = right->data;
    result_t* r = NEW(result_t);
    r->data = outera;
    r->s = right->s;
    return r;
}

result_t* parse_expr6() {
	expr_t* term = NULL;
	while (token.type == T_NAME || token.type == T_NUM || token.type == T_PUNCT || token.type == T_LPAREN) {
		expr_t* arg = NEW(expr_t);
		switch (token.type) {
		case T_NAME: arg->tag = EXPR_VAR; arg->var = strdup(token.s); break;
		case T_NUM: arg->tag = EXPR_NUM; arg->num = token.value; break;
		case T_PUNCT: arg->tag = EXPR_VAR; arg->var = strdup(token.s); break;
		case T_LPAREN: {
				next();
				result_t* inner = parse_expr();
				arg = inner->data;
				assert(token.type == T_RPAREN);
			}
			break;

		default:
			printf("No use for current token %s\n", token_to_string(&token));
			exit(1);
			break;
		}
		if (term == NULL)
			term = arg;
		else {
			expr_t* call = NEW(expr_t);
			call->tag = EXPR_APP;
			call->app.fun = term;
			call->app.arg = arg;
			term = call;
		}
		next();
	}
	result_t* res = NEW(result_t);
	res->data = term;
	res->s = NULL;
	return res;
}
result_t* parse_expr5() {
	result_t* r1 = parse_expr6();
	if (r1) {
		if (token.type == T_MUL || token.type == T_DIV) {
			Token type = token.type;
			next();
			result_t* r2 = parse_expr5();
			return op_apply(type, r1, r2);
		}
	}
	return r1;
}
result_t* parse_expr4() {
	result_t* r1 = parse_expr5();
	if (r1) {
		if (token.type == T_ADD || token.type == T_SUB) {
			Token type = token.type;
			next();
			result_t* r2 = parse_expr4(s);
			return op_apply(type, r1, r2);
		}
	}
	return r1;
}
result_t* parse_expr3() {
	result_t* r1 = parse_expr4();
	result_t* r2;
	Token type;
	if (r1) {
		switch (token.type) {
		case T_LE:
		case T_GE:
		case T_LT:
		case T_GT:
			type = token.type;
			next();
			r2 = parse_expr3(s);
			return op_apply(type, r1, r2);
		default:
			break;
		}
	}
	return r1;
}
result_t* parse_expr2() {
	result_t* r1 = parse_expr3(s);
	if (r1) {
		if (token.type == T_AND) {
			result_t* r2 = parse_expr2();
			return op_apply(token.type, r1, r2);
		}
	}
	return r1;
}
result_t* parse_expr1() {
	result_t* r1 = parse_expr2();

	if (r1) {
		if (token.type == T_OR) {
			result_t* r2 = parse_expr1();
			return op_apply(token.type, r1, r2);
		}
	}
	return r1;
}
result_t *parse_expr()
{
	return parse_expr1();
}

result_t *parse_def(void)
{
    result_t *r = NULL;
    result_t *rname = NULL;
    list_t* rargs = NULL;
    result_t *rrhs = NULL;

    if (token.type != T_NAME)
		return NULL;
    rname = NEW(result_t);
    rname->data = strdup(token.s);

    next();

	while (token.type == T_NAME) {
		//r = NEW(result_t);
		//r->data = token.s;
		rargs = cons(strdup(token.s), rargs);
		next();
    }

    assert(token.type == T_EQUALS);
    next();

    //rrhs = parse_expr_new();
    rrhs = parse_expr();

    if(!rrhs) return NULL;
    assert(token.type == T_SEMI || token.type == T_EOF);
    next();

    r = NEW(result_t);
    r->s = s;
    def_t *def = NEW(def_t);
    def->name = rname->data;
    def->args = rargs;
    def->rhs = rrhs->data;
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
    r->s = NULL;

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
	Halt, Take, Push, Enter, EnterT, Op
} InstructionType;
const char* insToString(InstructionType ins) {
	switch (ins) {
	case Halt: return "Halt";
	case Take: return "Take";
	case Push: return "Push";
	case Enter: return "Enter";
	case EnterT: return "EnterT";
	case Op: return "Op";
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
	Add, Sub, Mul, Div, Lt
} OpType;
const char* opToString(OpType op) {
	switch (op) {
	case Add: return "Add";
	case Sub: return "Sub";
	case Mul: return "Mul";
	case Div: return "Div";
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
	unsigned ins:3;
	union {
		AddressMode addr;
		unsigned take;
		OpType op;
	};
} Instruction;
const char* instructionToString(Instruction* ins) {
	static char text[50];
	text[0] = 0;
	switch (ins->ins) {
	case Halt: return insToString(ins->ins);
	case Take: snprintf(text, 50, "%s %d", insToString(ins->ins), ins->take); break;
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
	for (int i=0; i<target->code_size; ++i) {
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
void takeInstruction(CodeArray* code, unsigned take) {
	Instruction n;
	n.ins = Take;
	n.take = take;
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
#if 0
static
void pushArgInstruction(CodeArray* code, unsigned arg) {
	AddressMode mode;
	mode.mode = Arg;
	mode.params.arg = arg;
	pushInstruction(code, mode);
}
#endif
static
void pushLabelInstruction(CodeArray* code, ptrdiff_t label) {
	Instruction n;
	n.ins = Push;
	n.addr.mode = Label;
	n.addr.params.address = label;
	addInstruction(code, n);
}
#if 0
static
void pushSuperInstruction(CodeArray* code, ptrdiff_t label) {
	Instruction n;
	n.ins = Push;
	n.addr.mode = Super;
	n.addr.params.address = label;
	addInstruction(code, n);
}
#endif
static
void pushNumInstruction(CodeArray* code, ptrdiff_t num) {
	Instruction n;
	n.ins = Push;
	n.addr.mode = Num;
	n.addr.params.address = num;
	addInstruction(code, n);
}
#if 0
static
void pushCodeInstruction(CodeArray* code, CodeArray* dest) {
	Instruction n;
	n.ins = Push;
	n.addr.mode = List;
	n.addr.params.code = dest;
	addInstruction(code, n);
}
#endif
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
#if 0
static
void enterLabelInstruction(CodeArray* code, ptrdiff_t label) {
	Instruction n;
	n.ins = Enter;
	n.addr.mode = Label;
	n.addr.params.address = label;
	addInstruction(code, n);
}
static
void enterSuperInstruction(CodeArray* code, ptrdiff_t label) {
	Instruction n;
	n.ins = Enter;
	n.addr.mode = Super;
	n.addr.params.address = label;
	addInstruction(code, n);
}
#endif
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
#if 0
static
void pprint_env(list_t* env) {
	for (; env; env = env->next) {
		struct env_t* entry = env->data;
		printf("env %p entry %p", env, entry); fflush(stdout);
		printf(" %s args %d mode %s\n", entry->name, entry->args, amToString(entry->mode.mode));
	}
}
#endif
static
list_t* envAddArgs(list_t* env, const list_t* args) {
	int kArg = 0;
	for (; args; args=args->next) {
		char* arg = args->data;
        struct env_t* entry = NEW(struct env_t);
        entry->name = arg;
#if 1
        entry->mode.mode = Label;
        entry->mode.params.address = 2*kArg;
        kArg++;
#else
        entry->mode.mode = Arg;
        entry->mode.params.arg = kArg++;
#endif
        env = cons(entry, env);
	}
    //printf("envAddArgs "); pprint_env(env);
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

static
CodeArray* compilePushVar(CodeArray* code, expr_var_t var, list_t* env) {
	AddressMode mode;
	if (find_mode(env, var, &mode)) {
		pushInstruction(code, mode);
	}
	return code;
}
static
CodeArray* compilePushNum(CodeArray* code, expr_num_t num, list_t* env) {
	pushNumInstruction(code, num);
	return code;
}
static
CodeArray* compileEnterExp(CodeArray* code, const expr_t* exp, list_t* env);
static
CodeArray* compilePushExp(CodeArray* code, const expr_t* exp, list_t* env);
static
CodeArray* compilePushApp(CodeArray* code, expr_app_t app, list_t* env) {
	code = compilePushExp(code, app.arg, env);
	code = compileEnterExp(code, app.fun, env);
	printf("Review %s in this scenario:\n", __FUNCTION__);
	expr_t fake;
	fake.tag = EXPR_APP;
	fake.app = app;
	pprint_expr(0, &fake);
	printf("%s","\n");
	return code;
}
static
CodeArray* compilePushExp(CodeArray* code, const expr_t* exp, list_t* env) {
	switch (exp->tag)
	{
	case EXPR_APP: return compilePushApp(code, exp->app, env);
	case EXPR_NUM: return compilePushNum(code, exp->num, env);
	case EXPR_VAR: return compilePushVar(code, exp->var, env);
	case EXPR_OPER: puts("EXPR_OPER not handled in compilePushExp"); exit(1); return NULL;
	case EXPR_STR: puts("EXPR_STR not handled in compilePushExp"); exit(1); return NULL;
	}
	return code;
}
static
CodeArray* compileEnterVar(CodeArray* code, expr_var_t var, list_t* env) {
	AddressMode mode;
	if (find_mode(env, var, &mode)) {
		enterInstruction(code, mode);
	}
	return code;
}
static
CodeArray* compileEnterNum(CodeArray* code, expr_num_t num, list_t* env) {
	enterNumInstruction(code, num);
	return code;
}
static
CodeArray* compileEnterExp(CodeArray* code, const expr_t* exp, list_t* env);
static
CodeArray* compileEnterApp(CodeArray* code, expr_app_t app, list_t* env) {
	code = compilePushExp(code, app.arg, env);
	code = compileEnterExp(code, app.fun, env);
	return code;
}
static
CodeArray* compileEnterExp(CodeArray* code, const expr_t* exp, list_t* env) {
	switch (exp->tag)
	{
	case EXPR_APP: return compileEnterApp(code, exp->app, env);
	case EXPR_NUM: return compileEnterNum(code, exp->num, env);
	case EXPR_VAR: return compileEnterVar(code, exp->var, env);
	case EXPR_OPER: puts("EXPR_OPER not handled in compileEnterExp"); exit(1); return NULL;
	case EXPR_STR: puts("EXPR_STR not handled in compileEnterExp"); exit(1); return NULL;
	}
	return code;
}
static
AddressMode compileAExp(const expr_t* exp, list_t* env);
static
CodeArray* compileRExp(CodeArray* code, const expr_t* exp, list_t* env);
static
AddressMode compileAExp(const expr_t* exp, list_t* env) {
	AddressMode m;
	switch (exp->tag) {
	default: {
		//return compileAApp(exp->app, env);
		CodeArray* code = NEW(CodeArray); code->code=0; code->code_capy=0; code->code_size=0;
		code = compileRExp(code, exp, env);
		m.mode = List;
		m.params.code = code;
		break;
	}
	case EXPR_NUM: {
		m.mode = Num;
		m.params.address = exp->num;
		break;
	}
	case EXPR_VAR: {
		if (!find_mode(env, exp->var, &m)) {
			m.mode = Super;
			m.params.address = 9999;
		}
		break;
	}
	}
	return m;
}
static
CodeArray* compileRVar(CodeArray* code, expr_var_t var, list_t* env) {
	AddressMode mode;
	if (!find_mode(env, var, &mode)) {
		printf("Missing definition of a variable %s\n", var);
		mode.mode = Super;
		mode.params.address = 9999;
	}
	enterInstruction(code, mode);
	return code;
}
static
CodeArray* compileRNum(CodeArray* code, expr_num_t num, list_t* env) {
	return compileEnterNum(code, num, env);
}
static
CodeArray* compileRApp(CodeArray* code, expr_app_t app, list_t* env) {
	// compileR (EAp e1 e2) env = Push (compileA e2 env) : compileR e1 env
	AddressMode toPush = compileAExp(app.arg, env);
	pushInstruction(code, toPush);
	code = compileRExp(code, app.fun, env);
	//AddressMode e1;
	//e1.mode = List;
	//e1.params.code = code;
	//return e1;
	return code;
}
static
CodeArray* compileROper(CodeArray* code, const expr_oper_t op, list_t* env) {
	AddressMode mode;
	bool found;
	switch (op) {
	case T_ADD: found = find_mode(env, "+", &mode); break;
	case T_SUB: found = find_mode(env, "-", &mode); break;
	case T_MUL: found = find_mode(env, "*", &mode); break;
	case T_DIV: found = find_mode(env, "/", &mode); break;
	case T_LT: found = find_mode(env, "<", &mode); break;
	default:
		puts("Unhandled operator in compileROper"); exit(1); break;
	}
	if (!found) {
		printf("Missing definition of an operator\n");
		mode.mode = Super;
		mode.params.address = 9999;
	}
	enterInstruction(code, mode);
	return code;
}
static
CodeArray* compileRExp(CodeArray* code, const expr_t* exp, list_t* env) {
	switch (exp->tag)
	{
	case EXPR_APP: {
		return compileRApp(code, exp->app, env);
	}
	case EXPR_NUM: {
		return compileRNum(code, exp->num, env);
	}
	case EXPR_VAR: {
		return compileRVar(code, exp->var, env);
	}
	case EXPR_OPER: {
		return compileROper(code, exp->op, env);
	}
	case EXPR_STR: {
		puts("EXPR_STR in COMPILE_R_EXP");
		exit(1);
	}
	}
	return code;
}
static
CodeArray* compileDef(CodeArray* code, def_t* def, list_t* env) {
	//def->args;
	//def->name;
	//def->rhs; // expr_t
	takeInstruction(code, length(def->args));
	list_t* new_env = envDup(env);
	new_env = envAddArgs(new_env, def->args);
	compileRExp(code, def->rhs, new_env);
	for (int i=0; i<code->code_size; ++i) {
		switch (code->code[i].ins) {
		default:
			break;
		case Enter:
		case Push:
			if (code->code[i].addr.mode == List) {
				printf("Found a List at %d appending it at %d\n", i, code->code_size);
				// Capture the code that needs to be appended to the linear sequence
				CodeArray* target = code->code[i].addr.params.code;
				// Convert the current instruction to a jump to the appended code.
				code->code[i].addr.mode = Label;
				code->code[i].addr.params.address = code->code_size;
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
static ptrdiff_t MARKER = -2;

struct Closure {
	ptrdiff_t pc;
	union {
	struct Closure* frame;
	ptrdiff_t value;
	};
};
list_t* stack;
struct Closure state;
void stepTake(const Instruction* ins) {
	state.frame = NEWA(struct Closure, ins->take);
	for (int i=0; i<ins->take; ++i) {
		state.frame[i] = *(struct Closure*)head(stack);
		stack = tail(stack);
	}
	printf("New Frame: ");
	for (int i=0; i<ins->take; ++i) {
		printf("%ld|%p ", state.frame[i].pc, state.frame[i].frame);
	}
	puts("");
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
		toPush = NEW(struct Closure);
		*toPush = state.frame[ins->addr.params.arg];
		break;
	case Label:
		toPush = NEW(struct Closure);
		toPush->pc = ins->addr.params.address;
		toPush->frame = state.frame;
		break;
	case Super:
		toPush = NEW(struct Closure);
		toPush->pc = ins->addr.params.address;
		toPush->frame = NULL;
		break;
	case Marker:
		toPush = NEW(struct Closure);
		toPush->pc = MARKER - ins->addr.params.arg;
		toPush->frame = state.frame;
		break;
	default:
		puts("Invalid push instruction"); exit(1); break;
	}
	stack = cons(toPush, stack);
}
void stepEnter(const Instruction* ins) {
	switch (ins->addr.mode) {
	case Num:
		state.pc = SELF;
		state.value = ins->addr.params.address;
		break;
	case Arg:
		state = state.frame[ins->addr.params.arg];
		break;
	case Label:
		state.pc = ins->addr.params.address;
		break;
	case Super:
		state.pc = ins->addr.params.address;
		break;
	default:
		puts("Invalid enter instruction"); exit(1); break;
	}
}
void stepEnterT(const Instruction* ins) {
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
	case Lt:  state.value = leftC->value < rightC->value; break;
	}
}
void step(CodeArray* code) {
	if (state.pc < 0)
		return; // We don't run SELF or MARKER yet.
	if (code->code[state.pc].ins == Halt)
		return;
	ptrdiff_t old_pc = state.pc;
	state.pc ++;

	switch (code->code[old_pc].ins) {
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
	}
	if (state.pc == SELF) {
		struct Closure* mark = head(stack);
		while (mark->pc <= MARKER) {
			struct Closure* fr = mark->frame;
			int slot = MARKER - mark->pc;
			fr[slot] = state;
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
	takeInstruction(code, 2);
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
void addCondCombinator(CodeArray* code, list_t** env) {
	unsigned res = code->code_size;
	takeInstruction(code, 3);
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
	entry->name = "cond";
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
    char *buf = malloc(BUFSIZE);
    FILE* fp;
    size_t len;
    result_t *r;

    // slurp all input
    if (argc>1)
    {
    	fp = fopen(argv[1],"rt");
    	len = fread(buf, 1, BUFSIZE-1, fp);
    }
    else {
    	len = fread(buf, 1, BUFSIZE-1, stdin);
    }
   	buf[len] = 0;

	s = buf;
	next();
    r = parse_defs(buf);

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
    addOpCombinator(&code, Lt,  "<", &env);
    addCondCombinator(&code, &env);
    list_t* pdef;
    for (pdef = defs; pdef; pdef=pdef->next) {
        def_t* def = pdef->data;
		struct env_t* entry = NEW(struct env_t);
		entry->name = def->name;
		entry->args = length(def->args);
		entry->mode.mode = Super;
		entry->mode.params.address = code.code_size;
		env = cons(entry, env);
		compileDef(&code, def, env);
    }
    unsigned haltat = code.code_size;
    haltInstruction(&code);
    for (int i=0; i<code.code_size; ++i) {
		printf("%d: %s\n", i, instructionToString(&code.code[i]));
    }
    AddressMode start_am;
    find_mode(env, "main", &start_am);
    state.pc = start_am.params.address;
    struct Closure* ret = NEW(struct Closure);
    ret->pc = haltat;
    ret->frame = NULL;
	stack = cons(ret, stack);
	while (code.code[state.pc].ins != Halt) {
		printf("%ld: %s\n", state.pc, instructionToString(&code.code[state.pc]));
		for (list_t* sp = stack; sp; sp = sp->next) {
			struct Closure* closure = sp->data;
			if (closure->pc == -1)
				printf(" %ld|%ld", closure->pc, closure->value);
			else
				printf(" %ld|%p", closure->pc, closure->frame);
		}
		printf("%s", "\n");
		step(&code);
	}
	printf("%ld: %s\n", state.pc, instructionToString(&code.code[state.pc]));
	for (list_t* sp = stack; sp; sp=sp->next) {
		struct Closure* closure = sp->data;
		if (closure->pc == -1)
			printf(" %ld|%ld", closure->pc, closure->value);
		else
			printf(" %ld|%p", closure->pc, closure->frame);
	}


	return 0;
}
