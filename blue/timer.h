#ifndef __BLUE_TIMRE_H__
#define __BLUE_TIMER_H__
#include <memory>
#include <set>
#include "mthread.h"
namespace blue
{
    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer>
    {
    // 友元
    friend class TimerManager;
    
    public:
        using TimerPtr = std::shared_ptr<Timer>;
        // 取消定时任务
        bool cancel();
        /*
            从当前时间重置
            m_next : 下一次定时任务执行时间
            m_ms : 定时器循环周期,保持不变
        */
        bool refresh();

        /*
            重置定时任务下一次执行时间点
            ms : 定时器循环周期
            form_now : true(从当前时间开始重置)
        */
        bool reset(uint64_t ms, bool from_now);

    private:
        // 私有构造函数,不能使用make_shared构造智能指针    

        /*
            ms : 定时器循环周期
            cb : 定时器回调函数
            recurring : 是否设为循环定时器
            manager : TimerManager 指针
        */
        explicit Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);
        /*
            next : 下次执行时间
        */
        explicit Timer(uint64_t next);

    public:
        Timer() = default;
        // 按照下次执行时间创建定时器
        static std::shared_ptr<Timer> Create_by_next(uint64_t next);

        /*
            创建定时器
            ms : 循环周期
            cb : 任务函数
            recurring : 是否循环定时
            manager : 定时器管理器指针
        */
        static std::shared_ptr<Timer> Create_by_ms(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);

    private:
        bool m_recurring = false;                       // 是否循环定时
        uint64_t m_ms = 0;                              // 定时器循环周期
        uint64_t m_next = 0;                            // 下一次执行的时间点
        std::function<void()> m_cb;                     // 定时器任务
        TimerManager *m_managger = nullptr;             // 定时器管理指针

    private:
        struct Comparator
        {
            bool operator()(const Timer::TimerPtr &lhs, const Timer::TimerPtr &rhs) const;
        };
    };

    class TimerManager
    {
    // 友元
    friend class Timer;

    public:
        using MRWmutexType = blue::MRWmutex;
        // 构造函数
        TimerManager();
        virtual ~TimerManager() = default;

        /* 
            添加定时器
            ms : 循环周期
            cb : 任务函数
            recurring : 是否循环定时
        */
        Timer::TimerPtr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

        /*
            添加条件定时器
            ms : 循环周期
            cb : 任务函数
            weak_cond : 条件
            recurring : 是否循环定时
        */
        Timer::TimerPtr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

        /*
            获取下一次任务的执行时间
        */
        uint64_t getNextTime();

        /* 
            列出过期的任务
        */
        void listExpiredCb(std::vector<std::function<void()>> &cbs);

        /*
            是否有定时器
        */
        bool hasTimer();

    protected:
        /*
            当某一任务被 新建，重置，刷新后,时间最小,那么需要调用此函数用于唤醒epoll_wait
        */
        virtual void onTimerInsertedAtFront() = 0;

    private:
        /*
            当系统时间被修改后需要做出调整
        */
        bool detectClockRollover(uint64_t now_time);

    private:
        MRWmutexType m_mutex;                                           // 互斥锁
        // 后序可以换成最小堆优化
        std::set<Timer::TimerPtr, Timer::Comparator> m_timers;          // 定时器集合
        bool m_tickle = false;                                          // fasle:防止频繁调用onTimerInsertedAtFront()
        uint64_t m_previousTime = 0;                                    // 系统上一次的时间
    };
}
#endif