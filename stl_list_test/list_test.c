 
#include<iostream>  
#include<string>  
#include<list>  
using namespace std;  

typedef list<int> LISTINT;  
  
int main(int argc, char* argv[])  
{  
    LISTINT test;  
    for(int i=0;i<5;i++)  
    {  
        test.push_back(i+1);  
    }  
  
    LISTINT::iterator it = test.begin();  
    for(;it!=test.end();it++)  
    {  
        cout<<*it<<"\t";  
    }  
    cout<<endl;  
  
    //reverse show  
    LISTINT::reverse_iterator rit = test.rbegin();  
    for(;rit!=test.rend();rit++)  
    {  
        cout<<*rit<<"\t";  
    }  
    cout<<endl;  
    return 0;  
}  