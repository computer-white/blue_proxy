#ifndef __BLUE_MTHREAD_H__
#define __BLUE_MTHREAD_H__
#include <thread>
#include <functional>
#include <future>
#include <memory>
#include <semaphore.h>
#include <pthread.h>
#include <atomic>
namespace blue
{
    // 信号量
    class Semaphore
    {
    public:
        /**
         * @brief 信号量构造函数
         * @param value 信号值
         * @return
         */
        Semaphore(uint32_t value = 0);
        ~Semaphore();

        /**
         * @brief 信号量等待被唤醒
         * @return
         */
        void wait();

        /**
         * @brief 唤醒信号量
         * @return
         */
        void notify();

    public:
        Semaphore(const Semaphore &) = delete;
        Semaphore(Semaphore &&) = delete;
        Semaphore &operator=(const Semaphore &) = delete;
        Semaphore &operator=(Semaphore &&) = delete;

    private:
        sem_t m_semaphore;
    };

    // 互斥锁模板
    template <typename T>
    class ScopedlockedImpl
    {
    public:
        ScopedlockedImpl(T &mutex) : m_mutex(mutex)
        {
            lock();
        }
        ~ScopedlockedImpl()
        {
            unlock();
        }

        /**
         * @brief 尝试获取锁
         * @return if successful, return 0
         */
        int try_lock()
        {
            int ret = m_mutex.try_lock();
            if (ret == 0)
            {
                m_locked = true;
            }
            return ret;
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    public:
        ScopedlockedImpl(const ScopedlockedImpl &) = delete;
        ScopedlockedImpl &operator=(const ScopedlockedImpl &) = delete;

    private:
        T &m_mutex;
        bool m_locked;
    };

    // 读锁模板
    template <typename T>
    class ReadScopedlockedImpl
    {
    public:
        ReadScopedlockedImpl(T &mutex) : m_mutex(mutex)
        {
            lock();
        }
        ~ReadScopedlockedImpl()
        {
            unlock();
        }

        /**
         * @brief 尝试获取锁
         * @return if successful, return 0
         */
        int try_lock()
        {
            int ret = m_mutex.try_rdlock();
            if (ret == 0)
            {
                m_locked = true;
            }
            return ret;
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    public:
        ReadScopedlockedImpl(const ReadScopedlockedImpl &) = delete;
        ReadScopedlockedImpl &operator=(const ReadScopedlockedImpl &) = delete;

    private:
        T &m_mutex;
        bool m_locked = false;
    };

    // 写锁模板
    template <typename T>
    class WriteScopedlockedImpl
    {
    public:
        WriteScopedlockedImpl(T &mutex) : m_mutex(mutex)
        {
            lock();
        }
        ~WriteScopedlockedImpl()
        {
            unlock();
        }

        /**
         * @brief 尝试获取锁
         * @return if successful, return 0
         */
        int try_lock()
        {
            int ret = m_mutex.try_wrlock();
            if (ret == 0)
            {
                m_locked = true;
            }
            return ret;
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    public:
        WriteScopedlockedImpl(const WriteScopedlockedImpl &) = delete;
        WriteScopedlockedImpl &operator=(const WriteScopedlockedImpl &) = delete;

    private:
        T &m_mutex;
        bool m_locked = false;
    };

    // 互斥锁
    class Mmutex
    {
    public:
        using lockSco = ScopedlockedImpl<Mmutex>;
        Mmutex()
        {
            // 初始化互斥锁,将其初始化为PTHREAD_MUTEX_INITIALIZER
            pthread_mutex_init(&m_mutex, nullptr);
        }

        ~Mmutex()
        {
            pthread_mutex_destroy(&m_mutex);
        }

        /**
         * @brief 尝试获取锁
         * @return if successful, return 0
         */
        int try_lock()
        {
            return pthread_mutex_trylock(&m_mutex);
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            int ret = pthread_mutex_lock(&m_mutex);
            if (ret != 0)
            {
                throw std::system_error(ret, std::system_category(),
                                        "pthread_mutex_lock failed");
            }
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            int ret = pthread_mutex_unlock(&m_mutex);
            if (ret != 0)
            {
                throw std::system_error(ret, std::system_category(),
                                        "pthread_mutex_unlock failed");
            }
        }

    private:
        pthread_mutex_t m_mutex;
    };

    // 自旋锁
    class SpinLockMutex
    {
    public:
        using lockSco = ScopedlockedImpl<SpinLockMutex>;
        SpinLockMutex()
        {
            // PTHREAD_PROCESS_PRIVATE 表示自旋锁仅能被初始化该锁的进程下的线程访问
            // PTHREAD_PROCESS_SHARED 表示自旋锁可以被来自不同进程下的线程访问
            int ret = pthread_spin_init(&m_lock, PTHREAD_PROCESS_PRIVATE);
            if (ret != 0)
            {
                throw std::system_error(ret, std::system_category(),
                                        "pthread_spin_init failed");
            }
        }

        ~SpinLockMutex()
        {
            // 销毁锁
            pthread_spin_destroy(&m_lock);
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            int ret = pthread_spin_lock(&m_lock);
            if (ret != 0)
            {
                throw std::system_error(ret, std::system_category(),
                                        "pthread_spin_lock failed");
            }
        }

        /**
         * @brief 尝试获取锁
         * @return if successful, return 0
         */
        int try_lock()
        {
            return pthread_spin_trylock(&m_lock);
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            int ret = pthread_spin_unlock(&m_lock);
            if (ret != 0)
            {
                throw std::system_error(ret, std::system_category(),
                                        "pthread_spin_unlock failed");
            }
        }

    public:
        // 禁止拷贝和移动
        SpinLockMutex(const SpinLockMutex &) = delete;
        SpinLockMutex &operator=(const SpinLockMutex &) = delete;
        SpinLockMutex(SpinLockMutex &&) = delete;
        SpinLockMutex &operator=(SpinLockMutex &&) = delete;

    private:
        pthread_spinlock_t m_lock;
    };

    // 自旋锁(原子实现)
    class CASlock
    {
    public:
        using lockSco = ScopedlockedImpl<CASlock>;
        CASlock() : m_flag(ATOMIC_FLAG_INIT)
        {
            lock();
        }

        ~CASlock()
        {
            unlock();
        }

        /**
         * @brief 获取锁
         * @return
         */
        void lock()
        {
            // 获取内存序,保证后面的带有指定内存序的操作不会被排到他之前，保证先获取到在进行操作
            while (std::atomic_flag_test_and_set_explicit(&m_flag, std::memory_order_acquire))
                ;
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            std::atomic_flag_clear_explicit(&m_flag, std::memory_order_release);
        }

    private:
        volatile std::atomic_flag m_flag;
    };

    // 读写锁
    class MRWmutex
    {
    public:
        using ReadlockSco = ReadScopedlockedImpl<MRWmutex>;
        using WritelockSco = WriteScopedlockedImpl<MRWmutex>;
        MRWmutex()
        {
            pthread_rwlock_init(&m_rwlock, nullptr);
        }

        ~MRWmutex()
        {
            pthread_rwlock_destroy(&m_rwlock);
        }

        /**
         * @brief 尝试获取读锁
         * @return if successful, return 0
         */
        int try_rdlock()
        {
            return pthread_rwlock_tryrdlock(&m_rwlock);
        }

        /**
         * @brief 尝试获取写锁
         * @return if successful, return 0
         */
        int try_wrlock()
        {
            return pthread_rwlock_trywrlock(&m_rwlock);
        }

        /**
         * @brief 获取读锁
         * @return
         */
        void rdlock()
        {
            int ret = pthread_rwlock_rdlock(&m_rwlock);
            if (ret != 0)
            {
                throw std::system_error(ret, std::system_category(),
                                        "pthread_rwlock_rdlock failed");
            }
        }

        /**
         * @brief 获取写锁
         * @return
         */
        void wrlock()
        {
            int ret = pthread_rwlock_wrlock(&m_rwlock);
            if (ret != 0)
            {
                throw std::system_error(ret, std::system_category(),
                                        "pthread_rwlock_wrlock failed");
            }
        }

        /**
         * @brief 解锁
         * @return
         */
        void unlock()
        {
            pthread_rwlock_unlock(&m_rwlock);
        }

    public:
        MRWmutex(const MRWmutex &othr) = delete;
        MRWmutex(MRWmutex &&othr) = delete;
        MRWmutex &operator=(const MRWmutex &othr) = delete;
        MRWmutex &operator=(const MRWmutex &&othr) = delete;

    private:
        pthread_rwlock_t m_rwlock;
    };

    // 无锁(对比有锁测试)
    class NullMutex
    {
    public:
        using lockSco = ScopedlockedImpl<NullMutex>;
        NullMutex() {}
        ~NullMutex() {}

        void lock() {}
        void unlock() {}
    };
    class NullRWmutex
    {
    public:
        using ReadLockSco = ReadScopedlockedImpl<NullRWmutex>;
        using WriteLockSco = WriteScopedlockedImpl<NullRWmutex>;
        NullRWmutex() {}
        ~NullRWmutex() {}
        void rdlock() {}
        void wrlock() {}
        void unlock() {}
    };

    // 线程
    class Mthread
    {
    public:
        using MthreadPtr = std::shared_ptr<Mthread>;

        Mthread(std::function<void()> cb, const std::string &name);
        ~Mthread();

        /**
         * @brief 获取线程id
         * @return pid
         */
        pid_t getID() const { return m_id; }

        /**
         * @brief 获取线程名称
         * @return 线程名称
         */
        const std::string &getName() const { return m_name; }

        /**
         * @brief 线程会和
         * @return
         */
        void join();

        /**
         * @brief 获取当前线程
         * @return 当前线程的裸指针
         */
        static Mthread *GetThis();

        /**
         * @brief 获取线程名称
         * @return 线程名称
         */
        static const std::string &GetName();

        /**
         * @brief 设置线程名称
         * @param name 新的线程名称
         * @return
         */
        static void SetThreadName(const std::string &name);

    private:
        /**
         * @brief 线程执行函数
         * @param arg 需要执行的线程指针
         * @return nullptr
         */
        static void *run(void *arg);

    public:
        Mthread(const Mthread &) = delete;
        Mthread(Mthread &&) = delete;
        Mthread &operator=(const Mthread &) = delete;

    private:
        pid_t m_id = -1;            // 线程id
        pthread_t m_thread = 0;     // 线程identify
        std::function<void()> m_cb; // 回调函数
        std::string m_name;         // 线程名称

        Semaphore m_semaphore; // 信号量
    };
}
#endif // __BLUE_MTHREAD_H__