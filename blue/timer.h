#ifndef BLUE_TIMRE_H
#define BLUE_TIMER_H
#include <memory>
#include <set>
#include "mthread.h"

// 定时器
namespace blue
{
    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer>
    {
        // 友元
        friend class TimerManager;

    public:
        using TimerPtr = std::shared_ptr<Timer>;

        /**
         * @brief 取消定时任务
         * @return 成功返回 true 失败 false
         */
        bool cancel();

        /**
         * @brief 从当前时间重置定时任务
         * @return 成功返回 true 失败 false
         */
        bool refresh();

        /**
         * @brief 重置定时任务下一次执行时间点
         * @param ms 定时器循环周期
         * @param form_now 是否从当前开始重置
         * @return 成功返回 true 失败 false
         */
        bool reset(uint64_t ms, bool from_now);

    private:
        // 私有构造函数,不能使用make_shared构造智能指针
        /**
         * @brief 构造函数
         * @param ms 定时器循环周期
         * @param cb 定时回调函数
         * @param recurring 是否设为循环定时器
         * @param manager TimerManager 指针
         * @return
         * @note 不允许隐式转换
         */
        explicit Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);

        /**
         * @brief 构造函数
         * @param next 下次执行时间
         * @return
         * @note 不允许隐式转换
         */
        explicit Timer(uint64_t next);

    public:
        Timer() = default;

        /**
         * @brief 按照下次执行时间创建定时器
         * @param next 下次执行时间
         * @return 定时器智能指针
         */
        static std::shared_ptr<Timer> Create_by_next(uint64_t next);

        /**
         * @brief 创建定时器
         * @param ms 定时器循环周期
         * @param cb 定时回调函数
         * @param recurring 是否设为循环定时器
         * @param manager TimerManager 指针
         * @return 定时器智能指针
         */
        static std::shared_ptr<Timer> Create_by_ms(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);

    private:
        bool m_recurring = false;           // 是否循环定时
        uint64_t m_ms = 0;                  // 定时器循环周期
        uint64_t m_next = 0;                // 下一次执行的时间点
        std::function<void()> m_cb;         // 定时器任务
        TimerManager *m_managger = nullptr; // 定时器管理指针

    private:
        /**
         * @brief 定时器比较器
         * @return
         */
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

        /**
         * @brief TimerManager构造函数
         * @return
         */
        TimerManager();
        virtual ~TimerManager() = default;

        /**
         * @brief 添加定时器
         * @param ms 定时器循环周期
         * @param cb 定时回调函数
         * @param recurring 是否设为循环定时器
         * @return
         */
        Timer::TimerPtr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

        /**
         * @brief 添加条件定时器
         * @param ms 定时器循环周期
         * @param cb 定时回调函数
         * @param weak_cond 条件
         * @param recurring 是否设为循环定时器
         * @return
         */
        Timer::TimerPtr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

        /**
         * @brief 获取下一次任务的执行时间
         * @return 下一次任务的执行时间
         */
        uint64_t getNextTime();

        /**
         * @brief 列出过期的任务
         * @param cbs 存放过期任务的容器
         * @return
         */
        void listExpiredCb(std::vector<std::function<void()>> &cbs);

        /**
         * @brief 是否有定时器
         * @return 有返回true 否则 false
         */
        bool hasTimer();

    protected:
        /**
         * @brief 当某一任务被 新建，重置，刷新后,时间最小,那么需要调用此函数用于唤醒epoll_wait
         * @return
         */
        virtual void onTimerInsertedAtFront() = 0;

    private:
        /**
         * @brief 当系统时间被修改后需要做出调整
         * @param noe_time 当前时间
         * @return 修改了返回true 否则 false
         */
        bool detectClockRollover(uint64_t now_time);

    private:
        MRWmutexType m_mutex;                                  // 互斥锁
        std::set<Timer::TimerPtr, Timer::Comparator> m_timers; // 定时器集合
        bool m_tickle = false;                                 // fasle:防止频繁调用onTimerInsertedAtFront()
        uint64_t m_previousTime = 0;                           // 系统上一次的时间
    };
}
#endif