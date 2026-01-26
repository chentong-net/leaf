//
// Created by Chen Tong on 2026/1/26.
//

#ifndef LEAF_LFSTATE_H
#define LEAF_LFSTATE_H

#include "LFDef.h"

/**
 * 响应式状态容器
 */
template<typename T>
class LFState : public std::enable_shared_from_this<LFState<T>> {
public:
    using Ptr = std::shared_ptr<LFState<T>>;
    using Listener = std::function<void(const T&)>;

    // 工厂方法
    static Ptr create(T initialValue) {
        return std::shared_ptr<LFState<T>>(new LFState(initialValue));
    }

    // 获取当前值
    T get() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_value;
    }

    // 设置新值 (如果值变化，触发 bind 的回调)
    void set(const T& newValue) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_value == newValue) return; // 值没变，直接返回
            m_value = newValue;
        }
        notify(); // 锁外通知
    }

    // 绑定监听 (Lambda)
    // invokeNow: 是否立即执行一次回调
    void bind(Listener listener, bool invokeNow = true) {
        if (invokeNow) {
            T currentValue;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                currentValue = m_value;
            }
            listener(currentValue);
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_listeners.push_back(listener);
    }

    // 语法糖：取值 int a = state();
    T operator()() const {
        return get();
    }

    // 语法糖：赋值 state << 100;
    void operator<<(const T& newValue) {
        set(newValue);
    }

private:
    LFState(T val) : m_value(val) {}

    // 禁止拷贝
    LFState(const LFState&) = delete;
    LFState& operator=(const LFState&) = delete;

    void notify() {
        std::vector<Listener> listenersCopy;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            listenersCopy = m_listeners; // 拷贝副本，防止回调死锁
        }

        for (const auto& listener : listenersCopy) {
            listener(m_value);
        }
    }

    T m_value;
    std::vector<Listener> m_listeners;
    mutable std::mutex m_mutex;
};

#endif // LEAF_LFSTATE_H
