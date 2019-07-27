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
    EXPR_NUM
} exprtag_t;

typedef char *expr_var_t;

typedef struct {
    const struct expr *fun;
    const struct expr *arg;
} expr_app_t;

typedef char *expr_str_t;

typedef int expr_num_t;

typedef struct expr {
    exprtag_t tag;
    union {
        expr_var_t var;
        expr_app_t app;
        expr_str_t str;
        expr_num_t num;
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
	T_RE,
	T_OR,
	T_AND,
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

	while (!isalnum(*t) && *t!=';' &&*t !=')' && *t!='(' && *t!=' ' && *t!='\t' && *t!='\n' && *t!='#') {
		++t;
	}
	free(token.s);
	token.s = NEWA(char, t-s+1);
	memcpy(token.s, s, t-s);
	token.s[t-s]=0;
	token.type = T_PUNCT;
	s = t;
	return;
#if 0
	if (ispunct(*t) && *t!=';' &&*t !=')' && *t!='(') {
		++t;
		while (ispunct(*t) && *t!=';' &&*t !=')' && *t!='(') {
			++t;
		}
		free(token.s);
		token.s = NEWA(char, t-s+1);
		memcpy(token.s, s, t-s);
		token.s[t-s]=0;
		token.type = T_PUNCT;
		return;
	}
#endif
}
int isname0(char c)
{
    return isalpha(c);
}

int isname1(char c)
{
    return (isalnum(c) || c=='_' || c=='\'');
}

result_t *parse_name(const char *s)
{
    const char *t = s;
    result_t *r = NULL;

    if(isname0(*t)) {
        t++;
        while(isname1(*t)) t++;

        r = NEW(result_t);
        char *name = NEWA(char, t-s+1);
        memcpy(name, s, t-s);
        name[t-s] = 0;

        r->data = name;
        r->s = t;
    }

    return r;
}

result_t *parse_punct(const char *s)
{
    const char *t = s;
    result_t *r = NULL;

    if(ispunct(*t) && *t!=';' &&*t!=')' && *t!='(') {
        t++;
        while(ispunct(*t) && *t!=';' && *t!=')' && *t!='(') t++;

        r = NEW(result_t);
        char *name = NEWA(char, t-s+1);
        memcpy(name, s, t-s);
        name[t-s] = 0;

        r->data = name;
        r->s = t;
    }

    return r;
}

result_t *parse_comment(const char *s);

int ishspace(char c)
{
    return (c==' ' || c=='\t');
}

const char *skipws(const char *s)
{
    const char *t = NULL;
    result_t *rcmt = NULL;

    while(s!=t) {
        t = s;
        while(ishspace(*s)) s++;    // skip horizontal whitespace

        // skip an end-of-line comment
        rcmt = parse_comment(s);
        if(rcmt) s = rcmt->s;

        // cross newline if indentation follows
        // XXX this doesn't work with "\r\n" but i don't really care.
        if(*s == '\n' && ishspace(*(s+1)))
            s++;
    }

    return s;
}

result_t *parse_var(const char *s)
{
    result_t *r = NULL;
    result_t *rname = NULL;

    rname = parse_name(s);
    if(!rname) return NULL;

    r = NEW(result_t);
    r->s = rname->s;
    expr_t *e = NEW(expr_t);
    e->tag = EXPR_VAR;
    e->var = rname->data;
    r->data = e;

    return r;
}

result_t *parse_oper(const char *s)
{
    result_t *r = NULL;
    result_t *rname = NULL;

    rname = parse_punct(s);
    if(!rname) return NULL;

    r = NEW(result_t);
    r->s = rname->s;
    expr_t *e = NEW(expr_t);
    e->tag = EXPR_VAR;
    e->var = rname->data;
    r->data = e;

    return r;
}

result_t *parse_nonapp(const char *s);

result_t *parse_try_app(result_t *r1)
{
    result_t *rarg = NULL;
    result_t *r = NULL;

    rarg = parse_nonapp(r1->s);
    if(!rarg) return r1;

    r = NEW(result_t);
    r->s = rarg->s;
    expr_t *e = NEW(expr_t);
    e->tag = EXPR_APP;
    e->app.fun = r1->data;
    e->app.arg = rarg->data;
    r->data = e;

    return parse_try_app(r);
}

result_t *parse_expr(const char *s);

result_t *parse_parens(const char *s)
{
    result_t *r = NULL;

    if(*s != '(') return NULL;
    r = parse_expr(s+1);
    if(!r) return NULL;
    s = skipws(r->s);
    if(*s != ')') return NULL;

    r->s = s+1;

    return r;
}

result_t *parse_str(const char *s)
{
    result_t *r = NULL;
    char c;
    const char *t = s;

    if(*t++ != '"') return NULL;
    while((c = *t++) != '"')
        if(c == '\\') t++;

    expr_t *e = NEW(expr_t);
    e->tag = EXPR_STR;
    e->str = NEWA(char, t-s+1);
    memcpy(e->str, s, t-s);
    e->str[t-s] = '\0';

    r = NEW(result_t);
    r->s = t;
    r->data = e;

    return r;
}

result_t *parse_num(const char *s)
{
    result_t *r = NULL;

    if(!isdigit(*s)) return NULL;

    r = NEW(result_t);
    int i = 0;
    while(isdigit(*s))
        i = 10*i + (*s++) - '0';
    expr_t *e = NEW(expr_t);
    e->tag = EXPR_NUM;
    e->num = i;
    r->s = s;
    r->data = e;

    return r;
}

result_t *parse_literal(const char *s)
{
    result_t *r = NULL;

    r = parse_num(s);
    if(!r) r = parse_str(s);

    return r;
}

result_t *parse_nonapp(const char *s)
{
    result_t *r = NULL;
    s = skipws(s);

    r = parse_parens(s);
    if(!r) r = parse_literal(s);
    if(!r) r = parse_var(s);
    if(!r) r = parse_oper(s);

    return r;
}

result_t *parse_expr(const char *s)
{
    result_t *r1 = NULL;
    result_t *r2 = NULL;

    r1 = parse_nonapp(s);
    if(!r1) return NULL;
    return parse_try_app(r1);
}

result_t *parse_eol(const char *s)
{
    result_t *r = NULL;
    char *t = NULL;

    if(*s == '\n') {
        s += 1;
        t = "\n";
    } else if(!strncmp(s, "\r\n", 2)) {
        s += 2;
        t = "\r\n";
    } else if(*s == '\0') {
        t = "";
    } else {
        return NULL;
    }

    r = NEW(result_t);
    r->data = t;
    r->s = s;

    return r;
}

result_t *parse_comment(const char *s)
{
    result_t *r = NULL;

    if(strncmp(s, "#", 1))
        return NULL;

    while(!parse_eol(s)) s++;

    r = NEW(result_t);
    r->data = NULL;
    r->s = s;

    return r;
}

const char *skip_empty_lines(const char *s)
{
    result_t *r;
    while(r = parse_eol(skipws(s))) {
        if(r->s == s) break;
        s = r->s;
    }
    return s;
}

result_t *parse_args(const char *s)
{
    result_t *rx = parse_name(skipws(s));
    result_t *r = NEW(result_t);
    if(!rx) {
        r->s = s;
        r->data = NULL; // empty list
    } else {
        result_t *rxs = parse_args(rx->s);
        assert(rxs != NULL);
        r->s = rxs->s;
        r->data = cons(rx->data, rxs->data);
    }
    return r;
}

result_t* parse_expr2()
{
	list_t* terms;
    while (token.type != T_EOF && token.type != T_SEMI) {
		terms = cons(token.s, terms);
		next();
    }
    return terms;
}
result_t *parse_def()
{
    result_t *r = NULL;
    result_t *rname = NULL;
    result_t *rargs = NULL;
    result_t *rrhs = NULL;
    result_t *rend = NULL;

    if (token.type != T_NAME)
		return NULL;
    rname = NEW(result_t);
    rname->data = token.s;

    next();

    while (token.type == T_NAME) {
		r = NEW(result_t);
		r->data = token.s;
		rargs = cons(r, rargs);
		next();
    }

    assert(token.type == T_PUNCT && strcmp(token.s,"=")==0);
    next();

    rrhs = parse_expr2();

    if(!rrhs) return NULL;
    assert(token.type == T_SEMI);
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

result_t *parse_defs(const char *s)
{
    result_t *r =  NULL;
    result_t *rdef =  NULL;
    list_t *defs = NULL;
    list_t *last = NULL;

    for (rdef = parse_def(s); rdef!=NULL; rdef=parse_def(s)) {
        s = rdef->s;
        if(last) {
            last = last->next = cons(rdef->data, NULL);
        } else {
            last = defs = cons(rdef->data, NULL);
        }
    }

    r = NEW(result_t);
    r->data = defs;
    r->s = skip_empty_lines(s);

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
    default:
        printf("?%d?\n", e->tag);
        break;
    }
}

void pprint_def(int col, const def_t *def)
{
    const list_t *p = def->args;

    printf("%s", def->name);
    for(; p; p=p->next) printf(" %s", p->data);
    printf(" :=\n");
    indent(col+2);
    pprint_expr(col+2, def->rhs);
}

void pprint_defs(int col, const list_t *defs)
{
    for(; defs; defs=defs->next)
        pprint_def(col, defs->data);
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
	while (token.type != T_EOF) {
		switch (token.type) {
		case T_NAME: printf("T_NAME: %s\n", token.s); break;
		case T_PUNCT: printf("T_PUNCT: %s\n", token.s); break;
		case T_NUM: printf("T_NUM: %d\n", token.value); break;
		case T_SEMI: printf("T_SEMI\n"); break;
		case T_LPAREN: printf("T_LPAREN\n"); break;
		case T_RPAREN: printf("T_RPAREN\n"); break;

		}
		next();
	}
	return 0;
    r = parse_defs(buf);

    if(r == NULL) {
        fprintf(stderr, "no parse\n");
        return 1;
    }

    list_t *defs = r->data;
    pprint_defs(0, defs);

	return 0;
}


