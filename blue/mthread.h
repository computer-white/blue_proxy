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
            Semaphore(uint32_t value = 0);
            ~Semaphore();

            // 信号量 : 等待
            void wait();
            // 信号量 : 唤醒
            void notify();
        public:
            Semaphore(const Semaphore&) = delete;
            Semaphore(Semaphore&&) = delete;
            Semaphore& operator= (const Semaphore&) = delete;
        private:
            sem_t m_semaphore;
    };
    
    // 互斥锁模板
    template <typename T>
    class ScopedlockedImpl
    {
        public:
            ScopedlockedImpl(T& mutex):
            m_mutex(mutex)
            {
                lock();
            }
            ~ScopedlockedImpl()
            {
                unlock();
            }

            void lock()
            {
                if (!m_locked)
                {
                    m_mutex.lock();
                    m_locked = true;
                }
            }
            void unlock()
            {
                if (m_locked)
                {
                    m_mutex.unlock();
                    m_locked = false;
                }
            }
        public:
            ScopedlockedImpl(const ScopedlockedImpl&) = delete;
            ScopedlockedImpl& operator= (const ScopedlockedImpl&) = delete;
        private:
            T& m_mutex;
            bool m_locked;
    };

    // 读锁模板
    template <typename T>
    class ReadScopedlockedImpl
    {
        public:
            ReadScopedlockedImpl(T& mutex):
            m_mutex(mutex)
            {
                lock();
            }
            ~ReadScopedlockedImpl()
            {
                unlock();
            }

            void lock()
            {
                if (!m_locked)
                {
                    m_mutex.rdlock();
                    m_locked = true;
                }
            }
            void unlock()
            {
                if (m_locked)
                {
                    m_mutex.unlock();
                    m_locked = false;
                }
            }
        public:
            ReadScopedlockedImpl(const ReadScopedlockedImpl&) = delete;
            ReadScopedlockedImpl& operator= (const ReadScopedlockedImpl&) = delete;
        private:
            T& m_mutex;
            bool m_locked = false;
    };

    // 写锁模板
    template <typename T>
    class WriteScopedlockedImpl
    {
        public:
            WriteScopedlockedImpl(T& mutex):
            m_mutex(mutex)
            {
                lock();
            }
            ~WriteScopedlockedImpl()
            {
                unlock();
            }

            void lock()
            {
                if (!m_locked)
                {
                    m_mutex.wrlock();
                    m_locked = true;
                }
            }
            void unlock()
            {
                if (m_locked)
                {
                    m_mutex.unlock();
                    m_locked = false;
                }
            }
        public:
            WriteScopedlockedImpl(const WriteScopedlockedImpl&) = delete;
            WriteScopedlockedImpl& operator= (const WriteScopedlockedImpl&) = delete;
        private:
            T& m_mutex;
            bool m_locked = false;
    };

    // 互斥锁
    class Mmutex
    {
        public:
            using lockSco =  ScopedlockedImpl<Mmutex>;
            Mmutex()
            {
                pthread_mutex_init(&m_mutex,nullptr);
            }

            ~Mmutex()
            {
                pthread_mutex_destroy(&m_mutex);
            }

            void lock()
            {
                pthread_mutex_lock(&m_mutex);
            }

            void unlock()
            {
                pthread_mutex_unlock(&m_mutex);
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
                int ret = pthread_spin_init(&m_mutex, PTHREAD_PROCESS_PRIVATE);
                if (ret != 0) 
                {
                    throw std::system_error(ret, std::system_category(), 
                                        "pthread_spin_init failed");
                }
            }
            ~SpinLockMutex()
            {
                pthread_spin_destroy(&m_mutex);
            }
            void lock()
            {
                int ret = pthread_spin_lock(&m_mutex);
                if (ret != 0) 
                {
                    throw std::system_error(ret, std::system_category(),
                                        "pthread_spin_lock failed");
                }
            }
            void unlock()
            {
                int ret = pthread_spin_unlock(&m_mutex);
                if (ret != 0) 
                {
                    throw std::system_error(ret, std::system_category(),
                                        "pthread_spin_unlock failed");
                }
            }
        public:
            // 禁止拷贝和移动
            SpinLockMutex(const SpinLockMutex&) = delete;
            SpinLockMutex& operator=(const SpinLockMutex&) = delete;
            SpinLockMutex(SpinLockMutex&&) = delete;
            SpinLockMutex& operator=(SpinLockMutex&&) = delete;
        private:
            pthread_spinlock_t m_mutex;
    };

    // 自旋锁(原子实现)
    class CASlock
    {
        public:
            using lockSco = ScopedlockedImpl<CASlock>;
            CASlock():m_flag(ATOMIC_FLAG_INIT)
            {
                lock();
            }

            ~CASlock()
            {
                unlock();
            }

            void lock()
            {
                // 获取内存序,保证后面的带有指定内存序的操作不会被排到他之前，保证先获取到在进行操作
                while (std::atomic_flag_test_and_set_explicit(&m_flag,std::memory_order_acquire));
            }

            void unlock()
            {
                std::atomic_flag_clear_explicit(&m_flag,std::memory_order_release);
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
                pthread_rwlock_init(&m_rwlock,nullptr);
            }

            ~MRWmutex()
            {
                pthread_rwlock_destroy(&m_rwlock);
            }

            void rdlock()
            {
                pthread_rwlock_rdlock(&m_rwlock);
            }

            void wrlock()
            {
                pthread_rwlock_wrlock(&m_rwlock);
            }

            void unlock()
            {
                pthread_rwlock_unlock(&m_rwlock);
            }
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
            
            Mthread(std::function<void()> cb, const std::string& name);
            ~Mthread();
            
            // 获取线程id
            pid_t getID() const { return m_id; }
            // 返回线程名称
            const std::string& getName() const { return m_name; }

            // 线程会和
            void join();
            // 获取当前线程
            static Mthread* GetThis();
            // 获取线程名称
            static const std::string& GetName();
            // 设置线程名称
            static void SetThreadName(const std::string& name);
        private:
            static void* run(void* );
        public:
            Mthread(const Mthread& ) = delete;
            Mthread(Mthread&&) = delete;
            Mthread& operator= (const Mthread& ) = delete;
        private:
            pid_t m_id = -1;                    // 线程id
            pthread_t m_thread = 0;             // 线程identify
            std::function<void()> m_cb;         // 回调函数
            std::string m_name;                 // 线程名称

            Semaphore m_semaphore;              // 信号量
    };
}
#endif // __BLUE_MTHREAD_H__