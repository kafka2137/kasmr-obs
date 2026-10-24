#include <future>
#include "windows-input.hpp"
#include <stdexcept>

namespace kasmr {
thread_local WindowsInput *WindowsInput::current_ = nullptr;
WindowsInput::WindowsInput(std::function<void(KeyEvent)> callback) : callback_(std::move(callback))
{
    std::promise<bool> ready;
    auto result = ready.get_future();
    thread_ = std::thread(&WindowsInput::run, this, std::move(ready));
    if (!result.get()) {
        thread_.join();
        throw std::runtime_error("Windows keyboard hook could not be installed");
    }
}
WindowsInput::~WindowsInput()
{
    if (thread_.joinable()) {
        PostThreadMessageW(thread_id_, WM_QUIT, 0, 0);
        thread_.join();
    }
}
void WindowsInput::run(std::promise<bool> ready)
{
    current_ = this;
    thread_id_ = GetCurrentThreadId();
    MSG message{};
    PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    auto installed = SetWindowsHookExW(WH_KEYBOARD_LL, hook, GetModuleHandleW(nullptr), 0);
    ready.set_value(installed != nullptr);
    if (!installed) return;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    UnhookWindowsHookEx(installed);
    current_ = nullptr;
}
LRESULT CALLBACK WindowsInput::hook(int code, WPARAM message, LPARAM data)
{
    if (code == HC_ACTION && current_ && (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP)) {
        const auto &key = *reinterpret_cast<const KBDLLHOOKSTRUCT *>(data);
        const bool release = (key.flags & LLKHF_UP) != 0;
        const bool extended = (key.flags & LLKHF_EXTENDED) != 0;
        const unsigned identity = (key.scanCode & 0xff) + (extended ? 256 : 0);
        if (current_->keys_.accept(identity, release))
            current_->callback_({key.scanCode, extended, release, (key.flags & LLKHF_INJECTED) != 0});
    }
    return CallNextHookEx(nullptr, code, message, data);
}
}
