#include "context.h"
#include "macro.h"
#include <ucontext.h>

// 上下文切换
namespace blue
{
    static thread_local Context *t_current = nullptr;

    struct Context::Impl
    {
        ucontext_t ctx;
        std::function<void()> func;
        bool is_main = false;
    };

    void Context::EntryPoint()
    {
        Context *self = Current();
        self->m_impl->func();
    }

    Context::Context()
        : m_impl(std::make_unique<Impl>())
    {
        m_impl->is_main = true;
        BLUE_ASSERT(getcontext(&m_impl->ctx) == 0);
    }

    Context::Context(std::function<void()> cb, void *stack, size_t size)
        : m_impl(std::make_unique<Impl>())
    {
        m_impl->func = std::move(cb);
        BLUE_ASSERT(getcontext(&m_impl->ctx) == 0);
        m_impl->ctx.uc_link = nullptr;
        m_impl->ctx.uc_stack.ss_sp = stack;
        m_impl->ctx.uc_stack.ss_size = size;
        SetCurrent(this);
        makecontext(&m_impl->ctx, EntryPoint, 0);
    }

    Context::~Context()
    {
    }

    void Context::Swap(Context &from, Context &to)
    {
        SetCurrent(&to);
        BLUE_ASSERT(swapcontext(&from.m_impl->ctx, &to.m_impl->ctx) == 0);
    }

    Context *Context::Current()
    {
        return t_current;
    }

    void Context::SetCurrent(Context *ctx)
    {
        t_current = ctx;
    }
}