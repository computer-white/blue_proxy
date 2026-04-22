#ifndef __BLUE_FDMANAGER_H__
#define __BLUE_FDMANAGER_h__
#include <memory>
#include <unordered_map>
#include "mthread.h"
#include "singleton.h"
namespace blue
{
    class FdCxt : std::enable_shared_from_this<FdCxt>
    {
        public:
            using FdCxtPtr = std::shared_ptr<FdCxt>;
        
        public:
            FdCxt(int fd);
            ~FdCxt();

            // 初始化
            bool init();
            bool isInit() const { return m_isInit; }
            bool isSocket() const { return m_isSocket; }
            bool isClosed() const { return m_isClosed; }

            // 获得或者设置用户自己的nonblock
            bool getUserNonBlock() const { return m_UserNonBlock; }
            void setUserNonBlock(bool val) { m_UserNonBlock = val; }

            // 获得或者设置系统的nonblock
            bool getSysNonBlock() const { return m_SysNonBlock; }
            void setSysNoBlock(bool val) { m_SysNonBlock = val; }

            /*
                设置超时
                type : 超时事件类型(读/写)
                val  : 超时时间(秒)
            */
            void setTimeout(int type,uint64_t val);
            /*
                获取超时
                type    : 超时事件类型
                return  : (超时时间)秒
            */
            uint64_t getTimeout(int type);

        private:
            bool m_isInit : 1;
            bool m_isSocket : 1;
            bool m_SysNonBlock : 1;
            bool m_UserNonBlock : 1;
            bool m_isClosed : 1;
            uint64_t m_revTimeout;
            uint64_t m_sendTimeout;
            int m_fd;
    };
    class FdManager
    {
        public:
            using MRWmutexType = MRWmutex;
        public:
            // FdManager(int fd);
            FdManager();
            /*
                m_datas[fd]存在直接返回.
                不存在,若auto_create = true,创建新的FdCtx,并返回
            */
            FdCxt::FdCxtPtr get(int fd,bool auto_create = false);
            /*
                根据fd删除FdCxt
            */
            void del(int fd);

        private:
            MRWmutexType m_mutex;
            // std::vector<FdCxt::FdCxtPtr> m_datas;   // 按照fd作为数组下标,使用时保证fd不要跨度很大
            std::unordered_map<int,FdCxt::FdCxtPtr> m_datas;    // 选择使用map存储,fd描述符可能出现不连续
    };

    // 单例模式
    using FdManagerPtr = SingleTonPtr<FdManager>;
}
#endif