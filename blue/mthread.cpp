#include "mthread.h"
#include "log.h"
#include "util.h"
#include <exception>
namespace blue
{
    static thread_local Mthread* t_thread = nullptr;
    static thread_local std::string t_thread_name = "NOKNOW";
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    // 信号量构造函数
    Semaphore::Semaphore(uint32_t value)
    {
        if (sem_init(&m_semaphore,0,value))
        {
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore()
    {
        sem_destroy(&m_semaphore);
    }

    void Semaphore::wait()
    {
        // 成功返回0(执行信号量减一)
        if (sem_wait(&m_semaphore))
        {
            throw std::logic_error("sem_wait error");
        }
    }

    void Semaphore::notify()
    {
        if (sem_post(&m_semaphore))
        {
            throw std::logic_error("sem_post error");
        }
    }

    Mthread* Mthread::GetThis()
    {
        return t_thread;
    }

    const std::string& Mthread::GetName()
    {
        return t_thread_name;
    }

    void* Mthread::run(void* arg)
    {
        Mthread* thread = static_cast<Mthread*>(arg);
        t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = blue::GetThreadId();
        pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str());

        std::function<void()> cb;
        cb.swap(thread->m_cb);

        // 唤醒
        thread->m_semaphore.notify();

        cb();
        return nullptr;
    }

    void Mthread::SetThreadName(const std::string& name)
    {
        if (t_thread)
        {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }
    
    Mthread::Mthread(std::function<void()> cb, const std::string& name):
    m_cb(cb),
    m_name(name)
    {
        if (name.empty())
        {
            m_name = "NOKNOW";
        }
        int rt = pthread_create(&m_thread,nullptr,&Mthread::run,this);
        if (rt != 0)
        {
            BLUE_LOG_ERROR(g_logger) << "pthread_create file rt : " 
            << rt << " name is " << m_name;
            throw std::logic_error("pthread_create error");
        }

        // 等待run函数把一切都设置好
        m_semaphore.wait();
    }
    Mthread::~Mthread()
    {
        if (m_thread)
        {
            pthread_detach(m_thread);
        }
    }

    void Mthread::join()
    {
        int rt = pthread_join(m_thread,nullptr);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "pthread_join file rt : " 
            << rt << " name is " << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }

}