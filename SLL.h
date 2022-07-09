#pragma once

/*
 * Author: Noga Avraham
 * Description: Header file of a generelized void* data storing Single Linked List. 
                [Real-Time Group C course project]
 * Language:  C
 * Date: July 2021
*/

typedef struct Node_t Node_t;
typedef struct List List;
//structs
struct Node_t {
	void* _data;
	Node_t* _next;
};

struct List {
	Node_t* _pHead;
	Node_t* _pTail; //Adding tail to simplify add() function(i.e- no iterations from head to last node)
	size_t _count;
};

//creation and initialization:
List* create_list();
Node_t* create_node(void* data);

//List head handlers:
void push(List* list, Node_t* n);
Node_t* pop(List* list);

//List tail handlers:
void add_to_back(List* list, Node_t* n);
Node_t* remove_from_back(List* list);

//removes node at given position. positions range: 1-n
Node_t* remove_at(List* list, size_t pos);

//removing node according to the callback function ptr 'comperator' supplied by user
Node_t* remove_by_val(List* list, void* value, int(*cmp)(void* v1, void* v2));

//insert node before the given position. positions range: 1-n
int insert(List* list, Node_t* n, size_t pos);

//finds and returns the node at the given position. positions range: 1-n
Node_t* find(List* list, size_t pos);

//performs action on every node in the list, according to the supplied 'calculate' callback function pointer.
void for_each(List* list, void* result, void(*calculate)(void* data, void* result));

//printing functions:
//prints the list nodes according to the supplied 'print_data' callback function pointer
void print_list(List* list, void(*print_data)(void* data));

void print_list_by_range(List* list, size_t start_pos, size_t end_pos, void(*print_data)(void* data));

void clear_list(List* list);








