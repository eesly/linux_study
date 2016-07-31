/* CSingleton.h
 * author : sly
 * time : 2016 07 31
 */

#ifndef _CSINGLETON_H_
#define _CSINGLETON_H_

template<typename T>
class Singleton
{
public:
    static T * Instance()
    {
        static T obj;
        return &obj;
    }

private:
    Singleton(const Singleton &other){}
    Singleton &operator=(const Singleton &other){}

protected:
    Singleton(){}
    ~Singleton(){}
};

#endif
