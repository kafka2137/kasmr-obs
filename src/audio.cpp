#include "audio.hpp"
#define DR_MP3_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#include "dr_mp3.h"
#include "dr_wav.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace kasmr {
static std::vector<float> decode(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    const auto size = input.tellg();
    if (!input || size <= 0 || size > 8 * 1024 * 1024) throw std::runtime_error("Missing or oversized sample: " + path.u8string());
    std::vector<unsigned char> bytes(static_cast<size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(bytes.data()), size)) throw std::runtime_error("Cannot read sample");
    unsigned channels = 0, rate = 0;
    uint64_t frames = 0;
    std::vector<float> pcm;
    const bool wav = path.extension() == ".wav";
    if (wav) {
        drwav decoder{};
        if (!drwav_init_memory(&decoder, bytes.data(), bytes.size(), nullptr)) throw std::runtime_error("Cannot decode WAV: " + path.u8string());
        auto cleanup = [](drwav *p) { drwav_uninit(p); };
        std::unique_ptr<drwav, decltype(cleanup)> guard(&decoder, cleanup);
        channels = decoder.channels;
        rate = decoder.sampleRate;
        frames = decoder.totalPCMFrameCount;
        if (!channels || channels > 8 || rate < 8000 || rate > 192000 || !frames || frames > rate * 3ULL)
            throw std::runtime_error("WAV sample must be at most 3 seconds with 1-8 channels");
        pcm.resize(static_cast<size_t>(frames * channels));
        if (drwav_read_pcm_frames_f32(&decoder, frames, pcm.data()) != frames) throw std::runtime_error("Truncated WAV sample");
    } else {
        drmp3 decoder{};
        if (!drmp3_init_memory(&decoder, bytes.data(), bytes.size(), nullptr)) throw std::runtime_error("Cannot decode MP3: " + path.u8string());
        auto cleanup = [](drmp3 *p) { drmp3_uninit(p); };
        std::unique_ptr<drmp3, decltype(cleanup)> guard(&decoder, cleanup);
        channels = decoder.channels;
        rate = decoder.sampleRate;
        frames = drmp3_get_pcm_frame_count(&decoder);
        if (!channels || channels > 8 || rate < 8000 || rate > 192000 || !frames || frames > rate * 3ULL)
            throw std::runtime_error("MP3 sample must be at most 3 seconds with 1-8 channels");
        pcm.resize(static_cast<size_t>(frames * channels));
        if (drmp3_read_pcm_frames_f32(&decoder, frames, pcm.data()) != frames) throw std::runtime_error("Truncated MP3 sample");
    }
    std::vector<float> mono(static_cast<size_t>(frames * 48000 / rate));
    for (size_t i = 0; i < mono.size(); ++i) {
        const double position = static_cast<double>(i) * rate / 48000;
        const auto first = static_cast<uint64_t>(position);
        const auto second = std::min(first + 1, frames - 1);
        const float fraction = static_cast<float>(position - first);
        float sum = 0;
        for (unsigned c = 0; c < channels; ++c)
            sum += pcm[first * channels + c] * (1 - fraction) + pcm[second * channels + c] * fraction;
        mono[i] = std::isfinite(sum) ? std::clamp(sum / channels, -1.0f, 1.0f) : 0;
    }
    return mono;
}

std::shared_ptr<const SoundBank> load_bank(const std::filesystem::path &directory)
{
    auto bank = std::make_shared<SoundBank>();
    for (size_t i = 0; i < sample_names.size(); ++i) {
        auto path = directory / (std::string(sample_names[i]) + ".wav");
        if (!std::filesystem::exists(path)) path = directory / (std::string(sample_names[i]) + ".mp3");
        // Some recordings (e.g. MX Blues) have only generic key sounds.
        if (!std::filesystem::exists(path) && (i < 3 || (i >= 8 && i < 11))) {
            const std::string fallback = i < 3 ? "press/GENERIC_R2" : "release/GENERIC";
            path = directory / (fallback + ".wav");
            if (!std::filesystem::exists(path)) path = directory / (fallback + ".mp3");
        }
        bank->samples[i] = decode(path);
    }
    return bank;
}

void Mixer::play(std::shared_ptr<const SoundBank> bank, unsigned sample, float pan, float pitch, Case material)
{
    if (!bank || sample >= bank->samples.size() || bank->samples[sample].empty()) return;
    auto slot = std::find_if(voices_.begin(), voices_.end(), [](const Voice &v) { return !v.bank; });
    if (slot == voices_.end()) slot = voices_.begin() + (next_++ % voices_.size());
    *slot = Voice{};
    slot->bank = std::move(bank);
    slot->sample = sample;
    slot->speed = std::clamp(pitch, 0.5f, 2.0f);
    slot->left = std::sqrt((1 - std::clamp(pan, -1.0f, 1.0f)) * 0.5f);
    slot->right = std::sqrt((1 + std::clamp(pan, -1.0f, 1.0f)) * 0.5f);
    if (material == Case::Plastic) { slot->smoothing = 0.55f; slot->speed *= 0.96f; }
    if (material == Case::Aluminum) { slot->smoothing = 0.9f; slot->speed *= 1.06f; }
    if (material == Case::Wood) { slot->smoothing = 0.24f; slot->speed *= 0.88f; }
}

void Mixer::render(float *left, float *right, size_t frames, float gain)
{
    std::fill_n(left, frames, 0.0f);
    std::fill_n(right, frames, 0.0f);
    for (auto &voice : voices_) {
        if (!voice.bank) continue;
        const auto &sample = voice.bank->samples[voice.sample];
        for (size_t i = 0; i < frames && voice.position < sample.size(); ++i) {
            const auto first = static_cast<size_t>(voice.position);
            const auto second = std::min(first + 1, sample.size() - 1);
            const auto fraction = static_cast<float>(voice.position - first);
            const float value = sample[first] * (1 - fraction) + sample[second] * fraction;
            voice.filtered += voice.smoothing * (value - voice.filtered);
            left[i] += voice.filtered * voice.left;
            right[i] += voice.filtered * voice.right;
            voice.position += voice.speed;
        }
        if (voice.position >= sample.size()) voice.bank.reset();
    }
    for (size_t i = 0; i < frames; ++i) {
        left[i] = std::clamp(left[i] * gain, -1.0f, 1.0f);
        right[i] = std::clamp(right[i] * gain, -1.0f, 1.0f);
    }
}
void Mixer::clear() { for (auto &voice : voices_) voice.bank.reset(); }
}
