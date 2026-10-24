#pragma once
#include "keyboard.hpp"
#include <windows.h>
#include <atomic>
#include <functional>
#include <future>
#include <thread>

namespace kasmr {
struct KeyEvent { unsigned scan; bool extended; bool release; bool injected; };
class WindowsInput {
public:
    explicit WindowsInput(std::function<void(KeyEvent)> callback);
    ~WindowsInput();
    WindowsInput(const WindowsInput &) = delete;
    WindowsInput &operator=(const WindowsInput &) = delete;
private:
    static LRESULT CALLBACK hook(int code, WPARAM message, LPARAM data);
    void run(std::promise<bool> ready);
    std::function<void(KeyEvent)> callback_;
    KeyState keys_;
    std::thread thread_;
    DWORD thread_id_ = 0;
    static thread_local WindowsInput *current_;
};
}
