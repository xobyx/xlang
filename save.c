#include "xlang_main.h"


void setup_ref(node_stack* stack);
node* find_with_id_b(node * s,int m, node_stack* stack);
node* find_with_id_f(node * s,int m, node_stack* stack);
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

		fwrite(&r, 1, 1, f);

		//int y = int(top);
	//	fwrite(&y, 4, 1, f);////id
		fwrite(&top->btype, sizeof(node_type_raw), 1, f);
		fwrite(&top->fflag, sizeof(node_type_raw), 1, f);
		fwrite(&top->is_flagged, 1, 1, f);
		fwrite(&top->line, 4, 1, f);
		if (top->next != NULL)
		{
			int u = (int)top->next;
			fwrite(&u, 4, 1, f);
		}
		else
		{
			fwrite(&e, 4, 1, f);
		}
		if (top->parent != NULL)
		{
			int u = (int)top->parent;
			fwrite(&u, 4, 1, f);
		}
		else
		{
			fwrite(&e, 4, 1, f);
		}

		int msize = 0;
		if (top->value_raw != NULL)
		{
			
			if ((top->btype.value & non_one_char) == 0)
				msize = 1;
			else if(top->type_==keyword)
			    msize=1;
			else
				//if(top->btype.value ==index || top->btype.value==size)
				//	msize=4;

				msize = strlen((char*)top->value_raw);

			fwrite(&msize, 4, 1, f);
			int yb=(int)top->value_raw;
			fwrite(top->type_==keyword?&yb:top->value_raw, msize, 1, f);
		}
		else
		{
			fwrite(&msize, 4, 1, f);
		}
		int r_size = 0;
		if (top->opt_raw != NULL)
		{
			r_size = strlen((char*)top->opt_raw);
			fwrite(&r_size, 4, 1, f);
			fwrite(top->opt_raw, r_size, 1, f);
		}
		else
		{
			fwrite(&r_size, 4, 1, f);
		}

		if (top->ref_node != NULL)
		{
			int u = (int)top->ref_node;
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

void read_file_parse(FILE* f, node_stack* nodes)
{
	int i=0;
	
	while (!feof(f))
	{
		char v=0;
		fread(&v, 1, 1, f);
		if(v==0) break;
		i++;
		node* top = new_node(nodes);	
		int id = 0;
		fread(&id, 4, 1, f);
		top->id = id;
		fread(&top->btype, sizeof(node_type_raw), 1, f);
		fread(&top->fflag, sizeof(node_type_raw), 1, f);
		fread(&top->is_flagged, 1, 1, f);
		fread(&top->line, 4, 1, f);

		int next_id = 0;
		fread(&next_id, 4, 1, f);
		top->next = (node*)next_id;

		int parent_id = 0;
		fread(&parent_id, 4, 1, f);
		top->parent = (node*)parent_id;


		int msize = 0;
		fread(&msize, 4, 1, f);
		if (msize != 0)
		{
			char* m_value = (char*)malloc(msize + 1);
			memset(m_value, 0, msize + 1);


			fread(m_value, msize, 1, f);
			if (top->btype.value)//& (index|size))
			{
				top->value_raw = (int*)m_value;
			}
			else if(top->type_==keyword)
				top->value_raw = m_value;
			else
				top->value_raw = m_value;
		}

		int r_size = 0;
		fread(&r_size, 4, 1, f);
		if (r_size != 0)
		{
			void* r_buff = malloc(r_size + 1);

			memset(r_buff, 0, r_size + 1);
			fread(r_buff, r_size, 1, f);
			top->opt_raw = r_buff;
		}

		int ref_node=0;
		fread(&ref_node, 4, 1, f);
		top->ref_node = (node*)ref_node;
		ftell(f);
	}


	fclose(f);
	setup_ref(nodes);

	s_compile_(nodes);
}

void setup_ref(node_stack* stack)
{
	for (node* n = stack->top; n != NULL; n = n->stack_parent)
	{
		if (n->parent != NULL)
		{
			n->parent = find_with_id_b(n,(int)n->parent, stack);
			//	n->parent->next =n;
		}
		if (n->next !=  NULL)
		{
			n->next = find_with_id_f(n,(int)n->next, stack);
			//	n->next->parent =n;
		}
		if (n->ref_node!= NULL)
		{
			n->ref_node = find_with_id_b(NULL,(int)n->ref_node, stack);
			//	n->next->parent =n;
		}
		//else if(n->parent!=NULL)
		//	s_compile__n(n);
	}

	//debuge k;
	//k.print_line_debuge(nodes->top->stack_parent,0);
	//s_compile__n(nodes->top->stack_parent);
}

node* find_with_id_b(node * s,int m, node_stack* stack)
{
	for (node* n =s==NULL? stack->top:s; n != NULL; n = n->stack_parent)
	{
		if (n->id == m)
			return n;
	}

	return NULL;
}
node* find_with_id_f(node * s,int m, node_stack* stack)
{
	for (node* n =s==NULL? stack->root:s; n != NULL; n = n->stack_next)
	{
		if (n->id == m)
			return n;
	}

	return NULL;
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
