#include "xlang_main.h"


void setup_ref(node_stack* stack);

void s_compile_(node_stack* stack);

void save_file(char* file, node_stack* a, unsigned int MD5_hash[4])
{
	node* top = a->root;


	FILE* f = fopen(file, "wb");
	if (!f) return;
	char r = 0xff;
	int e = 0;
	fwrite(MD5_hash, 16, 1, f);

	while (top != NULL)
	{
		//fwrite(top,sizeof(node),1,f);

		fwrite(&r, 1, 1, f);   // 0xff
		fwrite(&top->id, 4, 1, f);

		//int y = int(top);
	//	fwrite(&y, 4, 1, f);////id
		fwrite(&top->btype, sizeof(node_type_raw), 1, f);
		fwrite(&top->fflag, sizeof(node_type_raw), 1, f);
		fwrite(&top->is_flagged, 1, 1, f);
		fwrite(&top->line, 4, 1, f);

		if (top->next != NULL)
		{
			int u = top->next->id;
			fwrite(&u, 4, 1, f);
		}
		else
		{
			fwrite(&e, 4, 1, f);
		}
		if (top->parent != NULL)
		{
			int u = top->parent->id;
			fwrite(&u, 4, 1, f);
		}
		else
		{
			fwrite(&e, 4, 1, f);
		}

		int msize = 0;
		if (true)//top->value_raw != NULL)
		{
			if(top->type_==operators_n)
                       {
                        msize=1;
			fwrite(&msize, 4, 1, f);
			fwrite(top->value_char_ptr, msize, 1, f);
			}
			else if(top->type_==keyword)
			{
			msize=4;
			fwrite(&msize, 4, 1, f);
			fwrite(&top->value_keyword,msize, 1, f);
			}
			else if(top->type_==var_name)
			{
			msize = strlen((char*)top->value_raw);
			fwrite(&msize, 4, 1, f);
			fwrite(top->value_char_ptr,msize, 1, f);

			}
			else if(top->type_==value)
			{
				if(top->opt_type_ptr == T_INT ||top->opt_type_ptr == T_BOOL ||
				top->opt_type_ptr == T_FLOAT ||top->opt_type_ptr == T_LONG)
				{
				msize = 4;
				fwrite(&msize, 4, 1, f);
				fwrite(top->value_int,msize, 1, f);
				}
				else if(top->opt_type_ptr == T_STRING)
				{
				msize = strlen(top->value_char_ptr);
				fwrite(&msize, 4, 1, f);
				fwrite(top->value_char_ptr,msize, 1, f);
				}


			}
			else if(top->type_==itype)
			{

			msize = 4;
			fwrite(&msize, 4, 1, f);
			fwrite(&top->value_type->type_id,msize, 1, f);
			}
			else
			{
			fwrite(&msize, 4, 1, f);
			}

		}

		int r_size = 0;
		//opt
			//r_size = strlen((char*)top->opt_raw);
		fwrite(&top->opt_type, 4, 1, f); // op_type
		if(top->opt_type==1) //value
		{

		fwrite(&top->opt_type_ptr->type_id, 4, 1, f);
		}
		else
		{
		fwrite(&top->opt_name_type, 4, 1, f); //var name
		}


		if (top->ref_node != NULL)
		{
			int u = top->ref_node->id;
			fwrite(&u, 4, 1, f);
		}
		else
		{
			fwrite(&e, 4, 1, f);
		}

		top = top->stack_next;
	}

	fclose(f);
}
node * node_by_idx(int id,node_stack* nodes)
{
	for(node* n=nodes->root;n!=NULL;n=n->next)
	{
		if(n->id==id) return n;
	}
	return NULL;
}
int ac[][2]={{1,1}};

void read_file_parse(FILE* f, node_stack* nodes)
{
	int i=0;
	int read=0;

	while (!feof(f))
	{
		char v=0;
		read=fread(&v, 1, 1, f);
		if(v==0) break;
		i++;


		int id = 0;
		read=fread(&id, 4, 1, f);
		node* top = get_node_id(nodes,id);

		top->id = id;

		read=fread(&top->btype, 4, 1, f);
		read=fread(&top->fflag, 4, 1, f);
		read=fread(&top->is_flagged, 1, 1, f);
		read=fread(&top->line, 4, 1, f);
		//read=fread(&top->id, 4, 1, f);

		int next_id = 0;
		read=fread(&next_id, 4, 1, f);
		if(next_id!=0)
		{
			node* next = get_node_id(nodes,next_id);
			next->id=next_id;
			top->next = next;
        }
        int parent_id = 0;
        read=fread(&parent_id, 4, 1, f);
        if(parent_id!=0)
        {
			node* parent =  get_node_id(nodes,parent_id);
			top->parent = parent;
			parent->id=parent_id;
        }

		int msize = 0;
		read=fread(&msize, 4, 1, f);
		if (msize != 0)
		{
			byte * m_value = (byte*)malloc(msize + 1);
			memset(m_value, 0, msize + 1);


			read=fread(&m_value, msize, 1, f);
			if (top->btype.value)//& (index|size))
			{
				top->value_raw = (int*)m_value;
			}
			else if(top->type_==keyword)
				top->value_keyword =*(int*) m_value;
			else
				top->value_raw = m_value;
		}

		int mopt;
		read=fread(&top->opt_type, 4, 1, f); //opt
		read=fread(&mopt, 4, 1, f);
		if (top->opt_type == 1)
		{

			top->opt_type_ptr = SIMPLE_TYPE+ mopt;
		}
		else
		{
            top->opt_name_type =(var_name_def) mopt;
		}

		int ref_node_id=0;
		read=fread(&ref_node_id, 4, 1, f);

		if(ref_node_id!=0)
		{
		node* ref_node = get_node_id(nodes,ref_node_id);

		top->ref_node = ref_node;
		ref_node->id=ref_node_id;
		}
		else
		{
		top->ref_node= NULL;
		}
		read=ftell(f);
	}



	//setup_ref(nodes);

	s_compile_(nodes);
}


//static debuge b;
void s_compile_(node_stack* stack)
{

	node* i;
	for (node* x = stack->root; x != NULL; x = i->stack_next)
	{
		i = x;
		while (i->next != NULL)
		{
			i = i->next;
		}


#ifdef DEBUG_P
//  b.print_line_debuge(i, 0);
#endif


		if(x->type_ != var_name || x->value_raw == NULL || !eql((char*)x->value_raw,"import"))COMPILE_1_P(i);

	}
}
