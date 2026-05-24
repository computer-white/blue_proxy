#include <vector>
#include "timer.h"
#include "util.h"
#include "log.h"
#include "macro.h"

// 定时器
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    // 用于set内部排序的 重载函数
    bool Timer::Comparator::operator()(const Timer::TimerPtr &lhs,
                                       const Timer::TimerPtr &rhs) const
    {
        if (lhs == nullptr || rhs == nullptr)
        {
            if (lhs == nullptr && rhs == nullptr)
            {
                return false;
            }
            return lhs == nullptr ? true : false;
        }
        if (lhs->m_next == rhs->m_next)
        {
            return lhs.get() < rhs.get();
        }
        return lhs->m_next < rhs->m_next;
    }

    /*-------------------- private --------------------*/

    Timer::Timer(uint64_t ms, std::function<void()> cb,
                 bool recurring, TimerManager *manager)
        : m_recurring(recurring), m_ms(ms), m_cb(cb), m_managger(manager)
    {
        // m_next = blue::GetCurrentMs() + m_ms;
        m_next = blue::GetCurrentMsbyc() + m_ms;
    }

    Timer::Timer(uint64_t next) : m_next(next) {}

    /*-------------------- private --------------------*/

    /*-------------------- static --------------------*/

    std::shared_ptr<Timer> Timer::Create_by_next(uint64_t next)
    {
        return std::shared_ptr<Timer>(new Timer(next));
    }

    std::shared_ptr<Timer> Timer::Create_by_ms(uint64_t ms, std::function<void()> cb,
                                               bool recurring, TimerManager *manager)
    {
        return std::shared_ptr<Timer>(new Timer(ms, cb, recurring, manager));
    }
    /*-------------------- static --------------------*/

    bool Timer::cancel()
    {
        {
            TimerManager::MRWmutexType::ReadlockSco lock(m_managger->m_mutex);
            if (!m_cb)
            {
                return false;
            }
        }
        TimerManager::MRWmutexType::WritelockSco lock(m_managger->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        m_cb = nullptr;
        auto it = m_managger->m_timers.find(shared_from_this());
        m_managger->m_timers.erase(it);
        return true;
    }

    bool Timer::refresh()
    {
        {
            TimerManager::MRWmutexType::ReadlockSco lock(m_managger->m_mutex);
            if (!m_cb)
            {
                return false;
            }
        }
        TimerManager::MRWmutexType::WritelockSco lock(m_managger->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_managger->m_timers.find(shared_from_this());
        if (it == m_managger->m_timers.end())
        {
            return false;
        }
        m_managger->m_timers.erase(it);
        // 重置下次执行时间点
        // m_next = m_ms + blue::GetCurrentMs();
        m_next = m_ms + blue::GetCurrentMsbyc();
        // 重置后重新加入集合
        it = m_managger->m_timers.insert(shared_from_this()).first;
        // 判断是否需要唤醒epoll_wait
        bool at_front = (it == m_managger->m_timers.begin()) && !m_managger->m_tickle;
        if (at_front)
        {
            m_managger->m_tickle = true;
        }
        lock.unlock();
        if (at_front)
        {
            m_managger->onTimerInsertedAtFront();
        }
        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now)
    {
        {
            TimerManager::MRWmutexType::ReadlockSco lock(m_managger->m_mutex);
            if (ms == m_ms && !from_now)
            {
                return true;
            }
            if (!m_cb)
            {
                return false;
            }
        }
        TimerManager::MRWmutexType::WritelockSco lock(m_managger->m_mutex);
        if (ms == m_ms && !from_now)
        {
            return true;
        }
        if (!m_cb)
        {
            return false;
        }

        auto it = m_managger->m_timers.find(shared_from_this());
        if (it == m_managger->m_timers.end())
        {
            return false;
        }
        m_managger->m_timers.erase(it);

        uint64_t start = 0;
        if (from_now)
        {
            // 从当前时间
            // start = blue::GetCurrentMs();
            start = blue::GetCurrentMsbyc();
        }
        else
        {
            // 得到上次设置这个任务的时间点
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = m_ms + start;
        // 重置后重新加入集合
        it = m_managger->m_timers.insert(shared_from_this()).first;
        // 判断是否需要唤醒epoll_wait
        bool at_front = (it == m_managger->m_timers.begin()) && !m_managger->m_tickle;
        if (at_front)
        {
            m_managger->m_tickle = true;
        }
        lock.unlock();
        if (at_front)
        {
            m_managger->onTimerInsertedAtFront();
        }
        return true;
    }

    TimerManager::TimerManager()
    {
        // 初始化上次的系统时间
        // m_previousTime = blue::GetCurrentMs();
        m_previousTime = blue::GetCurrentMsbyc();
    }

    Timer::TimerPtr TimerManager::addTimer(uint64_t ms,
                                           std::function<void()> cb, bool recurring)
    {
        Timer::TimerPtr timer = blue::Timer::Create_by_ms(ms, cb, recurring, this);
        MRWmutexType::WritelockSco lock(m_mutex);
        auto it = m_timers.insert(timer).first;
        bool at_front = (it == m_timers.begin()) && !m_tickle;
        if (at_front)
        {
            m_tickle = true; // 改为true,防止频繁调用addTimer并且每次都调用onTimerInsertedAtFront()去唤醒epoll_wait(),导致性能问题
        }
        lock.unlock();
        if (at_front)
        {
            onTimerInsertedAtFront();
        }
        return timer;
    }

    // 用于addConditionTimer的辅助函数
    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        std::shared_ptr<void> tem = weak_cond.lock();
        if (tem)
        {
            cb();
        }
    }

    Timer::TimerPtr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &vcb)
    {
        // uint64_t now_ms = blue::GetCurrentMs();
        uint64_t now_ms = blue::GetCurrentMsbyc();
        std::vector<Timer::TimerPtr> expired;
        {
            MRWmutexType::ReadlockSco lock(m_mutex);
            if (m_timers.empty())
            {
                return;
            }
        }
        MRWmutexType::WritelockSco lock(m_mutex);
        if (m_timers.empty())
        {
            return;
        }

        bool rollover = detectClockRollover(now_ms);
        if (!rollover && (*m_timers.begin())->m_next > now_ms)
        {
            return;
        }
        Timer::TimerPtr now_timer = blue::Timer::Create_by_next(now_ms);
        auto it = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        // BLUE_LOG_INFO(g_logger) << "expired.size : " << expired.size();
        vcb.reserve(expired.size()); // 使用reverse,不使用resize,否则后面push_back就会使得vcb.size() == 2*expired.size()
        // BLUE_LOG_INFO(g_logger) << "before vcb.size : " << vcb.size();
        for (auto &t : expired)
        {
            vcb.push_back(t->m_cb);
            // 循环加入m_timers
            if (t->m_recurring)
            {
                t->m_next = t->m_ms + now_ms;
                m_timers.insert(t);
            }
            else
            {
                // 取消
                t->m_cb = nullptr;
            }
        }
        // BLUE_LOG_INFO(g_logger) << "affter vcb.size : " << vcb.size();
    }

    bool TimerManager::hasTimer()
    {
        MRWmutexType::ReadlockSco lock(m_mutex);
        return !m_timers.empty();
    }

    uint64_t TimerManager::getNextTime()
    {
        {
            MRWmutexType::WritelockSco lock(m_mutex);
            m_tickle = false;
        }
        MRWmutexType::ReadlockSco lock(m_mutex);
        if (m_timers.empty())
        {
            return ~0ull; // UINT64_MAX
        }

        const Timer::TimerPtr &next = *m_timers.begin();
        // uint64_t now_ms = blue::GetCurrentMs();
        uint64_t now_ms = blue::GetCurrentMsbyc();
        if (now_ms >= next->m_next)
        {
            // 立即执行
            return 0;
        }
        else
        {
            return next->m_next - now_ms;
        }
    }

    bool TimerManager::detectClockRollover(uint64_t now_time)
    {
        bool rollover = false;
        if (now_time < m_previousTime &&
            now_time < (m_previousTime - 60 * 60 * 1000))
        {
            rollover = true;
        }
        m_previousTime = now_time;
        return rollover;
    }

}