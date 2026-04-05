
// Minimal C interpreter

#include "minic.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t minic_name_hash(const char *name) {
	uint32_t h = 5381;
	while (*name) {
		h = ((h << 5) + h) ^ (uint8_t)(*name++);
	}
	return h;
}

// ████████╗ ██████╗ ██╗  ██╗███████╗███╗   ██╗
// ╚══██╔══╝██╔═══██╗██║ ██╔╝██╔════╝████╗  ██║
//    ██║   ██║   ██║█████╔╝ █████╗  ██╔██╗ ██║
//    ██║   ██║   ██║██╔═██╗ ██╔══╝  ██║╚██╗██║
//    ██║   ╚██████╔╝██║  ██╗███████╗██║ ╚████║
//    ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝

typedef enum {
	TOK_INT,
	TOK_FLOAT,
	TOK_CHAR,
	TOK_DOUBLE,
	TOK_BOOL,
	TOK_RETURN,
	TOK_IF,
	TOK_ELSE,
	TOK_WHILE,
	TOK_FOR,
	TOK_BREAK,
	TOK_CONTINUE,
	TOK_STRUCT,
	TOK_TYPEDEF,
	TOK_ENUM,
	TOK_VOID,
	TOK_ID,
	TOK_IDENT,
	TOK_NUMBER,
	TOK_CHAR_LIT,
	TOK_STR_LIT,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_LBRACKET,
	TOK_RBRACKET,
	TOK_SEMICOLON,
	TOK_COMMA,
	TOK_ASSIGN,
	TOK_PLUS_ASSIGN,
	TOK_MINUS_ASSIGN,
	TOK_MUL_ASSIGN,
	TOK_DIV_ASSIGN,
	TOK_MOD_ASSIGN,
	TOK_EQ,
	TOK_NEQ,
	TOK_LT,
	TOK_GT,
	TOK_LE,
	TOK_GE,
	TOK_AND,
	TOK_OR,
	TOK_NOT,
	TOK_AMP,
	TOK_PLUS,
	TOK_MINUS,
	TOK_INC,
	TOK_DEC,
	TOK_STAR,
	TOK_SLASH,
	TOK_MOD,
	TOK_DOT,
	TOK_ARROW,
	TOK_EOF
} minic_tok_type_t;

typedef struct {
	minic_tok_type_t type;
	char             text[64];
	minic_val_t      val;     // TOK_NUMBER, TOK_CHAR_LIT, TOK_STR_LIT
	int              src_pos; // source char position (for error reporting)
} minic_token_t;

typedef struct {
	const char    *src;
	int            pos; // source char index OR token index (stream mode)
	minic_token_t  cur;
	minic_token_t *tokens; // NULL = source mode, non-NULL = token stream mode
	int            token_count;
} minic_lexer_t;

static minic_u8 *minic_active_mem      = NULL;
static int      *minic_active_mem_used = NULL;

static void minic_lex_next(minic_lexer_t *l) {
	// Token-stream mode: replay cached tokens
	if (l->tokens != NULL) {
		if (l->pos < l->token_count) {
			l->cur = l->tokens[l->pos++];
		}
		else {
			l->cur.type = TOK_EOF;
		}
		// Re-allocate string literals into current arena
		if (l->cur.type == TOK_STR_LIT && l->cur.text[0] != '\0') {
			int len     = (int)strlen(l->cur.text) + 1;
			int aligned = (*minic_active_mem_used + 7) & ~7;
			memcpy(&minic_active_mem[aligned], l->cur.text, len);
			*minic_active_mem_used = (aligned + len + 7) & ~7;
			l->cur.val             = minic_val_ptr((void *)&minic_active_mem[aligned]);
		}
		return;
	}

	// Source mode: lex from source string
	// Skip whitespace and comments
	for (;;) {
		while (l->src[l->pos] != '\0' && isspace((unsigned char)l->src[l->pos])) {
			l->pos++;
		}
		if (l->src[l->pos] == '/' && l->src[l->pos + 1] == '/') {
			while (l->src[l->pos] != '\0' && l->src[l->pos] != '\n') {
				l->pos++;
			}
			continue;
		}
		// Skip preprocessor directives
		if (l->src[l->pos] == '#') {
			while (l->src[l->pos] != '\0' && l->src[l->pos] != '\n') {
				l->pos++;
			}
			continue;
		}
		if (l->src[l->pos] == '/' && l->src[l->pos + 1] == '*') {
			l->pos += 2;
			while (l->src[l->pos] != '\0' && !(l->src[l->pos] == '*' && l->src[l->pos + 1] == '/')) {
				l->pos++;
			}
			if (l->src[l->pos] != '\0') {
				l->pos += 2;
			}
			continue;
		}
		break;
	}
	l->cur.src_pos = l->pos;

	if (l->src[l->pos] == '\0') {
		l->cur.type = TOK_EOF;
		return;
	}

	char c = l->src[l->pos];

	if (c == '0' && (l->src[l->pos + 1] == 'x' || l->src[l->pos + 1] == 'X')) {
		l->pos += 2; // Consume '0x'
		unsigned int n = 0;
		while (isxdigit((unsigned char)l->src[l->pos])) {
			char h     = l->src[l->pos++];
			int  digit = (h >= '0' && h <= '9') ? h - '0' : (h >= 'a' && h <= 'f') ? h - 'a' + 10 : h - 'A' + 10;
			n          = n * 16 + digit;
		}
		l->cur.val  = minic_val_int((int)n);
		l->cur.type = TOK_NUMBER;
		return;
	}

	if (isdigit((unsigned char)c)) {
		double n = 0;
		while (isdigit((unsigned char)l->src[l->pos])) {
			n = n * 10 + (l->src[l->pos++] - '0');
		}
		bool is_float = false;
		if (l->src[l->pos] == '.') {
			l->pos++;
			double frac = 0.1;
			while (isdigit((unsigned char)l->src[l->pos])) {
				n += (l->src[l->pos++] - '0') * frac;
				frac *= 0.1;
			}
			is_float = true;
		}
		if (l->src[l->pos] == 'f' || l->src[l->pos] == 'F') {
			l->pos++;
			is_float = true;
		}
		if (is_float) {
			l->cur.val = minic_val_float((float)n);
		}
		else {
			l->cur.val = minic_val_int((int)n);
		}
		l->cur.type = TOK_NUMBER;
		return;
	}

	if (c == '"') {
		l->pos++; // Consume opening '"'
		// Allocate space in the active context's arena for the string
		int start = (*minic_active_mem_used + 7) & ~7; // Aligned start
		int wi    = start;
		while (l->src[l->pos] != '"' && l->src[l->pos] != '\0') {
			char ch;
			if (l->src[l->pos] == '\\') {
				l->pos++;
				switch (l->src[l->pos++]) {
				case 'n':
					ch = '\n';
					break;
				case 't':
					ch = '	';
					break;
				case '0':
					ch = '\0';
					break;
				case '\\':
					ch = '\\';
					break;
				case '"':
					ch = '"';
					break;
				case 'r':
					ch = '\r';
					break;
				case '\n': // Line continuation: backslash-newline, skip both
					continue;
				case '\r': // Handle \r\n line endings
					if (l->src[l->pos] == '\n') {
						l->pos++;
					}
					continue;
				default:
					ch = '\0';
					break;
				}
			}
			else {
				ch = l->src[l->pos++];
			}
			minic_active_mem[wi++] = (minic_u8)ch;
		}
		minic_active_mem[wi++] = '\0';
		*minic_active_mem_used = (wi + 7) & ~7;
		if (l->src[l->pos] == '"') {
			l->pos++; // Consume closing '"'
		}
		l->cur.type = TOK_STR_LIT;
		// Store a real pointer into the context arena so the dispatcher passes it correctly
		l->cur.val = minic_val_ptr((void *)&minic_active_mem[start]);
		return;
	}

	if (c == '\'') {
		l->pos++; // Consume opening '
		int v = 0;
		if (l->src[l->pos] == '\\') {
			l->pos++;
			switch (l->src[l->pos++]) {
			case 'n':
				v = '\n';
				break;
			case 't':
				v = '	';
				break;
			case '0':
				v = '\0';
				break;
			case '\\':
				v = '\\';
				break;
			case '\'':
				v = '\'';
				break;
			default:
				v = 0;
				break;
			}
		}
		else {
			v = (unsigned char)l->src[l->pos++];
		}
		l->pos++; // Consume closing '
		l->cur.type = TOK_CHAR_LIT;
		l->cur.val  = minic_val_int(v);
		return;
	}

	if (isalpha((unsigned char)c) || c == '_') {
		int i = 0;
		while (isalnum((unsigned char)l->src[l->pos]) || l->src[l->pos] == '_') {
			l->cur.text[i++] = l->src[l->pos++];
		}
		l->cur.text[i] = '\0';

		// M7: keyword dispatch by first letter + length
		int len = i;
		switch (l->cur.text[0]) {
		case 'b':
			if (len == 4 && strcmp(l->cur.text, "bool") == 0) {
				l->cur.type = TOK_BOOL;
				return;
			}
			if (len == 5 && strcmp(l->cur.text, "break") == 0) {
				l->cur.type = TOK_BREAK;
				return;
			}
			break;
		case 'c':
			if (len == 4 && strcmp(l->cur.text, "char") == 0) {
				l->cur.type = TOK_CHAR;
				return;
			}
			if (len == 8 && strcmp(l->cur.text, "continue") == 0) {
				l->cur.type = TOK_CONTINUE;
				return;
			}
			break;
		case 'd':
			if (len == 6 && strcmp(l->cur.text, "double") == 0) {
				l->cur.type = TOK_DOUBLE;
				return;
			}
			break;
		case 'e':
			if (len == 4 && strcmp(l->cur.text, "else") == 0) {
				l->cur.type = TOK_ELSE;
				return;
			}
			if (len == 4 && strcmp(l->cur.text, "enum") == 0) {
				l->cur.type = TOK_ENUM;
				return;
			}
			break;
		case 'f':
			if (len == 3 && strcmp(l->cur.text, "for") == 0) {
				l->cur.type = TOK_FOR;
				return;
			}
			if (len == 5 && strcmp(l->cur.text, "float") == 0) {
				l->cur.type = TOK_FLOAT;
				return;
			}
			if (len == 5 && strcmp(l->cur.text, "false") == 0) {
				l->cur.type = TOK_NUMBER;
				l->cur.val  = minic_val_int(0);
				return;
			}
			break;
		case 'i':
			if (len == 2 && strcmp(l->cur.text, "if") == 0) {
				l->cur.type = TOK_IF;
				return;
			}
			if (len == 3 && strcmp(l->cur.text, "int") == 0) {
				l->cur.type = TOK_INT;
				return;
			}
			if (len == 2 && strcmp(l->cur.text, "id") == 0) {
				l->cur.type = TOK_ID;
				return;
			}
			break;
		case 'r':
			if (len == 6 && strcmp(l->cur.text, "return") == 0) {
				l->cur.type = TOK_RETURN;
				return;
			}
			break;
		case 's':
			if (len == 6 && strcmp(l->cur.text, "struct") == 0) {
				l->cur.type = TOK_STRUCT;
				return;
			}
			break;
		case 't':
			if (len == 4 && strcmp(l->cur.text, "true") == 0) {
				l->cur.type = TOK_NUMBER;
				l->cur.val  = minic_val_int(1);
				return;
			}
			if (len == 7 && strcmp(l->cur.text, "typedef") == 0) {
				l->cur.type = TOK_TYPEDEF;
				return;
			}
			break;
		case 'v':
			if (len == 4 && strcmp(l->cur.text, "void") == 0) {
				l->cur.type = TOK_VOID;
				return;
			}
			break;
		case 'w':
			if (len == 5 && strcmp(l->cur.text, "while") == 0) {
				l->cur.type = TOK_WHILE;
				return;
			}
			break;
		}
		l->cur.type = TOK_IDENT;
		return;
	}

	l->pos++;
	switch (c) {
	case '(':
		l->cur.type = TOK_LPAREN;
		return;
	case ')':
		l->cur.type = TOK_RPAREN;
		return;
	case '{':
		l->cur.type = TOK_LBRACE;
		return;
	case '}':
		l->cur.type = TOK_RBRACE;
		return;
	case '[':
		l->cur.type = TOK_LBRACKET;
		return;
	case ']':
		l->cur.type = TOK_RBRACKET;
		return;
	case ';':
		l->cur.type = TOK_SEMICOLON;
		return;
	case ',':
		l->cur.type = TOK_COMMA;
		return;
	case '.':
		l->cur.type = TOK_DOT;
		return;
	case '*':
		if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_MUL_ASSIGN;
		}
		else {
			l->cur.type = TOK_STAR;
		}
		return;
	case '/':
		if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_DIV_ASSIGN;
		}
		else {
			l->cur.type = TOK_SLASH;
		}
		return;
	case '%':
		if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_MOD_ASSIGN;
		}
		else {
			l->cur.type = TOK_MOD;
		}
		return;
	case '+':
		if (l->src[l->pos] == '+') {
			l->pos++;
			l->cur.type = TOK_INC;
		}
		else if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_PLUS_ASSIGN;
		}
		else {
			l->cur.type = TOK_PLUS;
		}
		return;
	case '-':
		if (l->src[l->pos] == '-') {
			l->pos++;
			l->cur.type = TOK_DEC;
		}
		else if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_MINUS_ASSIGN;
		}
		else if (l->src[l->pos] == '>') {
			l->pos++;
			l->cur.type = TOK_ARROW;
		}
		else {
			l->cur.type = TOK_MINUS;
		}
		return;
	case '=':
		if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_EQ;
		}
		else {
			l->cur.type = TOK_ASSIGN;
		}
		return;
	case '!':
		if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_NEQ;
		}
		else {
			l->cur.type = TOK_NOT;
		}
		return;
	case '&':
		if (l->src[l->pos] == '&') {
			l->pos++;
			l->cur.type = TOK_AND;
		}
		else {
			l->cur.type = TOK_AMP;
		}
		return;
	case '|':
		if (l->src[l->pos] == '|') {
			l->pos++;
			l->cur.type = TOK_OR;
		}
		return;
	case '<':
		if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_LE;
		}
		else {
			l->cur.type = TOK_LT;
		}
		return;
	case '>':
		if (l->src[l->pos] == '=') {
			l->pos++;
			l->cur.type = TOK_GE;
		}
		else {
			l->cur.type = TOK_GT;
		}
		return;
	}
}

// ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
// ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
// █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
// ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
// ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
// ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝

void *minic_alloc(int size) {
	// Align to 8 bytes
	int aligned            = (*minic_active_mem_used + 7) & ~7;
	*minic_active_mem_used = aligned + size;
	return &minic_active_mem[aligned];
}

// --- M15: Compiled function prototype (forward for minic_func_t) ---
typedef struct {
	uint32_t     *code;
	int           code_count;
	int           num_regs;
	minic_val_t  *constants;
	int           const_count;
} minic_proto_t;

typedef struct {
	char        name[64];
	uint32_t    name_hash;
	minic_val_t val;
} minic_var_t;

typedef struct {
	char         name[64];
	int          offset; // index into global arr_data[]
	int          count;
	minic_type_t elem_type;
} minic_arr_t;

typedef struct {
	char name[64];
	char params[MINIC_MAX_PARAMS][64];
	char param_structs[MINIC_MAX_PARAMS][64]; // struct type name per param, or ""
	// Per-param declared type
	minic_type_t param_types[MINIC_MAX_PARAMS];
	int          param_count;
	int          body_pos; // lexer position of '{' that starts the body
	minic_type_t ret_type;
	minic_ctx_t *ctx; // owning context, set at parse time
	// M4: body token cache
	minic_token_t *body_tokens; // NULL = not cached yet, malloc'd
	int            body_token_count;
	// M15: compiled bytecode
	minic_proto_t *proto; // NULL = not compiled yet
} minic_func_t;

#define MINIC_INC_DELTA(l) ((l)->cur.type == TOK_INC ? 1.0 : -1.0)


typedef struct {
	char         name[64];
	char         fields[MINIC_MAX_STRUCT_FIELDS][64];
	char         field_struct_names[MINIC_MAX_STRUCT_FIELDS][64]; // struct type name for PTR fields, or ""
	uint32_t     field_hashes[MINIC_MAX_STRUCT_FIELDS];           // pre-computed name hash for fast lookup
	int          field_count;
	int          size; // sizeof the native C struct, 0 if unknown
	bool         has_native_layout;
	int          field_offsets[MINIC_MAX_STRUCT_FIELDS];
	minic_type_t field_native_types[MINIC_MAX_STRUCT_FIELDS];
	minic_type_t field_deref_types[MINIC_MAX_STRUCT_FIELDS]; // pointed-to type for PTR fields
} minic_struct_def_t;

// Maps a variable name to its struct type name
typedef struct {
	char     var_name[64];
	char     struct_name[64];
	uint32_t var_name_hash;
} minic_vartype_t;

typedef struct minic_env_s {
	minic_lexer_t       lex;
	const char         *filename;
	minic_var_t        *vars;
	int                 var_count;
	int                 var_cap;
	minic_arr_t        *arrs;
	int                 arr_count;
	int                 arr_cap;
	minic_val_t        *arr_data;      // global array element storage
	int                *arr_data_used; // pointer to shared counter
	minic_func_t       *funcs;
	int                 func_count;
	int                 func_cap;
	minic_struct_def_t *structs; // shared across calls
	int                 struct_count;
	int                 struct_cap;
	minic_vartype_t    *vartypes; // local: var->struct type mapping
	int                 vartype_count;
	int                 vartype_cap;
	bool                returning;
	bool                breaking;
	bool                continuing;
	bool                error;
	minic_val_t         return_val;
	minic_type_t        decl_type;
	struct minic_env_s *global_env; // top-level env that owns the script globals
} minic_env_t;

struct minic_ctx_s {
	minic_u8   *mem;
	int         mem_used;
	minic_env_t e;
	float       result;
	char       *src_copy;
};

static minic_env_t *minic_active_env = NULL;

static minic_proto_t *vm_compile_body(const char *src, int body_pos, minic_env_t *env, minic_func_t *fn);
static minic_val_t minic_vm_exec(minic_ctx_t *ctx, minic_proto_t *proto, minic_val_t *args, int argc);
static minic_val_t minic_parse_expr(minic_env_t *e);
static minic_val_t minic_parse_cond(minic_env_t *e);
static void        minic_skip_block(minic_env_t *e);
static void        minic_parse_stmt(minic_env_t *e);
static void        minic_parse_block(minic_env_t *e);

static const char *minic_tok_name(minic_tok_type_t t) {
	switch (t) {
	case TOK_INT:
		return "'int'";
	case TOK_FLOAT:
		return "'float'";
	case TOK_CHAR:
		return "'char'";
	case TOK_DOUBLE:
		return "'double'";
	case TOK_BOOL:
		return "'bool'";
	case TOK_RETURN:
		return "'return'";
	case TOK_IF:
		return "'if'";
	case TOK_ELSE:
		return "'else'";
	case TOK_WHILE:
		return "'while'";
	case TOK_FOR:
		return "'for'";
	case TOK_BREAK:
		return "'break'";
	case TOK_CONTINUE:
		return "'continue'";
	case TOK_IDENT:
		return "identifier";
	case TOK_NUMBER:
		return "number";
	case TOK_CHAR_LIT:
		return "char literal";
	case TOK_STR_LIT:
		return "string literal";
	case TOK_LPAREN:
		return "'('";
	case TOK_RPAREN:
		return "')'";
	case TOK_LBRACE:
		return "'{'";
	case TOK_RBRACE:
		return "'}'";
	case TOK_LBRACKET:
		return "'['";
	case TOK_RBRACKET:
		return "']'";
	case TOK_SEMICOLON:
		return "';'";
	case TOK_COMMA:
		return "','";
	case TOK_ASSIGN:
		return "'='";
	case TOK_EQ:
		return "'=='";
	case TOK_NEQ:
		return "'!='";
	case TOK_LT:
		return "'<'";
	case TOK_GT:
		return "'>'";
	case TOK_LE:
		return "'<='";
	case TOK_GE:
		return "'>='";
	case TOK_AND:
		return "'&&'";
	case TOK_OR:
		return "'||'";
	case TOK_AMP:
		return "'&'";
	case TOK_PLUS:
		return "'+'";
	case TOK_MINUS:
		return "'-'";
	case TOK_INC:
		return "'++'";
	case TOK_DEC:
		return "'--'";
	case TOK_PLUS_ASSIGN:
		return "'+='";
	case TOK_MINUS_ASSIGN:
		return "'-='";
	case TOK_MUL_ASSIGN:
		return "'*='";
	case TOK_DIV_ASSIGN:
		return "'/='";
	case TOK_MOD_ASSIGN:
		return "'%='";
	case TOK_STAR:
		return "'*'";
	case TOK_SLASH:
		return "'/'";
	case TOK_MOD:
		return "'%'";
	case TOK_DOT:
		return "'.'";
	case TOK_ARROW:
		return "'->'";
	case TOK_STRUCT:
		return "'struct'";
	case TOK_VOID:
		return "'void'";
	case TOK_ID:
		return "'id'";
	case TOK_EOF:
		return "end of file";
	default:
		return "unknown token";
	}
}

static int minic_current_line(minic_env_t *e) {
	int line = 1;
	int pos  = e->lex.cur.src_pos;
	for (int i = 0; i < pos; i++) {
		if (e->lex.src[i] == '\n')
			line++;
	}
	return line;
}

static void minic_error(minic_env_t *e, const char *msg) {
	if (!e->error) {
		fprintf(stderr, "%s:%d: error: %s (got %s)\n", e->filename, minic_current_line(e), msg, minic_tok_name(e->lex.cur.type));
		e->error     = true;
		e->returning = true;
	}
}

static void minic_expect(minic_env_t *e, minic_tok_type_t expected) {
	if (e->lex.cur.type != expected) {
		char msg[128];
		snprintf(msg, sizeof(msg), "expected %s", minic_tok_name(expected));
		minic_error(e, msg);
		return;
	}
	minic_lex_next(&e->lex);
}

static minic_val_t minic_var_get(minic_env_t *e, const char *name) {
	uint32_t h = minic_name_hash(name);
	for (int i = e->var_count - 1; i >= 0; --i) {
		if (e->vars[i].name_hash == h && strcmp(e->vars[i].name, name) == 0) {
			return e->vars[i].val;
		}
	}
	if (e->global_env != NULL) {
		for (int i = e->global_env->var_count - 1; i >= 0; --i) {
			if (e->global_env->vars[i].name_hash == h && strcmp(e->global_env->vars[i].name, name) == 0) {
				return e->global_env->vars[i].val;
			}
		}
	}
	return minic_val_int(0);
}

static void minic_var_set(minic_env_t *e, const char *name, minic_val_t val) {
	uint32_t h = minic_name_hash(name);
	for (int i = e->var_count - 1; i >= 0; --i) {
		if (e->vars[i].name_hash == h && strcmp(e->vars[i].name, name) == 0) {
			// Preserve declared type, coerce if needed
			if (e->vars[i].val.type != val.type) {
				val = minic_val_cast(val, e->vars[i].val.type);
			}
			// Preserve deref_type for pointer variables (it was set at declaration)
			if (e->vars[i].val.type == MINIC_T_PTR && e->vars[i].val.deref_type != MINIC_T_PTR) {
				val.deref_type = e->vars[i].val.deref_type;
			}
			e->vars[i].val = val;
			return;
		}
	}
	// Fall back to globals: write directly into the top-level env
	if (e->global_env != NULL) {
		minic_env_t *g = e->global_env;
		for (int i = g->var_count - 1; i >= 0; --i) {
			if (g->vars[i].name_hash == h && strcmp(g->vars[i].name, name) == 0) {
				if (g->vars[i].val.type != val.type) {
					val = minic_val_cast(val, g->vars[i].val.type);
				}
				if (g->vars[i].val.type == MINIC_T_PTR && g->vars[i].val.deref_type != MINIC_T_PTR) {
					val.deref_type = g->vars[i].val.deref_type;
				}
				g->vars[i].val = val;
				return;
			}
		}
	}
	if (e->var_count < e->var_cap) {
		strncpy(e->vars[e->var_count].name, name, 63);
		e->vars[e->var_count].name_hash = h;
		e->vars[e->var_count].val       = val;
		e->var_count++;
	}
}

static minic_val_t *minic_var_get_ptr(minic_env_t *e, const char *name) {
	uint32_t h = minic_name_hash(name);
	for (int i = e->var_count - 1; i >= 0; --i) {
		if (e->vars[i].name_hash == h && strcmp(e->vars[i].name, name) == 0) {
			return &e->vars[i].val;
		}
	}
	if (e->global_env != NULL) {
		minic_env_t *g = e->global_env;
		for (int i = g->var_count - 1; i >= 0; --i) {
			if (g->vars[i].name_hash == h && strcmp(g->vars[i].name, name) == 0) {
				return &g->vars[i].val;
			}
		}
	}
	return NULL;
}

static void minic_var_decl(minic_env_t *e, const char *name, minic_type_t type, minic_val_t init) {
	// Coerce init to declared type
	minic_val_t v = minic_val_cast(init, type);
	// If this is a typed pointer, preserve the deref_type from init
	if (type == MINIC_T_PTR) {
		v.deref_type = init.deref_type;
	}
	if (e->var_count < e->var_cap) {
		strncpy(e->vars[e->var_count].name, name, 63);
		e->vars[e->var_count].name_hash = minic_name_hash(name);
		e->vars[e->var_count].val       = v;
		e->var_count++;
	}
}

static minic_val_t minic_var_addr(minic_env_t *e, const char *name) {
	uint32_t h = minic_name_hash(name);
	for (int i = e->var_count - 1; i >= 0; --i) {
		if (e->vars[i].name_hash == h && strcmp(e->vars[i].name, name) == 0) {
			// Address of a minic_val_t — deref reads a full minic_val_t
			minic_val_t addr = minic_val_ptr(&e->vars[i].val);
			addr.deref_type  = MINIC_T_PTR; // sentinel: deref reads minic_val_t
			return addr;
		}
	}
	// Fall back to globals: return address into the top-level env's storage
	if (e->global_env != NULL) {
		minic_env_t *g = e->global_env;
		for (int i = g->var_count - 1; i >= 0; --i) {
			if (g->vars[i].name_hash == h && strcmp(g->vars[i].name, name) == 0) {
				minic_val_t addr = minic_val_ptr(&g->vars[i].val);
				addr.deref_type  = MINIC_T_PTR;
				return addr;
			}
		}
	}
	// Create it as a local
	if (e->var_count < e->var_cap) {
		strncpy(e->vars[e->var_count].name, name, 63);
		e->vars[e->var_count].name_hash = h;
		e->vars[e->var_count].val       = minic_val_int(0);
		minic_val_t addr                = minic_val_ptr(&e->vars[e->var_count].val);
		addr.deref_type                 = MINIC_T_PTR;
		e->var_count++;
		return addr;
	}
	return minic_val_ptr(NULL);
}

static minic_arr_t *minic_arr_get(minic_env_t *e, const char *name) {
	for (int i = 0; i < e->arr_count; ++i) {
		if (strcmp(e->arrs[i].name, name) == 0) {
			return &e->arrs[i];
		}
	}
	if (e->global_env != NULL) {
		minic_env_t *g = e->global_env;
		for (int i = 0; i < g->arr_count; ++i) {
			if (strcmp(g->arrs[i].name, name) == 0) {
				return &g->arrs[i];
			}
		}
	}
	return NULL;
}

static void minic_arr_decl(minic_env_t *e, const char *name, int count, minic_type_t elem_type) {
	if (e->arr_count >= e->arr_cap) {
		return;
	}
	minic_arr_t *a = &e->arrs[e->arr_count++];
	strncpy(a->name, name, 63);
	a->offset    = *e->arr_data_used;
	a->count     = count;
	a->elem_type = elem_type;
	*e->arr_data_used += count;
	// Zero-initialise
	for (int i = 0; i < count; i++) {
		e->arr_data[a->offset + i] = minic_val_coerce(0.0, elem_type);
	}
}

static minic_val_t minic_arr_elem_get(minic_env_t *e, const char *name, int idx) {
	minic_arr_t *a = minic_arr_get(e, name);
	if (a != NULL && idx >= 0 && idx < a->count) {
		return e->arr_data[a->offset + idx];
	}
	// Fall back to native pointer subscript
	minic_val_t pv = minic_var_get(e, name);
	if (pv.type == MINIC_T_PTR && pv.p != NULL) {
		switch (pv.deref_type) {
		case MINIC_T_FLOAT:
			return minic_val_float(((float *)pv.p)[idx]);
		case MINIC_T_INT:
			return minic_val_int(((int32_t *)pv.p)[idx]);
		default:
			return minic_val_ptr(((void **)pv.p)[idx]);
		}
	}
	return minic_val_int(0);
}

static void minic_arr_elem_set(minic_env_t *e, const char *name, int idx, minic_val_t val) {
	minic_arr_t *a = minic_arr_get(e, name);
	if (a != NULL && idx >= 0 && idx < a->count) {
		e->arr_data[a->offset + idx] = minic_val_cast(val, a->elem_type);
		return;
	}
	// Fall back to native pointer subscript
	minic_val_t pv = minic_var_get(e, name);
	if (pv.type == MINIC_T_PTR && pv.p != NULL) {
		switch (pv.deref_type) {
		case MINIC_T_FLOAT:
			((float *)pv.p)[idx] = (float)minic_val_to_d(val);
			break;
		case MINIC_T_INT:
			((int32_t *)pv.p)[idx] = (int32_t)minic_val_to_d(val);
			break;
		default:
			((void **)pv.p)[idx] = minic_val_to_ptr(val);
			break;
		}
	}
}

static minic_func_t *minic_func_get(minic_env_t *e, const char *name) {
	for (int i = 0; i < e->func_count; ++i) {
		if (strcmp(e->funcs[i].name, name) == 0) {
			return &e->funcs[i];
		}
	}
	return NULL;
}

static minic_struct_def_t *minic_struct_get(minic_env_t *e, const char *name);

static void minic_vartype_set(minic_env_t *e, const char *var_name, const char *struct_name) {
	if (e->vartype_count < e->vartype_cap) {
		strncpy(e->vartypes[e->vartype_count].var_name, var_name, 63);
		strncpy(e->vartypes[e->vartype_count].struct_name, struct_name, 63);
		e->vartypes[e->vartype_count].var_name_hash = minic_name_hash(var_name);
		e->vartype_count++;
	}
}

static minic_struct_def_t *minic_var_struct(minic_env_t *e, const char *var_name) {
	uint32_t h = minic_name_hash(var_name);
	for (int i = e->vartype_count - 1; i >= 0; --i) {
		if (e->vartypes[i].var_name_hash == h && strcmp(e->vartypes[i].var_name, var_name) == 0) {
			return minic_struct_get(e, e->vartypes[i].struct_name);
		}
	}
	if (e->global_env != NULL) {
		for (int i = e->global_env->vartype_count - 1; i >= 0; --i) {
			if (e->global_env->vartypes[i].var_name_hash == h && strcmp(e->global_env->vartypes[i].var_name, var_name) == 0) {
				return minic_struct_get(e, e->global_env->vartypes[i].struct_name);
			}
		}
	}
	return NULL;
}

static minic_struct_def_t *minic_struct_get(minic_env_t *e, const char *name) {
	for (int i = 0; i < e->struct_count; ++i) {
		if (strcmp(e->structs[i].name, name) == 0) {
			return &e->structs[i];
		}
	}
	// Fall back to global registry (e.g. runtime-registered native structs)
	for (int i = 0; i < minic_global_struct_count; ++i) {
		if (strcmp(minic_global_structs[i].name, name) == 0) {
			// Copy into context for fast future lookups
			if (e->struct_count < e->struct_cap) {
				minic_struct_def_t    *dst = &e->structs[e->struct_count++];
				minic_global_struct_t *src = &minic_global_structs[i];
				strncpy(dst->name, src->name, 63);
				dst->field_count       = src->field_count;
				dst->size              = src->size;
				dst->has_native_layout = src->has_native_layout;
				for (int j = 0; j < dst->field_count; j++) {
					strncpy(dst->fields[j], src->fields[j], 63);
					strncpy(dst->field_struct_names[j], src->field_struct_names[j], 63);
					dst->field_hashes[j]       = minic_name_hash(src->fields[j]);
					dst->field_offsets[j]      = src->field_offsets[j];
					dst->field_native_types[j] = src->field_native_types[j];
					dst->field_deref_types[j]  = src->field_deref_types[j];
				}
				return dst;
			}
			return NULL;
		}
	}
	return NULL;
}

static int minic_struct_field_idx(minic_struct_def_t *def, const char *field) {
	uint32_t h = minic_name_hash(field);
	for (int i = 0; i < def->field_count; ++i) {
		if (def->field_hashes[i] == h && strcmp(def->fields[i], field) == 0) {
			return i;
		}
	}
	return -1;
}

static minic_val_t minic_struct_field_get_base(minic_env_t *e, void *base, minic_struct_def_t *def, const char *field) {
	int idx = minic_struct_field_idx(def, field);
	if (idx < 0) {
		fprintf(stderr, "%s:%d: error: struct '%s' has no field '%s'\n", e->filename, minic_current_line(e), def->name, field);
		e->error = e->returning = true;
		return minic_val_int(0);
	}
	bool in_arena = (base != NULL && (minic_u8 *)base >= minic_active_mem && (minic_u8 *)base < minic_active_mem + MINIC_MEM_SIZE);
	if (def->has_native_layout && !in_arena) {
		char *p = (char *)base + def->field_offsets[idx];
		switch (def->field_native_types[idx]) {
		case MINIC_T_PTR:
			return minic_val_typed_ptr(*(void **)p, def->field_deref_types[idx]);
		case MINIC_T_FLOAT:
			return minic_val_float(*(float *)p);
		default:
			return minic_val_int(*(int32_t *)p);
		}
	}
	minic_val_t v;
	memcpy(&v, (minic_val_t *)base + idx, sizeof(minic_val_t));
	return v;
}

static void minic_struct_field_set_base(minic_env_t *e, void *base, minic_struct_def_t *def, const char *field, minic_val_t val) {
	int idx = minic_struct_field_idx(def, field);
	if (idx < 0) {
		fprintf(stderr, "%s:%d: error: struct '%s' has no field '%s'\n", e->filename, minic_current_line(e), def->name, field);
		e->error = e->returning = true;
		return;
	}
	bool in_arena = (base != NULL && (minic_u8 *)base >= minic_active_mem && (minic_u8 *)base < minic_active_mem + MINIC_MEM_SIZE);
	if (def->has_native_layout && !in_arena) {
		char *p = (char *)base + def->field_offsets[idx];
		switch (def->field_native_types[idx]) {
		case MINIC_T_PTR:
			*(void **)p = minic_val_to_ptr(val);
			break;
		case MINIC_T_FLOAT:
			*(float *)p = (float)minic_val_to_d(val);
			break;
		default:
			*(int32_t *)p = (int32_t)minic_val_to_d(val);
			break;
		}
		return;
	}
	memcpy((minic_val_t *)base + idx, &val, sizeof(minic_val_t));
}

static minic_val_t minic_struct_field_get(minic_env_t *e, const char *var_name, minic_struct_def_t *def, const char *field) {
	minic_val_t base_val = minic_var_get(e, var_name);
	void       *base     = minic_val_to_ptr(base_val);
	return minic_struct_field_get_base(e, base, def, field);
}

static void minic_struct_field_set(minic_env_t *e, const char *var_name, minic_struct_def_t *def, const char *field, minic_val_t val) {
	minic_val_t base_val = minic_var_get(e, var_name);
	void       *base     = minic_val_to_ptr(base_val);
	minic_struct_field_set_base(e, base, def, field, val);
}

// Pre-lex a function body from source, producing a token array.
// body_pos points at the '{' character in source.
// Returns malloc'd array; sets *out_count.
static minic_token_t *minic_lex_body_tokens(const char *src, int body_pos, int *out_count) {
	minic_lexer_t tmp = {0};
	tmp.src           = src;
	tmp.pos           = body_pos;
	tmp.tokens        = NULL; // source mode

	int            cap    = 128;
	int            count  = 0;
	minic_token_t *tokens = (minic_token_t *)malloc(cap * sizeof(minic_token_t));

	minic_lex_next(&tmp);
	int depth = 0;
	do {
		if (count >= cap) {
			cap *= 2;
			tokens = (minic_token_t *)realloc(tokens, cap * sizeof(minic_token_t));
		}
		tokens[count] = tmp.cur;
		// For string literals, copy string data into text[] for persistence
		// (arena pointer in val.p will be stale after arena reset)
		if (tmp.cur.type == TOK_STR_LIT) {
			const char *s = (const char *)tmp.cur.val.p;
			if (s != NULL) {
				strncpy(tokens[count].text, s, 63);
				tokens[count].text[63] = '\0';
			}
			else {
				tokens[count].text[0] = '\0';
			}
		}
		count++;

		if (tmp.cur.type == TOK_LBRACE)
			depth++;
		if (tmp.cur.type == TOK_RBRACE)
			depth--;
		if (depth == 0 || tmp.cur.type == TOK_EOF)
			break;

		minic_lex_next(&tmp);
	} while (1);

	*out_count = count;
	return tokens;
}

static minic_val_t minic_call(minic_env_t *e, minic_func_t *fn, minic_val_t *args, int argc) {
	// M15: fast path â execute compiled bytecode via VM
	if (fn->proto != NULL && fn->ctx != NULL) {
		return minic_vm_exec(fn->ctx, fn->proto, args, argc);
	}

	int saved_mem_used = *minic_active_mem_used;

	// M4: lazy body token cache
	if (fn->body_tokens == NULL) {
		fn->body_tokens = minic_lex_body_tokens(e->lex.src, fn->body_pos, &fn->body_token_count);
	}

	minic_env_t child     = {0};
	child.lex.src         = e->lex.src;
	child.lex.tokens      = fn->body_tokens;
	child.lex.token_count = fn->body_token_count;
	child.lex.pos         = 0; // start of token array
	child.filename        = e->filename;
	child.var_cap         = 64;
	child.vars            = minic_alloc(child.var_cap * sizeof(minic_var_t));
	child.global_env      = e->global_env != NULL ? e->global_env : e;
	child.arr_cap         = 32;
	child.arrs            = minic_alloc(child.arr_cap * sizeof(minic_arr_t));
	child.arr_data        = e->arr_data;
	child.arr_data_used   = e->arr_data_used;
	child.func_count      = e->func_count;
	child.func_cap        = e->func_cap;
	child.funcs           = e->funcs;
	child.struct_count    = e->struct_count;
	child.struct_cap      = e->struct_cap;
	child.structs         = e->structs;
	child.vartype_cap     = 32;
	child.vartypes        = minic_alloc(child.vartype_cap * sizeof(minic_vartype_t));
	// Bind parameters
	for (int i = 0; i < argc && i < fn->param_count; ++i) {
		minic_val_t v = minic_val_cast(args[i], fn->param_types[i]);
		minic_var_decl(&child, fn->params[i], fn->param_types[i], v);
		if (fn->param_structs[i][0] != '\0') {
			minic_vartype_set(&child, fn->params[i], fn->param_structs[i]);
		}
	}
	minic_lex_next(&child.lex);
	minic_parse_block(&child);
	*minic_active_mem_used = saved_mem_used;
	return child.return_val;
}

minic_val_t minic_call_fn(void *fn_ptr, minic_val_t *args, int argc) {
	if (fn_ptr == NULL) {
		return minic_val_int(0);
	}
	minic_func_t *fn  = (minic_func_t *)fn_ptr;
	minic_ctx_t  *ctx = fn->ctx;
	if (ctx == NULL) {
		return minic_val_int(0);
	}
	minic_u8    *prev_mem      = minic_active_mem;
	int         *prev_mem_used = minic_active_mem_used;
	minic_env_t *prev_env      = minic_active_env;
	minic_active_mem           = ctx->mem;
	minic_active_mem_used      = &ctx->mem_used;
	minic_active_env           = &ctx->e;
	int         saved_mem_used = ctx->mem_used;
	minic_val_t r              = minic_call(&ctx->e, fn, args, argc);
	ctx->mem_used              = saved_mem_used;
	minic_active_env           = prev_env;
	minic_active_mem           = prev_mem;
	minic_active_mem_used      = prev_mem_used;
	return r;
}

static minic_val_t minic_arith(minic_val_t a, minic_val_t b, char op) {
	// Fast path: both INT, skip double conversion
	if (a.type == MINIC_T_INT && b.type == MINIC_T_INT) {
		int ia = a.i, ib = b.i;
		switch (op) {
		case '+':
			return minic_val_int(ia + ib);
		case '-':
			return minic_val_int(ia - ib);
		case '*':
			return minic_val_int(ia * ib);
		case '/':
			return minic_val_int(ib != 0 ? ia / ib : 0);
		case '%':
			return minic_val_int(ib != 0 ? ia % ib : 0);
		default:
			return minic_val_int(0);
		}
	}

	// Determine result type (widening: int < float < ptr)
	minic_type_t rt;
	if (a.type == MINIC_T_PTR || b.type == MINIC_T_PTR)
		rt = MINIC_T_PTR;
	else if (a.type == MINIC_T_FLOAT || b.type == MINIC_T_FLOAT)
		rt = MINIC_T_FLOAT;
	else
		rt = MINIC_T_INT;

	double da = minic_val_to_d(a);
	double db = minic_val_to_d(b);
	double r;
	switch (op) {
	case '+':
		r = da + db;
		break;
	case '-':
		r = da - db;
		break;
	case '*':
		r = da * db;
		break;
	case '/':
		r = (db != 0.0) ? da / db : 0.0;
		break;
	case '%':
		r = (db != 0.0) ? fmod(da, db) : 0.0;
		break;
	default:
		r = 0.0;
	}
	return minic_val_coerce(r, rt);
}

// primary: '&' IDENT | '*' primary | '-' primary | '++' IDENT | '--' IDENT | NUMBER | CHAR_LIT | IDENT ['[' expr ']'] ['(' args ')'] | '(' expr ')'
static minic_val_t minic_parse_primary(minic_env_t *e) {
	if (e->lex.cur.type == TOK_AMP) {
		minic_lex_next(&e->lex); // Consume '&'
		const char  *aname = e->lex.cur.text;
		minic_arr_t *arr   = minic_arr_get(e, aname);
		minic_val_t  addr;
		if (arr != NULL) {
			addr = minic_val_ptr(&e->arr_data[arr->offset]);
		}
		else if (minic_var_struct(e, aname) != NULL) {
			// Struct var holds real base pointer
			addr = minic_var_get(e, aname);
		}
		else {
			addr = minic_var_addr(e, aname);
		}
		minic_lex_next(&e->lex); // Consume ident
		return addr;
	}
	if (e->lex.cur.type == TOK_STAR) {
		// Dereference: *expr → read through pointer using deref_type
		minic_lex_next(&e->lex); // consume '*'
		minic_val_t pv  = minic_parse_primary(e);
		void       *ptr = minic_val_to_ptr(pv);
		if (ptr == NULL) {
			return minic_val_int(0);
		}
		// Deref_type == MINIC_T_PTR means "pointer to minic_val_t" (interpreter-internal)
		// any other deref_type means a native C scalar at that address
		switch (pv.deref_type) {
		case MINIC_T_INT: {
			int v;
			memcpy(&v, ptr, sizeof(int));
			return minic_val_int(v);
		}
		case MINIC_T_FLOAT: {
			float v;
			memcpy(&v, ptr, sizeof(float));
			return minic_val_float(v);
		}
		default: {
			// MINIC_T_PTR or unknown: read a full minic_val_t (interpreter-internal ptr)
			minic_val_t v;
			memcpy(&v, ptr, sizeof(minic_val_t));
			return v;
		}
		}
	}
	if (e->lex.cur.type == TOK_MINUS) {
		minic_lex_next(&e->lex);
		minic_val_t v = minic_parse_primary(e);
		return minic_val_coerce(-minic_val_to_d(v), v.type);
	}
	if (e->lex.cur.type == TOK_NOT) {
		minic_lex_next(&e->lex);
		minic_val_t v = minic_parse_primary(e);
		return minic_val_int(!minic_val_is_true(v));
	}
	if (e->lex.cur.type == TOK_INC || e->lex.cur.type == TOK_DEC) {
		double delta = MINIC_INC_DELTA(&e->lex);
		minic_lex_next(&e->lex);
		char name[64];
		strncpy(name, e->lex.cur.text, 63);
		minic_lex_next(&e->lex);
		minic_val_t ov = minic_var_get(e, name);
		minic_val_t nv = minic_val_coerce(minic_val_to_d(ov) + delta, ov.type);
		minic_var_set(e, name, nv);
		return nv;
	}
	if (e->lex.cur.type == TOK_NUMBER || e->lex.cur.type == TOK_CHAR_LIT || e->lex.cur.type == TOK_STR_LIT) {
		minic_val_t v = e->lex.cur.val;
		minic_lex_next(&e->lex);
		return v;
	}
	if (e->lex.cur.type == TOK_IDENT) {
		char name[64];
		strncpy(name, e->lex.cur.text, 63);
		minic_lex_next(&e->lex);

		if (strcmp(name, "sizeof") == 0) {
			minic_expect(e, TOK_LPAREN); // checks and consumes '('
			char type_name[64];
			strncpy(type_name, e->lex.cur.text, 63);
			minic_lex_next(&e->lex);     // Consume type name
			minic_expect(e, TOK_RPAREN); // checks and consumes ')'
			minic_struct_def_t *def = minic_struct_get(e, type_name);
			return minic_val_int(def != NULL ? def->size : 0);
		}

		if (e->lex.cur.type == TOK_LBRACKET) {
			minic_lex_next(&e->lex); // Consume '['
			int idx = (int)minic_val_to_d(minic_parse_expr(e));
			minic_lex_next(&e->lex); // Consume ']'
			return minic_arr_elem_get(e, name, idx);
		}

		if (e->lex.cur.type == TOK_LPAREN) {
			minic_lex_next(&e->lex); // Consume '('
			minic_val_t args[MINIC_MAX_PARAMS];
			int         argc = 0;
			while (e->lex.cur.type != TOK_RPAREN && e->lex.cur.type != TOK_EOF) {
				args[argc++] = minic_parse_expr(e);
				if (e->lex.cur.type == TOK_COMMA) {
					minic_lex_next(&e->lex);
				}
			}
			minic_lex_next(&e->lex); // Consume ')'
			minic_func_t *fn = minic_func_get(e, name);
			if (fn != NULL) {
				return minic_call(e, fn, args, argc);
			}
			minic_ext_func_t *ext = minic_ext_func_get(name);
			if (ext != NULL) {
				return minic_dispatch(ext, args, argc);
			}
			fprintf(stderr, "%s:%d: error: unknown function '%s'\n", e->filename, minic_current_line(e), name);
			e->error     = true;
			e->returning = true;
			return minic_val_int(0);
		}

		if (e->lex.cur.type == TOK_DOT) {
			minic_lex_next(&e->lex); // Consume '.'
			char field[64];
			strncpy(field, e->lex.cur.text, 63);
			minic_lex_next(&e->lex);
			minic_struct_def_t *def = minic_var_struct(e, name);
			if (def == NULL) {
				fprintf(stderr, "%s:%d: error: '%s' is not a struct\n", e->filename, minic_current_line(e), name);
				e->error = e->returning = true;
				return minic_val_int(0);
			}
			return minic_struct_field_get(e, name, def, field);
		}

		if (e->lex.cur.type == TOK_ARROW) {
			minic_lex_next(&e->lex); // Consume '->'
			char field[64];
			strncpy(field, e->lex.cur.text, 63);
			minic_lex_next(&e->lex);
			minic_struct_def_t *def = minic_var_struct(e, name);
			if (def == NULL) {
				fprintf(stderr, "%s:%d: error: '%s' is not a struct pointer\n", e->filename, minic_current_line(e), name);
				e->error = e->returning = true;
				return minic_val_int(0);
			}
			minic_val_t base_val  = minic_var_get(e, name);
			void       *base      = minic_val_to_ptr(base_val);
			minic_val_t field_val = minic_struct_field_get_base(e, base, def, field);
			// Handle chained -> or . access (e.g. node->inputs->buffer)
			while ((e->lex.cur.type == TOK_ARROW || e->lex.cur.type == TOK_DOT) && !e->error) {
				int fidx = minic_struct_field_idx(def, field);
				if (fidx < 0 || def->field_struct_names[fidx][0] == '\0')
					break;
				minic_struct_def_t *next_def = minic_struct_get(e, def->field_struct_names[fidx]);
				if (next_def == NULL)
					break;
				minic_lex_next(&e->lex); // Consume '->' or '.'
				strncpy(field, e->lex.cur.text, 63);
				minic_lex_next(&e->lex);
				base      = minic_val_to_ptr(field_val);
				def       = next_def;
				field_val = minic_struct_field_get_base(e, base, def, field);
			}
			if (e->lex.cur.type == TOK_LBRACKET) {
				minic_lex_next(&e->lex); // Consume '['
				int idx = (int)minic_val_to_d(minic_parse_expr(e));
				minic_expect(e, TOK_RBRACKET);
				if (field_val.type == MINIC_T_PTR && field_val.p != NULL) {
					switch (field_val.deref_type) {
					case MINIC_T_FLOAT:
						return minic_val_float(((float *)field_val.p)[idx]);
					case MINIC_T_INT:
						return minic_val_int(((int32_t *)field_val.p)[idx]);
					default:
						return minic_val_ptr(((void **)field_val.p)[idx]);
					}
				}
				return minic_val_int(0);
			}
			return field_val;
		}

		// If not a variable, check if it's a minic function (pass-as-pointer)
		minic_func_t *fn_val = minic_func_get(e, name);
		if (fn_val != NULL) {
			return minic_val_ptr(fn_val);
		}
		// Check for known enum constant
		{
			int ec = minic_enum_const_get(name);
			if (ec >= 0)
				return minic_val_int(ec);
		}
		return minic_var_get(e, name);
	}
	if (e->lex.cur.type == TOK_LPAREN) {
		minic_lex_next(&e->lex);
		minic_val_t v = minic_parse_expr(e);
		minic_lex_next(&e->lex); // Consume ')'
		return v;
	}
	return minic_val_int(0);
}

// term: primary (('*' | '/') primary)*
static minic_val_t minic_parse_term(minic_env_t *e) {
	minic_val_t v = minic_parse_primary(e);
	while (e->lex.cur.type == TOK_STAR || e->lex.cur.type == TOK_SLASH || e->lex.cur.type == TOK_MOD) {
		char op = (e->lex.cur.type == TOK_STAR) ? '*' : (e->lex.cur.type == TOK_SLASH) ? '/' : '%';
		minic_lex_next(&e->lex);
		minic_val_t r = minic_parse_primary(e);
		v             = minic_arith(v, r, op);
	}
	return v;
}

// expr: term (('+' | '-') term)*
static minic_val_t minic_parse_expr(minic_env_t *e) {
	minic_val_t v = minic_parse_term(e);
	while (e->lex.cur.type == TOK_PLUS || e->lex.cur.type == TOK_MINUS) {
		char op = (e->lex.cur.type == TOK_PLUS) ? '+' : '-';
		minic_lex_next(&e->lex);
		minic_val_t r = minic_parse_term(e);
		v             = minic_arith(v, r, op);
	}
	return v;
}

// cmp: expr (('=='|'!='|'<'|'>'|'<='|'>=') expr)?
static minic_val_t minic_parse_cmp(minic_env_t *e) {
	minic_val_t      v  = minic_parse_expr(e);
	minic_tok_type_t op = e->lex.cur.type;
	if (op == TOK_EQ || op == TOK_NEQ || op == TOK_LT || op == TOK_GT || op == TOK_LE || op == TOK_GE) {
		minic_lex_next(&e->lex);
		minic_val_t r = minic_parse_expr(e);
		int         res;
		// Fast path: both ID -- compare uint64_t directly (no double precision loss)
		if (v.type == MINIC_T_ID && r.type == MINIC_T_ID) {
			uint64_t uv = v.u64, ur = r.u64;
			if (op == TOK_EQ)
				res = uv == ur;
			else if (op == TOK_NEQ)
				res = uv != ur;
			else if (op == TOK_LT)
				res = uv < ur;
			else if (op == TOK_GT)
				res = uv > ur;
			else if (op == TOK_LE)
				res = uv <= ur;
			else
				res = uv >= ur;
		}
		else {
			double dv = minic_val_to_d(v);
			double dr = minic_val_to_d(r);
			if (op == TOK_EQ)
				res = dv == dr;
			else if (op == TOK_NEQ)
				res = dv != dr;
			else if (op == TOK_LT)
				res = dv < dr;
			else if (op == TOK_GT)
				res = dv > dr;
			else if (op == TOK_LE)
				res = dv <= dr;
			else
				res = dv >= dr;
		}
		return minic_val_int(res);
	}
	return v;
}

// cond: cmp (('&&' | '||') cmp)*
static minic_val_t minic_parse_cond(minic_env_t *e) {
	minic_val_t v = minic_parse_cmp(e);
	while (e->lex.cur.type == TOK_AND || e->lex.cur.type == TOK_OR) {
		minic_tok_type_t op = e->lex.cur.type;
		minic_lex_next(&e->lex);
		minic_val_t r  = minic_parse_cmp(e);
		int         vi = minic_val_is_true(v);
		int         ri = minic_val_is_true(r);
		v              = minic_val_int(op == TOK_AND ? (vi && ri) : (vi || ri));
	}
	return v;
}

static void minic_skip_block(minic_env_t *e) {
	if (e->lex.cur.type != TOK_LBRACE) {
		// Single-statement body: skip until ';'
		while (e->lex.cur.type != TOK_SEMICOLON && e->lex.cur.type != TOK_EOF)
			minic_lex_next(&e->lex);
		minic_lex_next(&e->lex); // Consume ';'
		return;
	}
	minic_lex_next(&e->lex); // Consume '{'
	int depth = 1;
	while (depth > 0 && e->lex.cur.type != TOK_EOF) {
		if (e->lex.cur.type == TOK_LBRACE)
			depth++;
		if (e->lex.cur.type == TOK_RBRACE)
			depth--;
		minic_lex_next(&e->lex);
	}
}

static minic_type_t minic_tok_to_type(minic_tok_type_t t) {
	switch (t) {
	case TOK_INT:
	case TOK_CHAR:
	case TOK_BOOL:
		return MINIC_T_INT;
	case TOK_FLOAT:
	case TOK_DOUBLE:
		return MINIC_T_FLOAT;
	case TOK_ID:
		return MINIC_T_ID;
	default:
		return MINIC_T_PTR; // void * -> PTR
	}
}

static void minic_parse_stmt(minic_env_t *e) {
	// Skip bare typedef declarations inside function bodies
	if (e->lex.cur.type == TOK_TYPEDEF) {
		while (e->lex.cur.type != TOK_SEMICOLON && e->lex.cur.type != TOK_EOF) {
			minic_lex_next(&e->lex);
		}
		if (e->lex.cur.type == TOK_SEMICOLON) {
			minic_lex_next(&e->lex);
		}
		return;
	}

	if (e->lex.cur.type == TOK_STRUCT) {
		minic_lex_next(&e->lex); // Consume 'struct'
		char sname[64];
		strncpy(sname, e->lex.cur.text, 63);
		minic_lex_next(&e->lex); // Consume struct type name

		bool is_ptr = false;
		if (e->lex.cur.type == TOK_STAR) {
			is_ptr = true;
			minic_lex_next(&e->lex);
		}
		char vname[64];
		strncpy(vname, e->lex.cur.text, 63);
		minic_lex_next(&e->lex);

		minic_struct_def_t *def = minic_struct_get(e, sname);
		if (def == NULL) {
			fprintf(stderr, "%s:%d: error: unknown struct '%s'\n", e->filename, minic_current_line(e), sname);
			e->error = e->returning = true;
			return;
		}
		minic_vartype_set(e, vname, sname);
		if (is_ptr) {
			minic_expect(e, TOK_ASSIGN);
			minic_val_t v = minic_parse_expr(e);
			minic_var_decl(e, vname, MINIC_T_PTR, v);
			minic_expect(e, TOK_SEMICOLON);
		}
		else {
			void *base = minic_alloc(def->field_count * (int)sizeof(minic_val_t));
			memset(base, 0, def->field_count * sizeof(minic_val_t));
			if (e->lex.cur.type == TOK_ASSIGN) {
				minic_lex_next(&e->lex);
				minic_val_t v = minic_parse_expr(e);
				if (v.type == MINIC_T_PTR && v.p)
					memcpy(base, v.p, def->field_count * sizeof(minic_val_t));
			}
			minic_var_decl(e, vname, MINIC_T_PTR, minic_val_ptr(base));
			minic_expect(e, TOK_SEMICOLON);
		}
		return;
	}

	// Typedef'd int name (e.g. from typedef enum): alias_t var = expr;
	if (e->lex.cur.type == TOK_IDENT && minic_is_int_typedef(e->lex.cur.text)) {
		minic_lex_next(&e->lex); // Consume alias name
		char vname[64];
		strncpy(vname, e->lex.cur.text, 63);
		minic_lex_next(&e->lex); // Consume var name
		minic_val_t v = minic_val_int(0);
		if (e->lex.cur.type == TOK_ASSIGN) {
			minic_lex_next(&e->lex);
			v = minic_parse_cond(e);
		}
		minic_var_decl(e, vname, MINIC_T_INT, minic_val_coerce(minic_val_to_d(v), MINIC_T_INT));
		minic_expect(e, TOK_SEMICOLON);
		return;
	}

	// Typedef'd struct name used as variable type: alias_t var; or alias_t *var = ...;
	if (e->lex.cur.type == TOK_IDENT) {
		minic_struct_def_t *def = minic_struct_get(e, e->lex.cur.text);
		if (def != NULL) {
			char sname[64];
			strncpy(sname, e->lex.cur.text, 63);
			minic_lex_next(&e->lex); // Consume alias name
			bool is_ptr = (e->lex.cur.type == TOK_STAR);
			if (is_ptr) {
				minic_lex_next(&e->lex);
			}
			char vname[64];
			strncpy(vname, e->lex.cur.text, 63);
			minic_lex_next(&e->lex);
			minic_vartype_set(e, vname, sname);
			if (is_ptr) {
				minic_val_t v = minic_val_ptr(NULL);
				if (e->lex.cur.type == TOK_ASSIGN) {
					minic_lex_next(&e->lex);
					v = minic_parse_expr(e);
				}
				minic_var_decl(e, vname, MINIC_T_PTR, v);
			}
			else {
				void *base = minic_alloc(def->field_count * (int)sizeof(minic_val_t));
				memset(base, 0, def->field_count * sizeof(minic_val_t));
				if (e->lex.cur.type == TOK_ASSIGN) {
					minic_lex_next(&e->lex);
					minic_val_t v = minic_parse_expr(e);
					if (v.type == MINIC_T_PTR && v.p)
						memcpy(base, v.p, def->field_count * sizeof(minic_val_t));
				}
				minic_var_decl(e, vname, MINIC_T_PTR, minic_val_ptr(base));
			}
			minic_expect(e, TOK_SEMICOLON);
			return;
		}
	}

	if (e->lex.cur.type == TOK_INT || e->lex.cur.type == TOK_FLOAT || e->lex.cur.type == TOK_CHAR || e->lex.cur.type == TOK_DOUBLE ||
	    e->lex.cur.type == TOK_BOOL || e->lex.cur.type == TOK_VOID || e->lex.cur.type == TOK_ID) {
		minic_type_t base_type = minic_tok_to_type(e->lex.cur.type); // Type before '*'
		minic_type_t dtype     = base_type;
		minic_lex_next(&e->lex); // Consume type keyword

		bool is_ptr = false;
		if (e->lex.cur.type == TOK_STAR) {
			is_ptr = true;
			dtype  = MINIC_T_PTR;
			minic_lex_next(&e->lex);
		}
		char name[64];
		strncpy(name, e->lex.cur.text, 63);
		minic_lex_next(&e->lex);

		if (e->lex.cur.type == TOK_LBRACKET) {
			minic_lex_next(&e->lex); // Consume '['
			int count = (int)minic_val_to_d(minic_parse_expr(e));
			minic_expect(e, TOK_RBRACKET);
			minic_arr_decl(e, name, count, is_ptr ? MINIC_T_PTR : dtype);
			minic_expect(e, TOK_SEMICOLON);
			return;
		}

		minic_expect(e, TOK_ASSIGN);
		minic_val_t v = minic_parse_cond(e);
		if (is_ptr) {
			v.type = MINIC_T_PTR;
			if (v.p == NULL) {
				// NULL literal passed as integer 0 — convert
				uintptr_t ua = (uintptr_t)(uint64_t)minic_val_to_d(v);
				v.p          = (ua == 0) ? NULL : (void *)ua;
			}
			// Only stamp the declared element type for native C pointers.
			// Pointers into the active arena (e.g. from &var) use MINIC_T_PTR sentinel
			// to signal that dereferencing reads a full minic_val_t.
			bool in_minic = (v.p != NULL && (minic_u8 *)v.p >= minic_active_mem && (minic_u8 *)v.p < minic_active_mem + MINIC_MEM_SIZE);
			if (!in_minic) {
				v.deref_type = base_type;
			}
		}
		minic_var_decl(e, name, dtype, v);
		minic_expect(e, TOK_SEMICOLON);
		return;
	}

	if (e->lex.cur.type == TOK_RETURN) {
		minic_lex_next(&e->lex);
		minic_val_t v = minic_parse_cond(e);
		e->return_val = v;
		e->returning  = true;
		minic_expect(e, TOK_SEMICOLON);
		return;
	}

	if (e->lex.cur.type == TOK_IDENT) {
		char name[64];
		strncpy(name, e->lex.cur.text, 63);
		minic_lex_next(&e->lex);

		// Unknown opaque type followed by '*' + ident: local pointer declaration.
		// e.g. ui_handle_t *h; or MyType *p = create_p();
		if (e->lex.cur.type == TOK_STAR) {
			minic_lex_next(&e->lex); // Consume '*'
			char vname[64];
			strncpy(vname, e->lex.cur.text, 63);
			minic_lex_next(&e->lex); // Consume var name
			minic_val_t v = minic_val_ptr(NULL);
			if (e->lex.cur.type == TOK_ASSIGN) {
				minic_lex_next(&e->lex);
				v = minic_parse_expr(e);
			}
			minic_var_decl(e, vname, MINIC_T_PTR, v);
			minic_expect(e, TOK_SEMICOLON);
			return;
		}

		if (e->lex.cur.type == TOK_DOT || e->lex.cur.type == TOK_ARROW) {
			bool is_arrow = (e->lex.cur.type == TOK_ARROW);
			minic_lex_next(&e->lex);
			char field[64];
			strncpy(field, e->lex.cur.text, 63);
			minic_lex_next(&e->lex);
			minic_struct_def_t *def = minic_var_struct(e, name);
			if (def == NULL) {
				fprintf(stderr, "%s:%d: error: '%s' is not a struct%s\n", e->filename, minic_current_line(e), name, is_arrow ? " pointer" : "");
				e->error = e->returning = true;
				return;
			}
			void *base;
			if (is_arrow) {
				minic_val_t base_val = minic_var_get(e, name);
				base                 = minic_val_to_ptr(base_val);
			}
			else {
				minic_val_t base_val = minic_var_get(e, name);
				base                 = minic_val_to_ptr(base_val);
			}
			if (e->lex.cur.type == TOK_LBRACKET) {
				minic_lex_next(&e->lex);
				int idx = (int)minic_val_to_d(minic_parse_expr(e));
				minic_expect(e, TOK_RBRACKET);
				minic_expect(e, TOK_ASSIGN);
				minic_val_t v         = minic_parse_expr(e);
				minic_val_t field_val = minic_struct_field_get_base(e, base, def, field);
				if (field_val.type == MINIC_T_PTR && field_val.p != NULL) {
					switch (field_val.deref_type) {
					case MINIC_T_FLOAT:
						((float *)field_val.p)[idx] = (float)minic_val_to_d(v);
						break;
					case MINIC_T_INT:
						((int32_t *)field_val.p)[idx] = (int32_t)minic_val_to_d(v);
						break;
					default:
						((void **)field_val.p)[idx] = minic_val_to_ptr(v);
						break;
					}
				}
			}
			else {
				minic_expect(e, TOK_ASSIGN);
				minic_val_t v = minic_parse_expr(e);
				minic_struct_field_set_base(e, base, def, field, v);
			}
			minic_expect(e, TOK_SEMICOLON);
			return;
		}

		if (e->lex.cur.type == TOK_LPAREN) {
			minic_lex_next(&e->lex);
			minic_val_t args[MINIC_MAX_PARAMS];
			int         argc = 0;
			while (e->lex.cur.type != TOK_RPAREN && e->lex.cur.type != TOK_EOF) {
				args[argc++] = minic_parse_expr(e);
				if (e->lex.cur.type == TOK_COMMA) {
					minic_lex_next(&e->lex);
				}
			}
			minic_expect(e, TOK_RPAREN);
			minic_func_t *fn = minic_func_get(e, name);
			if (fn != NULL) {
				minic_call(e, fn, args, argc);
			}
			else {
				minic_ext_func_t *ext = minic_ext_func_get(name);
				if (ext != NULL) {
					minic_dispatch(ext, args, argc);
				}
				else {
					fprintf(stderr, "%s:%d: error: unknown function '%s'\n", e->filename, minic_current_line(e), name);
					e->error     = true;
					e->returning = true;
				}
			}
			minic_expect(e, TOK_SEMICOLON);
			return;
		}

		if (e->lex.cur.type == TOK_INC || e->lex.cur.type == TOK_DEC) {
			double delta = MINIC_INC_DELTA(&e->lex);
			minic_lex_next(&e->lex);
			minic_val_t ov = minic_var_get(e, name);
			minic_var_set(e, name, minic_val_coerce(minic_val_to_d(ov) + delta, ov.type));
			minic_expect(e, TOK_SEMICOLON);
			return;
		}

		if (e->lex.cur.type == TOK_PLUS_ASSIGN || e->lex.cur.type == TOK_MINUS_ASSIGN || e->lex.cur.type == TOK_MUL_ASSIGN ||
		    e->lex.cur.type == TOK_DIV_ASSIGN || e->lex.cur.type == TOK_MOD_ASSIGN) {
			minic_tok_type_t op = e->lex.cur.type;
			minic_lex_next(&e->lex);
			minic_val_t dv = minic_parse_expr(e);
			minic_val_t ov = minic_var_get(e, name);
			double      a = minic_val_to_d(ov), b = minic_val_to_d(dv);
			double      r = (op == TOK_PLUS_ASSIGN)    ? a + b
			                : (op == TOK_MINUS_ASSIGN) ? a - b
			                : (op == TOK_MUL_ASSIGN)   ? a * b
			                : (op == TOK_DIV_ASSIGN)   ? (b != 0.0 ? a / b : 0.0)
			                                           : (b != 0.0 ? fmod(a, b) : 0.0);
			minic_var_set(e, name, minic_val_coerce(r, ov.type));
			minic_expect(e, TOK_SEMICOLON);
			return;
		}

		if (e->lex.cur.type == TOK_LBRACKET) {
			minic_lex_next(&e->lex);
			int idx = (int)minic_val_to_d(minic_parse_expr(e));
			minic_expect(e, TOK_RBRACKET);
			minic_expect(e, TOK_ASSIGN);
			minic_val_t v = minic_parse_expr(e);
			minic_arr_elem_set(e, name, idx, v);
			minic_expect(e, TOK_SEMICOLON);
			return;
		}

		minic_expect(e, TOK_ASSIGN);
		minic_val_t v = minic_parse_expr(e);
		minic_var_set(e, name, v);
		minic_expect(e, TOK_SEMICOLON);
		return;
	}

	if (e->lex.cur.type == TOK_IF) {
		minic_lex_next(&e->lex);
		minic_expect(e, TOK_LPAREN);
		int taken = minic_val_is_true(minic_parse_cond(e));
		minic_expect(e, TOK_RPAREN);
		if (taken) {
			minic_parse_block(e);
		}
		else {
			minic_skip_block(e);
			taken = 0;
		}
		while (!taken && e->lex.cur.type == TOK_ELSE) {
			minic_lex_next(&e->lex);
			if (e->lex.cur.type == TOK_IF) {
				minic_lex_next(&e->lex);
				minic_expect(e, TOK_LPAREN);
				int c2 = minic_val_is_true(minic_parse_cond(e));
				minic_expect(e, TOK_RPAREN);
				if (c2) {
					minic_parse_block(e);
					taken = 1;
				}
				else {
					minic_skip_block(e);
				}
			}
			else {
				minic_parse_block(e);
				taken = 1;
			}
		}
		while (e->lex.cur.type == TOK_ELSE) {
			minic_lex_next(&e->lex);
			if (e->lex.cur.type == TOK_IF) {
				minic_lex_next(&e->lex);
				minic_expect(e, TOK_LPAREN);
				minic_parse_cond(e);
				minic_expect(e, TOK_RPAREN);
			}
			minic_skip_block(e);
		}
		return;
	}

	if (e->lex.cur.type == TOK_FOR) {
		minic_lex_next(&e->lex);
		minic_expect(e, TOK_LPAREN);

		if (e->lex.cur.type == TOK_INT || e->lex.cur.type == TOK_FLOAT || e->lex.cur.type == TOK_CHAR || e->lex.cur.type == TOK_DOUBLE ||
		    e->lex.cur.type == TOK_BOOL || e->lex.cur.type == TOK_VOID || e->lex.cur.type == TOK_ID) {
			minic_lex_next(&e->lex);
		}
		{
			char iname[64];
			strncpy(iname, e->lex.cur.text, 63);
			minic_lex_next(&e->lex);
			minic_expect(e, TOK_ASSIGN);
			minic_val_t iv = minic_parse_expr(e);
			minic_var_set(e, iname, iv);
		}
		int cond_pos = e->lex.pos;
		minic_lex_next(&e->lex); // Consume ';'

		int incr_pos, body_pos;
		{
			minic_lexer_t tmp = {0};
			tmp.src           = e->lex.src;
			tmp.tokens        = e->lex.tokens;
			tmp.token_count   = e->lex.token_count;
			tmp.pos           = cond_pos;
			minic_lex_next(&tmp);
			while (tmp.cur.type != TOK_SEMICOLON && tmp.cur.type != TOK_EOF) {
				minic_lex_next(&tmp);
			}
			incr_pos = tmp.pos;
			minic_lex_next(&tmp);
			while (tmp.cur.type != TOK_RPAREN && tmp.cur.type != TOK_EOF) {
				minic_lex_next(&tmp);
			}
			minic_lex_next(&tmp);
			body_pos = tmp.pos - 1;
		}

		for (;;) {
			e->continuing = false;
			e->lex.pos    = cond_pos;
			minic_lex_next(&e->lex);
			int cond = minic_val_is_true(minic_parse_cond(e));
			if (!cond || e->returning || e->breaking) {
				e->lex.pos = body_pos;
				minic_lex_next(&e->lex);
				minic_skip_block(e);
				e->breaking = false;
				break;
			}
			e->lex.pos = body_pos;
			minic_lex_next(&e->lex);
			minic_parse_block(e);
			if (e->returning || e->breaking) {
				e->breaking = false;
				break;
			}
			e->lex.pos = incr_pos;
			minic_lex_next(&e->lex);
			if (e->lex.cur.type == TOK_INC || e->lex.cur.type == TOK_DEC) {
				double delta = MINIC_INC_DELTA(&e->lex);
				minic_lex_next(&e->lex);
				char iname[64];
				strncpy(iname, e->lex.cur.text, 63);
				minic_val_t *vp = minic_var_get_ptr(e, iname);
				if (vp && vp->type == MINIC_T_INT) {
					vp->i += (int)delta;
				}
				else {
					minic_val_t ov = *vp;
					minic_var_set(e, iname, minic_val_coerce(minic_val_to_d(ov) + delta, ov.type));
				}
			}
			else if (e->lex.cur.type == TOK_IDENT) {
				char iname[64];
				strncpy(iname, e->lex.cur.text, 63);
				minic_lex_next(&e->lex);
				if (e->lex.cur.type == TOK_INC || e->lex.cur.type == TOK_DEC) {
					double       delta = MINIC_INC_DELTA(&e->lex);
					minic_val_t *vp    = minic_var_get_ptr(e, iname);
					if (vp && vp->type == MINIC_T_INT) {
						vp->i += (int)delta;
					}
					else {
						minic_val_t ov = minic_var_get(e, iname);
						minic_var_set(e, iname, minic_val_coerce(minic_val_to_d(ov) + delta, ov.type));
					}
				}
				else if (e->lex.cur.type == TOK_PLUS_ASSIGN || e->lex.cur.type == TOK_MINUS_ASSIGN || e->lex.cur.type == TOK_MUL_ASSIGN ||
				         e->lex.cur.type == TOK_DIV_ASSIGN || e->lex.cur.type == TOK_MOD_ASSIGN) {
					minic_tok_type_t op = e->lex.cur.type;
					minic_lex_next(&e->lex);
					minic_val_t dv = minic_parse_expr(e);
					minic_val_t ov = minic_var_get(e, iname);
					double      a = minic_val_to_d(ov), b = minic_val_to_d(dv);
					double      r = (op == TOK_PLUS_ASSIGN)    ? a + b
					                : (op == TOK_MINUS_ASSIGN) ? a - b
					                : (op == TOK_MUL_ASSIGN)   ? a * b
					                : (op == TOK_DIV_ASSIGN)   ? (b != 0.0 ? a / b : 0.0)
					                                           : (b != 0.0 ? fmod(a, b) : 0.0);
					minic_var_set(e, iname, minic_val_coerce(r, ov.type));
				}
				else if (e->lex.cur.type == TOK_ASSIGN) {
					minic_lex_next(&e->lex);
					minic_val_t v = minic_parse_expr(e);
					minic_var_set(e, iname, v);
				}
			}
		}
		return;
	}

	if (e->lex.cur.type == TOK_STAR) {
		// Pointer write: *expr = val;  or  *expr += val;  or  *expr++;  etc.
		minic_lex_next(&e->lex);
		minic_val_t pv  = minic_parse_primary(e);
		void       *ptr = minic_val_to_ptr(pv);
		if (e->lex.cur.type == TOK_INC || e->lex.cur.type == TOK_DEC) {
			double delta = MINIC_INC_DELTA(&e->lex);
			minic_lex_next(&e->lex);
			if (ptr != NULL) {
				switch (pv.deref_type) {
				case MINIC_T_INT: {
					int ov = 0;
					memcpy(&ov, ptr, sizeof(int));
					ov += (int)delta;
					memcpy(ptr, &ov, sizeof(int));
					break;
				}
				case MINIC_T_FLOAT: {
					float ov = 0.0f;
					memcpy(&ov, ptr, sizeof(float));
					ov += (float)delta;
					memcpy(ptr, &ov, sizeof(float));
					break;
				}
				default: {
					minic_val_t ov;
					memcpy(&ov, ptr, sizeof(minic_val_t));
					minic_val_t nv = minic_val_coerce(minic_val_to_d(ov) + delta, ov.type);
					memcpy(ptr, &nv, sizeof(minic_val_t));
					break;
				}
				}
			}
			minic_expect(e, TOK_SEMICOLON);
			return;
		}
		minic_tok_type_t op = e->lex.cur.type; // TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN, TOK_MUL_ASSIGN, TOK_DIV_ASSIGN, TOK_MOD_ASSIGN
		minic_lex_next(&e->lex);
		minic_val_t v = minic_parse_expr(e);
		if (ptr != NULL) {
			switch (pv.deref_type) {
			case MINIC_T_INT: {
				int ov = 0;
				memcpy(&ov, ptr, sizeof(int));
				double b  = minic_val_to_d(v);
				double r  = (op == TOK_PLUS_ASSIGN)    ? ov + b
				            : (op == TOK_MINUS_ASSIGN) ? ov - b
				            : (op == TOK_MUL_ASSIGN)   ? ov * b
				            : (op == TOK_DIV_ASSIGN)   ? (b != 0.0 ? ov / b : 0.0)
				            : (op == TOK_MOD_ASSIGN)   ? (b != 0.0 ? fmod(ov, b) : 0.0)
				                                       : b;
				int    iv = (int)r;
				memcpy(ptr, &iv, sizeof(int));
				break;
			}
			case MINIC_T_FLOAT: {
				float ov = 0.0f;
				memcpy(&ov, ptr, sizeof(float));
				double b  = minic_val_to_d(v);
				double r  = (op == TOK_PLUS_ASSIGN)    ? ov + b
				            : (op == TOK_MINUS_ASSIGN) ? ov - b
				            : (op == TOK_MUL_ASSIGN)   ? ov * b
				            : (op == TOK_DIV_ASSIGN)   ? (b != 0.0 ? ov / b : 0.0)
				            : (op == TOK_MOD_ASSIGN)   ? (b != 0.0 ? fmod(ov, b) : 0.0)
				                                       : b;
				float  fv = (float)r;
				memcpy(ptr, &fv, sizeof(float));
				break;
			}
			default: {
				minic_val_t ov;
				memcpy(&ov, ptr, sizeof(minic_val_t));
				double      a = minic_val_to_d(ov), b = minic_val_to_d(v);
				double      r  = (op == TOK_PLUS_ASSIGN)    ? a + b
				                 : (op == TOK_MINUS_ASSIGN) ? a - b
				                 : (op == TOK_MUL_ASSIGN)   ? a * b
				                 : (op == TOK_DIV_ASSIGN)   ? (b != 0.0 ? a / b : 0.0)
				                 : (op == TOK_MOD_ASSIGN)   ? (b != 0.0 ? fmod(a, b) : 0.0)
				                                            : b;
				minic_val_t nv = (op == TOK_ASSIGN) ? v : minic_val_coerce(r, ov.type);
				memcpy(ptr, &nv, sizeof(minic_val_t));
				break;
			}
			}
		}
		minic_expect(e, TOK_SEMICOLON);
		return;
	}

	if (e->lex.cur.type == TOK_BREAK) {
		minic_lex_next(&e->lex);
		minic_expect(e, TOK_SEMICOLON);
		e->breaking = true;
		return;
	}
	if (e->lex.cur.type == TOK_CONTINUE) {
		minic_lex_next(&e->lex);
		minic_expect(e, TOK_SEMICOLON);
		e->continuing = true;
		return;
	}
	if (e->lex.cur.type == TOK_WHILE) {
		minic_lex_next(&e->lex);
		int cond_pos = e->lex.pos - 1;
		for (;;) {
			e->continuing = false;
			e->lex.pos    = cond_pos;
			minic_lex_next(&e->lex);
			minic_lex_next(&e->lex); // Consume '('
			int cond = minic_val_is_true(minic_parse_cond(e));
			minic_lex_next(&e->lex); // Consume ')'
			if (!cond || e->returning || e->breaking) {
				minic_skip_block(e);
				e->breaking = false;
				break;
			}
			minic_parse_block(e);
			if (e->breaking) {
				e->breaking = false;
				break;
			}
		}
		return;
	}

	{
		char msg[128];
		snprintf(msg, sizeof(msg), "unexpected token at start of statement");
		minic_error(e, msg);
	}
}

static void minic_parse_block(minic_env_t *e) {
	int saved_var_count     = e->var_count;
	int saved_vartype_count = e->vartype_count;
	if (e->lex.cur.type != TOK_LBRACE) {
		// Single-statement body without braces
		minic_parse_stmt(e);
		e->var_count     = saved_var_count;
		e->vartype_count = saved_vartype_count;
		return;
	}
	minic_expect(e, TOK_LBRACE);
	while (e->lex.cur.type != TOK_RBRACE && e->lex.cur.type != TOK_EOF && !e->returning && !e->breaking && !e->continuing && !e->error) {
		minic_parse_stmt(e);
	}
	if (e->lex.cur.type != TOK_RBRACE) {
		int depth = 1;
		while (depth > 0 && e->lex.cur.type != TOK_EOF) {
			if (e->lex.cur.type == TOK_LBRACE)
				depth++;
			if (e->lex.cur.type == TOK_RBRACE)
				depth--;
			minic_lex_next(&e->lex);
		}
	}
	else {
		minic_lex_next(&e->lex); // Consume '}'
	}
	e->var_count     = saved_var_count;
	e->vartype_count = saved_vartype_count;
}

// ██████╗ ██╗   ██╗███╗   ██╗
// ██╔══██╗██║   ██║████╗  ██║
// ██████╔╝██║   ██║██╔██╗ ██║
// ██╔══██╗██║   ██║██║╚██╗██║
// ██║  ██║╚██████╔╝██║ ╚████║
// ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝

// Consume a type keyword (int, float, char, double, bool, void), return true if found
static bool minic_lex_type(minic_env_t *e) {
	if (e->lex.cur.type == TOK_INT || e->lex.cur.type == TOK_FLOAT || e->lex.cur.type == TOK_CHAR || e->lex.cur.type == TOK_DOUBLE ||
	    e->lex.cur.type == TOK_BOOL || e->lex.cur.type == TOK_VOID || e->lex.cur.type == TOK_ID) {
		minic_lex_next(&e->lex);
		return true;
	}
	if (e->lex.cur.type == TOK_STRUCT) {
		minic_lex_next(&e->lex); // Consume 'struct'
		minic_lex_next(&e->lex); // Consume struct name
		return true;
	}
	// Typedef'd struct name used as a type specifier
	if (e->lex.cur.type == TOK_IDENT && minic_struct_get(e, e->lex.cur.text) != NULL) {
		minic_lex_next(&e->lex);
		return true;
	}
	// Typedef'd int name (e.g. from typedef enum) used as a type specifier
	if (e->lex.cur.type == TOK_IDENT && minic_is_int_typedef(e->lex.cur.text)) {
		minic_lex_next(&e->lex);
		return true;
	}
	return false;
}

// Zero pass: scan for struct definitions or typedef struct
static void minic_register_structs(minic_env_t *e) {
	minic_lexer_t l = {0};
	l.src           = e->lex.src;
	l.pos           = 0;
	minic_lex_next(&l);
	while (l.cur.type != TOK_EOF) {
		bool is_typedef = (l.cur.type == TOK_TYPEDEF);
		if (is_typedef) {
			minic_lex_next(&l); // Consume 'typedef'
		}
		if (l.cur.type == TOK_ENUM) {
			minic_lex_next(&l); // Consume 'enum'
			if (l.cur.type == TOK_IDENT) {
				minic_lex_next(&l); // Optional tag name
			}
			if (l.cur.type != TOK_LBRACE) {
				continue;
			}
			minic_lex_next(&l); // Consume '{'
			int val = 0;
			while (l.cur.type != TOK_RBRACE && l.cur.type != TOK_EOF) {
				if (l.cur.type == TOK_IDENT && minic_enum_const_count < MINIC_MAX_ENUM_CONSTS) {
					strncpy(minic_enum_consts[minic_enum_const_count].name, l.cur.text, 63);
					minic_enum_consts[minic_enum_const_count].value = val;
					minic_enum_const_count++;
					minic_lex_next(&l);
					if (l.cur.type == TOK_ASSIGN) {
						minic_lex_next(&l); // Consume '='
						val                                                 = (int)minic_val_to_d(l.cur.val);
						minic_enum_consts[minic_enum_const_count - 1].value = val;
						minic_lex_next(&l); // Consume number
					}
					val++;
				}
				else {
					minic_lex_next(&l);
				}
				if (l.cur.type == TOK_COMMA) {
					minic_lex_next(&l);
				}
			}
			if (l.cur.type == TOK_RBRACE) {
				minic_lex_next(&l);
			}
			if (is_typedef && l.cur.type == TOK_IDENT && minic_int_typedef_count < MINIC_MAX_INT_TYPEDEFS) {
				strncpy(minic_int_typedefs[minic_int_typedef_count++].name, l.cur.text, 63);
				minic_lex_next(&l);
			}
			while (l.cur.type != TOK_SEMICOLON && l.cur.type != TOK_EOF) {
				minic_lex_next(&l);
			}
			if (l.cur.type == TOK_SEMICOLON) {
				minic_lex_next(&l);
			}
			continue;
		}

		if (l.cur.type != TOK_STRUCT) {
			minic_lex_next(&l);
			continue;
		}
		minic_lex_next(&l); // Consume 'struct'

		// Optional struct tag name
		char struct_name[64] = "";
		if (l.cur.type == TOK_IDENT) {
			strncpy(struct_name, l.cur.text, 63);
			minic_lex_next(&l); // Consume struct name
		}

		if (l.cur.type != TOK_LBRACE) {
			continue; // Forward decl or typedef-without-body — skip
		}

		if (e->struct_count >= e->struct_cap) {
			break;
		}
		minic_struct_def_t *def = &e->structs[e->struct_count];
		strncpy(def->name, struct_name, 63);
		def->field_count = 0;
		minic_lex_next(&l); // Consume '{'

		while (l.cur.type != TOK_RBRACE && l.cur.type != TOK_EOF) {
			if (l.cur.type == TOK_STRUCT) {
				minic_lex_next(&l);
				minic_lex_next(&l);
			}
			else if (l.cur.type == TOK_INT || l.cur.type == TOK_FLOAT || l.cur.type == TOK_CHAR || l.cur.type == TOK_DOUBLE || l.cur.type == TOK_BOOL) {
				minic_lex_next(&l);
			}
			else if (l.cur.type == TOK_IDENT) {
				minic_lex_next(&l); // Typedef'd field type — consume and fall through to field name
			}
			else {
				minic_lex_next(&l);
				continue;
			}
			if (l.cur.type == TOK_STAR) {
				minic_lex_next(&l);
			}
			if (l.cur.type == TOK_IDENT && def->field_count < MINIC_MAX_STRUCT_FIELDS) {
				strncpy(def->fields[def->field_count], l.cur.text, 63);
				def->field_hashes[def->field_count] = minic_name_hash(l.cur.text);
				def->field_count++;
				minic_lex_next(&l);
			}
			while (l.cur.type != TOK_SEMICOLON && l.cur.type != TOK_RBRACE && l.cur.type != TOK_EOF) {
				minic_lex_next(&l);
			}
			if (l.cur.type == TOK_SEMICOLON) {
				minic_lex_next(&l);
			}
		}
		if (l.cur.type == TOK_RBRACE) {
			minic_lex_next(&l);
		}

		if (is_typedef && l.cur.type == TOK_IDENT) {
			// typedef struct [Name] { ... } alias;
			char alias[64];
			strncpy(alias, l.cur.text, 63);
			minic_lex_next(&l); // Consume alias name
			if (struct_name[0]) {
				// Register under struct tag name first
				e->struct_count++;
				// Also register a copy under the alias name
				if (e->struct_count < e->struct_cap) {
					minic_struct_def_t *adef = &e->structs[e->struct_count];
					*adef                    = *def;
					strncpy(adef->name, alias, 63);
					e->struct_count++;
				}
			}
			else {
				// Anonymous struct: name it after the alias
				strncpy(def->name, alias, 63);
				e->struct_count++;
			}
		}
		else {
			// Plain struct definition: must have a tag name to be usable
			if (struct_name[0]) {
				e->struct_count++;
			}
		}

		while (l.cur.type != TOK_SEMICOLON && l.cur.type != TOK_EOF) {
			minic_lex_next(&l);
		}
		if (l.cur.type == TOK_SEMICOLON) {
			minic_lex_next(&l);
		}
	}
}

// First pass: register all function definitions, stop at 'main'
static void minic_register_funcs(minic_env_t *e) {
	while (e->lex.cur.type != TOK_EOF) {
		// Remember the return-type token before consuming it
		minic_type_t ret_type = minic_tok_to_type(e->lex.cur.type);

		char decl_struct[64] = "";
		if (e->lex.cur.type == TOK_IDENT && minic_struct_get(e, e->lex.cur.text) != NULL) {
			strncpy(decl_struct, e->lex.cur.text, 63);
		}
		if (!minic_lex_type(e)) {
			if (e->lex.cur.type == TOK_IDENT) {
				// Unknown typedef'd type (e.g. ui_handle_t) — consume it and
				// treat as an opaque pointer type so the variable/function is
				// registered properly.
				strncpy(decl_struct, e->lex.cur.text, 63);
				minic_lex_next(&e->lex);
				ret_type = MINIC_T_PTR;
			}
			else {
				minic_lex_next(&e->lex);
				continue;
			}
		}
		if (e->lex.cur.type == TOK_STAR) {
			ret_type = MINIC_T_PTR;
			minic_lex_next(&e->lex);
		}
		if (e->lex.cur.type != TOK_IDENT) {
			continue;
		}
		char fname[64];
		strncpy(fname, e->lex.cur.text, 63);
		minic_lex_next(&e->lex);

		if (e->lex.cur.type != TOK_LPAREN) {
			// Global variable declaration: type [*] ident [= expr] ;
			if (decl_struct[0] != '\0') {
				minic_vartype_set(e, fname, decl_struct);
			}
			minic_val_t init = minic_val_coerce(0.0, ret_type);
			if (e->lex.cur.type == TOK_ASSIGN) {
				minic_lex_next(&e->lex); // Consume '='
				e->decl_type = ret_type;
				init         = minic_parse_expr(e);
			}
			minic_var_decl(e, fname, ret_type, init);
			while (e->lex.cur.type != TOK_SEMICOLON && e->lex.cur.type != TOK_EOF) {
				minic_lex_next(&e->lex);
			}
			if (e->lex.cur.type == TOK_SEMICOLON) {
				minic_lex_next(&e->lex);
			}
			continue;
		}
		minic_lex_next(&e->lex); // Consume '('

		minic_func_t fn = {0};
		strncpy(fn.name, fname, 63);
		fn.ret_type = ret_type;

		while (e->lex.cur.type != TOK_RPAREN && e->lex.cur.type != TOK_EOF) {
			char         pstruct[64] = "";
			minic_type_t ptype       = MINIC_T_INT;
			if (e->lex.cur.type == TOK_STRUCT) {
				minic_lex_next(&e->lex);
				strncpy(pstruct, e->lex.cur.text, 63);
				minic_lex_next(&e->lex);
				ptype = MINIC_T_PTR;
			}
			else {
				// Capture typedef'd struct name before consuming the type token
				if (e->lex.cur.type == TOK_IDENT && minic_struct_get(e, e->lex.cur.text) != NULL) {
					strncpy(pstruct, e->lex.cur.text, 63);
				}
				ptype = minic_tok_to_type(e->lex.cur.type);
				minic_lex_type(e); // consume type keyword
			}
			if (e->lex.cur.type == TOK_STAR) {
				ptype = MINIC_T_PTR;
				minic_lex_next(&e->lex);
			}
			if (e->lex.cur.type == TOK_IDENT && fn.param_count < MINIC_MAX_PARAMS) {
				int pi = fn.param_count++;
				strncpy(fn.params[pi], e->lex.cur.text, 63);
				strncpy(fn.param_structs[pi], pstruct, 63);
				fn.param_types[pi] = ptype;
				minic_lex_next(&e->lex);
			}
			if (e->lex.cur.type == TOK_COMMA) {
				minic_lex_next(&e->lex);
			}
		}
		minic_lex_next(&e->lex); // Consume ')'

		fn.body_pos = e->lex.pos - 1;

		if (strcmp(fname, "main") == 0) {
			break;
		}
		if (e->func_count < e->func_cap) {
			e->funcs[e->func_count++] = fn;
		}

		// Skip function body
		int depth = 1;
		minic_lex_next(&e->lex); // Consume '{'
		while (depth > 0 && e->lex.cur.type != TOK_EOF) {
			if (e->lex.cur.type == TOK_LBRACE)
				depth++;
			if (e->lex.cur.type == TOK_RBRACE)
				depth--;
			minic_lex_next(&e->lex);
		}
	}
}

minic_ctx_t *minic_ctx_create(const char *src) {
	minic_register_builtins();

	minic_ctx_t *ctx = (minic_ctx_t *)malloc(sizeof(minic_ctx_t));
	memset(ctx, 0, sizeof(minic_ctx_t));
	ctx->mem      = (minic_u8 *)malloc(MINIC_MEM_SIZE);
	ctx->mem_used = 0;
	int src_len   = (int)strlen(src);
	ctx->src_copy = (char *)malloc(src_len + 1);
	memcpy(ctx->src_copy, src, src_len + 1);

	minic_active_mem      = ctx->mem;
	minic_active_mem_used = &ctx->mem_used;

	int var_cap      = 256;
	int arr_cap      = 32;
	int arr_data_cap = 512;
	int func_cap     = 32;
	int struct_cap   = MINIC_MAX_STRUCTS;

	minic_env_t *e    = &ctx->e;
	e->lex.src        = ctx->src_copy;
	e->lex.pos        = 0;
	e->filename       = "<script>";
	e->var_cap        = var_cap;
	e->vars           = minic_alloc(var_cap * sizeof(minic_var_t));
	e->arr_cap        = arr_cap;
	e->arrs           = minic_alloc(arr_cap * sizeof(minic_arr_t));
	e->arr_data       = minic_alloc(arr_data_cap * sizeof(minic_val_t));
	e->arr_data_used  = minic_alloc(sizeof(int));
	*e->arr_data_used = 0;
	e->func_cap       = func_cap;
	e->funcs          = minic_alloc(func_cap * sizeof(minic_func_t));
	e->struct_cap     = struct_cap;
	e->structs        = minic_alloc(struct_cap * sizeof(minic_struct_def_t));
	e->vartype_cap    = 64;
	e->vartypes       = minic_alloc(e->vartype_cap * sizeof(minic_vartype_t));

	for (int i = 0; i < minic_global_struct_count && e->struct_count < e->struct_cap; ++i) {
		minic_struct_def_t *dst = &e->structs[e->struct_count++];
		strncpy(dst->name, minic_global_structs[i].name, 63);
		dst->field_count       = minic_global_structs[i].field_count;
		dst->size              = minic_global_structs[i].size;
		dst->has_native_layout = minic_global_structs[i].has_native_layout;
		for (int j = 0; j < dst->field_count; j++) {
			strncpy(dst->fields[j], minic_global_structs[i].fields[j], 63);
			strncpy(dst->field_struct_names[j], minic_global_structs[i].field_struct_names[j], 63);
			dst->field_hashes[j]       = minic_name_hash(minic_global_structs[i].fields[j]);
			dst->field_offsets[j]      = minic_global_structs[i].field_offsets[j];
			dst->field_native_types[j] = minic_global_structs[i].field_native_types[j];
			dst->field_deref_types[j]  = minic_global_structs[i].field_deref_types[j];
		}
	}

	minic_register_structs(e);
	minic_lex_next(&e->lex);
	minic_register_funcs(e);
	for (int i = 0; i < e->func_count; ++i) {
		e->funcs[i].ctx = ctx;
	}

	// M15: compile function bodies to bytecode
	for (int i = 0; i < e->func_count; ++i) {
		minic_func_t *fn = &e->funcs[i];
		fn->proto = vm_compile_body(e->lex.src, fn->body_pos, e, fn);
	}

	if (e->lex.cur.type == TOK_RPAREN) {
		minic_lex_next(&e->lex);
	}

	return ctx;
}

void minic_ctx_run(minic_ctx_t *ctx) {
	if (!ctx)
		return;

	minic_u8    *prev_mem      = minic_active_mem;
	int         *prev_mem_used = minic_active_mem_used;
	minic_env_t *prev_env      = minic_active_env;

	minic_active_mem      = ctx->mem;
	minic_active_mem_used = &ctx->mem_used;
	minic_env_t *e        = &ctx->e;
	minic_active_env      = e;

	if (e->lex.cur.type != TOK_EOF) {
		minic_parse_block(e);
	}

	minic_active_env      = prev_env;
	minic_active_mem      = prev_mem;
	minic_active_mem_used = prev_mem_used;

	ctx->result = e->error ? -1.0f : (float)minic_val_to_d(e->return_val);
}

minic_ctx_t *minic_eval_named(const char *src, const char *filename) {
	minic_ctx_t *ctx = minic_ctx_create(src);
	minic_ctx_run(ctx);
	(void)filename;
	return ctx;
}

minic_ctx_t *minic_eval(const char *src) {
	return minic_eval_named(src, "<script>");
}

void minic_ctx_free(minic_ctx_t *ctx) {
	if (ctx) {
		for (int i = 0; i < ctx->e.func_count; i++) {
			if (ctx->e.funcs[i].body_tokens != NULL) {
				free(ctx->e.funcs[i].body_tokens);
			}
		}
		free(ctx->mem);
		free(ctx->src_copy);
		free(ctx);
	}
}

float minic_ctx_result(minic_ctx_t *ctx) {
	return ctx ? ctx->result : -1.0f;
}

void *minic_ctx_get_fn(minic_ctx_t *ctx, const char *name) {
	if (!ctx || !name)
		return NULL;
	for (int i = 0; i < ctx->e.func_count; i++) {
		if (strcmp(ctx->e.funcs[i].name, name) == 0) {
			return &ctx->e.funcs[i];
		}
	}
	return NULL;
}

minic_val_t minic_ctx_call_fn(minic_ctx_t *ctx, void *fn_ptr, minic_val_t *args, int argc) {
	if (!ctx || !fn_ptr) {
		return minic_val_int(0);
	}
	minic_u8    *prev_mem      = minic_active_mem;
	int         *prev_mem_used = minic_active_mem_used;
	minic_env_t *prev_env      = minic_active_env;
	minic_active_mem           = ctx->mem;
	minic_active_mem_used      = &ctx->mem_used;
	minic_active_env           = &ctx->e;
	int         saved_mem_used = ctx->mem_used;
	minic_val_t r              = minic_call(&ctx->e, (minic_func_t *)fn_ptr, args, argc);
	ctx->mem_used              = saved_mem_used;
	minic_active_env           = prev_env;
	minic_active_mem           = prev_mem;
	minic_active_mem_used      = prev_mem_used;
	return r;
}

// ============================================================
// M15: BYTECODE VM — Register-based virtual machine
// ============================================================

// --- Opcodes ---
typedef enum {
	// Constants
	OP_CONSTANT,    // R(A) = const_pool[BX]
	OP_CONST_INT,   // R(A) = (minic_val_t){.type=MINIC_T_INT, .i = (int)(int16_t)BX}
	OP_CONST_ZERO,  // R(A) = int(0)
	OP_CONST_ONE,   // R(A) = int(1)
	OP_CONST_FZERO, // R(A) = float(0.0)
	OP_CONST_NULL,  // R(A) = ptr(NULL)
	// Arithmetic
	OP_ADD, // R(A) = R(B) + R(C)
	OP_SUB, // R(A) = R(B) - R(C)
	OP_MUL, // R(A) = R(B) * R(C)
	OP_DIV, // R(A) = R(B) / R(C)
	OP_MOD, // R(A) = R(B) % R(C)
	OP_NEG, // R(A) = -R(B)
	OP_NOT, // R(A) = !R(B)
	// Comparison
	OP_EQ,  // R(A) = (R(B) == R(C)) ? 1 : 0
	OP_NEQ, // R(A) = (R(B) != R(C)) ? 1 : 0
	OP_LT,  // R(A) = (R(B) <  R(C)) ? 1 : 0
	OP_GT,  // R(A) = (R(B) >  R(C)) ? 1 : 0
	OP_LE,  // R(A) = (R(B) <= R(C)) ? 1 : 0
	OP_GE,  // R(A) = (R(B) >= R(C)) ? 1 : 0
	// Control flow
	OP_JMP,       // pc += BX (signed)
	OP_JMP_FALSE, // if (!R(A)) pc += BX
	OP_JMP_TRUE,  // if (R(A)) pc += BX
	OP_LOOP,      // pc -= BX (backward jump)
	// Load/Store globals (by index)
	OP_LOAD_GLOBAL,  // R(A) = vm_globals[BX]
	OP_STORE_GLOBAL, // vm_globals[BX] = R(A)
	// Function calls
	OP_CALL,        // call user func R(A), argc=B, result in C
	OP_CALL_EXT,    // call ext func index BX, argc from A..A+B-1
	OP_RETURN,      // return R(A)
	OP_RETURN_VOID, // return
	// Misc
	OP_HALT,
	OP_INC, // R(A).i++
	OP_DEC, // R(A).i--
		OP_MOV, // R(A) = R(B)
	// Compound assign
	OP_ADD_ASSIGN, // R(A) += R(B)
	OP_SUB_ASSIGN, // R(A) -= R(B)
	OP_MUL_ASSIGN, // R(A) *= R(B)
	OP_DIV_ASSIGN, // R(A) /= R(B)
	OP_MOD_ASSIGN, // R(A) %= R(B)
	// Array element access (uses vm_arrs table + vm_arr_data)
	OP_LOAD_ARR,  // R(A) = vm_arr_data[vm_arrs[BX].offset + R(B)]; next word = arr_idx
	OP_STORE_ARR, // vm_arr_data[vm_arrs[BX].offset + R(B)] = R(A); next word = arr_idx
	OP_COUNT
} minic_opcode_t;

// --- Instruction encoding ---
// Format: [8:op][8:A][8:B][8:C] or [8:op][8:A][16:BX]
#define VM_OP(i)                 ((i) >> 24)
#define VM_A(i)                  (((i) >> 16) & 0xFF)
#define VM_B(i)                  (((i) >> 8) & 0xFF)
#define VM_C(i)                  ((i) & 0xFF)
#define VM_BX(i)                 ((int16_t)((i) & 0xFFFF))
#define VM_MAKE_ABC(op, a, b, c) (((uint32_t)(op) << 24) | ((uint32_t)(a) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(c))
#define VM_MAKE_ABX(op, a, bx)   (((uint32_t)(op) << 24) | ((uint32_t)(a) << 16) | ((uint32_t)((bx) & 0xFFFF)))

// --- VM call frame ---
#define VM_MAX_FRAMES 256
#define VM_STACK_SIZE (64 * 1024)

typedef struct {
	minic_val_t   *regs; // points into vm_stack
	uint32_t      *code;
	int            pc;
	int            reg_base;
	int            num_regs;
	int            return_reg;
	minic_proto_t *proto;
} minic_frame_t;

// --- VM globals table ---
#define VM_MAX_GLOBALS 256
static int vm_active = 0;
static minic_val_t vm_globals[VM_MAX_GLOBALS];
static char        vm_global_names[VM_MAX_GLOBALS][64];
static uint32_t    vm_global_hashes[VM_MAX_GLOBALS];
static int         vm_global_count = 0;
static minic_val_t vm_stack_reentry[VM_STACK_SIZE];
static minic_val_t vm_saved_frames_buf[(VM_MAX_FRAMES * sizeof(minic_frame_t)) / sizeof(minic_val_t) + 1];
static minic_val_t vm_saved_globals_buf[VM_MAX_GLOBALS];
static int         vm_global_find_or_add(const char *name) {
    uint32_t h = minic_name_hash(name);
    for (int i = 0; i < vm_global_count; i++) {
        if (vm_global_hashes[i] == h && strcmp(vm_global_names[i], name) == 0)
            return i;
    }
    if (vm_global_count >= VM_MAX_GLOBALS)
        return 0;
    int idx = vm_global_count++;
    strncpy(vm_global_names[idx], name, 63);
    vm_global_names[idx][63] = '\0';
    vm_global_hashes[idx]    = h;
    vm_globals[idx]          = minic_val_int(0);
    return idx;
}

// --- VM array table ---
#define VM_MAX_ARRS 64
typedef struct {
    char         name[64];
    uint32_t     name_hash;
    int          offset; // index into arr_data
    int          count;
    minic_type_t elem_type;
} vm_arr_meta_t;
static vm_arr_meta_t vm_arrs[VM_MAX_ARRS];
static int           vm_arr_count = 0;
static minic_val_t  *vm_arr_data = NULL; // pointer to env->arr_data, set at sync

static int vm_arr_find_or_add(const char *name) {
    uint32_t h = minic_name_hash(name);
    for (int i = 0; i < vm_arr_count; i++) {
        if (vm_arrs[i].name_hash == h && strcmp(vm_arrs[i].name, name) == 0)
            return i;
    }
    if (vm_arr_count >= VM_MAX_ARRS)
        return 0;
    int idx = vm_arr_count++;
    strncpy(vm_arrs[idx].name, name, 63);
    vm_arrs[idx].name[63]  = '\0';
    vm_arrs[idx].name_hash = h;
    vm_arrs[idx].offset    = 0;
    vm_arrs[idx].count     = 0;
    vm_arrs[idx].elem_type = MINIC_T_INT;
    return idx;
}

// Sync env->arrs into vm_arrs (called before VM execution)
static void vm_arrs_load(minic_env_t *e) {
    vm_arr_data = e->arr_data;
    for (int i = 0; i < vm_arr_count; i++) {
        uint32_t h = vm_arrs[i].name_hash;
        // Search in env->arrs (function-local) and global_env->arrs
        minic_arr_t *a = minic_arr_get(e, vm_arrs[i].name);
        if (a) {
            vm_arrs[i].offset    = a->offset;
            vm_arrs[i].count     = a->count;
            vm_arrs[i].elem_type = a->elem_type;
        }
    }
}

// Sync context vars -> vm_globals (before VM execution)
static void vm_globals_load(minic_env_t *e) {
    for (int i = 0; i < vm_global_count; i++) {
        uint32_t h = vm_global_hashes[i];
        for (int j = e->var_count - 1; j >= 0; --j) {
            if (e->vars[j].name_hash == h && strcmp(e->vars[j].name, vm_global_names[i]) == 0) {
                vm_globals[i] = e->vars[j].val;
                break;
            }
        }
        // Don't touch vm_globals[i] for globals not in this context
    }
}

// Sync vm_globals -> context vars (after VM execution)
// Only writes back globals that the current context owns (has in its var table)
static void vm_globals_store(minic_env_t *e) {
    for (int i = 0; i < vm_global_count; i++) {
        uint32_t h = vm_global_hashes[i];
        for (int j = e->var_count - 1; j >= 0; --j) {
            if (e->vars[j].name_hash == h && strcmp(e->vars[j].name, vm_global_names[i]) == 0) {
                e->vars[j].val = vm_globals[i];
                break;
            }
        }
    }
}

// --- VM helpers ---
static inline minic_val_t vm_val_to_d_val(minic_val_t v) {
	switch (v.type) {
	case MINIC_T_INT:
		return minic_val_float((float)v.i);
	case MINIC_T_FLOAT:
		return v;
	default:
		return minic_val_float(0.0f);
	}
}


// --- VM execution engine ---
static minic_val_t minic_vm_exec_inner(minic_ctx_t *ctx, minic_proto_t *proto, minic_val_t *args, int argc) {
	static minic_val_t   vm_stack[VM_STACK_SIZE];
	static minic_frame_t vm_frames[VM_MAX_FRAMES];
	// Re-entry guard: save outer VM state when native callbacks call back into minic
	minic_frame_t *saved_frames = NULL;
	minic_val_t   *saved_globals = NULL;
	bool is_reentrant = (vm_active > 0);
	if (is_reentrant) {
		saved_frames  = (minic_frame_t *)vm_saved_frames_buf;
		saved_globals = vm_saved_globals_buf;
		memcpy(saved_frames, vm_frames, VM_MAX_FRAMES * sizeof(minic_frame_t));
		memcpy(saved_globals, vm_globals, VM_MAX_GLOBALS * sizeof(minic_val_t));
	}
	vm_active++;

	minic_val_t _vm_result = minic_val_int(0);
	int                  frame_top = 0;
	minic_val_t         *active_stack = is_reentrant ? vm_stack_reentry : vm_stack;

	// Init frame 0
	minic_frame_t *frame = &vm_frames[0];
	frame->proto         = proto;
	frame->code          = proto->code;
	frame->pc            = 0;
	frame->reg_base      = 0;
	frame->num_regs      = proto->num_regs;
	frame->regs          = active_stack;
	frame->return_reg    = 0;

	// Bind arguments
	for (int i = 0; i < argc && i < frame->num_regs; i++) {
		frame->regs[i] = args[i];
	}

	for (;;) {
		uint32_t inst = frame->code[frame->pc++];
		uint32_t op   = VM_OP(inst);
		int      a    = VM_A(inst);
		int      b    = VM_B(inst);
		int      c    = VM_C(inst);
		int      bx   = VM_BX(inst);

		minic_val_t *ra = &frame->regs[a];
		minic_val_t *rb = &frame->regs[b];
		minic_val_t *rc = &frame->regs[c];

		switch (op) {
		// Constants
		case OP_CONSTANT:
			*ra = frame->proto->constants[bx];
			break;
		case OP_CONST_INT:
			*ra = minic_val_int((int)(int16_t)bx);
			break;
		case OP_CONST_ZERO:
			*ra = minic_val_int(0);
			break;
		case OP_CONST_ONE:
			*ra = minic_val_int(1);
			break;
		case OP_CONST_FZERO:
			*ra = minic_val_float(0.0f);
			break;
		case OP_CONST_NULL:
			*ra = minic_val_ptr(NULL);
			break;

		// Arithmetic — type-specialized INT fast path
		case OP_ADD:
			if (ra->type == MINIC_T_INT && rb->type == MINIC_T_INT) {
				ra->type = MINIC_T_INT;
				ra->i    = rb->i + rc->i;
			}
			else if (ra->type == MINIC_T_FLOAT || rb->type == MINIC_T_FLOAT || rc->type == MINIC_T_FLOAT) {
				ra->type = MINIC_T_FLOAT;
				ra->f    = (float)(minic_val_to_d(*rb) + minic_val_to_d(*rc));
			}
			else {
				*ra = minic_arith(*rb, *rc, '+');
			}
			break;
		case OP_SUB:
			if (rb->type == MINIC_T_INT && rc->type == MINIC_T_INT) {
				ra->type = MINIC_T_INT;
				ra->i    = rb->i - rc->i;
			}
			else {
				*ra = minic_arith(*rb, *rc, '-');
			}
			break;
		case OP_MUL:
			if (rb->type == MINIC_T_INT && rc->type == MINIC_T_INT) {
				ra->type = MINIC_T_INT;
				ra->i    = rb->i * rc->i;
			}
			else {
				*ra = minic_arith(*rb, *rc, '*');
			}
			break;
		case OP_DIV:
			if (rb->type == MINIC_T_INT && rc->type == MINIC_T_INT && rc->i != 0) {
				ra->type = MINIC_T_INT;
				ra->i    = rb->i / rc->i;
			}
			else {
				*ra = minic_arith(*rb, *rc, '/');
			}
			break;
		case OP_MOD:
			if (rb->type == MINIC_T_INT && rc->type == MINIC_T_INT && rc->i != 0) {
				ra->type = MINIC_T_INT;
				ra->i    = rb->i % rc->i;
			}
			else {
				*ra = minic_arith(*rb, *rc, '%');
			}
			break;
		case OP_NEG:
			*ra = minic_val_coerce(-minic_val_to_d(*rb), rb->type);
			break;
		case OP_NOT:
			*ra = minic_val_int(!minic_val_is_true(*rb));
			break;

		// Comparison
		case OP_EQ:
			*ra = minic_val_int(minic_val_to_d(*rb) == minic_val_to_d(*rc) ? 1 : 0);
			break;
		case OP_NEQ:
			*ra = minic_val_int(minic_val_to_d(*rb) != minic_val_to_d(*rc) ? 1 : 0);
			break;
		case OP_LT:
			*ra = minic_val_int(minic_val_to_d(*rb) < minic_val_to_d(*rc) ? 1 : 0);
			break;
		case OP_GT:
			*ra = minic_val_int(minic_val_to_d(*rb) > minic_val_to_d(*rc) ? 1 : 0);
			break;
		case OP_LE:
			*ra = minic_val_int(minic_val_to_d(*rb) <= minic_val_to_d(*rc) ? 1 : 0);
			break;
		case OP_GE:
			*ra = minic_val_int(minic_val_to_d(*rb) >= minic_val_to_d(*rc) ? 1 : 0);
			break;

		// Control flow
		case OP_JMP:
			frame->pc += bx;
			break;
		case OP_JMP_FALSE:
			if (!minic_val_is_true(*ra))
				frame->pc += bx;
			break;
		case OP_JMP_TRUE:
			if (minic_val_is_true(*ra))
				frame->pc += bx;
			break;
		case OP_LOOP:
			frame->pc -= bx;
			break;

		// Load/Store globals
		case OP_LOAD_GLOBAL:
			*ra = vm_globals[bx];
			break;
		case OP_STORE_GLOBAL:
			vm_globals[bx] = *ra;
			break;

		// Inc/Dec
		case OP_INC:
			ra->i++;
			break;
		case OP_DEC:
			ra->i--;
			break;
			case OP_MOV:
				*ra = *rb;
				break;

		// Compound assign
		case OP_ADD_ASSIGN:
			if (ra->type == MINIC_T_INT && rb->type == MINIC_T_INT) {
				ra->i += rb->i;
			}
			else {
				*ra = minic_arith(*ra, *rb, '+');
			}
			break;
		case OP_SUB_ASSIGN:
			if (ra->type == MINIC_T_INT && rb->type == MINIC_T_INT) {
				ra->i -= rb->i;
			}
			else {
				*ra = minic_arith(*ra, *rb, '-');
			}
			break;
		case OP_MUL_ASSIGN:
			if (ra->type == MINIC_T_INT && rb->type == MINIC_T_INT) {
				ra->i *= rb->i;
			}
			else {
				*ra = minic_arith(*ra, *rb, '*');
			}
			break;
		case OP_DIV_ASSIGN:
			if (ra->type == MINIC_T_INT && rb->type == MINIC_T_INT && rb->i != 0) {
				ra->i /= rb->i;
			}
			else {
				*ra = minic_arith(*ra, *rb, '/');
			}
			break;
		case OP_MOD_ASSIGN:
			if (ra->type == MINIC_T_INT && rb->type == MINIC_T_INT && rb->i != 0) {
				ra->i %= rb->i;
			}
			else {
				*ra = minic_arith(*ra, *rb, '%');
			}
			break;

		// Array element access
		case OP_LOAD_ARR: {
			// A=dest, BX=arr_table_idx, next word=idx_reg
			int arr_idx_reg = (int)frame->code[frame->pc++];
			int elem_idx    = (int)minic_val_to_d(frame->regs[arr_idx_reg]);
			vm_arr_meta_t *am = &vm_arrs[bx];
			if (vm_arr_data && elem_idx >= 0 && elem_idx < am->count) {
				*ra = vm_arr_data[am->offset + elem_idx];
			}
			else {
				*ra = minic_val_int(0);
			}
			break;
		}
		case OP_STORE_ARR: {
			// A=val_reg, BX=arr_table_idx, next word=idx_reg
			int arr_idx_reg = (int)frame->code[frame->pc++];
			int elem_idx    = (int)minic_val_to_d(frame->regs[arr_idx_reg]);
			vm_arr_meta_t *am = &vm_arrs[bx];
			if (vm_arr_data && elem_idx >= 0 && elem_idx < am->count) {
				vm_arr_data[am->offset + elem_idx] = minic_val_cast(*ra, am->elem_type);
			}
			break;
		}

			// Function calls
			case OP_CALL_EXT: {
				// A=dest, BX=(argc<<8)|arg_base, next word=const_idx
				int ext_argc  = (bx >> 8) & 0xFF;
				int ext_base  = bx & 0xFF;
				int ext_cidx  = (int)frame->code[frame->pc++];
				minic_ext_func_t *ef = (minic_ext_func_t *)frame->proto->constants[ext_cidx].p;
				minic_val_t call_args[MINIC_MAX_PARAMS];
				for (int ai = 0; ai < ext_argc && ai < MINIC_MAX_PARAMS; ai++)
					call_args[ai] = frame->regs[ext_base + ai];
				frame->regs[a] = minic_dispatch(ef, call_args, ext_argc);
				break;
			}
			case OP_CALL: {
				// A=dest, B=func_index, C=argc, next word=arg_base
				int call_base2 = (int)frame->code[frame->pc++];
				minic_func_t *fn = &ctx->e.funcs[b];
				if (fn->proto != NULL && frame_top + 1 < VM_MAX_FRAMES) {
					// Push a new frame — stay in the same VM execution
					int new_top = frame_top + 1;
					minic_frame_t *nf = &vm_frames[new_top];
					nf->proto     = fn->proto;
					nf->code       = fn->proto->code;
					nf->pc         = 0;
					nf->reg_base   = frame->reg_base + frame->num_regs;
					nf->num_regs   = fn->proto->num_regs;
					nf->regs       = &active_stack[nf->reg_base];
					nf->return_reg = a;
					for (int ai = 0; ai < c && ai < nf->num_regs; ai++)
						nf->regs[ai] = frame->regs[call_base2 + ai];
					frame_top = new_top;
					frame = nf;
				} else {
					minic_val_t call_args[MINIC_MAX_PARAMS];
					for (int ai = 0; ai < c && ai < MINIC_MAX_PARAMS; ai++)
						call_args[ai] = frame->regs[call_base2 + ai];
					minic_val_t r = minic_call(&ctx->e, fn, call_args, c);
					frame->regs[a] = r;
				}
				break;
			}
				case OP_RETURN: {
					minic_val_t ret = *ra;
					if (frame_top > 0) {
						int ret_reg = frame->return_reg;
						frame_top--;
						frame = &vm_frames[frame_top];
						frame->regs[ret_reg] = ret;
					}
					else {
						_vm_result = ret; goto vm_cleanup;
					}
					break;
				}
		case OP_RETURN_VOID:
			if (frame_top > 0) {
				frame_top--;
				frame = &vm_frames[frame_top];
			}
			else {
				_vm_result = minic_val_int(0); goto vm_cleanup;
			}
			break;
		case OP_HALT:
			if (frame_top > 0) {
				// Return from inner frame back to caller
				int ret_reg = vm_frames[frame_top].return_reg;
				frame_top--;
				frame = &vm_frames[frame_top];
				frame->regs[ret_reg] = minic_val_int(0);
				break;
			}
			_vm_result = *ra; goto vm_cleanup;
		default:
			_vm_result = minic_val_int(0); goto vm_cleanup;
		}
		continue;
vm_cleanup:
		vm_active--;
		if (is_reentrant) {
			memcpy(vm_frames, saved_frames, VM_MAX_FRAMES * sizeof(minic_frame_t));
			memcpy(vm_globals, saved_globals, VM_MAX_GLOBALS * sizeof(minic_val_t));
		}
		return _vm_result;
	}
}


static minic_val_t minic_vm_exec(minic_ctx_t *ctx, minic_proto_t *proto, minic_val_t *args, int argc) {
	bool is_reentry = (vm_active > 0);
	if (!is_reentry) vm_globals_load(&ctx->e);
	if (!is_reentry) vm_arrs_load(&ctx->e);
	minic_val_t r = minic_vm_exec_inner(ctx, proto, args, argc);
	if (!is_reentry) vm_globals_store(&ctx->e);
	return r;
}

#include "minic_vm_compiler.c"

