#include <iostream>
#include <ostream>
using namespace std;

template<typename T>
class CTest {
public:
    CTest();
    ~CTest();

    void Display() {
        cout<<"sizeof(T):"<<sizeof(m_tdata)<<endl;
    }
private:
    T m_tdata;
};


int main(int argc, char* argv[])
{
   ostream tt(0);
   tt<<"abc"<<endl;
}