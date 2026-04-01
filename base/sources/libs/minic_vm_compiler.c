// minic_vm_compiler.c — Bytecode compiler for Minic
// Include at end of minic.c to access internal types

typedef struct {
	minic_lexer_t lex;
	const char   *filename;
	minic_env_t  *env;
	int           free_reg;
	int           max_reg;
	struct {
		char     name[64];
		uint32_t hash;
		int      slot;
	} locals[256];
	int         local_count;
	minic_val_t constants[4096];
	int         const_count;
	uint32_t    code[65536];
	int         code_count;
	bool        error;
	int         break_addrs[64];
	int         break_count;
	int         continue_addrs[64];
	int         continue_count;
	int         loop_top; // pc of loop header for continue
} vc_t;

static void vc_emit(vc_t *c, uint32_t inst) {
	if (c->code_count < 65536)
		c->code[c->code_count++] = inst;
}
static int vc_emit_jmp(vc_t *c, uint32_t inst) {
	int a = c->code_count;
	vc_emit(c, inst);
	return a;
}
static void vc_patch(vc_t *c, int addr, uint32_t inst) {
	if (addr >= 0 && addr < c->code_count)
		c->code[addr] = inst;
}
static int vc_const(vc_t *c, minic_val_t v) {
	for (int i = 0; i < c->const_count; i++)
		if (c->constants[i].type == v.type &&
		    ((v.type == MINIC_T_INT && c->constants[i].i == v.i) ||
		     (v.type == MINIC_T_FLOAT && c->constants[i].f == v.f) ||
		     (v.type == MINIC_T_PTR && c->constants[i].p == v.p)))
			return i;
	if (c->const_count >= 4096)
		return 0;
	int i           = c->const_count++;
	c->constants[i] = v;
	return i;
}
static int vc_reg(vc_t *c) {
	int r = c->free_reg++;
	if (r > c->max_reg)
		c->max_reg = r;
	return r;
}
static void vc_free(vc_t *c, int r) {
	if (r == c->free_reg - 1)
		c->free_reg--;
}
static int vc_find_local(vc_t *c, const char *name) {
	uint32_t h = minic_name_hash(name);
	for (int i = c->local_count - 1; i >= 0; i--)
		if (c->locals[i].hash == h && strcmp(c->locals[i].name, name) == 0)
			return c->locals[i].slot;
	return -1;
}
static int vc_decl_local(vc_t *c, const char *name) {
	// Re-use existing slot if already declared (e.g. in same scope)
	int existing = vc_find_local(c, name);
	if (existing >= 0)
		return existing;
	if (c->local_count >= 256)
		return c->free_reg;
	int s = vc_reg(c);
	strncpy(c->locals[c->local_count].name, name, 63);
	c->locals[c->local_count].name[63] = '\0';
	c->locals[c->local_count].hash     = minic_name_hash(name);
	c->locals[c->local_count].slot     = s;
	c->local_count++;
	return s;
}
typedef struct {
	int lc;
	int fr;
} vc_scope_t;
static vc_scope_t vc_scope_save(vc_t *c) {
	return (vc_scope_t){c->local_count, c->free_reg};
}
static void vc_scope_restore(vc_t *c, vc_scope_t s) {
	c->local_count = s.lc;
	c->free_reg    = s.fr;
}
static bool vc_check(vc_t *c, minic_tok_type_t t) {
	return c->lex.cur.type == t;
}
static void vc_expect(vc_t *c, minic_tok_type_t t) {
	if (vc_check(c, t))
		minic_lex_next(&c->lex);
	else
		c->error = true;
}
static void vc_advance(vc_t *c) {
	minic_lex_next(&c->lex);
}

// --- Forward ---
static void vc_block(vc_t *c);
static void vc_stmt(vc_t *c);
static int  vc_expr(vc_t *c);

// --- Expression compiler ---
static int vc_primary(vc_t *c) {
	if (c->error)
		return vc_reg(c);
	minic_token_t tok = c->lex.cur;
	switch (tok.type) {
	case TOK_NUMBER: {
		vc_advance(c);
		int d = vc_reg(c);
		if (tok.val.type == MINIC_T_INT) {
			int v = tok.val.i;
			if (v == 0)
				vc_emit(c, VM_MAKE_ABC(OP_CONST_ZERO, d, 0, 0));
			else if (v == 1)
				vc_emit(c, VM_MAKE_ABC(OP_CONST_ONE, d, 0, 0));
			else if (v >= -32768 && v <= 32767)
				vc_emit(c, VM_MAKE_ABX(OP_CONST_INT, d, v));
			else
				vc_emit(c, VM_MAKE_ABX(OP_CONSTANT, d, vc_const(c, tok.val)));
		}
		else {
			if (tok.val.f == 0.0f)
				vc_emit(c, VM_MAKE_ABC(OP_CONST_FZERO, d, 0, 0));
			else
				vc_emit(c, VM_MAKE_ABX(OP_CONSTANT, d, vc_const(c, tok.val)));
		}
		return d;
	}
	case TOK_STR_LIT: {
		vc_advance(c);
		int d = vc_reg(c);
		vc_emit(c, VM_MAKE_ABX(OP_CONSTANT, d, vc_const(c, tok.val)));
		return d;
	}
	case TOK_CHAR_LIT: {
		vc_advance(c);
		int d = vc_reg(c);
		vc_emit(c, VM_MAKE_ABX(OP_CONST_INT, d, tok.val.i));
		return d;
	}
	case TOK_IDENT: {
		char name[64];
		strncpy(name, tok.text, 63);
		name[63] = '\0';
		vc_advance(c);

		// Function call: name(args)
		if (vc_check(c, TOK_LPAREN)) {
			vc_expect(c, TOK_LPAREN);
			// Compile args into consecutive registers
			int arg_base = c->free_reg;
			int argc     = 0;
			if (!vc_check(c, TOK_RPAREN)) {
				c->free_reg++; // reserve arg slot 0
				int ar = vc_expr(c);
				if (ar != arg_base + argc)
					vc_emit(c, VM_MAKE_ABC(OP_MOV, arg_base + argc, ar, 0));
				c->free_reg = arg_base + argc + 1; // collapse free_reg
				argc++;
				while (vc_check(c, TOK_COMMA)) {
					vc_advance(c);
					c->free_reg++; // reserve next arg slot
					ar = vc_expr(c);
					if (ar != arg_base + argc)
						vc_emit(c, VM_MAKE_ABC(OP_MOV, arg_base + argc, ar, 0));
					c->free_reg = arg_base + argc + 1;
					argc++;
				}
			}
			if (arg_base + argc > c->max_reg)
				c->max_reg = arg_base + argc;
			vc_expect(c, TOK_RPAREN);

			int dest = vc_reg(c);

			// Check ext func
			minic_ext_func_t *ef = minic_ext_func_get(name);
			if (ef) {
				// Store ext func pointer in constant pool
				minic_val_t fv;
				fv.type       = MINIC_T_PTR;
				fv.deref_type = MINIC_T_PTR;
				fv.p          = ef;
				int ci = vc_const(c, fv);
				// OP_CALL_EXT: A=dest, B=argc, C=arg_base, BX=const_idx
				// Use ABC format: A=dest, B=argc, C=arg_base
				// But we need const_idx too... Use ABX: A=dest, BX has packed info
				// Better: emit two instructions - load constant, then call
				// Simplest: pack as OP_CALL_EXT A=dest B=argc C=arg_base
				// and store const_idx in next word... no, fixed width.
				// Use: A=dest, BX = (argc << 8) | const_idx - limits to 256 consts/args
				// Actually let's use: A=dest, B=argc, C=arg_base
				// And store the ext func ptr in a separate OP_CONSTANT before
				// Or: just encode as A=dest, B=argc, BX is too small
				// Simplest workable approach: OP_CALL_EXT A=dest, B=argc, C=arg_base
				// The ext func is looked up by name hash stored as constant
				// Let's use a different encoding:
				// OP_CALL_EXT ABX: A=dest, upper BX bits = argc (8), lower = arg_base (8)
				// Then next instruction has the constant index
				// Even simpler: just pack into one instruction
				// A=dest(8), B=argc(8), BX has arg_base and const_idx
				// Let's just use: A=dest, B=argc, C=arg_base, and store
				// the ef pointer at a known location in constants
				// VM reads constants[bx] for the ef ptr
				vc_emit(c, VM_MAKE_ABX(OP_CALL_EXT, dest, (argc << 8) | arg_base));
				// Patch: actually need const_idx. Let's encode differently.
				// Use ABC: A=dest, B=argc, C=arg_base, then emit OP_CONSTANT as next inst
				// VM pops next instruction as const_idx. No that's hacky.
				//
				// Better approach: emit OP_CONSTANT to load ef into a temp reg,
				// then OP_CALL_EXT references that register.
				// Or: define OP_CALL_EXT as: A=dest, BX = const_pool_idx
				// with argc and arg_base encoded separately.
				//
				// Final decision: use 2 instructions
				// OP_CALL_EXT A=dest, B=argc, C=arg_base
				// followed by OP_CONSTANT 0, 0, const_idx (VM reads next inst)
				// No. Let me just redefine the encoding:
				//
				// OP_CALL_EXT: [op:8][dest:8][argc:8][arg_base:8]
				// The ext func ptr is stored in the constant pool.
				// We emit the const_idx as the *next* code word (a 32-bit literal).
				// The VM reads code[pc++] after the instruction.
				// This is the "inline constant" pattern.
				vc_emit(c, (uint32_t)ci); // inline constant index
				c->free_reg = dest + 1;   // preserve dest, free below it
				return dest;
			}

			// User function call
			if (c->env) {
				for (int i = 0; i < c->env->func_count; i++) {
					if (strcmp(c->env->funcs[i].name, name) == 0) {
						// OP_CALL: A=dest, B=func_index, C=argc
						// args are at consecutive regs from arg_base
						vc_emit(c, VM_MAKE_ABC(OP_CALL, dest, i, argc));
						vc_emit(c, (uint32_t)arg_base); // inline: arg_base
						c->free_reg = dest + 1;
						return dest;
					}
				}
			}
			c->error = true;
			c->free_reg = arg_base;
			return dest;
		}

		// Array subscript: name[expr]
		if (vc_check(c, TOK_LBRACKET)) {
			vc_advance(c);
			int idx_reg = vc_expr(c);
			vc_expect(c, TOK_RBRACKET);
			int d = vc_reg(c);
			int ai = vm_arr_find_or_add(name);
			vc_emit(c, VM_MAKE_ABX(OP_LOAD_ARR, d, ai));
			vc_emit(c, (uint32_t)idx_reg); // inline: index register
			vc_free(c, idx_reg);
			return d;
		}

		// Variable load
		int slot = vc_find_local(c, name);
		if (slot >= 0) {
			// Local: just return the slot (no instruction needed, it's already there)
			return slot;
		}
		// Function reference: pass user function as pointer value
		if (c->env) {
			for (int i = 0; i < c->env->func_count; i++) {
				if (strcmp(c->env->funcs[i].name, name) == 0) {
					minic_val_t fv;
					fv.type       = MINIC_T_PTR;
					fv.deref_type = MINIC_T_PTR;
					fv.p          = &c->env->funcs[i];
					int ci = vc_const(c, fv);
					int d  = vc_reg(c);
					vc_emit(c, VM_MAKE_ABX(OP_CONSTANT, d, ci));
					return d;
				}
			}
		}
		// Enum constant
		{
			int ec = minic_enum_const_get(name);
			if (ec >= 0) {
				int d = vc_reg(c);
				if (ec == 0)
					vc_emit(c, VM_MAKE_ABC(OP_CONST_ZERO, d, 0, 0));
				else if (ec == 1)
					vc_emit(c, VM_MAKE_ABC(OP_CONST_ONE, d, 0, 0));
				else if (ec >= -32768 && ec <= 32767)
					vc_emit(c, VM_MAKE_ABX(OP_CONST_INT, d, ec));
				else
					vc_emit(c, VM_MAKE_ABX(OP_CONSTANT, d, vc_const(c, minic_val_int(ec))));
				return d;
			}
		}
		// Global
		int gi = vm_global_find_or_add(name);
		int d  = vc_reg(c);
		vc_emit(c, VM_MAKE_ABX(OP_LOAD_GLOBAL, d, gi));
		return d;
	}
	case TOK_LPAREN: {
		vc_advance(c);
		// Cast: (int)expr, (float)expr, etc.
		if (vc_check(c, TOK_INT) || vc_check(c, TOK_FLOAT) || vc_check(c, TOK_CHAR) ||
		    vc_check(c, TOK_DOUBLE) || vc_check(c, TOK_BOOL) || vc_check(c, TOK_VOID)) {
			vc_advance(c);  // skip type keyword
			vc_expect(c, TOK_RPAREN);
			// Compile the sub-expression (cast is a no-op at VM level;
			// type coercion happens naturally via minic_val_to_d)
			return vc_primary(c);
		}
		int r = vc_expr(c);
		vc_expect(c, TOK_RPAREN);
		return r;
	}
	case TOK_MINUS: {
		vc_advance(c);
		int b = vc_primary(c);
		int d = vc_reg(c);
		vc_emit(c, VM_MAKE_ABC(OP_NEG, d, b, 0));
		vc_free(c, b);
		return d;
	}
	case TOK_NOT: {
		vc_advance(c);
		int b = vc_primary(c);
		int d = vc_reg(c);
		vc_emit(c, VM_MAKE_ABC(OP_NOT, d, b, 0));
		vc_free(c, b);
		return d;
	}
	default:
		c->error = true;
		return vc_reg(c);
	}
}

static int vc_unary(vc_t *c) {
	return vc_primary(c);
}

static int vc_term(vc_t *c) {
	int l = vc_unary(c);
	while (vc_check(c, TOK_STAR) || vc_check(c, TOK_SLASH) || vc_check(c, TOK_MOD)) {
		minic_opcode_t op = vc_check(c, TOK_STAR) ? OP_MUL : vc_check(c, TOK_SLASH) ? OP_DIV : OP_MOD;
		vc_advance(c);
		int r = vc_unary(c);
		int d = vc_reg(c);
		vc_emit(c, VM_MAKE_ABC(op, d, l, r));
		vc_free(c, l);
		vc_free(c, r);
		l = d;
	}
	return l;
}

static int vc_addsub(vc_t *c) {
	int l = vc_term(c);
	while (vc_check(c, TOK_PLUS) || vc_check(c, TOK_MINUS)) {
		minic_opcode_t op = vc_check(c, TOK_PLUS) ? OP_ADD : OP_SUB;
		vc_advance(c);
		int r = vc_term(c);
		int d = vc_reg(c);
		vc_emit(c, VM_MAKE_ABC(op, d, l, r));
		vc_free(c, l);
		vc_free(c, r);
		l = d;
	}
	return l;
}

static int vc_cmp(vc_t *c) {
	int l = vc_addsub(c);
	while (vc_check(c, TOK_LT) || vc_check(c, TOK_GT) || vc_check(c, TOK_LE) || vc_check(c, TOK_GE)) {
		minic_opcode_t op = vc_check(c, TOK_LT) ? OP_LT : vc_check(c, TOK_GT) ? OP_GT : vc_check(c, TOK_LE) ? OP_LE : OP_GE;
		vc_advance(c);
		int r = vc_addsub(c);
		int d = vc_reg(c);
		vc_emit(c, VM_MAKE_ABC(op, d, l, r));
		vc_free(c, l);
		vc_free(c, r);
		l = d;
	}
	return l;
}

static int vc_eq(vc_t *c) {
	int l = vc_cmp(c);
	while (vc_check(c, TOK_EQ) || vc_check(c, TOK_NEQ)) {
		minic_opcode_t op = vc_check(c, TOK_EQ) ? OP_EQ : OP_NEQ;
		vc_advance(c);
		int r = vc_cmp(c);
		int d = vc_reg(c);
		vc_emit(c, VM_MAKE_ABC(op, d, l, r));
		vc_free(c, l);
		vc_free(c, r);
		l = d;
	}
	return l;
}

// && with short-circuit: if left is false, skip right
static int vc_and(vc_t *c) {
	int l = vc_eq(c);
	while (vc_check(c, TOK_AND)) {
		vc_advance(c);
		// If l is false, result is 0 (don't eval right)
		// Save l, emit JMP_FALSE over right side
		int dest = vc_reg(c);
		// Copy l to dest
		vc_emit(c, VM_MAKE_ABC(OP_MOV, dest, l, 0));
		vc_free(c, l);
		int skip = vc_emit_jmp(c, VM_MAKE_ABX(OP_JMP_FALSE, dest, 0));
		// Right side: dest = right (only reached if left was true)
		int r = vc_eq(c);
		vc_emit(c, VM_MAKE_ABC(OP_MOV, dest, r, 0));
		vc_free(c, r);
		// Patch skip to jump here
		vc_patch(c, skip, VM_MAKE_ABX(OP_JMP_FALSE, dest, c->code_count - skip));
		l = dest;
	}
	return l;
}

// || with short-circuit: if left is true, skip right
static int vc_or(vc_t *c) {
	int l = vc_and(c);
	while (vc_check(c, TOK_OR)) {
		vc_advance(c);
		int dest = vc_reg(c);
		vc_emit(c, VM_MAKE_ABC(OP_MOV, dest, l, 0));
		vc_free(c, l);
		int skip = vc_emit_jmp(c, VM_MAKE_ABX(OP_JMP_TRUE, dest, 0));
		int r = vc_eq(c);
		vc_emit(c, VM_MAKE_ABC(OP_MOV, dest, r, 0));
		vc_free(c, r);
		vc_patch(c, skip, VM_MAKE_ABX(OP_JMP_TRUE, dest, c->code_count - skip));
		l = dest;
	}
	return l;
}

static int vc_expr(vc_t *c) {
	return vc_or(c);
}

// --- Statement compiler ---
static void vc_stmt(vc_t *c) {
	if (c->error)
		return;

	// Var decl: int/float/char/double/bool/void name [= expr];
	if (c->lex.cur.type == TOK_INT || c->lex.cur.type == TOK_FLOAT || c->lex.cur.type == TOK_CHAR || c->lex.cur.type == TOK_DOUBLE ||
	    c->lex.cur.type == TOK_BOOL || c->lex.cur.type == TOK_VOID) {
		vc_advance(c);
		if (vc_check(c, TOK_STAR))
			vc_advance(c);
		if (vc_check(c, TOK_IDENT)) {
			char name[64];
			strncpy(name, c->lex.cur.text, 63);
			name[63] = '\0';
			vc_advance(c);

			// Array decl — skip
			if (vc_check(c, TOK_LBRACKET)) {
				while (!vc_check(c, TOK_SEMICOLON) && !vc_check(c, TOK_EOF))
					vc_advance(c);
				vc_expect(c, TOK_SEMICOLON);
				return;
			}

			int slot = vc_decl_local(c, name);
			if (vc_check(c, TOK_ASSIGN)) {
				vc_advance(c);
				int val = vc_expr(c);
				if (val != slot)
					vc_emit(c, VM_MAKE_ABC(OP_MOV, slot, val, 0));
			}
			else {
				// Initialize to zero
				vc_emit(c, VM_MAKE_ABC(OP_CONST_ZERO, slot, 0, 0));
			}
			vc_expect(c, TOK_SEMICOLON);
		}
		else {
			vc_expect(c, TOK_SEMICOLON);
		}
		return;
	}

	// If/else
	if (vc_check(c, TOK_IF)) {
		vc_advance(c);
		vc_expect(c, TOK_LPAREN);
		int cr = vc_expr(c);
		vc_expect(c, TOK_RPAREN);
		vc_free(c, cr);
		int jmp_then = vc_emit_jmp(c, VM_MAKE_ABX(OP_JMP_FALSE, cr, 0));
		vc_block(c);
		if (vc_check(c, TOK_ELSE)) {
			vc_advance(c);
			int jmp_else = vc_emit_jmp(c, VM_MAKE_ABX(OP_JMP, 0, 0));
			vc_patch(c, jmp_then, VM_MAKE_ABX(OP_JMP_FALSE, cr, c->code_count - jmp_then - 1));
			vc_block(c);
			vc_patch(c, jmp_else, VM_MAKE_ABX(OP_JMP, 0, c->code_count - jmp_else - 1));
		}
		else {
			vc_patch(c, jmp_then, VM_MAKE_ABX(OP_JMP_FALSE, cr, c->code_count - jmp_then - 1));
		}
		return;
	}

	// While
	if (vc_check(c, TOK_WHILE)) {
		vc_advance(c);
		vc_expect(c, TOK_LPAREN);
		int saved_break    = c->break_count;
		int saved_continue = c->continue_count;
		int loop_top       = c->code_count;
		int cr             = vc_expr(c);
		vc_expect(c, TOK_RPAREN);
		vc_free(c, cr);
		int jmp_out        = vc_emit_jmp(c, VM_MAKE_ABX(OP_JMP_FALSE, cr, 0));
		c->loop_top        = loop_top;
		vc_block(c);
		vc_emit(c, VM_MAKE_ABX(OP_LOOP, 0, c->code_count - loop_top + 1));
		vc_patch(c, jmp_out, VM_MAKE_ABX(OP_JMP_FALSE, cr, c->code_count - jmp_out - 1));
		// Patch breaks
		for (int i = saved_break; i < c->break_count; i++)
			vc_patch(c, c->break_addrs[i], VM_MAKE_ABX(OP_JMP, 0, c->code_count - c->break_addrs[i] - 1));
		// Patch continues
		for (int i = saved_continue; i < c->continue_count; i++)
			vc_patch(c, c->continue_addrs[i], VM_MAKE_ABX(OP_LOOP, 0, c->continue_addrs[i] + 1 - loop_top));
		c->break_count    = saved_break;
		c->continue_count = saved_continue;
		return;
	}

	// For
	if (vc_check(c, TOK_FOR)) {
		vc_advance(c);
		vc_expect(c, TOK_LPAREN);
		vc_scope_t scope = vc_scope_save(c);
		int saved_break    = c->break_count;
		int saved_continue = c->continue_count;

		// Optional type keyword
		if (c->lex.cur.type == TOK_INT || c->lex.cur.type == TOK_FLOAT || c->lex.cur.type == TOK_CHAR ||
		    c->lex.cur.type == TOK_DOUBLE || c->lex.cur.type == TOK_BOOL || c->lex.cur.type == TOK_VOID)
			vc_advance(c);

		// Init
		if (!vc_check(c, TOK_SEMICOLON)) {
			char iname[64] = "";
			if (vc_check(c, TOK_IDENT)) {
				strncpy(iname, c->lex.cur.text, 63);
			}
			// Could be: ident = expr; (init statement)
			int slot = -1;
			if (vc_check(c, TOK_IDENT)) {
				vc_advance(c);
				if (vc_check(c, TOK_ASSIGN)) {
					vc_advance(c);
					int val = vc_expr(c);
					slot = vc_decl_local(c, iname);
					if (val != slot)
						vc_emit(c, VM_MAKE_ABC(OP_MOV, slot, val, 0));
				}
				// else: complex init, just compile as expr
				else {
					// Put the name back... just compile as expression
					// This handles cases we don't fully parse
					int v = vc_expr(c);
					vc_free(c, v);
				}
			}
			else {
				int v = vc_expr(c);
				vc_free(c, v);
			}
		}
		vc_expect(c, TOK_SEMICOLON);

		// Condition
		int cond_pos = c->code_count;
		int cr = -1;
		if (!vc_check(c, TOK_SEMICOLON)) {
			cr = vc_expr(c);
		}
		else {
			cr = vc_reg(c);
			vc_emit(c, VM_MAKE_ABC(OP_CONST_ONE, cr, 0, 0)); // always true
		}
		vc_expect(c, TOK_SEMICOLON);

		int jmp_cond = vc_emit_jmp(c, VM_MAKE_ABX(OP_JMP_FALSE, cr, 0));
		vc_free(c, cr);

		// Increment: save full lexer state, skip tokens, compile later
		minic_lexer_t incr_lex = c->lex;
		{
			minic_lexer_t tmp = {0};
			tmp.src           = c->lex.src;
			tmp.pos           = c->lex.pos;
			minic_lex_next(&tmp);
			while (tmp.cur.type != TOK_RPAREN && tmp.cur.type != TOK_EOF)
				minic_lex_next(&tmp);
			c->lex.pos = tmp.pos; // skip past ')'
			minic_lex_next(&c->lex); // re-prime lexer so body sees '{'
		}

		// Body
		int body_start = c->code_count;
		c->loop_top = cond_pos;
		vc_block(c);

		// Increment: compile now (after body, before loop-back)
		int incr_pos = c->code_count;
		{
			minic_lexer_t body_end = c->lex;
			c->lex = incr_lex; // restore full lexer state at increment start
			// Compile increment — like vc_stmt but without trailing semicolon
			if (vc_check(c, TOK_IDENT)) {
				char iname[64];
				strncpy(iname, c->lex.cur.text, 63);
				iname[63] = '\0';
				vc_advance(c);
				// i++ / i--
				if (vc_check(c, TOK_INC) || vc_check(c, TOK_DEC)) {
					minic_opcode_t op = vc_check(c, TOK_INC) ? OP_INC : OP_DEC;
					vc_advance(c);
					int slot = vc_find_local(c, iname);
					if (slot >= 0)
						vc_emit(c, VM_MAKE_ABC(op, slot, 0, 0));
				}
				// i += expr, etc.
				else if (vc_check(c, TOK_PLUS_ASSIGN) || vc_check(c, TOK_MINUS_ASSIGN) ||
				         vc_check(c, TOK_MUL_ASSIGN) || vc_check(c, TOK_DIV_ASSIGN) || vc_check(c, TOK_MOD_ASSIGN)) {
					minic_opcode_t op = (vc_check(c, TOK_PLUS_ASSIGN)) ? OP_ADD_ASSIGN
					                    : (vc_check(c, TOK_MINUS_ASSIGN)) ? OP_SUB_ASSIGN
					                    : (vc_check(c, TOK_MUL_ASSIGN)) ? OP_MUL_ASSIGN
					                    : (vc_check(c, TOK_DIV_ASSIGN)) ? OP_DIV_ASSIGN
					                                                      : OP_MOD_ASSIGN;
					vc_advance(c);
					int val = vc_expr(c);
					int slot = vc_find_local(c, iname);
					if (slot >= 0)
						vc_emit(c, VM_MAKE_ABC(op, slot, slot, val));
					vc_free(c, val);
				}
				// i = expr
				else if (vc_check(c, TOK_ASSIGN)) {
					vc_advance(c);
					int val = vc_expr(c);
					int slot = vc_find_local(c, iname);
					if (slot >= 0) {
						if (val != slot)
							vc_emit(c, VM_MAKE_ABC(OP_MOV, slot, val, 0));
					}
					vc_free(c, val);
				}
			}
			else {
				// Bare expression increment
				int v = vc_expr(c);
					vc_free(c, v);
			}
			// ')' is now current token — skip it, then restore body-end state
			if (vc_check(c, TOK_RPAREN))
				vc_advance(c);
			c->lex = body_end;
		}

		// Loop back to condition
		vc_emit(c, VM_MAKE_ABX(OP_LOOP, 0, c->code_count - cond_pos + 1));

		// Patch condition jump
		vc_patch(c, jmp_cond, VM_MAKE_ABX(OP_JMP_FALSE, cr, c->code_count - jmp_cond - 1));

		// Patch breaks
		for (int i = saved_break; i < c->break_count; i++)
			vc_patch(c, c->break_addrs[i], VM_MAKE_ABX(OP_JMP, 0, c->code_count - c->break_addrs[i] - 1));
		// Patch continues
		for (int i = saved_continue; i < c->continue_count; i++)
			vc_patch(c, c->continue_addrs[i], VM_MAKE_ABX(OP_LOOP, 0, c->continue_addrs[i] + 1 - incr_pos));

		c->break_count    = saved_break;
		c->continue_count = saved_continue;
		vc_scope_restore(c, scope);
		return;
	}

	// Return
	if (vc_check(c, TOK_RETURN)) {
		vc_advance(c);
		if (vc_check(c, TOK_SEMICOLON)) {
			vc_expect(c, TOK_SEMICOLON);
			vc_emit(c, VM_MAKE_ABC(OP_RETURN_VOID, 0, 0, 0));
			return;
		}
		int v = vc_expr(c);
		vc_expect(c, TOK_SEMICOLON);
		vc_emit(c, VM_MAKE_ABC(OP_RETURN, v, 0, 0));
		vc_free(c, v);
		return;
	}

	// Break
	if (vc_check(c, TOK_BREAK)) {
		vc_advance(c);
		vc_expect(c, TOK_SEMICOLON);
		if (c->break_count < 64)
			c->break_addrs[c->break_count++] = vc_emit_jmp(c, VM_MAKE_ABX(OP_JMP, 0, 0));
		return;
	}

	// Continue
	if (vc_check(c, TOK_CONTINUE)) {
		vc_advance(c);
		vc_expect(c, TOK_SEMICOLON);
		if (c->loop_top >= 0) {
			vc_emit(c, VM_MAKE_ABX(OP_LOOP, 0, c->code_count - c->loop_top + 1));
		}
		return;
	}

	// Assignment / compound assignment / expression statement
	if (vc_check(c, TOK_IDENT)) {
		char name[64];
		strncpy(name, c->lex.cur.text, 63);
		name[63] = '\0';
		vc_advance(c);

		// Array element assignment: name[expr] = expr;
		if (vc_check(c, TOK_LBRACKET)) {
			vc_advance(c);
			int idx_reg = vc_expr(c);
			vc_expect(c, TOK_RBRACKET);
			vc_expect(c, TOK_ASSIGN);
			int val = vc_expr(c);
			int ai = vm_arr_find_or_add(name);
			vc_emit(c, VM_MAKE_ABX(OP_STORE_ARR, val, ai));
			vc_emit(c, (uint32_t)idx_reg); // inline: index register
			vc_free(c, idx_reg);
			vc_free(c, val);
			vc_expect(c, TOK_SEMICOLON);
			return;
		}

		// Simple assignment: name = expr;
		if (vc_check(c, TOK_ASSIGN)) {
			vc_advance(c);
			int val = vc_expr(c);
			int slot = vc_find_local(c, name);
			if (slot >= 0) {
				if (val != slot)
					vc_emit(c, VM_MAKE_ABC(OP_MOV, slot, val, 0));
			}
			else {
				int gi = vm_global_find_or_add(name);
				vc_emit(c, VM_MAKE_ABX(OP_STORE_GLOBAL, val, gi));
			}
			vc_free(c, val);
			vc_expect(c, TOK_SEMICOLON);
			return;
		}

		// Compound assignment: name += expr; etc.
		if (vc_check(c, TOK_PLUS_ASSIGN) || vc_check(c, TOK_MINUS_ASSIGN) || vc_check(c, TOK_MUL_ASSIGN) ||
		    vc_check(c, TOK_DIV_ASSIGN) || vc_check(c, TOK_MOD_ASSIGN)) {
			minic_opcode_t op = (vc_check(c, TOK_PLUS_ASSIGN))    ? OP_ADD_ASSIGN
			                    : (vc_check(c, TOK_MINUS_ASSIGN)) ? OP_SUB_ASSIGN
			                    : (vc_check(c, TOK_MUL_ASSIGN))   ? OP_MUL_ASSIGN
			                    : (vc_check(c, TOK_DIV_ASSIGN))   ? OP_DIV_ASSIGN
			                                                      : OP_MOD_ASSIGN;
			vc_advance(c);
			int val = vc_expr(c);
			int slot = vc_find_local(c, name);
			if (slot >= 0) {
				vc_emit(c, VM_MAKE_ABC(op, slot, slot, val));
			}
			else {
				int gi  = vm_global_find_or_add(name);
				int tmp = vc_reg(c);
				vc_emit(c, VM_MAKE_ABX(OP_LOAD_GLOBAL, tmp, gi));
				vc_emit(c, VM_MAKE_ABC(op, tmp, tmp, val));
				vc_emit(c, VM_MAKE_ABX(OP_STORE_GLOBAL, tmp, gi));
				vc_free(c, tmp);
			}
			vc_free(c, val);
			vc_expect(c, TOK_SEMICOLON);
			return;
		}

		// Post-increment/decrement: name++ / name--
		if (vc_check(c, TOK_INC) || vc_check(c, TOK_DEC)) {
			minic_opcode_t op = vc_check(c, TOK_INC) ? OP_INC : OP_DEC;
			vc_advance(c);
			int slot = vc_find_local(c, name);
			if (slot >= 0) {
				vc_emit(c, VM_MAKE_ABC(op, slot, 0, 0));
			}
			else {
				int gi  = vm_global_find_or_add(name);
				int tmp = vc_reg(c);
				vc_emit(c, VM_MAKE_ABX(OP_LOAD_GLOBAL, tmp, gi));
				vc_emit(c, VM_MAKE_ABC(op, tmp, 0, 0));
				vc_emit(c, VM_MAKE_ABX(OP_STORE_GLOBAL, tmp, gi));
				vc_free(c, tmp);
			}
			vc_expect(c, TOK_SEMICOLON);
			return;
		}

		// Function call as statement: name(args);
		if (vc_check(c, TOK_LPAREN)) {
			vc_expect(c, TOK_LPAREN);
			int arg_base = c->free_reg;
			int argc     = 0;
			if (!vc_check(c, TOK_RPAREN)) {
				c->free_reg++; // reserve arg slot 0
				int ar = vc_expr(c);
				if (ar != arg_base + argc)
					vc_emit(c, VM_MAKE_ABC(OP_MOV, arg_base + argc, ar, 0));
				c->free_reg = arg_base + argc + 1;
				argc++;
				while (vc_check(c, TOK_COMMA)) {
					vc_advance(c);
					c->free_reg++;
					ar = vc_expr(c);
					if (ar != arg_base + argc)
						vc_emit(c, VM_MAKE_ABC(OP_MOV, arg_base + argc, ar, 0));
					c->free_reg = arg_base + argc + 1;
					argc++;
				}
			}
			vc_expect(c, TOK_RPAREN);

			// Ext func
			minic_ext_func_t *ef = minic_ext_func_get(name);
			if (ef) {
				minic_val_t fv;
				fv.type       = MINIC_T_PTR;
				fv.deref_type = MINIC_T_PTR;
				fv.p          = ef;
				int ci = vc_const(c, fv);
				int dest = vc_reg(c);
				vc_emit(c, VM_MAKE_ABX(OP_CALL_EXT, dest, (argc << 8) | arg_base));
				vc_emit(c, (uint32_t)ci);
				vc_free(c, dest);
				c->free_reg = arg_base;
				vc_expect(c, TOK_SEMICOLON);
				return;
			}
			// User func
			if (c->env) {
				for (int i = 0; i < c->env->func_count; i++) {
					if (strcmp(c->env->funcs[i].name, name) == 0) {
						int dest = vc_reg(c);
						vc_emit(c, VM_MAKE_ABC(OP_CALL, dest, i, argc));
						vc_emit(c, (uint32_t)arg_base);
						vc_free(c, dest);
						c->free_reg = arg_base;
						vc_expect(c, TOK_SEMICOLON);
						return;
					}
				}
			}
			c->error = true;
			vc_expect(c, TOK_SEMICOLON);
			return;
		}

		// Fallback: expression statement starting with ident
		// (already consumed ident, this is incomplete — error)
		c->error = true;
		return;
	}

	// Expression statement
	{
		int v = vc_expr(c);
		vc_free(c, v);
		vc_expect(c, TOK_SEMICOLON);
	}
}

static void vc_block(vc_t *c) {
	vc_scope_t scope = vc_scope_save(c);
	if (vc_check(c, TOK_LBRACE)) {
		vc_advance(c);
		while (!vc_check(c, TOK_RBRACE) && !vc_check(c, TOK_EOF) && !c->error)
			vc_stmt(c);
		vc_expect(c, TOK_RBRACE);
	}
	else {
		vc_stmt(c);
	}
	vc_scope_restore(c, scope);
}

// --- Public: compile a function body ---
static minic_proto_t *vm_compile_body(const char *src, int body_pos, minic_env_t *env, minic_func_t *fn) {
	vc_t c          = {0};
	c.lex.src       = src;
	c.lex.pos       = body_pos;
	c.env           = env;
	c.loop_top      = -1;

	// Bind parameters as locals at consecutive registers starting from 0
	for (int pi = 0; pi < fn->param_count; pi++) {
		vc_decl_local(&c, fn->params[pi]);
	}

	minic_lex_next(&c.lex);
	vc_block(&c);
	vc_emit(&c, VM_MAKE_ABC(OP_HALT, 0, 0, 0));

	minic_proto_t *p = (minic_proto_t *)malloc(sizeof(minic_proto_t));
	p->code_count    = c.code_count;
	p->code          = (uint32_t *)malloc(c.code_count * sizeof(uint32_t));
	memcpy(p->code, c.code, c.code_count * sizeof(uint32_t));
	p->num_regs    = c.max_reg + 1;
	p->const_count = c.const_count;
	p->constants   = NULL;
	if (c.const_count > 0) {
		p->constants = (minic_val_t *)malloc(c.const_count * sizeof(minic_val_t));
		memcpy(p->constants, c.constants, c.const_count * sizeof(minic_val_t));
	}
	return p;
}
