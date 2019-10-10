#include "sortpublic.h"

#include <iostream>
using namespace std;

void Printfbtree(int arr[], int length)
{
    int level = 1;

    for(int i = 0; i < length; )
    {
        for(int j=level; j > 0; j--,i++)
        {
            cout<<arr[i] << "  ";
        }
        cout<<endl;
        level++;
    }
    cout<<"======="<<endl;
}