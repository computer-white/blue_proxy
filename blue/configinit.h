#ifndef BLUE_CONFIGINIT_H
#define BLUE_CONFIGINIT_H
#include "dbmanager.h"
#include "redismanager.h"
#include <string>

namespace blue
{
    namespace http
    {
        std::string s_db_host = "";
        std::string s_db_user = "";
        std::string s_db_database = "";
        std::string s_db_passward = "";
        uint16_t s_db_port = 3306;
        blue::DbManager::DbManagerPtr s_dbmanager_ptr = nullptr;

        std::string s_redis_host = "";
        uint16_t s_redis_port = 6379;
        std::string s_redis_passward = "";
        blue::RedisManager::RedisManagerPtr s_redismanager_ptr = nullptr;

        uint64_t s_rate_limit = 0;
        uint64_t s_rate_limit_expire = 0;
        uint64_t s_cache_expire = 0;

        uint64_t s_select_timeout = 0;

        uint32_t s_httpconnpool_mxsize = 0;
        size_t s_mysqlpool_mxsize = 0;
        void IniteConfig();
    }
}

#endif