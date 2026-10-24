#include <obs.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

std::atomic<unsigned> blocks{0};
std::atomic<unsigned> audible{0};
void capture(void *, obs_source_t *, const audio_data *audio, bool)
{
    ++blocks;
    auto *samples = reinterpret_cast<const float *>(audio->data[0]);
    for (uint32_t i = 0; i < audio->frames; ++i) {
        if (std::abs(samples[i]) > 0.00001f) { ++audible; break; }
    }
}
int main(int argc, char **argv)
{
    if (argc < 3 || !obs_startup("en-US", nullptr, nullptr)) return 1;
    obs_audio_info audio{};
    audio.samples_per_sec = argc > 3 ? 44100 : 48000;
    audio.speakers = SPEAKERS_STEREO;
    if (!obs_reset_audio(&audio)) return 2;
    obs_module_t *module = nullptr;
    if (obs_open_module(&module, argv[1], argv[2]) != MODULE_SUCCESS || !obs_init_module(module)) return 3;
    auto *source = obs_source_create("kasmr_keyboard_audio", "Smoke test", nullptr, nullptr);
    if (!source) return 4;
    obs_source_inc_active(source);
    obs_source_add_audio_capture_callback(source, capture, nullptr);
    auto *props = obs_source_properties(source);
    obs_property_button_clicked(obs_properties_get(props, "preview"), source);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    obs_properties_destroy(props);
    const auto initial = audible.load();
    auto *settings = obs_source_get_settings(source);
    obs_data_set_string(settings, "switch", "mxblue");
    obs_data_set_int(settings, "case", 3);
    obs_data_set_int(settings, "layout", 4);
    obs_source_update(source, settings);
    props = obs_source_properties(source);
    obs_property_button_clicked(obs_properties_get(props, "preview"), source);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    obs_properties_destroy(props);
    const auto changed = audible.load();
    obs_data_set_bool(settings, "enabled", false);
    obs_source_update(source, settings);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto before_disabled_preview = audible.load();
    props = obs_source_properties(source);
    obs_property_button_clicked(obs_properties_get(props, "preview"), source);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    const bool disabled_is_silent = audible == before_disabled_preview;
    obs_properties_destroy(props);
    obs_data_release(settings);
    obs_source_remove_audio_capture_callback(source, capture, nullptr);
    obs_source_dec_active(source);
    obs_source_release(source);
    obs_shutdown();
    std::cout << "Audio blocks: " << blocks << "; audible blocks: " << changed << "; disabled silent: " << disabled_is_silent << '\n';
    return blocks > 0 && initial > 0 && changed > initial && disabled_is_silent ? 0 : 5;
}
