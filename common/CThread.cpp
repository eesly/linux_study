/* CThread.h
 * author : sly
 * time : 2016 7 31
 */

#include <stdio.h>
#include "CThread.h"

CThread::CThread()
{
    printf("base thread construct!\n");
    int stack_size = 8*1024;
    pthread_attr_init(&m_attr);
    pthread_attr_setstacksize(&m_attr, stack_size);
    //pthread_attr_setdetachstate(&m_attr);
    m_tid = -1;
}

CThread::~CThread()
{
    printf("base thread deconstruct!\n"); 
    pthread_attr_destroy(&m_attr);
}

int CThread::start()
{
    int ret = pthread_create(&m_tid, &m_attr, CThread::run, this);
    if (ret != 0)
        perror("create thread error!\n");
    
    return ret;
}

void CThread::join()
{
    pthread_join(m_tid, NULL);
}

void CThread::detach()
{
    pthread_detach(m_tid);
}

int CThread::settls(void * value)
{
    return pthread_setspecific(CThreadKey::Instance()->get_key(), value);
}

void * CThread::gettls()
{
    return pthread_getspecific(CThreadKey::Instance()->get_key());
}

void * CThread::run(void * handler)
{
    if (handler == NULL)
        return NULL;

    CThread* ph = (CThread*)handler;

    return ph->thread_function(NULL);
}

void * CThread::thread_function(void * handler)
{
    printf("base thread class, if printf this msg, some error happen!\n");
    return NULL;
}
