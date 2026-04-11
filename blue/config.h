#ifndef __BLUE_CONFIG_H__
#define __BLUE_CONFIG_H__
#include <memory>
#include <sstream>
#include <string>
#include <exception>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "log.h"
#include "mthread.h"
namespace blue
{
    // 配置基类
    class ConfigVarBase
    {
    public:
        using ConfigVarBasePtr = std::shared_ptr<ConfigVarBase>;
        ConfigVarBase(const std::string &name, const std::string &description = "") : m_name(name), m_description(description)
        {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), [](unsigned char x)
                           { return std::tolower(x); });
        }
        virtual ~ConfigVarBase() = default;

        const std::string &GetName() const { return m_name; }
        const std::string &GetDescription() const { return m_description; }
        // 纯虚函数,支持序列化和反序列化
        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &val) = 0;
        // 如果有报错,支持得到类型名称
        virtual std::string GetTypeName() const = 0;

    protected:
        std::string m_name;        // 配置名称
        std::string m_description; // 关于配置的描述
    };

    // 支持基础类型到string的转换
    template <typename F, typename T>
    class LexicalCast
    {
    public:
        T operator()(const F &val)
        {
            return boost::lexical_cast<T>(val);
        }
    };

    // 一系列特化的序列化和反序列化类模板
    // 特化string -> vector<T>
    template <typename T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::vector<T> vec; // typename告诉编译器std::vector<T>是一个类型
            std::stringstream ss;
            for (unsigned i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss.clear();
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化vector<T> -> string
    template <typename T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &val)
        {
            YAML::Node node;
            for (auto &x : val)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化list<T> -> string
    template <typename T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &val)
        {
            YAML::Node node;
            for (auto &x : val)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> list<T>
    template <typename T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::list<T> lis; // typename告诉编译器std::list<T>是一个类型
            std::stringstream ss;
            for (unsigned i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss.clear();
                ss << node[i];
                lis.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return lis; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化set<T> -> string
    template <typename T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &val)
        {
            YAML::Node node;
            for (auto &x : val)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> set<T>
    template <typename T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::set<T> set_; // typename告诉编译器std::set<T>是一个类型
            std::stringstream ss;
            for (unsigned i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss.clear();
                ss << node[i];
                set_.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return set_; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化unordered_set<T> -> string
    template <typename T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &val)
        {
            YAML::Node node;
            for (auto &x : val)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> unordered_set<T>
    template <typename T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::unordered_set<T> unorset_; // typename告诉编译器std::set<T>是一个类型
            std::stringstream ss;
            for (unsigned i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss.clear();
                ss << node[i];
                unorset_.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return unorset_; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化map<std::string,T> -> string
    template <typename T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &val)
        {
            YAML::Node node;
            for (auto &[map_key, map_values] : val)
            {
                node[map_key] = YAML::Load(LexicalCast<T, std::string>()(map_values));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> map<std::string,T>
    template <typename T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::map<std::string, T> map_; // typename告诉编译器std::map<std::string,T>是一个类型
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str(""); // 每次清空流
                ss.clear();
                ss << it->second;
                // map_.insert(std::make_pair(it->first.Scalar(),
                // LexicalCast<std::string,T>() (ss.str())));
                map_.emplace(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str()));
            }
            return map_; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化unordered_map<std::string,T> -> string
    template <typename T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &val)
        {
            YAML::Node node;
            for (auto &[map_key, map_values] : val)
            {
                node[map_key] = YAML::Load(LexicalCast<T, std::string>()(map_values));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> unordered_map<std::string,T>
    template <typename T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::unordered_map<std::string, T> map_; // typename告诉编译器std::map<std::string,T>是一个类型
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str(""); // 每次清空流
                ss.clear();
                ss << it->second;
                // map_.insert(std::make_pair(it->first.Scalar(),
                // LexicalCast<std::string,T>() (ss.str())));
                map_.emplace(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str()));
            }
            return map_; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 派生类模板,支持T类型的序列化和反序列化
    // FromStr T opertaor() (const std::string&)
    // ToString std::string opertaor(const T&)
    template <typename T,
              typename FromStr = LexicalCast<std::string, T>,
              typename ToString = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using RWmutexType = blue::MRWmutex;
        using ConfigVarPtr = std::shared_ptr<ConfigVar<T>>;                           // 智能指针
        using on_change_cb = std::function<void(const T &old_val, const T &new_val)>; // 回调函数
        ConfigVar(const std::string &name, const T &default_val,
                  const std::string &description) : ConfigVarBase(name, description), m_val(default_val)
        {
        }
        ~ConfigVar() override = default;
        // 转换为string类型
        virtual std::string toString() override
        {
            try
            {
                // return boost::lexical_cast<std::string>(m_val);
                RWmutexType::ReadlockSco lock(m_mutex);
                return ToString()(m_val);
            }
            catch (std::exception &e)
            {
                BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "ConfigVar::toString excption"
                                                        << e.what() << "convert: " << typeid(m_val).name() << "to string";
            }
            return std::string();
        }
        // 转回m_val
        virtual bool fromString(const std::string &val) override
        {
            try
            {
                // m_val = boost::lexical_cast<T>(val);
                SetValue(FromStr()(val));
                return true;
            }
            catch (std::exception &e)
            {
                BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "ConfigVar::toString excption "
                                                        << e.what() << " convert: string to " << typeid(m_val).name();
            }
            return false;
        }
        const T GetValue() const // 返回m_val
        {
            RWmutexType::ReadlockSco lock(m_mutex);
            return m_val; 
        }

        void SetValue(const T &new_val)            // 设置m_val
        {
            {
                RWmutexType::ReadlockSco lock(m_mutex);
                if (new_val == m_val) {
                    return;  // 值未变，直接返回
                }
            }  // 读锁释放
            T old_val;
            {
                RWmutexType::WritelockSco lock(m_mutex);
                if (new_val == m_val)
                {
                    return;
                }
                old_val = m_val;
                m_val = new_val;
            }  // 写锁释放
            for (auto &[key, func] : m_mapfunc) {
                func(old_val, m_val);
            }
        }
        virtual std::string GetTypeName() const override { return typeid(T).name(); } // 返回T的类型

        // 设置监听器,配置被更改后需要做出一些操作,一般是更新删除修改
        uint64_t addListener(on_change_cb cb) // 添加监听器
        {
            static uint64_t s_fun_id = 0; // 静态局部变量的初始化是线程安全的
            RWmutexType::WritelockSco lock(m_mutex);
            ++s_fun_id;
            m_mapfunc[s_fun_id] = cb;
            return s_fun_id;
        }

        // 删除监听器
        bool delListener(uint64_t key) // 删除监听
        {
            auto it = m_mapfunc.find(key);
            RWmutexType::WritelockSco lock(m_mutex);
            if (it != m_mapfunc.end())
            {
                m_mapfunc.erase(it);
                return true;
            }
            return false;
        }

        // 获取监听器
        on_change_cb getListener(uint64_t key)
        {
            RWmutexType::ReadlockSco lock(m_mutex);
            auto it = m_mapfunc.find(key);
            return it == m_mapfunc.end() ? nullptr : it->second;
        }

        // 清空监听器
        void clearListener()
        {
            RWmutexType::WritelockSco lock(m_mutex);
            m_mapfunc.clear();
        }

    private:
        mutable RWmutexType m_mutex;
        // 为从.yml文件中加载出来的内容的string被转化到T类型，
        // 支持基础类型,set,map,unordered_set,
        // unordered_map,list以及自定义类型(需要自己实现转换)
        T m_val;
        // 变更回调函数组,一般是如果m_val被修改后,我需要调用函数做一些处理
        std::map<uint64_t, on_change_cb> m_mapfunc;
    };

    class Config
    {
    public:
        using ConfigVarMaps = std::map<std::string, ConfigVarBase::ConfigVarBasePtr>;
        using RWmutexType = blue::MRWmutex;

        // 只查找配置名称，不创建
        template <class T>
        static typename ConfigVar<T>::ConfigVarPtr Lookup(const std::string &name)
        {
            static auto &m_datas = getConfigVarMaps();
            {
                RWmutexType::ReadlockSco lock(GetMutex());
                auto it = m_datas.find(name);
                if (it != m_datas.end())
                {
                    // name存在,转化为ConfigVar<T>
                    auto config_var = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                    if (config_var)
                    {
                        return config_var;
                    }
                    // 转化失败说明，类型不匹配，返回空
                }
                return nullptr;
            }
        }

        // 查找配置名称,没有就创建
        template <class T>
        static typename ConfigVar<T>::ConfigVarPtr Lookup(const std::string &name,
                                                          const T &default_value,
                                                          const std::string &description = "")
        {   
            // 验证名称合法性
            if (name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
            {
                BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                    << "Lookup name invalid: [" << name << "]";
                throw std::invalid_argument(name);
            }
            static auto &m_datas = getConfigVarMaps();
            {
                RWmutexType::ReadlockSco lock(GetMutex());
                auto it = m_datas.find(name);
                if (it != m_datas.end())
                {
                    auto config_var = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                    if (config_var)
                    {
                        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "Lookup name is ["
                                                            << name << "] exists";
                        return config_var;
                    }
                    // 如果转化失败，说明类型不匹配，返回nullptr
                    BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                        << "Lookup name is " << name << " exists but type mismatch " << "this type is : " << typeid(T).name() << " "
                        << "the real type is " << it->second->GetTypeName() << " "
                        << "the string is " << it->second->toString();
                    return nullptr;
                }
            }

            // 创建新的配置项
            typename ConfigVar<T>::ConfigVarPtr v =
                std::make_shared<ConfigVar<T>>(name, default_value, description);
            {
                auto it = m_datas.find(name);
                RWmutexType::WritelockSco lock(GetMutex());
                // 再次判断
                // auto it = m_datas.find(name);
                if (it != m_datas.end())
                {
                    auto config_var = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                    if (config_var)
                    {
                        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "Lookup name is ["
                                                            << name << "] exists";
                        return config_var;
                    }
                    // 如果转化失败，说明类型不匹配，返回nullptr
                    BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                        << "Lookup name is " << name << " exists but type mismatch " << "this type is : " << typeid(T).name() << " "
                        << "the real type is " << it->second->GetTypeName() << " "
                        << "the string is " << it->second->toString();
                    return nullptr;
                }
                m_datas[name] = v;
                return v;
            }
        }

        // 加载配置,支持对从.yml中加载出来的root进行
        static void LoadFromYAML(const ::YAML::Node &root);
        
        // 按照名称查找基类指针
        static ConfigVarBase::ConfigVarBasePtr LookUpBase(const std::string &name);

        // 拿出m_datas数据来检查
        static void Visit(std::function<void(ConfigVarBase::ConfigVarBasePtr)> cb);
    private:
        
        // 必须返回引用或指针,否则相当于每次调用修改的是副本导致错误(他妈的，让我改了一个多小时)
        static ConfigVarMaps &getConfigVarMaps()
        {
            static ConfigVarMaps m_datas;
            return m_datas;
        }
        static RWmutexType& GetMutex()
        {
            static RWmutexType s_mutex;
            return s_mutex;
        }
    };
} // namespace blue

#endif // __BLUE_CONFIG_H__