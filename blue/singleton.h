#ifndef __BLUE_SINGLETON_H__
#define __BLUE_SINGLETON_H__
#include <memory>
// 单例模式
namespace blue 
{
    template <class T,class X = void, int N = 0>
    class SingleTon {
    public:
        static T* GetInstance() 
        {
            static T t;
            return &t;
        }
    };

    template <class T,class X = void, int N = 0>
    class SingleTonPtr 
    {
    public:
        static std::shared_ptr<T> GetInstance() 
        {
            static std::shared_ptr<T> v = std::make_shared<T>();
            return v;
        }
    };

}
#endif // __BLUE_SINGLETON_H__