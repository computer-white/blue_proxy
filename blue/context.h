#ifndef BLUE_CONTEXT_H
#define BLUE_CONTEXT_H
#include <memory>
#include <functional>

// 上下文切换
namespace blue
{
    class Context
    {
    public:
        using ContextPtr = std::shared_ptr<Context>;

    public:
        /**
         * @brief 创建协程上下文
         * @param fn 入口函数
         * @param stack 栈空间指针
         * @param size 栈大小
         */
        Context(std::function<void()> fn, void *stack, size_t size);

        /**
         * @brief 创建主协程上下文（当前执行位置的快照）
         */
        Context();

        ~Context();

        /**
         * @brief 从当前上下文切换到目标上下文
         * @param from 当前上下文（保存执行位置到此）
         * @param to 目标上下文（恢复执行位置）
         */
        static void Swap(Context &from, Context &to);

        /**
         * @brief 获取当前正在执行的上下文
         */
        static Context *Current();

        /**
         * @brief 设置当前正在执行的上下文
         */
        static void SetCurrent(Context *ctx);

    private:
        static void EntryPoint();

    private:
        // 内部实现细节，不暴露给外部
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

}

#endif