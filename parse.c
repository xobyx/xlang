#include "parse.h"
#include "pcre_fuction.h"


//[if[0],for[1],while[2],do[3],else[4],print[5],return[6]]

bool out_put;
#define ALL_POS 255





//must_after_found_type...


bool mbool = false;

void* getchar_x(const char f)
{
	for (int i = 0; i < 33; i += 2)
	{
		if (one_c[i] == f)
			return (void*)&one_c[i];
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




node* save = NULL;
int r = 0;



void parse_line(char* buff, node* n_node, const int line)
{
	
	
	n_node->type_ |= operators_n;

	while (buff != NULL && (*buff == ' ' || *buff == '\t' || *buff == '\n' || *buff == '\r' || *buff == ';'))
	{
		buff++;
		if(*buff=='\n')n_node->line++;
	}
	n_node->line = line;
	
	node_type a = (node_type)0;
	if (static_flag_check2x(&a))
	{
		n_node->type_ |= a;
	}
	///line end but still need to close node
	if (strlen(buff) == 0)
	{

		///check for unclosed node if yes next line will encluded to cun node
		if (static_flag_check2x(&a))
		{
			node* nextc = new_node(nodes);
			n_node->type_ = endl;
			n_node->line = line;
			n_node->next = nextc;
			nextc->parent = n_node;
			save = nextc;
			nextc->type_ = a | keyword | itype | var_name | n_node->parent->flag_;
			return;
		}
		n_node->type_ = endl;
		n_node->next = NULL;
		n_node->line = line;
		
		if (print_parse_log)
			debuge->print_line_debuge(debuge, n_node, line);


		compile(NULL, n_node, NULL, NULL);



		return;
	}

	
	node* next = new_node(nodes);
	next->parent = n_node;
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
		find* mfind = match("^(\\b\\w+)", buff);
		if (mfind->isFind)
			m_type = get_type_by_name(mfind->bn);

		if (m_type != NULL)///V3 if(strstr(buff,base_type[i])!=NULL)
		{
			n_node->line = line;
			//d->opt= new int(1);

			n_node->type_ = itype;
			n_node->value_type = m_type;


			next->type_ = var_name | s_index;  // itype->var_name->(->itype->var_name)
			if (n_node->parent != NULL && n_node->parent->_opt_ptr_ == function_def) //any node have _opt_ptr_==function_def
			{
				next->type_ = var_name;
				next->opt_name_type = fucnction_parm; // "p";

				next->is_flagged = true;
				next->flag_ = parentheses4_c;
			}

			parse_line(buff + strlen(m_type->type_name), next, line);
			return;
		}
		else
		{
			char * pos_error = "UNKNOWN TYPE {%s} in line {%d}";
			//printf("UNKNOWN TYPE {%s} in line {%d} :",mfind.bn,line);
			//exit(-1);
		}
	}
	n_node->btype.node_type_bit.itype = 0;
	if ((*buff == '[' && n_node->btype.node_type_bit.s_index) || (*buff == ']'&&n_node->btype.node_type_bit.s_index_c))
	{
		if (*buff == '[')
		{
			n_node->type_ = s_index;
			n_node->value_raw = getchar_x(*buff);

			static_flag_op2(s_index, n_node, false);
			static_flag_op2(s_index_c, n_node, true);

			next->type_ = value | var_name;

			if (n_node->parent->type_ == itype)
			{
				n_node->opt_name_type = psize;
			}
			else if (n_node->parent->type_ == var_name)
			{
				n_node->opt_name_type = pindex;
			}
			else
			{
				printf("Error");
				return;
			}
			next->parent = n_node;
			next->flag_ = s_index_c;
			/*,} */
			;
			next->is_flagged = true;
			parse_line(buff + 1, next, line);

			return;
		}
		else if (*buff == ']')
		{
			n_node->type_ = s_index_c;

			node* open_node = static_flag_op2(s_index_c, n_node, false);
			n_node->value_raw = getchar_x(*buff);
			//FIXME:

			if (open_node->opt_name_type == psize)
			{
				next->type_ = var_name;
				next->flag_ = equles | endl;
			}
			else
			{
				next->type_ = operators_n | equles;
				next->flag_ = endl;
			}


			next->parent = n_node;
			n_node->next = next;
			parse_line(buff + 1, next, line);
			return;
		}
	}
	if (*buff == '{' && n_node->btype.node_type_bit.parentheses1)
	{

		n_node->type_ = parentheses1;
		inherit_parent_flag(n_node, next);
		n_node->value_raw = getchar_x(*buff);

		mbool = true;

		static_flag_op2(parentheses1, n_node, false);
		static_flag_op2(parentheses1_c, n_node, true);
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
		parse_line(buff + 1, next, line);
		return;

	}
	if (*buff == '}' && n_node->btype.node_type_bit.parentheses1c)
	{


		node * open = static_flag_op2(parentheses1_c, n_node, false);
		n_node->ref_node = open;
		n_node->type_ = parentheses1_c;
		n_node->value_raw = getchar_x(*buff);

		mbool = false;

		next->type_ = endl | comma | itype | keyword | var_name;
		next->flag_ = parentheses1/*,} */;
		next->is_flagged = true;

		parse_line(buff + 1, next, line);
		return;

	}
	if (*buff == ',' && n_node->btype.node_type_bit.comma)
	{

		n_node->type_ = comma;
		n_node->value_raw = getchar_x(*buff);


		next->type_ = value | var_name | itype;
		next->flag_ = parentheses1_c | comma/*,} */;
		next->is_flagged = true;
		parse_line(buff + 1, next, line);
		return;

	}
	if ((*buff == '(' && n_node->btype.node_type_bit.parentheses4) || (*buff == ')'&& n_node->btype.node_type_bit.parentheses4c))
	{
		if (*buff == '(')
		{
			n_node->type_ = parentheses4;
			n_node->value_raw = getchar_x(*buff);

			static_flag_op2(parentheses4, n_node, false);
			static_flag_op2(parentheses4_c, n_node, true);

			next->type_ = value | var_name | parentheses4_c;
			if (n_node->parent != NULL && n_node->parent->type_ == var_name)
			{
				const int typ = n_node->parent->opt_name_type;
				if (typ == var_def)
				{
					next->type_ = itype | parentheses4_c;
					n_node->parent->_opt_ptr_ = function_def;//FUNCTION;//the type
					n_node->opt_name_type = function_def;// FUNCTION;
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
					next->type_ = value | var_name | parentheses4_c;
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
			parse_line(buff + 1, next, line);
			return;
		}
		else if (*buff == ')')
		{
			n_node->type_ = parentheses4_c;

			node* pp = static_flag_op2(parentheses4_c, n_node, false);
			n_node->ref_node = pp;
			n_node->value_raw = getchar_x(*buff);
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
				next->type_ = operators_n;
				next->flag_ = value | var_name | keyword;
				next->is_flagged = true;
			}


			next->parent = n_node;
			n_node->next = next;
			parse_line(buff + 1, next, line);
			return;
		}
	}
	if (n_node->btype.node_type_bit.keword)
	{
		int i = 0;
		for (i = 0; i < 10; i++)
		{
			if (eql(buff, key_word[i]))///V3 if(strstr(buff,key_word[i])!=NULL)
			{
				n_node->type_ = keyword;


				n_node->value_keyword = i;


				//if (eql("print", key_word[i]) != -1)
				//{
				//	next->type_ = var_name | value;
				//}
				if (i == _if_ || i == _while_ || i == _else_ || i == _eif_)

				{
					if (i == _else_)
					{
						if ((n_node->parent != NULL && n_node->parent->type_ == parentheses1_c))
						{
							n_node->ref_node = n_node->parent->ref_node->parent->ref_node->parent; ///if
							n_node->ref_node->next_jump = n_node;  //save next jump to if
						}
						else if (n_node->stack_parent->parent->type_ == parentheses1_c)
						{
							n_node->ref_node = get_first_type_with_value(get_root(n_node->stack_parent->parent), keyword, (void*)_eif_);
							if (n_node->ref_node == NULL)
							{
								n_node->ref_node = get_first_type_with_value(get_root(n_node->stack_parent->parent), keyword, (void*)_if_);
							}
							n_node->ref_node->next_jump = n_node;
						}
						else
						{
							printf("ERROR: no if body");
							exit(-1);
						}
						n_node->next_jump = NULL;
						next->flag_ = var_name | value;
						next->type_ = parentheses1;

						static_flag_op2(parentheses1, NULL, true);
						next->is_flagged = true;
						parse_line(buff + strlen(key_word[i]), next, line);
						return;

					}
					if (i == _eif_)
					{
						if ((n_node->parent != NULL && n_node->parent->type_ == parentheses1_c))
						{
							n_node->ref_node = n_node->parent->ref_node->parent->ref_node->parent;
						}

						else if (n_node->stack_parent->parent->type_ == parentheses1_c)
						{
							n_node->ref_node = get_first_type_with_value(get_root(n_node->stack_parent->parent), keyword, (void*)_eif_);
							if (n_node->ref_node == NULL)
							{
								n_node->ref_node = get_first_type_with_value(get_root(n_node->stack_parent->parent), keyword, (void*)_if_);

								//TODO:
							}
						}
						else
						{
							printf("ERROR: no if body");
						}

					}
					n_node->next_jump = NULL;
					next->flag_ = var_name | value;
					next->type_ = parentheses4;
					static_flag_op2(parentheses1, NULL, true);
					static_flag_op2(parentheses4, NULL, true);
					next->is_flagged = true;
				}
				else if (i == _return_)
				{
					next->type_ = var_name | value;
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
					next->type_ = var_name;
					next->flag_ = parentheses4;


					static_flag_op2(parentheses1, NULL, true);
					static_flag_op2(parentheses4, NULL, true);
				}
				else if (i == _class_)
				{
					next->type_ = var_name;

					next->flag_ = parentheses4;
					next->is_flagged = true;
					parse_line(buff + strlen(key_word[i]), next, line);
					return;
				}
				else
				{
					next->type_ = var_name | value;
				}
				inherit_parent_flag(n_node, next);
				if (n_node->is_flagged) { next->flag_ |= n_node->flag_; }


				parse_line(buff + strlen(key_word[i]), next, line);
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
		find* mfind = match("^[\"\'](.+?)[\"\']", buff);
		if (mfind->isFind && *(buff + 1) == *mfind->bn)

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
			scap_string(mfind->bn);
			n_node->value_raw = mfind->bn;

			//((var*)d->opt)->value=mfind->bn;

			///TODO: check type match
			//v2/setVar(&d);
			parse_line(buff + strlen(mfind->bn) + 2, next, line);
			//parse_line(buff + strlen(mfind->bn) , next, line);
			return;
		}
		mfind = match("^(false|true)", buff);
		if (mfind->isFind && *buff == *mfind->bn)
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
			parse_line(buff + strlen(mfind->bn), next, line);
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
			mfind = match("^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)", buff);
			if (mfind->isFind)
			{
				// parse   "\d+";

				n_node->type_ = value;
				n_node->opt_type_ptr = T_INT;
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
				parse_line(buff + strlen(mfind->bn), next, line);
				return;
			}
		}
	}
	n_node->btype.node_type_bit.value = 0;
	//TODO :if(d->type_==var_name|d->parent->type_==itype)
	if (n_node->btype.node_type_bit.var_name)
	{
		//?([0-9|A-Z|a-z_]+[_0-9|A-Z|a-z]+)[ \n]
		find* mfind = match("^(:?\\b\\w+)", buff);//var name



		if (mfind->isFind && *mfind->bn == *buff && strcmp(mfind->bn, "string") != 0)
		{
			//TODO:: if is BASE parse_obj

			n_node->line = line;
			n_node->value_raw = mfind->bn;
			n_node->type_ = var_name;
			next->parent = n_node;
			n_node->next = next;
			//v2//d->opt=&rv;

			if (n_node->opt_name_type == fucnction_parm)
			{
				next->type_ = comma | n_node->flag_;

				next->is_flagged = true;
				next->flag_ = itype | n_node->flag_;
				parse_line(buff + strlen(mfind->bn), next, line);
				return;
			}

			if (n_node->parent != NULL)
			{
				if (n_node->parent->type_ == keyword && n_node->parent->value_short == _class_)
				{
					next->type_ = parentheses4;
					n_node->opt_name_type = class_def; //k;
					next->is_flagged = true;
					next->flag_ = var_name | parentheses4_c;
					parse_line(buff + strlen(mfind->bn), next, line);
					return;
				}
				//class f(-)-
				if (n_node->parent->type_ == parentheses4 && n_node->parent->opt_name_type == class_base_def)
				{
					next->type_ = parentheses4_c;
					n_node->opt_raw = "i";
					next->is_flagged = true;
					next->flag_ = parentheses1;
					parse_line(buff + strlen(mfind->bn), next, line);
					return;
				}
				if (n_node->parent->type_ == itype)
				{
					n_node->opt_name_type = var_def;
				}
				else
				{
					n_node->opt_name_type = var_call;
				}
			}
			else
			{
				n_node->opt_name_type = var_call;
			}


			//if(d->parent->btype.node_type_bit.size)equles|endl;
			//if(d->parent->btype.node_type_bit.comma)equles|endl;

			next->type_ = (equles | endl | operators_n | s_index |
				parentheses4 | dot);
			inherit_parent_flag(n_node, next);
			if (n_node->is_flagged)
			{
				next->is_flagged = true;
				next->flag_ = n_node->flag_ | var_name;
				next->type_ |= n_node->flag_;
			}

			parse_line(buff + strlen(mfind->bn), next, line);
			//print_line_debuge(&nex,line,true);

			return;
		}
	}
	n_node->btype.node_type_bit.var_name = 0;
	if (*buff == '.' && n_node->btype.node_type_bit.dot)
	{
		n_node->type_ = dot;
		next->type_ = var_name;
		parse_line(buff + 1, next, line);
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
				n_node->value_raw = getchar_x(*i);


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
				next->type_ |= value | var_name | parentheses1 | equles;
				parse_line(i + 1, next, line);
				return;
				////do next->.
			}

			else if ((*i >= 42 && *i <= 47) || *i == 0x3c || *i == 0x3e || *i == 38 || *i == '|')//oprator +-*/
			{
				n_node->type_ = operators_n;
				n_node->value_raw = getchar_x(*i);
				next->parent = n_node;

				if (n_node->parent->type_ == operators_n)
				{
					if (*(char*)n_node->value_raw == *(char*)n_node->parent->value_raw)
					{
						next->type_ = var_name | value;
						inherit_parent_flag(n_node, next);
						if (n_node->parent->is_flagged || n_node->is_flagged)
						{
							next->flag_ = n_node->flag_;
							next->is_flagged = true;
						}

						parse_line(i + 1, next, line);
						return;
					}
					////////TEST
					next->type_ = endl;


					next->opt_raw = n_node->parent->opt_raw;

					//v2//var temps ;var* main=(var*)d->parent->opt;
					//v2//strcpy_s(temps.var_type ,main->var_type);
					//v2//temps.value = new int(1);

					parse_line(i + 1, next, line);
					return;
				}
				else
				{
					//next->parent=d;
					next->type_ = value | var_name | operators_n | equles;
					if (n_node->parent->is_flagged || n_node->is_flagged)
					{
						next->flag_ = n_node->flag_;
						next->is_flagged = true;
					}

					parse_line(i + 1, next, line);
					return;
				}
				//next->type_=value|var_name|operators_n;
			}
		}
		//d->next = NULL;
		//d->btype.nnType.operators_n = 0;
		//parse_line(buff, d, line);
		printf("\nunrecognized token : [ %s ] in line [ %d ] \n", buff, line);
		return;
	}
	//mdebuge.cprintf(12, "\nunrecognized token : [ %s ] in line [ %d ] \n", buff, line);
}

//if()
typedef var ver;

void pre_parse_line(char* buff, const int line)
{
	node* n;

	if (save == NULL)
	{
		n = new_node(nodes);
		n->line = line;
		n->type_ = 32775;
		n->is_flagged = false;
	}
	else
	{
		n = save;
		save = NULL;
	}


	parse_line(buff, n, line);
}

char* line[500] = { 0 };
int i;
char* diro;
#if  defined(__GNUC__)|| defined(__MINGW64__)
#define strcpy_s(x,y,z) strcpy(x,z)
#endif
void start_parse_lines(char* buffe, bool active)
{
	size_t size1 = strlen(buffe) + 1;
	char* buff = (char*)malloc(size1);
	memset(buff, 0, size1);
	strcpy_s(buff, size1, buffe);
	int mline = 1;

	/*char *temp;
	char *p = strtok_s(buff, "\n", &temp);
	do
	{
		if(*temp=='\n')++line;
		pre_parse_line(p, mline++);
	}
	while ((p = strtok_s(NULL, "\n", &temp)) != NULL);*/
	char* t = buff;
	//debuge::bit8_color y;
	//y.bf.background = 3;
	//y.bf.foreground = 15;
	int size = strlen(buff) - 1;
	char* save = buff;
	int i = 0;
	while (true)
	{
		if (*t == '\n' || t == buff + (size + 1))
		{
			if (i > 0)
			{
				char* m = (char*)malloc(i + 1);
				memset(m, 0, i + 1);
				memcpy(m, save, i);
				if (*m != '#')
					pre_parse_line(m, mline);
				//else
				//	mdebuge.cprintf(y.bf_color, "\ncomment: [%s]\n\n", m + 1);
				//printf("%2d : %s\n",mline,m);
				free(m);
			}
			if (t > buff + size)
				break;
			save = ++t;

			mline++;
			i = 0;
		}
		else
		{
			i++;
			t++;
		}
	}
	fl* ip = static_flag_check2();
	if (!active && ip != NULL && ip->waiting_node != NULL) //null in if() case
	{
		printf("Error : unclosed %s in line %d", parse_obj_to_str(ip->waiting_node->btype.name), ip->waiting_node->line);
	}
	/*char** at = get_lines_array(buff);


	for (char** y = at; *y != NULL; y++)
	{
		if(*y!="")
		pre_parse_line(*y, mline);

		mline++;
	}*/
	free(buff);
}
