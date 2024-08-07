// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__MEDALS__H4_MEDALS_HPP
#define RACCOON__MEDALS__H4_MEDALS_HPP

#include "base.hpp"

namespace Raccoon::Medals {
    using MapLoadEvent = Balltze::Event::MapLoadEvent;
    using MapLoadEventListenerHandle = Balltze::Event::EventListenerHandle<MapLoadEvent>;
    using PlayerHandle = Balltze::Engine::PlayerHandle;
    using TimePoint = std::chrono::steady_clock::time_point;
    using HudChatEvent = Balltze::Event::NetworkGameHudMessageEvent;
    using HudChatEventListenerHandle = Balltze::Event::EventListenerHandle<HudChatEvent>;

    class H4RenderQueue : public RenderQueue {
    private:
        std::optional<TimePoint> m_last_pushed_medal;
        std::chrono::microseconds m_medal_slide_duration = std::chrono::milliseconds(60);
        std::unique_ptr<Medal> m_glow_sprite;
        MedalSequence m_medals_sequence;
        MedalSequence m_glow_sequence;
        MapLoadEventListenerHandle m_map_load_event_listener;

        void render() noexcept override;
        void set_up_medal_sequences() noexcept;
        void set_up_event_listeners() noexcept;
        void update_tags_references() noexcept;

    public:
        H4RenderQueue() : RenderQueue(6) {
            set_up_medal_sequences();
            set_up_event_listeners();
        }
    };

    class H4Medals {
    private:
        H4RenderQueue m_render_queue;

        /** State */
        std::size_t m_killing_spree_count = 0;
        std::size_t m_dying_spree_count = 0;
        std::size_t m_deaths_count = 0;
        std::map<PlayerHandle, std::optional<std::size_t>> m_multikill_counts;
        std::map<PlayerHandle, std::optional<TimePoint>> m_multikill_timestamps;
        std::optional<TimePoint> m_last_death_timestamp;
        PlayerHandle m_last_killed_by = PlayerHandle::null();
        std::string m_last_medal;

        /** Event listeners */
        HudChatEventListenerHandle m_handle_multiplayer_events_listener;
        MapLoadEventListenerHandle m_map_load_event_listener;

        void set_up_event_listeners() noexcept;

    public:
        void show_medal(std::string name) noexcept;
        
        H4Medals() {
            set_up_event_listeners();
        };
    };
}

#endif
