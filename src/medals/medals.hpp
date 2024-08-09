// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__MEDALS__MEDALS_HPP
#define RACCOON__MEDALS__MEDALS_HPP

#include "base.hpp"

namespace Raccoon::Medals {
    enum MedalsStyle {
        STYLE_H4
    };

    class MedalsHandler {
    private:
        struct PlayerKill {
            Engine::PlayerHandle player;
            std::optional<TimePoint> timestamp;
        };

        struct PlayerState {
            std::vector<PlayerKill> killing_spree;
            std::vector<PlayerKill> multikill_spree;
            std::optional<PlayerKill> last_death;
        };

        MedalsStyle m_style;
        std::vector<Medal> m_medals;
        std::unique_ptr<RenderQueue> m_render_queue;
        SoundPlaybackQueue m_sound_queue;
        std::map<Engine::PlayerHandle, PlayerState> m_player_states;
        std::string m_last_medal;

        /** Event listeners */
        Event::MapLoadEvent::ListenerHandle m_map_load_event_listener;
        Event::NetworkGameHudMessageEvent::ListenerHandle m_handle_multiplayer_events_listener;
        Event::NetworkGameMultiplayerSoundEvent::ListenerHandle m_multiplayer_sound_event_listener;

        void dispatch_medals(Engine::NetworkGameMultiplayerHudMessage message_type, Engine::PlayerHandle causer, Engine::PlayerHandle victim, Engine::PlayerHandle local_player) noexcept;
        bool mute_hud_message(Engine::NetworkGameMultiplayerHudMessage message_type) noexcept;
        bool mute_multiplayer_sound(Engine::NetworkGameMultiplayerSound sound) noexcept;
        void update_medals_tag_references() noexcept;
        void load_style() noexcept;
        void set_up_event_listeners() noexcept;

    public:
        Medal *get_medal(std::string name) noexcept;
        void add_medal(Medal medal) noexcept;
        void show_medal(std::string name) noexcept;
        MedalsStyle get_style() const noexcept;
        void set_style(MedalsStyle style) noexcept;
        MedalsHandler() noexcept;
        ~MedalsHandler() noexcept;
    };

    void set_up_medals();
}

#endif
