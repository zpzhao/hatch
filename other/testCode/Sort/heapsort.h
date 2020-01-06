#ifndef HAT_HEAP_SORT_H_H
#define HAT_HEAP_SORT_H_H

typedef struct heap_t
{
	int *arr;	       //point for an array to store heap value.
	int heapMaxIndex;	//heap element max index number
	int arrLength;	//array length of arr
	
}Heap;

int heapSortTest();

#endif