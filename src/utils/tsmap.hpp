#pragma once
#include <map>
#include <atomic>
#include <mutex>
template<typename Key, typename Value>
class tsmap
{
private:
    std::map<Key,Value> main;
    std::atomic<bool> lock{false};
    mutable std::mutex _mtx;
    void lock_map()
    {   
        std::lock_guard<std::mutex> lockit(_mtx);
        lock.store(true,std::memory_order_seq_cst);
    }
    void unlock_map()
    {
        std::lock_guard<std::mutex> lockit(_mtx);
        lock.store(false,std::memory_order_seq_cst);
    }
public: 
    bool isLocked() const
    {
        return lock.load(std::memory_order_seq_cst);
    }
    bool isUnlocked() const
    {
        return !lock.load(std::memory_order_seq_cst);
    }
    bool contains(const Key& key)
    {
        lock_map();
        bool rez = main.contains(key);
        unlock_map();
        return rez;
    }
    void insert(const Key& key, const Value& value)
    {
        lock_map();
        main.insert_or_assign(key,value);
        unlock_map();
    }

    Value at(const Key& key)
    {
        lock_map();
        Value val;
        val = main.at(key);
        unlock_map();
        return val;
    }
    
    void erase(const Key& key)
    {
        lock_map();
        main.erase(key);
        unlock_map();
    }

    void clear()
    {
        lock_map();
        main.clear();
        unlock_map();
    }
};