/*
 * demo for thread local storage
 * author : sly
 * date : 2016 07 31
 */

#include <stdio.h>
#include <string>
#include "../common/CThread.h"

/*
 * example data struct for tls
 */
struct TlsData
{
    int m_addr;
    std::string m_name;

    TlsData(const char * name)
    {
        m_name = name;
        m_addr = (int)this;
    }
};

/*
 * thread class 
 */
class SubThread: public CThread
{
public:
    SubThread(const char * name):m_name(name){
        printf("subclass %s construct\n", m_name.c_str());
    }
    ~SubThread(){
        printf("subclass %s deconstruct\n", m_name.c_str());        
    }

public:
    void * thread_function(void *)
    {
        //set tls var
        TlsData tls("tls data");
        printf("class: %s set %s addr 0x%0x\n",
            m_name.c_str(), tls.m_name.c_str(), tls.m_addr);
        settls(&tls);

        //do something

        //get tls var
        TlsData * ptls = (TlsData*)gettls();
        printf("class: %s get %s addr 0x%0x\n", m_name.c_str(), 
            ptls->m_name.c_str(), ptls->m_addr);
        
        return NULL;
    }

private:
    std::string m_name;
};

int main(int argc, char ** argv)
{
    SubThread th1("thread 1");
    SubThread th2("thread 2");

    th1.start();
    th2.start();

    th1.join();
    th2.join();

    return 0;
}
