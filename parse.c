#include "parse.h"
#include "lexer.h"
#include "ximport.h"


//[if[0],for[1],while[2],do[3],else[4],print[5],return[6]]
const char* key_word[] = { "if", "for", "while", "do", "else", "eif", "return", "break", "class", "static", "import", "new", "in" };
static char one_c[] = { '+', 0, '-', 0, '/', 0, '*', 0, '%', 0, '=', 0, '(', 0, ')', 0, '{', 0, '}', 0, '[', 0, ']', 0, ',', 0, '>', 0, '<', 0, '|', 0, '&', 0, '^', 0, '~', 0, '!', 0, '.', 0, ':', 0 };

bool out_put;
bool g_parse_only = false;






//must_after_found_type...


bool mbool = false;

char* getchar_x(const char f)
{
	for (size_t i = 0; i < sizeof(one_c); i += 2)
	{
		if (one_c[i] == f)
			return one_c + i;
	}
	return NULL;
}



/*if(op=='+') return FCAST(ac.,ac.value)+*(type*)y;
else if(op=='-') return (*(type*)x)-(*(type*)y);
else if(op=='*') return (*(type*)x)*(*(type*)y);
else if(op=='/') return (*(type*)x)/(*(type*)y);*/


#define MOP(type1,x,op,type2,y) if(op=='+') return *(type*)x+*(type*)y;\
					else if(op=='-') return (*(type*)x)-(*(type*)y);\
					else if(op=='*') return (*(type*)x)*(*(type*)y);\
					else if(op=='/') return (*(type*)x)/(*(type*)y);


#define VP(T) ((T*)(x[l].value))
#define V(T)  *(T*)x[INDEX].value
//#define VVP(INDEX)     x[INDEX].value


#define size_index TYPE::size|TYPE::index
//enum base_type {	INT,CHAR,STRING,LONG};

void inherit_parent_flag(node* current_node, node* next_node)
{
	// [(]  var [flag ')'  ]
	if (current_node->parent != NULL)
	{
		node* parent = current_node->parent;

		//if cur type not on parent flag add parent flags to cur          1 1 1  & 0 1 0  0 1 0
		if ((current_node->type_) & (parent->flag_ == 0))
		{
			next_node->is_flagged = true;
			next_node->flag_ |= parent->flag_;
		}
		else
		{
		}
	}
}




ParserContext* current_parser_ctx = NULL;

void parser_context_init(ParserContext* ctx, bool interactive)
{
	if (ctx == NULL)
		return;
	ctx->delim_capacity = 64;
	ctx->delim_stack = (fl*)calloc(ctx->delim_capacity, sizeof(fl));

	ctx->scope_capacity = 32;
	ctx->scope_top = 0;
	ctx->scope_stack = (ScopeEntry*)calloc(ctx->scope_capacity, sizeof(ScopeEntry));

	ctx->save = NULL;
	ctx->current_parsing_class_name = NULL;
	ctx->current_line = 1;
	ctx->interactive = interactive;
	ctx->has_error = false;
	ctx->error_msg[0] = '\0';
	ctx->prev_ctx = NULL;
}

void parser_context_cleanup(ParserContext* ctx)
{
	if (ctx == NULL)
		return;
	if (ctx->delim_stack != NULL)
	{
		free(ctx->delim_stack);
		ctx->delim_stack = NULL;
	}
	ctx->delim_capacity = 0;

	if (ctx->scope_stack != NULL)
	{
		free(ctx->scope_stack);
		ctx->scope_stack = NULL;
	}
	ctx->scope_top = 0;
	ctx->scope_capacity = 0;

	ctx->save = NULL;
	ctx->current_parsing_class_name = NULL;
}

ParserContext* parser_context_create(bool interactive)
{
	ParserContext* ctx = (ParserContext*)malloc(sizeof(ParserContext));
	parser_context_init(ctx, interactive);
	return ctx;
}

void parser_context_free(ParserContext* ctx)
{
	if (ctx != NULL)
	{
		parser_context_cleanup(ctx);
		free(ctx);
	}
}

node* parser_delim_op(ParserContext* ctx, const node_type mtype, node* w_node, bool added)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL || ctx->delim_stack == NULL)
		return NULL;

	node* p = NULL;
	if (added)
	{
		int target_idx = -1;
		for (int i = 0; i < ctx->delim_capacity; i++)
		{
			if (ctx->delim_stack[i].wait_type == 0)
			{
				target_idx = i;
				break;
			}
		}
		if (target_idx == -1)
		{
			int old_cap = ctx->delim_capacity;
			int new_cap = old_cap > 0 ? old_cap * 2 : 64;
			fl* new_stack = (fl*)realloc(ctx->delim_stack, new_cap * sizeof(fl));
			if (new_stack != NULL)
			{
				memset(new_stack + old_cap, 0, (new_cap - old_cap) * sizeof(fl));
				ctx->delim_stack = new_stack;
				ctx->delim_capacity = new_cap;
				target_idx = old_cap;
			}
			else
			{
				return NULL;
			}
		}
		ctx->delim_stack[target_idx].wait_type = mtype;
		ctx->delim_stack[target_idx].waiting_node = w_node;
	}
	else
	{
		for (int i = ctx->delim_capacity - 1; i >= 0; i--)
		{
			if (ctx->delim_stack[i].wait_type == mtype)
			{
				if (ctx->delim_stack[i].waiting_node != NULL)
				{
					ctx->delim_stack[i].waiting_node->ref_node = w_node;
					p = ctx->delim_stack[i].waiting_node;
				}
				ctx->delim_stack[i].wait_type = (node_type)0;
				ctx->delim_stack[i].waiting_node = NULL;
				break;
			}
		}
	}
	return p;
}

bool parser_delim_check_unclosed(ParserContext* ctx, node_type* m)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL || ctx->delim_stack == NULL)
		return false;

	int* ma = (int*)m;
	bool cont = false;
	for (int i = 0; i < ctx->delim_capacity; i++)
	{
		if (ctx->delim_stack[i].wait_type != 0)
		{
			cont = true;
			*ma |= ctx->delim_stack[i].wait_type;
		}
	}
	return cont;
}

void parser_delim_clear(ParserContext* ctx)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx != NULL && ctx->delim_stack != NULL)
	{
		memset(ctx->delim_stack, 0, ctx->delim_capacity * sizeof(fl));
		ctx->save = NULL;
	}
}

fl* parser_delim_get_first_unclosed(ParserContext* ctx)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL || ctx->delim_stack == NULL)
		return NULL;

	for (int i = 0; i < ctx->delim_capacity; i++)
	{
		if (ctx->delim_stack[i].wait_type != 0)
		{
			return &ctx->delim_stack[i];
		}
	}
	return NULL;
}

bool parser_is_inside_func_param(ParserContext* ctx)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL || ctx->delim_stack == NULL)
		return false;

	for (int i = 0; i < ctx->delim_capacity; i++)
	{
		if (ctx->delim_stack[i].wait_type == parentheses4_c &&
		    ctx->delim_stack[i].waiting_node != NULL &&
		    ctx->delim_stack[i].waiting_node->opt_name_type == function_def)
		{
			return true;
		}
	}
	return false;
}

bool parser_has_open_brace(ParserContext* ctx)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL || ctx->delim_stack == NULL)
		return false;

	for (int i = 0; i < ctx->delim_capacity; i++)
	{
		if (ctx->delim_stack[i].wait_type == parentheses1_c)
		{
			return true;
		}
	}
	return false;
}

void parser_scope_push(ParserContext* ctx, ScopeKind kind, const char* name, node* opening_node, int line)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL)
		return;

	if (ctx->scope_stack == NULL || ctx->scope_capacity <= 0)
	{
		ctx->scope_capacity = 32;
		ctx->scope_top = 0;
		ctx->scope_stack = (ScopeEntry*)calloc(ctx->scope_capacity, sizeof(ScopeEntry));
	}
	else if (ctx->scope_top >= ctx->scope_capacity)
	{
		int new_cap = ctx->scope_capacity * 2;
		ScopeEntry* new_stack = (ScopeEntry*)realloc(ctx->scope_stack, new_cap * sizeof(ScopeEntry));
		if (new_stack != NULL)
		{
			memset(new_stack + ctx->scope_capacity, 0, (new_cap - ctx->scope_capacity) * sizeof(ScopeEntry));
			ctx->scope_stack = new_stack;
			ctx->scope_capacity = new_cap;
		}
		else
		{
			return;
		}
	}

	ScopeEntry* entry = &ctx->scope_stack[ctx->scope_top++];
	entry->kind = kind;
	if (name != NULL)
	{
		strncpy(entry->name, name, sizeof(entry->name) - 1);
		entry->name[sizeof(entry->name) - 1] = '\0';
	}
	else
	{
		entry->name[0] = '\0';
	}
	entry->opening_node = opening_node;
	entry->line = line;

	if (kind == SCOPE_CLASS && name != NULL && name[0] != '\0')
	{
		ctx->current_parsing_class_name = entry->name;
	}
}

ScopeEntry* parser_scope_pop(ParserContext* ctx)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL || ctx->scope_stack == NULL || ctx->scope_top <= 0)
		return NULL;

	ctx->scope_top--;
	ScopeEntry* popped = &ctx->scope_stack[ctx->scope_top];

	/* Recompute current class name by walking down the scope stack */
	ctx->current_parsing_class_name = NULL;
	for (int i = ctx->scope_top - 1; i >= 0; i--)
	{
		if (ctx->scope_stack[i].kind == SCOPE_CLASS)
		{
			ctx->current_parsing_class_name = ctx->scope_stack[i].name;
			break;
		}
	}

	return popped;
}

ScopeEntry* parser_scope_current(ParserContext* ctx)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL || ctx->scope_stack == NULL || ctx->scope_top <= 0)
		return NULL;

	return &ctx->scope_stack[ctx->scope_top - 1];
}

const char* parser_current_class_name(ParserContext* ctx)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL)
		return NULL;

	if (ctx->current_parsing_class_name != NULL)
		return ctx->current_parsing_class_name;

	if (ctx->scope_stack != NULL)
	{
		for (int i = ctx->scope_top - 1; i >= 0; i--)
		{
			if (ctx->scope_stack[i].kind == SCOPE_CLASS)
			{
				return ctx->scope_stack[i].name;
			}
		}
	}
	return NULL;
}

bool parser_is_in_class(ParserContext* ctx)
{
	return parser_current_class_name(ctx) != NULL;
}

bool parser_is_in_function(ParserContext* ctx)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx == NULL || ctx->scope_stack == NULL)
		return false;

	for (int i = ctx->scope_top - 1; i >= 0; i--)
	{
		if (ctx->scope_stack[i].kind == SCOPE_FUNCTION)
			return true;
	}
	return false;
}

static bool is_function_def_ahead(const char* buff)
{
	if (buff == NULL || *buff != '(')
		return false;

	int depth = 0;
	const char* p = buff;
	const char* close_paren = NULL;
	while (*p != '\0')
	{
		if (*p == '(')
			depth++;
		else if (*p == ')')
		{
			depth--;
			if (depth == 0)
			{
				close_paren = p;
				break;
			}
		}
		p++;
	}

	if (close_paren == NULL)
		return false;

	const char* after = close_paren + 1;
	while (*after == ' ' || *after == '\t' || *after == '\r' || *after == '\n')
		after++;
	if (*after == '{')
		return true;

	const char* inside = buff + 1;
	while (*inside == ' ' || *inside == '\t')
		inside++;

	if (inside == close_paren)
		return false;

	if (*inside == '"' || *inside == '\'' || *inside == '-' || *inside == '+' || (*inside >= '0' && *inside <= '9'))
		return false;

	if ((*inside >= 'a' && *inside <= 'z') || (*inside >= 'A' && *inside <= 'Z') || *inside == '_')
	{
		char token[128];
		int tlen = 0;
		while (((*inside >= 'a' && *inside <= 'z') || (*inside >= 'A' && *inside <= 'Z') ||
		        (*inside >= '0' && *inside <= '9') || *inside == '_') && tlen < 127)
		{
			token[tlen++] = *inside++;
		}
		token[tlen] = '\0';

		type_def* td = get_type_by_name(token);
		if (td != NULL)
		{
			while (*inside == ' ' || *inside == '\t')
				inside++;
			if ((*inside >= 'a' && *inside <= 'z') || (*inside >= 'A' && *inside <= 'Z') || *inside == '_')
			{
				return true;
			}
		}
	}

	return false;
}

void parse_line_ctx(ParserContext* ctx, char* buff, node* n_node, const int line)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;

	n_node->type_ |= operators_n | parentheses4;

	while (buff != NULL && (*buff == ' ' || *buff == '\t' || *buff == '\n' || *buff == '\r' || *buff == ';'))
	{
		buff++;
		if(*buff=='\n')n_node->line++;
	}
	n_node->line = line;
	int cur_col = (ctx && ctx->current_line_start && buff >= ctx->current_line_start)
	              ? (int)(buff - ctx->current_line_start) + 1 : 1;
	n_node->col = cur_col;

	node_type a = (node_type)0;
	if (parser_delim_check_unclosed(ctx, &a))
	{
		n_node->type_ |= a;
	}
	///line end but still need to close node
	if (strlen(buff) == 0)
	{

		///check for unclosed node if yes next line will encluded to cun node
		if (parser_delim_check_unclosed(ctx, &a))
		{
			node* nextc = new_node(nodes);
			n_node->type_ = endl;
			n_node->line = line;
			n_node->col = cur_col;
			n_node->next = nextc;
			nextc->parent = n_node;
			nextc->line = line;
			nextc->col = cur_col;
			if (ctx != NULL)
				ctx->save = nextc;
			else if (current_parser_ctx != NULL)
				current_parser_ctx->save = nextc;
			nextc->type_ = a | keyword | itype | var_name | value | (n_node->parent != NULL ? n_node->parent->flag_ : 0);
			return;
		}
		n_node->type_ = endl;
		n_node->next = NULL;
		n_node->line = line;
		n_node->col = cur_col;

		if (print_parse_log)
			debuge->print_line_debuge(debuge, n_node, line);

		node* root = get_root(n_node);
		if (root != NULL && root->type_ == keyword && root->value_keyword == _import_)
		{
			const char* mod_name = NULL;
			node* p = root->next;
			if (p != NULL && p->type_ == parentheses4)
			{
				p = p->next;
			}
			if (p != NULL)
			{
				if (p->type_ == value && p->opt_type_ptr == T_STRING)
				{
					mod_name = (char*)p->value_raw;
				}
				else if (p->type_ == var_name && p->value_char_ptr != NULL)
				{
					mod_name = p->value_char_ptr;
				}
			}
			if (mod_name != NULL)
			{
				x_import_module(mod_name);
			}
		}

		if (!g_parse_only)
		{
			compile(NULL, n_node, NULL, NULL, NULL);
		}



		return;
	}


	node* next = new_node(nodes);
	next->parent = n_node;
	next->line = line;
	next->col = cur_col;
	n_node->next = next;

	if (*buff == ',')
	{
		n_node->type_ = comma;
	}
	if (*buff == '.')
	{
		n_node->type_ = dot;
	}
	if (n_node->btype.node_type_bit.itype)
	{
		type_def* m_type = NULL;
		find* mfind = lex_match_word(buff);
		if (mfind->isFind)
			m_type = get_type_by_name(mfind->bn);

		if (m_type != NULL)///V3 if(strstr(buff,base_type[i])!=NULL)
		{
			n_node->line = line;
			//d->opt= new int(1);

			n_node->type_ = itype;
			n_node->value_type = m_type;


			next->type_ = var_name | s_index | dot;  // itype->var_name->(->itype->var_name) or itype->dot (static method)
			bool is_func_param = false;
			if (n_node->parent != NULL && (n_node->parent->_opt_ptr_ == function_def || n_node->parent->opt_name_type == function_def))
			{
				is_func_param = true;
			}
			else if (parser_is_inside_func_param(ctx))
			{
				is_func_param = true;
			}
			if (is_func_param)
			{
				next->type_ = var_name;
				next->opt_name_type = fucnction_parm; // "p";

				next->is_flagged = true;
				next->flag_ = parentheses4_c;
			}

			parse_line_ctx(ctx, buff + strlen(m_type->type_name), next, line);
			return;
		}
		else
		{
			// UNKNOWN TYPE
		}
	}
	n_node->btype.node_type_bit.itype = 0;
	if ((*buff == '[' && n_node->btype.node_type_bit.s_index) || (*buff == ']'&&n_node->btype.node_type_bit.s_index_c))
	{
		if (*buff == '[')
		{
			n_node->type_ = s_index;
			n_node->value_char_ptr = getchar_x(*buff);

			static_flag_op2(s_index, n_node, false);
			static_flag_op2(s_index_c, n_node, true);

			next->type_ = value | var_name;

			if (n_node->parent != NULL && n_node->parent->type_ == itype)
			{
				n_node->opt_name_type = psize;
			}
			else if (n_node->parent != NULL && n_node->parent->type_ == var_name)
			{
				n_node->opt_name_type = pindex;
			}
			else
			{
				/* List Literal */
				n_node->opt_name_type = pindex;
				next->type_ = value | var_name | s_index_c | s_index | parentheses4 | keyword;
				next->flag_ = s_index_c | comma;
				next->is_flagged = true;
				next->parent = n_node;
				n_node->next = next;
				parse_line_ctx(ctx, buff + 1, next, line);
				return;
			}
			next->parent = n_node;
			next->flag_ = s_index_c;
			/*,} */
			
			next->is_flagged = true;
			parse_line_ctx(ctx, buff + 1, next, line);

			return;
		}
		else if (*buff == ']')
		{
			n_node->type_ = s_index_c;

			node* open_node = static_flag_op2(s_index_c, n_node, false);
			n_node->value_char_ptr = getchar_x(*buff);
			//FIXME:

			if (open_node != NULL && open_node->opt_name_type == psize)
			{
				next->type_ = var_name;
				next->flag_ = equles | endl;
			}
			else
			{
				next->type_ = operators_n | equles | comma | parentheses4_c | s_index_c | dot | endl;
				next->flag_ = endl;
			}


			next->parent = n_node;
			n_node->next = next;
			parse_line_ctx(ctx, buff + 1, next, line);
			return;
		}
	}
	if (*buff == '{' && n_node->btype.node_type_bit.parentheses1)
	{

		n_node->type_ = parentheses1;
		inherit_parent_flag(n_node, next);
		//n_node->value_char_ptr = getchar_x(*buff);

		mbool = true;

		node* open = static_flag_op2(parentheses1, n_node, false);
		static_flag_op2(parentheses1_c, n_node, true);

		ScopeKind s_kind = SCOPE_BLOCK;
		const char* s_name = NULL;
		if (open != NULL && open->ref_node != NULL && open->ref_node->opt_name_type == class_base_def)
		{
			s_kind = SCOPE_CLASS;
			s_name = (open->ref_node->parent && open->ref_node->parent->value_char_ptr) ?
			         open->ref_node->parent->value_char_ptr : (ctx ? ctx->current_parsing_class_name : NULL);
		}
		else if (open != NULL && open->ref_node != NULL && open->ref_node->opt_name_type == function_def)
		{
			s_kind = SCOPE_FUNCTION;
			s_name = (open->ref_node->parent && open->ref_node->parent->value_char_ptr) ?
			         open->ref_node->parent->value_char_ptr : "function";
		}
		else if (ctx != NULL && ctx->current_parsing_class_name != NULL && !parser_is_in_class(ctx))
		{
			s_kind = SCOPE_CLASS;
			s_name = ctx->current_parsing_class_name;
		}
		parser_scope_push(ctx, s_kind, s_name, n_node, line);

		next->type_ = value | var_name;
		//if(d->isFlag)
		//	nextc.flag=d->flag_|parentheses1c/*,} */;
		//else
		if (n_node->is_flagged && n_node->flag_ == (value | var_name | keyword))
		{
			next->type_ = value | var_name | keyword;
			next->flag_ = parentheses1_c/*,} */;
			mbool = true;
		}
		else
		{
			next->flag_ = comma | parentheses1_c/*,} */;
		}
		next->flag_ |= parentheses1_c;
		next->is_flagged = true;
		//TEST 2
		//next->parent=d;
		//d->next=next;
		parse_line_ctx(ctx, buff + 1, next, line);
		return;

	}
	if (*buff == '}' && n_node->btype.node_type_bit.parentheses1c)
	{


		node * open = static_flag_op2(parentheses1_c, n_node, false);
		n_node->ref_node = open;
		n_node->type_ = parentheses1_c;
		n_node->value_char_ptr = getchar_x(*buff);

		parser_scope_pop(ctx);
		if (ctx != NULL && !parser_has_open_brace(ctx))
		{
			ctx->current_parsing_class_name = NULL;
		}

		mbool = false;

		bool is_do_block = (open != NULL && open->parent != NULL &&
		                    open->parent->type_ == keyword && open->parent->value_keyword == _do_);
		if (is_do_block)
		{
			static_flag_op2(keyword, n_node, true);
			next->type_ = keyword;
			next->flag_ = parentheses4;
			next->is_flagged = true;
		}
		else
		{
			next->type_ = endl | comma | itype | keyword | var_name;
			next->flag_ = parentheses1/*,} */;
			next->is_flagged = true;
		}

		parse_line_ctx(ctx, buff + 1, next, line);
		return;

	}
	if (*buff == ',' && n_node->btype.node_type_bit.comma)
	{

		n_node->type_ = comma;
		n_node->value_char_ptr = getchar_x(*buff);


		next->type_ = value | var_name | itype | keyword | s_index;
		next->flag_ = parentheses1_c | comma/*,} */;
		next->is_flagged = true;
		parse_line_ctx(ctx, buff + 1, next, line);
		return;

	}
	if ((*buff == '(' && n_node->btype.node_type_bit.parentheses4) || (*buff == ')'&& n_node->btype.node_type_bit.parentheses4c))
	{
		if (*buff == '(')
		{
			n_node->type_ = parentheses4;
			n_node->value_char_ptr = getchar_x(*buff);

			static_flag_op2(parentheses4, n_node, false);
			static_flag_op2(parentheses4_c, n_node, true);

			next->type_ = value | var_name | parentheses4_c | parentheses4 | keyword | s_index;
			if (n_node->parent != NULL && n_node->parent->type_ == var_name)
			{
				const int typ = n_node->parent->opt_name_type;
				if (typ == var_def)
				{
					if (is_function_def_ahead(buff))
					{
						next->type_ = itype | parentheses4_c;
						n_node->parent->_opt_ptr_ = function_def;//FUNCTION;//the type
						n_node->opt_name_type = function_def;// FUNCTION;
					}
					else
					{
						n_node->opt_name_type = function_call;
						next->type_ = value | var_name | parentheses4_c | parentheses4 | keyword | s_index;
						next->flag_ = comma;
					}
				}
				///class a (var )
				else if (typ == class_def)

				{
					next->type_ = var_name | parentheses4_c;
					next->flag_ = parentheses1;
					n_node->opt_name_type = class_base_def;
				}
				else if (typ == var_call) //switch form var call
				{
					//WHY
					next->type_ = value | var_name | parentheses4_c | keyword | s_index;
					next->flag_ = comma;
					n_node->parent->_opt_ptr_ = function_call;//changed form var call
					n_node->opt_name_type = function_call;//a
				}
			}

			next->parent = n_node;
			next->flag_ = parentheses4_c | next->flag_;
			/*,} */
			;
			next->is_flagged = true;
			parse_line_ctx(ctx, buff + 1, next, line);
			return;
		}
		else if (*buff == ')')
		{
			n_node->type_ = parentheses4_c;

			node* pp = static_flag_op2(parentheses4_c, n_node, false);
			n_node->ref_node = pp;
			n_node->value_char_ptr = getchar_x(*buff);
			//FIXME:
			if (pp != NULL && pp->opt_name_type == function_def)
			{

				static_flag_op2(parentheses1, n_node, true);
				next->flag_ = value | var_name | keyword;
				next->type_ = parentheses1 /*,} */;
				next->is_flagged = true;
			}
			else if (pp != NULL && pp->opt_name_type == class_base_def)
			{

				static_flag_op2(parentheses1, n_node, true);
				next->flag_ = itype;
				next->type_ = parentheses1 /*,} */;
				next->is_flagged = true;
			}
			else
			{
				next->type_ = operators_n | parentheses4_c | endl;
				next->flag_ = value | var_name | keyword;
				next->is_flagged = true;
			}


			next->parent = n_node;
			n_node->next = next;
			parse_line_ctx(ctx, buff + 1, next, line);
			return;
		}
	}
	if (n_node->btype.node_type_bit.keword)
	{
		int i = 0;
		for (i = 0; i < (int)(sizeof(key_word) / sizeof(key_word[0])); i++)
		{
			if (eql(buff, key_word[i]))///V3 if(strstr(buff,key_word[i])!=NULL)
			{
				n_node->type_ = keyword;


				n_node->value_keyword = i;


				//if (eql("print", key_word[i]) != -1)
				//{
				//	next->type_ = var_name | value;
				//}
				if (i == _if_  || i == _else_ || i == _eif_)

				{
				node * prev_condtion=NULL;
					if (i == _else_)
					{
						node* cb = NULL;
						if (n_node->parent != NULL && n_node->parent->type_ == parentheses1_c)
							cb = n_node->parent;
						else if (n_node->stack_parent != NULL && n_node->stack_parent->parent != NULL && n_node->stack_parent->parent->type_ == parentheses1_c)
							cb = n_node->stack_parent->parent;

						if (cb != NULL && cb->ref_node != NULL && cb->ref_node->parent != NULL &&
						    cb->ref_node->parent->ref_node != NULL && cb->ref_node->parent->ref_node->parent != NULL)
						{
							prev_condtion = cb->ref_node->parent->ref_node->parent;
						}
						else
						{
							printf("ERROR: no if body");
							exit(-1);
						}

						n_node->ref_node = prev_condtion;
						n_node->ref_node->next_jump = n_node;
						n_node->next_jump = NULL;
						next->flag_ = var_name | value;
						next->type_ = parentheses1;

						static_flag_op2(parentheses1, NULL, true);
						next->is_flagged = true;
						parse_line_ctx(ctx, buff + strlen(key_word[i]), next, line);
						return;
					}
					if (i == _eif_)
					{
						node* cb = NULL;
						if (n_node->parent != NULL && n_node->parent->type_ == parentheses1_c)
							cb = n_node->parent;
						else if (n_node->stack_parent != NULL && n_node->stack_parent->parent != NULL && n_node->stack_parent->parent->type_ == parentheses1_c)
							cb = n_node->stack_parent->parent;

						if (cb != NULL && cb->ref_node != NULL && cb->ref_node->parent != NULL &&
						    cb->ref_node->parent->ref_node != NULL && cb->ref_node->parent->ref_node->parent != NULL)
						{
							prev_condtion = cb->ref_node->parent->ref_node->parent;
						}
						else
						{
							printf("ERROR: parent if-eif body");
							exit(-1);
						}
					}
					if(prev_condtion!=NULL)
                    {
						n_node->ref_node= prev_condtion;
						n_node->ref_node->next_jump = n_node;
                    }
                    else
                    {
						if(i != _if_ )
						{
                    	printf("ERROR: no parent if-eif body");
						exit(-1);
						}
                    }
					n_node->next_jump = NULL;
					next->flag_ = var_name | value;
					next->type_ = parentheses4;
					static_flag_op2(parentheses1, NULL, true);
					static_flag_op2(parentheses4, NULL, true);
					next->is_flagged = true;
				}
				else if( i == _while_)
				{
					node* do_close = static_flag_op2(keyword, n_node, false);
					bool is_do_while = (do_close != NULL);
					if (!is_do_while && n_node->parent != NULL && n_node->parent->type_ == parentheses1_c)
					{
						node* open = n_node->parent->ref_node;
						if (open != NULL && open->parent != NULL && open->parent->type_ == keyword && open->parent->value_keyword == _do_)
						{
							is_do_while = true;
						}
					}

					next->flag_ = var_name | value;
					next->type_ = parentheses4;
					if (!is_do_while)
					{
						static_flag_op2(parentheses1, NULL, true);
					}
					static_flag_op2(parentheses4, NULL, true);
					next->is_flagged = true;
				}
				else if (i == _do_)
				{
					next->type_ = parentheses1;
					static_flag_op2(parentheses1, n_node, true);
					next->is_flagged = true;
					parse_line_ctx(ctx, buff + strlen(key_word[i]), next, line);
					return;
				}
				else if (i == _return_)
				{
					next->type_ = var_name | value | keyword | s_index;
				}
				else if (i == _static_)
				{
					next->type_ = itype;
					next->flag_ = var_name;
				}
				else if (i == _break_)
				{
					next->type_ = endl;
					next->flag_ = parentheses1;
				}
				else if (i == _for_)
				{
					char* p = buff + strlen(key_word[i]);
					while (*p == ' ' || *p == '\t') p++;
					if (*p == '(')
					{
						next->type_ = parentheses4;
						next->flag_ = parentheses4;
						static_flag_op2(parentheses1, NULL, true);
					}
					else
					{
						char* pvar = p;
						while ((*pvar >= 'a' && *pvar <= 'z') || (*pvar >= 'A' && *pvar <= 'Z') || (*pvar >= '0' && *pvar <= '9') || *pvar == '_')
							pvar++;
						while (*pvar == ' ' || *pvar == '\t') pvar++;
						if (strncmp(pvar, "in", 2) == 0 && (pvar[2] == ' ' || pvar[2] == '\t' || pvar[2] == '[' || pvar[2] == '('))
						{
							next->type_ = var_name;
							next->flag_ = keyword;
							static_flag_op2(parentheses1, NULL, true);
						}
						else
						{
							next->type_ = var_name;
							next->flag_ = parentheses4;
							static_flag_op2(parentheses1, NULL, true);
							static_flag_op2(parentheses4, NULL, true);
						}
					}
				}
				else if (i == _class_)
				{
					next->type_ = var_name;

					next->flag_ = parentheses4;
					next->is_flagged = true;
					parse_line_ctx(ctx, buff + strlen(key_word[i]), next, line);
					return;
				}
				else if (i == _import_)
				{
					next->type_ = value | var_name | parentheses4;
					next->is_flagged = true;
					next->flag_ = value | var_name | parentheses4 | parentheses4_c | endl;
					parse_line_ctx(ctx, buff + strlen(key_word[i]), next, line);
					return;
				}
				else if (i == _new_)
				{
					next->type_ = var_name;
					next->flag_ = parentheses4;
					next->is_flagged = true;
					parse_line_ctx(ctx, buff + strlen(key_word[i]), next, line);
					return;
				}
				else if (i == _in_)
				{
					next->type_ = var_name | value | parentheses4 | s_index;
					next->flag_ = parentheses4_c | parentheses1;
					next->is_flagged = true;
					next->parent = n_node;
					n_node->next = next;
					parse_line_ctx(ctx, buff + strlen(key_word[i]), next, line);
					return;
				}
				else
				{
					next->type_ = var_name | value;
				}
				inherit_parent_flag(n_node, next);
				if (n_node->is_flagged) { next->flag_ |= n_node->flag_; }


				parse_line_ctx(ctx, buff + strlen(key_word[i]), next, line);
				return;
				//setVarName(buff,h);
			}
		}
	}
	if (n_node->btype.node_type_bit.value)//if(d->type_==(var_name|value))
	{
		n_node->next = next;
		next->parent = n_node;
		// parse   ".*" string;
		if (*buff == '"' && *(buff + 1) == '"')
		{
			if (n_node->is_flagged)
			{
				next->type_ = operators_n | n_node->flag_;
			}
			else
			{
				next->type_ = operators_n | endl;
			}
			n_node->opt_type_ptr = T_STRING;
			n_node->opt_type = 1;
			n_node->type_ = value;
			n_node->value_raw = (char*)calloc(1, 1);

			parse_line_ctx(ctx, buff + 2, next, line);
			return;
		}
		find* mfind = lex_match_string(buff);
		if (mfind->isFind)

		{
			if (n_node->is_flagged)
			{
				next->type_ = operators_n | n_node->flag_;
			}
			else
			{
				next->type_ = operators_n | endl;////
			}
			/*TODO: change it to unnamed var */

			n_node->opt_type_ptr = T_STRING;
			n_node->opt_type = 1;
			n_node->type_ = value;
			int orig_len = (int)strlen(mfind->bn);
			scap_string(mfind->bn);
			n_node->value_raw = mfind->bn;


			parse_line_ctx(ctx, buff + orig_len + 2, next, line);
			
			return;
		}
		mfind = lex_match_char(buff);
		if(mfind->isFind)
		{
			if (n_node->is_flagged)
			{
				next->type_ = operators_n | n_node->flag_;
			}
			else
			{
				next->type_ = operators_n | endl;////
			}
			int len= strlen(mfind->bn);
			if(len>1 && *mfind->bn=='\\'){if(*(mfind->bn+1)=='n'){n_node->value_raw = &"\n";}}
			else
			{
				n_node->value_raw = mfind->bn;
			}
			n_node->type_ = value;
			n_node->opt_type_ptr = T_CHAR;
			n_node->opt_type = 1;
			n_node->value_raw = mfind->bn;

			parse_line_ctx(ctx, buff +len + 2, next, line);
			return;
		}
		mfind = lex_match_bool(buff);
		if (mfind->isFind)
		{
			if (n_node->is_flagged)
			{
				next->type_ = n_node->flag_;
			}
			else
			{
				next->type_ = endl;////
			}
			n_node->opt_type_ptr = T_BOOL;
			n_node->type_ = value;
			n_node->opt_type = 1;

			n_node->value_raw = mfind->bn;

			//((var*)d->opt)->value=mfind->bn;

			///TODO: check type match
			//v2/setVar(&d);
			parse_line_ctx(ctx, buff + strlen(mfind->bn), next, line);
			return;
		}
		//TODO : V2
		//mfind= &isMatch("\\b[a-zA-z\\_]+\\b",buff);//var name
		//if(mfind->isFind)
		//{
		//	// parse   "\b\w+\b"; var name //check if is already definded
		//	if(d->parent->btype.nnType.equles)
		//		next1.type= endl|operators_n;

		//	return;
		//}
		if ((*buff >= 48 && *buff <= 57) || *buff == 46)
		{
			mfind = lex_match_number(buff);
			if (mfind->isFind)
			{
				// parse   "\d+";

				n_node->type_ = value;
				n_node->opt_type_ptr =strstr(mfind->bn,".")?T_FLOAT:T_INT;
				
				
				n_node->opt_type = 1;
				if (n_node->is_flagged)
				{
					next->type_ = n_node->flag_ | operators_n;
					next->is_flagged = true;
					next->flag_ = n_node->flag_;
				}
				else
				{
					next->type_ = endl | operators_n;////
				}

				n_node->value_raw = mfind->bn;


				//v2//var m =var();
				//v2//m.value=new int(atoi(mfind->bn));
				//v2//m.ref=&d;
				//v2//strcpy_s(m.name,"INPUT");
				//v2//var * on_it= getVar(&d);
				//v2//strcpy_s(m.var_type,on_it->var_type);

				n_node->next = next;
				next->parent = n_node;
				//v2//d->opt=&m;

				//setVar(&d);
				parse_line_ctx(ctx, buff + strlen(mfind->bn), next, line);
				return;
			}
		}
	}
	n_node->btype.node_type_bit.value = 0;
	//TODO :if(d->type_==var_name|d->parent->type_==itype)
	if (n_node->btype.node_type_bit.var_name)
	{
		//?([0-9|A-Z|a-z_]+[_0-9|A-Z|a-z]+)[ \n]
		find* mfind = lex_match_var_name(buff);//var name

		
		if (mfind->isFind && *mfind->bn == *buff && strcmp(mfind->bn, "string") != 0)
		{
			//TODO:: if is BASE parse_obj
			bool ref=false;
			n_node->line = line;
			if (*mfind->bn != '&')
			{
				n_node->value_char_ptr = mfind->bn;
			}
			else
			{
				n_node->value_char_ptr = mfind->bn + 1;
				ref=true;
			}
			n_node->type_ = var_name;
			next->parent = n_node;
			n_node->next = next;
			//v2//d->opt=&rv;

			if (n_node->opt_name_type == fucnction_parm)
			{
				next->type_ = comma | n_node->flag_;

				next->is_flagged = true;
				next->flag_ = itype | n_node->flag_;
				parse_line_ctx(ctx, buff + strlen(mfind->bn), next, line);
				return;
			}

			if (n_node->parent != NULL)
			{
				if (n_node->parent->type_ == keyword && n_node->parent->value_keyword == _class_)
				{
					next->type_ = parentheses4;
					n_node->opt_name_type = class_def; //k;
					if (ctx != NULL)
						ctx->current_parsing_class_name = n_node->value_char_ptr;
					else if (current_parser_ctx != NULL)
						current_parser_ctx->current_parsing_class_name = n_node->value_char_ptr;
					if (get_type_by_name(n_node->value_char_ptr) == NULL)
					{
						type_def* placeholder = new_type();
						placeholder->type_name = n_node->value_char_ptr;
					}
					next->is_flagged = true;
					next->flag_ = var_name | parentheses4_c;
					parse_line_ctx(ctx, buff + strlen(mfind->bn), next, line);
					return;
				}
				//class f(-)-
				if (n_node->parent->type_ == parentheses4 && n_node->parent->opt_name_type == class_base_def)
				{
					next->type_ = parentheses4_c;
					n_node->opt_raw = "i";
					next->is_flagged = true;
					next->flag_ = parentheses1;
					parse_line_ctx(ctx, buff + strlen(mfind->bn), next, line);
					return;
				}
				if (n_node->parent->type_ == keyword && n_node->parent->value_keyword == _import_)
				{
					n_node->opt_name_type = var_call;
					next->type_ = endl;
					parse_line_ctx(ctx, buff + strlen(mfind->bn), next, line);
					return;
				}
				if (n_node->parent->type_ == itype || n_node->parent->type_==s_index_c)
				{
					n_node->opt_name_type = var_def;
				}
				else 
				{
					n_node->opt_name_type = ref?var_call_ref: var_call;
				}
			}
			else
			{
				n_node->opt_name_type = var_call;
			}


			//if(d->parent->btype.node_type_bit.size)equles|endl;
			//if(d->parent->btype.node_type_bit.comma)equles|endl;

			next->type_ = (equles | endl | operators_n | s_index |
				parentheses4 | dot | keyword);
			inherit_parent_flag(n_node, next);
			if (n_node->is_flagged)
			{
				next->is_flagged = true;
				next->flag_ = n_node->flag_ | var_name;
				next->type_ |= n_node->flag_;
			}

			parse_line_ctx(ctx, buff + strlen(mfind->bn), next, line);
			//print_line_debuge(&nex,line,true);

			return;
		}
	}
	n_node->btype.node_type_bit.var_name = 0;
	if (*buff == '.' && n_node->btype.node_type_bit.dot)
	{
		n_node->type_ = dot;
		next->type_ = var_name;
		parse_line_ctx(ctx, buff + 1, next, line);
		return;
	}
	if (n_node->type_ == (equles | endl | operators_n) || n_node->btype.node_type_bit.operators_n || n_node->
		btype.node_type_bit.endl
		| n_node->btype.node_type_bit.equles)
	{
		for (char* i = buff; *i != 0; i++)
		{
			if (*i == ';')
			{
				n_node->type_ = endl;
				n_node->line = line;
			}
			else if (*i == '=')
			{
				n_node->type_ = equles;
				n_node->value_char_ptr = getchar_x(*i);


				if (n_node->is_flagged)
				{
					next->type_ = n_node->flag_;
					next->is_flagged = true;
					if (n_node->flag_ == parentheses1)
					{
						next->flag_ = comma;
					}
					else
					{
						next->flag_ = n_node->flag_;
					}
				}
				else
				{
					next->type_ = value | var_name | parentheses1;
					if (n_node->type_ != equles)
						next->type_ |= equles;
				}
				next->type_ |= value | var_name | parentheses1 | equles | parentheses4 | keyword | s_index;
				parse_line_ctx(ctx, i + 1, next, line);
				return;
				////do next->.
			}
			else if ((*i >= 42 && *i <= 47) || *i == 0x3c || *i == 0x3e || *i == 38 || *i == '|' || *i == '%' || *i == '!' || *i == '^' || *i == '~')//oprator +-*/%^~
			{
				n_node->type_ = operators_n;
				n_node->value_char_ptr = getchar_x(*i);
				next->parent = n_node;

				if (n_node->parent != NULL && n_node->parent->type_ == operators_n)
				{
					if (*(char*)n_node->value_raw == *(char*)n_node->parent->value_raw ||
					    *(char*)n_node->value_raw == '-' || *(char*)n_node->value_raw == '+')
					{
						next->type_ = var_name | value | parentheses4 | endl;
						inherit_parent_flag(n_node, next);
						if ((n_node->parent != NULL && n_node->parent->is_flagged) || n_node->is_flagged)
						{
							next->flag_ = n_node->flag_;
							next->is_flagged = true;
						}

						parse_line_ctx(ctx, i + 1, next, line);
						return;
					}
					////////TEST
					next->type_ = endl;


					if (n_node->parent != NULL)
						next->opt_raw = n_node->parent->opt_raw;

					//v2//var temps ;var* main=(var*)d->parent->opt;
					//v2//strcpy_s(temps.var_type ,main->var_type);
					//v2//temps.value = new int(1);

					parse_line_ctx(ctx, i + 1, next, line);
					return;
				}
				else
				{
					
					next->type_ = value | var_name | operators_n | equles |parentheses4;

					if ((n_node->parent != NULL && n_node->parent->is_flagged) || n_node->is_flagged)
					{
						next->flag_ = n_node->flag_;
						next->is_flagged = true;
					}

					parse_line_ctx(ctx, i + 1, next, line);
					return;
				}
				//next->type_=value|var_name|operators_n;
			}
		}
		//d->next = NULL;
		//d->btype.nnType.operators_n = 0;
		//parse_line(buff, d, line);
		int ucol = (ctx && ctx->current_line_start && buff >= ctx->current_line_start)
		         ? (int)(buff - ctx->current_line_start) + 1 : 1;
		int utlen = (int)strlen(buff);
		char errmsg[256];
		snprintf(errmsg, sizeof(errmsg), "unrecognized token: '%s'", buff);
		xdiag_report(DIAG_ERROR, "E0001", ctx ? ctx->source_file : NULL, line, ucol, utlen, NULL, errmsg, "check for syntax errors, typos, or unsupported symbols");
		if (ctx != NULL) ctx->has_error = true;
		return;
	}
	//mdebuge.cprintf(12, "\nunrecognized token : [ %s ] in line [ %d ] \n", buff, line);
}

void parse_line(char* buff, node* n_node, const int line)
{
	parse_line_ctx(current_parser_ctx, buff, n_node, line);
}

//if()
typedef var ver;

void pre_parse_line_ctx(ParserContext* ctx, char* buff, const int line)
{
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx != NULL)
		ctx->current_line_start = buff;

	node* n;
	node* cur_save = (ctx != NULL) ? ctx->save : NULL;

	if (cur_save == NULL)
	{
		n = new_node(nodes);
		n->line = line;
		n->type_ = 32775;
		n->is_flagged = false;
	}
	else
	{
		n = cur_save;
		if (ctx != NULL)
			ctx->save = NULL;
	}

	char* line_to_parse = buff;
	char* allocated_buff = NULL;

	if (buff != NULL)
	{
		const char* p = buff;
		while (*p == ' ' || *p == '\t')
			p++;

		if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
		{
			char id[128];
			int idlen = 0;
			while (((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
			        (*p >= '0' && *p <= '9') || *p == '_') && idlen < 127)
			{
				id[idlen++] = *p++;
			}
			id[idlen] = '\0';

			while (*p == ' ' || *p == '\t')
				p++;

			if (*p == '(' && is_function_def_ahead(p))
			{
				bool is_constr = false;
				const char* c_name = parser_current_class_name(ctx);
				if (c_name != NULL && strcmp(id, c_name) == 0)
					is_constr = true;
				else
				{
					type_def* td = get_type_by_name(id);
					if (td != NULL && !is_base_type(td))
						is_constr = true;
				}
				if (is_constr)
				{
					size_t new_len = strlen(buff) + 5;
					allocated_buff = (char*)malloc(new_len);
					snprintf(allocated_buff, new_len, "int %s", buff);
					line_to_parse = allocated_buff;
				}
			}
		}
	}

	parse_line_ctx(ctx, line_to_parse, n, line);

	if (allocated_buff != NULL)
		free(allocated_buff);
}

void pre_parse_line(char* buff, const int line)
{
	pre_parse_line_ctx(current_parser_ctx, buff, line);
}

#if  defined(__GNUC__)|| defined(__MINGW64__)
#define strcpy_s(x,y,z) strcpy(x,z)
#endif

static ParserContext interactive_ctx;
static bool interactive_ctx_inited = false;

void parser_interactive_cleanup(void)
{
	if (interactive_ctx_inited)
	{
		parser_context_cleanup(&interactive_ctx);
		interactive_ctx_inited = false;
	}
}

void start_parse_lines_ctx(ParserContext* ctx, char* buffe, bool active)
{
	if (buffe == NULL || *buffe == '\0')
		return;
	if (ctx == NULL)
		ctx = current_parser_ctx;
	if (ctx != NULL)
	{
		ctx->current_parsing_class_name = NULL;
		ctx->scope_top = 0;
		if (ctx->source_code == NULL)
			ctx->source_code = buffe;
		if (ctx->source_file == NULL)
			ctx->source_file = xdiag_get_current_file();
	}

	size_t size1 = strlen(buffe) + 1;
	char* buff = (char*)malloc(size1);
	memset(buff, 0, size1);
	strcpy_s(buff, size1, buffe);
	int mline = 1;

	char* t = buff;
	int size = strlen(buff) - 1;
	char* line_save = buff;
	int i = 0;
	while (true)
	{
		if (*t == '\n' || t == buff + (size + 1))
		{
			if (i > 0)
			{
				char* m = (char*)malloc(i + 1);
				memset(m, 0, i + 1);
				memcpy(m, line_save, i);
				char* trimmed = m;
				while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
				if (*trimmed != '#' && strncmp(trimmed, "//", 2) != 0 && *trimmed != '\0')
					pre_parse_line_ctx(ctx, m, mline);
				free(m);
			}
			if (t > buff + size)
				break;
			line_save = ++t;

			mline++;
			i = 0;
		}
		else
		{
			i++;
			t++;
		}
	}
	fl* ip = (ctx != NULL) ? parser_delim_get_first_unclosed(ctx) : static_flag_check2();
	if (!active && ip != NULL && ip->waiting_node != NULL)
	{
		int eline = ip->waiting_node->line;
		int ecol = ip->waiting_node->col > 0 ? ip->waiting_node->col : 1;
		char dmsg[256];
		snprintf(dmsg, sizeof(dmsg), "unclosed delimiter '%s'", parse_obj_to_str(ip->waiting_node->btype.name));
		xdiag_report(DIAG_ERROR, "E0002", ctx ? ctx->source_file : NULL, eline, ecol, 1, NULL, dmsg, "missing matching closing delimiter");
		if (ctx != NULL) ctx->has_error = true;
	}
	free(buff);
}

void start_parse_lines(char* buffe, bool active)
{
	if (buffe == NULL || *buffe == '\0')
		return;

	if (active)
	{
		if (!interactive_ctx_inited)
		{
			parser_context_init(&interactive_ctx, true);
			interactive_ctx_inited = true;
		}
		interactive_ctx.source_file = "<stdin>";
		interactive_ctx.source_code = buffe;
		current_parser_ctx = &interactive_ctx;
		start_parse_lines_ctx(&interactive_ctx, buffe, true);
		return;
	}

	ParserContext ctx;
	parser_context_init(&ctx, active);
	ctx.source_file = xdiag_get_current_file();
	ctx.source_code = buffe;

	/* Push previous context to support nested/re-entrant parsing */
	ctx.prev_ctx = current_parser_ctx;
	current_parser_ctx = &ctx;

	start_parse_lines_ctx(&ctx, buffe, active);

	/* Restore previous context */
	current_parser_ctx = ctx.prev_ctx;
	parser_context_cleanup(&ctx);
}
