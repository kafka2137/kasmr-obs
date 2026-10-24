#include <chrono>
#include <thread>

// Harmless process named obs64.exe to exercise the installer's running-OBS guard.
int main()
{
    std::this_thread::sleep_for(std::chrono::minutes(2));
}
