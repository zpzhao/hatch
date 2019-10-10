#include "heapsort.h"
#include "sortpublic.h"

#include <stdio.h>
#include <iostream>
using namespace std;

int arraysize = 0;

void maxHeapify(Heap *hp, unsigned int nodei)
{
	unsigned int l = (nodei+1) << 1 - 1;	//left child = 2i-1, -1 ?:arr[0..n-1]
	unsigned int r = (nodei+1) << 1 ;	// right child = 2i
	unsigned int largest = 0;
	int heapMaxI = hp->heapMaxIndex;
    Printfbtree(hp->arr, arraysize);
	if(l <= heapMaxI && hp->arr[l] > hp->arr[nodei])
		largest = l ;
	else
		largest = nodei ;
	
	if(r <= heapMaxI && hp->arr[r] > hp->arr[largest])
		largest = r ;

	if(largest!=nodei)
	{	
		//exchange 
		int tmp ;
		tmp = hp->arr[largest];
		hp->arr[largest] = hp->arr[nodei];
		hp->arr[nodei] = tmp;
		
		maxHeapify(hp,largest);
	}
    else
    {
		return ;
	}
	
}

Heap *createHeap(int *arrp, int arrLength,Heap *heap)
{
    int i;
    heap->arr = arrp;
    heap->heapMaxIndex = arrLength-1;
    heap->arrLength = arrLength;

    //for an heap a[0..n-1]; a[(n/2)..n-1] all are leaves
    for(i = arrLength>>1-1; i >=0; i--) 
        maxHeapify(heap,i);
    return heap;
}

void heapSort(Heap *hp)
{
	int tmp;
	int last;
	while(hp->heapMaxIndex>0)
	{
		last = hp->heapMaxIndex ;
		//exchange
		tmp = hp->arr[last];
		hp->arr[last] = hp->arr[0];
		hp->arr[0] = tmp;
		hp->heapMaxIndex -= 1;
		maxHeapify(hp,0);	//make heap from root node 0 to heapMaxIndex 
	}	
}

void printArr(int *p, int size)
{
	int i;
	for(i=0; i<size;i++)
	{
		printf("%d ",p[i]);
	}
}


//以下是针对堆进行调整
void HeapAjust(int data[],int i,int length)
{
	int nChild;
	int nTemp;
	for(nTemp=data[i];2*i+1<length;i=nChild)
	{
		nChild=2*i+1;
		if(nChild<length-1&&data[nChild+1]>data[nChild])//比较哪个孩子比自己大，如果是右孩子的话，就要将nChild++；
		{
			nChild++;
		}

		if(nTemp<data[nChild])//如果比自己的最大的孩子小，就交换
		{
			data[i]=data[nChild];
			data[nChild]=nTemp;
		}
		else//如果比最大的孩子还大，就不交换
			break;
	}
}

//堆排序
void HeapSort2(int data[],int length)
{
	for(int i=(length>>1)-1;i>=0;i--)//注意这个地方：i=(length>>1)-1，加上括号，原因：优先级的问题
	{
		HeapAjust(data,i,length);//初始化一个堆
	}
	for(int j=length-1;j>0;--j)
	{
		int temp=data[j];
		data[j]=data[0];
		data[0]=temp;
		HeapAjust(data,0,j);
	}
}

int heapSortTest()
{
	int a[]={15,25,32,23,1,-4,35,2,-85,42,0,12,26,56,45,12,145,17,25,21};
    //int a[]={15,25,32,23,1,-4};
	printArr(a,ARRAY_SIZE(a));
	printf("\n");

	//Heap hpa,*phpa;
   // arraysize = ARRAY_SIZE(a);
	//phpa =  createHeap(a,ARRAY_SIZE(a),&hpa);
	//heapSort(phpa);
    HeapSort2(a,ARRAY_SIZE(a));
	printArr(a,ARRAY_SIZE(a));
	putchar('\n');
	return 0;	
}