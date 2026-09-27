#include "debuge.h"

#include <stdarg.h>

static const char* node_opt_names[] = {
	"function_def", "function_call", "fucnction_parm", "var_def", "var_call",
	"class_def", "class_base_def", "psize", "pindex", "var_call_ref"
};

const char* parse_obj_str(node* nod)
{
	switch (nod->btype.name)
	{
	case itype: return "itype";
	case keyword: return "keyword";
	case var_name:
	{
		int idx = nod->opt_name_type - 10;
		if (idx >= 0 && idx < (int)(sizeof(node_opt_names) / sizeof(node_opt_names[0])))
			return node_opt_names[idx];
		return "var_name";
	}
	case value: return "value";
	case operators_n: return "operators_n";
	case equles: return "equles";
	case endl: return "endl";
	case s_index: return "s_index";
	case parentheses1: return "parentheses1";
	case parentheses1_c: return "parentheses1c";
	case comma: return "comma";
	case s_index_c: return "s_index_c";
	case parentheses4: return "parentheses4";
	case parentheses4_c: return "parentheses4c";
	case dot: return "dot";
	case twodot: return "twodot";
	case pars: return "PARS";
	case have_var_value: return "HAVE_VAR_VALUE";
	case a: return "a";
	case non_one_char: return "NON_ONE_CHAR";
	case none: break;
	default:
		return "unknown";
	}
	return NULL;
}

int _cprintf(Debug* x, byte color, const char* format, ...)
{
#if defined(_MSC_VER)
	if (x->hConsole)
		SetConsoleTextAttribute(x->hConsole, color);
#else
	(void)x;
	(void)color;
#endif
	va_list args;
	va_start(args, format);
	const int u = vprintf(format, args);
	va_end(args);
#if defined(_MSC_VER)
	if (x->hConsole)
		SetConsoleTextAttribute(x->hConsole, x->wOldColorAttrs);
#else
	printf("\x1B[0m");
#endif
	return u;
}

void _do_work(Debug* x, node* temp)
{
	x->cprintf(x, 0x8f, "\x1B[1;37m\x1B[47;100m%s", parse_obj_str(temp));
	if (temp->type_ == value && temp->opt_raw != NULL)
		x->cprintf(x, 0x05, "\x1B[35m\x1B[40m(%s)", temp->opt_type_ptr->type_name);
	x->checknode(x, temp);
	if (temp->ref_node != NULL)
		x->addnode(x, temp);

	if ((temp->btype.value) & (have_var_value))
	{
		printf(" [ ");
		if (temp->type_ == itype)
			x->cprintf(x, 0x02, "\x1B[32m\x1B[40m%s", temp->value_type->type_name);
		else if (temp->type_ == var_name)
			x->cprintf(x, 0x02, "\x1B[32m\x1B[40m%s", temp->value_char_ptr);
		else if (temp->type_ == keyword)
			x->cprintf(x, 0x02, "\x1B[32m\x1B[40m%s", key_word[temp->value_keyword]);
		else
			x->cprintf(x, 0x02, "\x1B[32m\x1B[40m%s", temp->value_raw != NULL ? (char*)temp->value_raw : "NONE");
		printf(" ]");
	}
	x->cprintf(x, 0x04, "%s", temp->type_ == endl ? "\n" : "\x1B[34m --> ");

	if (temp->next == NULL) printf("\n\n");
}

void _print_line_debuge(Debug* x, node* bx, int line)
{
	(void)line;
	node* temp = get_root(bx);
	while (temp != NULL)
	{
		x->do_work(x, temp);
		temp = temp->next;
	}
}

static const char* co[] = {"\x1B[41m %d ", "\x1B[42m %d ", "\x1B[43m %d ", "\x1B[44m %d ", "\x1B[45m %d ", "\x1B[46m %d "};

void _addnode(Debug* x, node* y)
{
	for (int i = 0; i < 10; i++)
	{
		if (x->stack[i].m == NULL)
		{
			x->stack[i].m = y->ref_node;
			x->stack[i].color.bf.foreground = 0;
			x->stack[i].color.bf.background = x->color_index;
			x->stack[i].t = x->t++;
			x->color_index++;
			x->cprintf(x, x->stack[i].color.bf_color, co[x->stack[i].color.bf.background], x->stack[i].t);
			x->color_index = x->color_index > 5 ? 0 : x->color_index;
			break;
		}
	}
}

void _checknode(Debug* x, node* y)
{
	for (int i = 9; i >= 0; i--)
	{
		if (x->stack[i].m == y)
		{
			x->stack[i].m = NULL;
			x->cprintf(x, x->stack[i].color.bf_color, co[x->stack[i].color.bf.background], x->stack[i].t);
			return;
		}
	}
}

struct Debug _debuge_ = {
#if defined(_MSC_VER)
	.hConsole = 0,
#endif
	.t = 0,
	.print_line_debuge = _print_line_debuge,
	.do_work = _do_work,
	.test_color = NULL,
	.cprintf = _cprintf,
	.addnode = _addnode,
	.checknode = _checknode,
	.list = {
		{"itype", 0x01}, {"keyword", 0x02}, {"var_name", 0x04}, {"value", 0x08},
		{"operators_n", 0x10}, {"equles", 0x20}, {"endl", 0x40}, {"size", 0x0080},
		{"parentheses1", 0x0100}, {"parentheses1c", 0x0200}, {"comma", 0x0400},
		{"index", 0x0800}, {"parentheses4", 0x1000}, {"parentheses4c", 0x2000},
		{"dot", 0x4000}, {"twodot", 0x8000}
	},
	.color_index = 0
};

Debug* init_debug(void)
{
	Debug* xc = &_debuge_;
#if defined(_MSC_VER)
	xc->hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(xc->hConsole, &xc->csbiInfo);
	xc->wOldColorAttrs = xc->csbiInfo.wAttributes;
#endif
	xc->color_index = 0;
	return xc;
}
