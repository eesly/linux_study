/* CThread.h
 * author : sly
 * time : 2016 7 31
 */

#ifndef _CTHREAD_H_
#define _CTHREAD_H_

#include <pthread.h>
#include "CSingleton.h"

/*
 * base thread class, subclasee need impl thread_function
 */
class CThread
{
public:
    CThread();
    virtual ~CThread();

public:
    int start();
    
    void join();
    void detach();
    
    int settls(void * value);
    void * gettls();

protected:
    static void * run(void * handler);
    virtual void * thread_function(void * handler);

private:
    pthread_t m_tid;
    pthread_attr_t m_attr;
};

/*
 * thread key singleton for thread local storage
 */
class CThreadKey: public Singleton<CThreadKey>
{
public:
    CThreadKey()
    {
        pthread_key_create(&key, NULL);
    }

    ~CThreadKey()
    {
        pthread_key_delete(key);
    }

public:
    pthread_key_t& get_key()
    {
        return key;
    }

private:
    pthread_key_t key;
};

#endif
