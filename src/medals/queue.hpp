// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__MEDALS__BASE_HPP
#define RACCOON__MEDALS__BASE_HPP

#include <raccoon/medals.hpp>

namespace Raccoon::Medals {
    class SoundPlaybackQueue {
    private:
        std::optional<std::chrono::steady_clock::time_point> m_current_playing_sound_start;
        std::optional<std::int64_t> m_current_playing_sound_duration;
        Engine::TagDefinitions::Sound *m_current_playing_sound = nullptr;
        std::queue<std::string> m_queue;
        Event::TickEvent::ListenerHandle m_tick_event_listener;
        Event::SoundPlaybackEvent::ListenerHandle m_sound_playback_event_listener;

    public:
        SoundPlaybackQueue() noexcept;
        ~SoundPlaybackQueue() noexcept;
        void enqueue_sound(std::string sound_tag_path) noexcept;
    };

    class RenderQueue {
    protected:
        std::size_t m_max_renders;
        std::deque<std::pair<TimePoint, const Medal *>> m_renders;
        std::queue<const Medal *> m_queue;
        Event::UIRenderEvent::ListenerHandle m_render_event_listener;
        Event::MapLoadEvent::ListenerHandle m_map_load_event_listener;
        
        virtual void render() noexcept = 0;

    public:
        RenderQueue(std::size_t max_renders) noexcept;
        ~RenderQueue() noexcept;
        void show_medal(const Medal *medal) noexcept;
    };
}

#endif
