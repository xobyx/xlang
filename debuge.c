#include "debuge.h"



//extern node_type staic_flag[];
/*
void(*print_line_debuge)(Debug x, const node* bx, int line);
void(*do_work)(Debug x, node* temp);
void(*test_color)(Debug x);
int(*cprintf)(Debug x, byte color, const char* format, ...);
void(*addnode)(Debug x, node* y);
void(*checknode)(Debug x, node* y);


*/




int _cprintf(Debug* x, byte color, const char* format, ...)
{
	//va_list arglist;
	//va_start(arglist, format);
	SetConsoleTextAttribute(x->hConsole, color);
	va_list args;
	va_start(args, format);
	const int u = vprintf(format, args);
	va_end(args);
	SetConsoleTextAttribute(x->hConsole, x->wOldColorAttrs);
	return u;
}



void _do_work(Debug* x, node* temp)
{
	x->cprintf(x, 0x8F, "%s", parse_obj_str(temp->btype.name));
	if (temp->opt != NULL && temp->type_ == value)
		x->cprintf(x, 0x05, " (%s)", ((type*)temp->opt)->name);
	x->checknode(x, temp);
	if (temp->ref_node != NULL)
		x->addnode(x, temp);
	if ((temp->btype.value) & (HAVE_VAR_VALUE))
	{
		printf(" [ ");
		if (temp->type_ == itype)
			x->cprintf(x, 0x02, "%s", ((type*)temp->value)->name);
		else if (temp->type_ == keyword)
			x->cprintf(x, 0x02, "%s", key_word[(int)temp->value]);
		else
			x->cprintf(x, 0x02, "%s", temp->value != NULL ? (char*)temp->value : "NONE");
		printf(" ]");
	}
	x->cprintf(x, 0x04, temp->type_ == endl ? "\n" : " --> ");


	if (temp->next == NULL) printf("\n\n");
}

void _test_color(Debug* x)
{
	bit8_color mcolor;
	for (int u = 1; u < 16; u++)
	{
		mcolor.bf.foreground = 0;
		mcolor.bf.background = u;
		SetConsoleTextAttribute(x->hConsole, mcolor.bf_color);
		printf("******%d******\n", u);
	}
}

void _print_line_debuge(Debug* x, node* bx, int line)
{
	const WORD colors[] =
	{
		0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F,
		0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6
	};
	//FlushConsoleInputBuffer(this->hConsole);

	//!out_put
	//if(false)return;
	node* temp = getRoot(bx);

	// COORD t= csbiInfo.dwSize;


	 //* First save the current color information


	// COORD tm;
	//	int i=0;
	while (temp != NULL)
	{
		//	CONSOLE_SCREEN_BUFFER_INFO y;
				//	GetConsoleScreenBufferInfo(hConsole,&y);

				//	y.dwCursorPosition.X=y.dwCursorPosition.X+  t.X/2;
				//	SetConsoleCursorPosition(this->hConsole,y.dwCursorPosition);
		x->do_work(x, temp);


		//if (temp->type_ == parse_obj::endl)
		//printf("[ f-%s ]\n--> ", this->list[b].name);


		temp = temp->next;




		//temp=temp==NULL?NULL:temp->next;
	}
	//
	SetConsoleTextAttribute(x->hConsole, x->wOldColorAttrs);
}

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
			cprintf(x, x->stack[i].color.bf_color, " %d ", x->stack[i].t);
			x->color_index = x->color_index > 15 ? 10 : x->color_index;

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
			cprintf(x, x->stack[i].color.bf_color, "% d ", x->stack[i].t);
			return;
		}
	}
}
Debug _debuge_ = {
		.hConsole = 0,
		.t = 0,
		.print_line_debuge = _print_line_debuge,
		.do_work = _do_work,
		.test_color = _test_color,
		.cprintf = _cprintf,
		.addnode = _addnode,
		.checknode = _checknode,
		.list = {
	{"itype",0x01},{"keyword",0x02},{"var_name",0x04},{"value",0x08},{"operators_n",0x10},{"equles",0x20},{"endl",0x40},
	{"size", 0x0080} ,{"parentheses1",0x0100},{"parentheses1c",0x0200},{"comma",0x0400}, {"index",0x0800},
	{"parentheses4" ,0x1000} , {"parentheses4c",0x2000}, {"dot",0x4000} ,{"twodot",0x8000}

	},
	.stack = {0}
};

Debug * init_debug()
{

	Debug * xc = &_debuge_;// (Debug*)malloc(sizeof(Debug));
	// memcpy(xc,&debuge,sizeof(Debug));
	xc->hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(xc->hConsole, &xc->csbiInfo);

	xc->wOldColorAttrs = xc->csbiInfo.wAttributes;
	xc->color_index = 10;


	return xc;
}