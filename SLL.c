/*
 * Author: Noga Avraham
 * Description: .cpp Implementation of a generelized void* data storing Single Linked List.
 *             [Real-Time Group C course project]
 * Language:  C
 * Date: July 2021
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>//memset
#include "SLL.h"


#define FAIL -1
#define SUCCESS 0


/*
Functions implementations :
----------------------------*/
//Prototypes:
//Note to tester: In some failure checks I did not use this assert function which results in exit in case of fail, and instead 
//                printed to stderr and returned Null, in order to give the user the option to handle the error himself and to recover.
void assert_condition(bool isValid, const char* errorMsg);


List* create_list() {
	//calloc ssures all pointers are set to NULL and var to 0 in a new list
	List* list = (List*)calloc(1, sizeof(List));
	assert_condition(list, "Error: function[create_list()]: Failed allocating memory for new list");
	return list;
}

Node_t* create_node(void* data) {

	Node_t* newNode = (Node_t*)calloc(1, sizeof(Node_t));
	assert_condition(newNode, "Error: function[create_node()]: Failed allocating memory for new node");
	newNode->_data = data;
	newNode->_next = NULL;
	return newNode;
}

//The given node is added at the head of the list
void push(List* list, Node_t* n) {
	assert_condition(list, "Error: function[push()]: Argument List* is NULL");
	assert_condition(n, "Error: function[push()]: Argument Node_t* is NULL");

	n->_next = list->_pHead;
	list->_pHead = n;
	list->_count++;

	//In case of adding the first node to an empty list:
	if (list->_count == 1) {
		list->_pTail = n;
	}
}

//The first node is removed from the head of the list
Node_t* pop(List* list) {
	assert_condition(list, "Error: function[pop()]: Argument List* is NULL");
	if (list->_count == 0) {
		//printf("function[pop()]: List is empty. No pop executed. Null is returned.\n");   //<- for debug log only: 
		return NULL;
	}
	if (list->_count == 1) {list->_pTail = NULL;}

	Node_t* toPop = list->_pHead;
	list->_pHead = list->_pHead->_next;
	list->_count--;

	toPop->_next = NULL;
	return toPop;
}

void add_to_back(List* list, Node_t* n) {
	assert_condition(n, "Error: function[add()]: Argument Node_t* is NULL");
	assert_condition(list, "Error: function[add()]: Argument List* is NULL");

	if (list->_count == 0) {
		push(list, n);
		return;
	}
	list->_pTail->_next = n;
	list->_pTail = n;
	n->_next = NULL;
	list->_count++;
}

Node_t* remove_from_back(List* list) {
	assert_condition(list, "Error: function[remove()]: Argument List* is NULL");
	if (list->_count <= 1)
		return pop(list);

	//The list contains at least 2 nodes
	Node_t* itr = list->_pHead;
	//Iterating to find the one node before the last node in the list (in order to be pointed by tail*)
	while (itr->_next->_next) {
		itr = itr->_next;
	}
	list->_pTail = itr;
	itr = itr->_next;
	list->_pTail->_next = NULL;

	return itr;
}
Node_t* remove_at(List* list, size_t pos) {
	assert_condition(list, "Error: function[remove_at()]: Argument List* is NULL");
	if (pos > list->_count) {
		fprintf(stderr, "Warning: function[remove_at()]: Argument 'pos' = %zu. Cannot be larger than list nodes count = %zu. Null returned\n.", pos, list->_count);
		return NULL;
	}

	Node_t* current = list->_pHead;
	Node_t* prev = NULL;

	if (pos == 1) { return pop(list); }
	if (pos == list->_count) { return remove_from_back(list); }

	while (current && pos>1) {
		prev = current;
		current = current->_next;
		pos--;
	}
	if (!current) {//This should not happen unless data curruption occured in the list memory space
		fprintf(stderr, "Warning: function[remove_at()]: Encountered Null ptr at position %zu in the list. Null returned\n.", list->_count - pos +1);
		return NULL;
	}

	prev->_next = current->_next;
	current->_next = NULL;
	list->_count--;

	return current;

}

Node_t* remove_by_val(List* list, void *value, int(*cmp)(void* v1, void *v2)){
	assert_condition(list, "Error: function[remove_by_val()]: Argument List* is NULL");
	assert_condition(cmp, "Error: function[remove_by_val()]: Argument func-pointer cmp* is NULL");

	Node_t* current = list->_pHead;
	Node_t* prev = NULL;

	while (current) {
		if (cmp(current->_data, value) == 0) {
			if (current == list->_pHead) { return pop(list); }
			if (current == list->_pTail) { return remove_from_back(list); }

			prev->_next = current->_next;
			current->_next = NULL;
			list->_count--;
			return current;
		}
		prev = current;
		current = current->_next;
	}

	//In case value not found in the list
	return NULL;

}

//inserting before the node at given pos
int insert(List* list, Node_t* n, size_t pos) {
	assert_condition(n, "Error: function[insert()]: Argument Node_t* is NULL");
	assert_condition(list, "Error: function[insert()]: Argument List* is NULL");

	Node_t* prev = NULL;
	Node_t* current = list->_pHead;

	if (pos > list->_count) {
		fprintf(stderr, "Warning: function[insert()]: Argument 'pos' = %zu. Cannot be larger than list nodes count = %zu. Null returned\n.", pos, list->_count);
		return FAIL;
	}
	if (list->_count == 0) {
		push(list, n);
		return SUCCESS;
	}
	pos--; //pos-1 is the number of iterations to perform
	while (current && pos) {
		prev = current;
		current = current->_next;
		--pos;
	}
	prev->_next = n;
	n->_next = current;
	list->_count++;
	return SUCCESS;
}

void clear_list(List* list) {

	//The data pointed to by void* in node_t was not allocated by SLL and therefor is not freed.
	while (list->_count) {
		free(pop(list));
	}
}

void print_list(List* list, void(*print_data)(void *data)) {
	assert_condition(list, "Error: function[print_list()]: Argument List* cannot be NULL");
	for (Node_t* itr = list->_pHead; itr != NULL; itr = itr->_next) {
		print_data(itr->_data);
	}
	puts("");
}

//returns node at the position given, otherwise returns NULL
Node_t* find(List* list, size_t pos) {
	assert_condition(list, "Error: function[find()]: Argument List* cannot be NULL");

	if (pos > list->_count) {
		//TODO: print fail
		return NULL;
	}
	Node_t* itr = list->_pHead;
	--pos;

	while( itr != NULL && pos) {
		--pos;
		itr = itr->_next;
	}
	return itr;
}

void print_list_by_range(List* list, size_t start_pos, size_t end_pos, void(*print_data)(void* data)) {
	assert_condition(list, "Error: function[print_list_by_range()]: Argument List* is NULL");
	Node_t* start = find(list, start_pos);
	Node_t* end = find(list, end_pos);
	for (Node_t* itr = start ; itr != end->_next; itr = itr->_next) {
		print_data(itr->_data);
	}
}

void for_each(List* list, void *result, void(*calculate)(void* data, void* result)) {
	for (Node_t* itr = list->_pHead; itr != NULL; itr = itr->_next) {
		calculate(itr->_data, result);
	}
}

void assert_condition(bool isValid, const char *errorMsg) { 
	if (!errorMsg) {
		fprintf(stderr, "%s", "Error: function[assert_condition()]: pointer provided to argument 'errorMsg' is Null. exitting");
		exit(EXIT_FAILURE);
	}
	if (!isValid) {
		if (errorMsg) {
			fprintf(stderr, "%s", errorMsg);
		}
		exit(EXIT_FAILURE);
	}
}


void print_int(void *val) {
	printf("%d, ", (int)val);
}


//TEST:
// 
//int main_test() {
//
//	Node_t* n = NULL;
//	List *list =create_list();
//
//	add_to_back(list, create_node((void*)10));
//	add_to_back(list, create_node((void*)20));
//	add_to_back(list, create_node((void*)30));
//	add_to_back(list, create_node((void*)40));
//	add_to_back(list, create_node((void*)50));
//	push(list, create_node((void*)1));
//	push(list, create_node((void*)2));
//	push(list, create_node((void*)3));
//	push(list, create_node((void*)4));
//
//	print_list(list, print_int);
//
//	n = pop(list);
//	printf("popped value: %d\n", (int)n->_data);
//	free(n);
//	print_list(list, print_int);
//
//	printf("popped value\n");
//	free(pop(list));
//	free(remove_from_back(list));
//	print_list(list, print_int);
//
//	//declaring a list as local and not via create_list() is optional only here from inside the .cpp
//	List list2 = { 0 };
//	push(&list2, remove_from_back(list));
//	return 0;
//}