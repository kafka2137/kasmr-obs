#include <future>
#include "windows-input.hpp"
#include "audio.hpp"
#include <obs-module.h>
#include <util/platform.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <random>
#include <string>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("kasmr-obs", "en-US")
MODULE_EXPORT const char *obs_module_description(void) { return "Mechanical keyboard typing audio for Windows. https://github.com/kafka2137/kasmr-obs"; }
MODULE_EXPORT const char *obs_module_name(void) { return "Kafka's Keyboard ASMR for OBS"; }
MODULE_EXPORT const char *obs_module_author(void) { return "kafka2137"; }

namespace {
struct Settings {
    std::shared_ptr<const kasmr::SoundBank> bank;
    kasmr::Layout layout = kasmr::Layout::Full;
    kasmr::Case material = kasmr::Case::Original;
    float gain = 0.5f, width = 0.45f, variation = 0.02f;
    bool enabled = true, releases = true, active_only = true, ignore_injected = true;
};
struct QueuedEvent { kasmr::KeyEvent key; bool preview = false; };

class KeyboardSource {
public:
    explicit KeyboardSource(obs_source_t *source) : source_(source) {}
    ~KeyboardSource()
    {
        input_.reset();
        stop_ = true;
        wake_.notify_all();
        if (audio_.joinable()) audio_.join();
    }
    void start()
    {
        input_ = std::make_unique<kasmr::WindowsInput>([this](auto key) { enqueue({key, false}); });
        audio_ = std::thread(&KeyboardSource::render_loop, this);
    }
    void update(obs_data_t *data)
    {
        std::lock_guard<std::mutex> guard(update_mutex_);
        auto previous = std::atomic_load(&settings_);
        auto settings = std::make_shared<Settings>();
        settings->bank = previous ? previous->bank : nullptr;
        std::string preset = obs_data_get_string(data, "switch");
        if (std::none_of(kasmr::switches.begin(), kasmr::switches.end(), [&](const auto &item) { return preset == item.id; })) preset = "cream";
        const std::string custom = obs_data_get_string(data, "custom_pack");
        char *root = obs_module_file(("sounds/" + preset).c_str());
        const auto directory = custom.empty() ? std::filesystem::u8path(root ? root : "") : std::filesystem::u8path(custom);
        bfree(root);
        if (!settings->bank || directory != loaded_directory_) {
            try {
                settings->bank = kasmr::load_bank(directory);
                loaded_directory_ = directory;
                status_ = "Sound pack loaded successfully.";
            } catch (const std::exception &error) {
                status_ = "Could not load sound pack; previous sounds retained. See OBS log.";
                blog(LOG_WARNING, "[kasmr-obs] %s", error.what());
                if (!settings->bank) throw;
            }
        }
        settings->layout = static_cast<kasmr::Layout>(std::clamp<long long>(obs_data_get_int(data, "layout"), 0, 4));
        settings->material = static_cast<kasmr::Case>(std::clamp<long long>(obs_data_get_int(data, "case"), 0, 3));
        settings->gain = static_cast<float>(std::clamp(obs_data_get_double(data, "volume"), 0.0, 100.0) / 100);
        settings->width = static_cast<float>(std::clamp(obs_data_get_double(data, "width"), 0.0, 100.0) / 100);
        settings->variation = static_cast<float>(std::clamp(obs_data_get_double(data, "variation"), 0.0, 10.0) / 100);
        settings->enabled = obs_data_get_bool(data, "enabled");
        settings->releases = obs_data_get_bool(data, "releases");
        settings->active_only = obs_data_get_bool(data, "active_only");
        settings->ignore_injected = obs_data_get_bool(data, "ignore_injected");
        std::atomic_store(&settings_, std::shared_ptr<const Settings>(settings));
    }
    void preview()
    {
        preview_requested_ = true;
    }
    std::string status()
    {
        std::lock_guard<std::mutex> guard(update_mutex_);
        return status_;
    }
private:
    void enqueue(QueuedEvent event)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_, std::try_to_lock);
        if (!lock || count_ == queue_.size()) return;
        queue_[(head_ + count_) % queue_.size()] = event;
        ++count_;
    }
    size_t drain(std::array<QueuedEvent, 256> &events)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        const auto count = count_;
        for (size_t i = 0; i < count; ++i) events[i] = queue_[(head_ + i) % queue_.size()];
        head_ = (head_ + count) % queue_.size();
        count_ = 0;
        return count;
    }
    void render_loop()
    {
        constexpr uint32_t frames = 480;
        constexpr uint64_t duration = 10000000;
        kasmr::Mixer mixer;
        std::mt19937 random(std::random_device{}());
        std::uniform_real_distribution<float> variation(-1, 1);
        std::array<float, frames> left{}, right{};
        std::array<QueuedEvent, 256> events{};
        auto next = os_gettime_ns();
        unsigned preview_block = 120;
        while (!stop_) {
            auto settings = std::atomic_load(&settings_);
            if (preview_requested_.exchange(false)) preview_block = 0;
            const bool accepting_keys = settings->enabled && (!settings->active_only || obs_source_active(source_));
            const bool playing = settings->enabled && (accepting_keys || preview_block < 120);
            auto count = drain(events);
            constexpr unsigned preview_scans[] = {0x1e, 0x30, 0x12, 0x39, 0x1c, 0x0e};
            if (preview_block < 90 && preview_block % 15 == 0 && count < events.size())
                events[count++] = {{preview_scans[preview_block / 15], false, false, false}, true};
            if (preview_block < 90 && preview_block % 15 == 6 && count < events.size())
                events[count++] = {{preview_scans[preview_block / 15], false, true, false}, true};
            if (preview_block < 120) ++preview_block;
            if (!playing) mixer.clear();
            for (size_t i = 0; i < count; ++i) {
                const auto &event = events[i];
                const auto &key = event.key;
                if (!settings->enabled || (!accepting_keys && !event.preview)) continue;
                if (settings->ignore_injected && key.injected) continue;
                if (key.release && !settings->releases) continue;
                if (!kasmr::layout_accepts(settings->layout, key.scan, key.extended)) continue;
                mixer.play(settings->bank, kasmr::key_sound(key.scan, key.release),
                    kasmr::key_pan(key.scan, key.extended) * settings->width,
                    1 + variation(random) * settings->variation, settings->material);
            }
            mixer.render(left.data(), right.data(), frames, settings->gain);
            obs_source_audio audio{};
            audio.data[0] = reinterpret_cast<uint8_t *>(left.data());
            audio.data[1] = reinterpret_cast<uint8_t *>(right.data());
            audio.frames = frames;
            audio.speakers = SPEAKERS_STEREO;
            audio.format = AUDIO_FORMAT_FLOAT_PLANAR;
            audio.samples_per_sec = 48000;
            audio.timestamp = next;
            obs_source_output_audio(source_, &audio);
            next += duration;
            const auto now = os_gettime_ns();
            if (now > next + 5 * duration) { next = now; mixer.clear(); }
            if (next > now) {
                std::unique_lock<std::mutex> lock(wait_mutex_);
                wake_.wait_for(lock, std::chrono::nanoseconds(next - now), [this] { return stop_.load(); });
            }
        }
    }
    obs_source_t *source_;
    std::shared_ptr<const Settings> settings_;
    std::filesystem::path loaded_directory_;
    std::string status_;
    std::unique_ptr<kasmr::WindowsInput> input_;
    std::thread audio_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> preview_requested_{false};
    std::mutex queue_mutex_, wait_mutex_, update_mutex_;
    std::condition_variable wake_;
    std::array<QueuedEvent, 256> queue_{};
    size_t head_ = 0, count_ = 0;
};

void defaults(obs_data_t *data)
{
    obs_data_set_default_string(data, "switch", "cream");
    obs_data_set_default_int(data, "layout", 0);
    obs_data_set_default_int(data, "case", 0);
    obs_data_set_default_double(data, "volume", 50);
    obs_data_set_default_double(data, "width", 45);
    obs_data_set_default_double(data, "variation", 2);
    for (auto key : {"enabled", "releases", "active_only", "ignore_injected"}) obs_data_set_default_bool(data, key, true);
}
bool preview(obs_properties_t *, obs_property_t *, void *context)
{
    if (context) static_cast<KeyboardSource *>(context)->preview();
    return false;
}
obs_properties_t *properties(void *context)
{
    auto *props = obs_properties_create();
    obs_properties_add_bool(props, "enabled", obs_module_text("Enabled"));
    auto *switches = obs_properties_add_list(props, "switch", obs_module_text("Switch"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    for (const auto &preset : kasmr::switches) obs_property_list_add_string(switches, preset.name, preset.id);
    auto *layout = obs_properties_add_list(props, "layout", obs_module_text("Keyboard"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    const char *layouts[] = {"Full size (104/105 keys)", "Tenkeyless (TKL)", "75%", "65%", "HHKB / 60%"};
    for (int i = 0; i < 5; ++i) obs_property_list_add_int(layout, layouts[i], i);
    auto *material = obs_properties_add_list(props, "case", obs_module_text("Case"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    const char *materials[] = {"Original recording", "Plastic (simulated)", "Aluminum (simulated)", "Wood / dampened (simulated)"};
    for (int i = 0; i < 4; ++i) obs_property_list_add_int(material, materials[i], i);
    obs_properties_add_float_slider(props, "volume", obs_module_text("Volume"), 0, 100, 1);
    obs_properties_add_float_slider(props, "width", obs_module_text("Width"), 0, 100, 1);
    obs_properties_add_float_slider(props, "variation", obs_module_text("Variation"), 0, 10, 0.5);
    obs_properties_add_bool(props, "releases", obs_module_text("Releases"));
    obs_properties_add_bool(props, "active_only", obs_module_text("ActiveOnly"));
    obs_properties_add_bool(props, "ignore_injected", obs_module_text("IgnoreInjected"));
    obs_properties_add_path(props, "custom_pack", obs_module_text("CustomPack"), OBS_PATH_DIRECTORY, nullptr, nullptr);
    obs_properties_add_button(props, "preview", obs_module_text("Preview"), preview);
    obs_properties_add_text(props, "help", obs_module_text("Help"), OBS_TEXT_INFO);
    if (context) obs_properties_add_text(props, "status", static_cast<KeyboardSource *>(context)->status().c_str(), OBS_TEXT_INFO);
    return props;
}
void *create(obs_data_t *data, obs_source_t *source)
{
    try {
        auto result = std::make_unique<KeyboardSource>(source);
        result->update(data);
        result->start();
        return result.release();
    } catch (const std::exception &error) { blog(LOG_ERROR, "[kasmr-obs] Cannot start: %s", error.what()); return nullptr; }
}
void update(void *context, obs_data_t *data)
{
    try { static_cast<KeyboardSource *>(context)->update(data); }
    catch (const std::exception &error) { blog(LOG_ERROR, "[kasmr-obs] Update failed: %s", error.what()); }
}
}
bool obs_module_load(void)
{
    obs_source_info info{};
    info.id = "kasmr_keyboard_audio";
    info.type = OBS_SOURCE_TYPE_INPUT;
    info.output_flags = OBS_SOURCE_AUDIO;
    info.get_name = [](void *) { return obs_module_text("SourceName"); };
    info.create = create;
    info.destroy = [](void *context) { delete static_cast<KeyboardSource *>(context); };
    info.get_defaults = defaults;
    info.get_properties = properties;
    info.update = update;
    info.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT;
    obs_register_source(&info);
    return true;
}
