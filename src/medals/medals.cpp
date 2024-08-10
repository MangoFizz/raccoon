// SPDX-License-Identifier: GPL-3.0-only

#include <list>
#include <balltze/api.hpp>
#include <balltze/plugin.hpp>
#include <balltze/command.hpp>
#include <balltze/config.hpp>
#include "../logger.hpp"
#include "h4.hpp"
#include "medals.hpp"


namespace Raccoon::Medals {    
#include <balltze/helpers/event_base.inl>

    template class EventHandler<MedalEvent>;

    static double milliseconds_since(std::chrono::steady_clock::time_point time) {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count();
    }
    
    void MedalsHandler::dispatch_medals(Engine::NetworkGameMultiplayerHudMessage message_type, Engine::PlayerHandle causer_handle, Engine::PlayerHandle victim_handle, Engine::PlayerHandle local_player_handle) noexcept {
        auto dispatch_medal = [&](std::string name) {
            if(causer_handle == local_player_handle) {
                show_medal(name);
            }
        };

        auto now = std::chrono::steady_clock::now();

        m_player_states.try_emplace(causer_handle, PlayerState());
        m_player_states.try_emplace(victim_handle, PlayerState());

        auto &players_table = Engine::get_player_table();
        auto *causer = players_table.get_player(causer_handle);
        auto *victim = players_table.get_player(victim_handle);

        switch(message_type) {
            case Engine::HUD_MESSAGE_LOCAL_KILLED_PLAYER: {
                // Update causer
                {
                    auto &[killing_spree, multikill_spree, last_death] = m_player_states[causer_handle];

                    if(m_last_medal != "kill") {
                        dispatch_medal("kill");
                    }

                    killing_spree.push_back({ .player = victim_handle, .timestamp = now });

                    switch(killing_spree.size()) {
                        case 5:
                            dispatch_medal("killing_spree");
                            break;
                        case 10:
                            dispatch_medal("killing_frenzy");
                            break;
                        case 15:
                            dispatch_medal("running_riot");
                            break;
                        case 20:
                            dispatch_medal("rampage");
                            break;
                        case 25:
                            dispatch_medal("untouchable");
                            break;
                        case 30:
                            dispatch_medal("invincible");
                            break;
                        case 35:
                            dispatch_medal("inconceivable");
                            break;
                        case 40:
                            dispatch_medal("unfriggenbelievable");
                            break;
                    }

                    multikill_spree.push_back({ .player = victim_handle, .timestamp = now });

                    if(multikill_spree.size() > 1) {
                        auto last_kill = multikill_spree[multikill_spree.size() - 2];
                        auto elapsed = milliseconds_since(*last_kill.timestamp);
                        
                        if(elapsed < 4500 && multikill_spree.size() <= 10) {
                            switch(multikill_spree.size()) {
                                case 2:
                                    dispatch_medal("double_kill");
                                    break;
                                case 3:
                                    dispatch_medal("triple_kill");
                                    break;
                                case 4:
                                    dispatch_medal("overkill");
                                    break;
                                case 5:
                                    dispatch_medal("killtacular");
                                    break;
                                case 6:
                                    dispatch_medal("killtrocity");
                                    break;
                                case 7:
                                    dispatch_medal("killamanjaro");
                                    break;
                                case 8:
                                    dispatch_medal("killtastrophe");
                                    break;
                                case 9:
                                    dispatch_medal("killpocalypse");
                                    break;
                                case 10:
                                    dispatch_medal("killionaire");
                                    break;
                            }
                        }
                        else {
                            multikill_spree.erase(multikill_spree.begin(), multikill_spree.end() - 1);                        
                        }
                    }

                    if(last_death) {
                        if(last_death->player == victim_handle && causer->respawn_time > 0) {
                            dispatch_medal("from_the_grave");
                        }

                        if(last_death->player == victim_handle && causer->respawn_time == 0) {
                            dispatch_medal("revenge");
                        }
                    }
                }

                // Update victim
                {
                    auto &[killing_spree, multikill_spree, last_death] = m_player_states[victim_handle];

                    if(multikill_spree.size() > 0) {
                        auto last_kill = multikill_spree.back();
                        auto elapsed = milliseconds_since(*last_kill.timestamp);
                    
                        if(Engine::network_game_current_game_is_team() && victim->team != causer->team) {
                            auto last_killed_player = players_table.get_player(last_kill.player);
                            if(causer->team == last_killed_player->team) {
                                if(elapsed <= 700) {
                                    dispatch_medal("avenger");
                                }
                            }
                        }

                        if(multikill_spree.size() >= 5) {
                            dispatch_medal("killjoy");
                        }

                        // Reset stats
                        killing_spree.clear();
                        multikill_spree.clear();
                        last_death = { .player = causer_handle, .timestamp = now };
                    }
                }

                break;
            }

            case Engine::HUD_MESSAGE_SUICIDE: {
                auto &[killing_spree, multikill_spree, last_death] = m_player_states[victim_handle];
                killing_spree.clear();
                multikill_spree.clear();
                last_death = std::nullopt;
                break;
            }

            case Engine::HUD_MESSAGE_LOCAL_CTF_SCORE: {
                dispatch_medal("flag_capture");
                break;
            }
        }
    }

    bool MedalsHandler::mute_hud_message(Engine::NetworkGameMultiplayerHudMessage message_type) noexcept {
        switch(message_type) {
            case Engine::HUD_MESSAGE_LOCAL_DOUBLE_KILL:
            case Engine::HUD_MESSAGE_LOCAL_TRIPLE_KILL:
            case Engine::HUD_MESSAGE_LOCAL_KILLING_SPREE:
            case Engine::HUD_MESSAGE_LOCAL_KILLTACULAR:
            case Engine::HUD_MESSAGE_LOCAL_RUNNING_RIOT: 
                return true;
            default:
                return false;
        }
    }

    bool MedalsHandler::mute_multiplayer_sound(Engine::NetworkGameMultiplayerSound sound) noexcept {
        switch(sound) {
            case Engine::MULTIPLAYER_SOUND_DOUBLE_KILL:
            case Engine::MULTIPLAYER_SOUND_TRIPLE_KILL:
            case Engine::MULTIPLAYER_SOUND_KILLTACULAR:
            case Engine::MULTIPLAYER_SOUND_RUNNING_RIOT:
            case Engine::MULTIPLAYER_SOUND_KILLING_SPREE:
                return true;
            default:
                return false;
        }
    }

    void MedalsHandler::update_medals_tag_references() noexcept {
        logger.debug("Updating medals tag references...");
        for(auto &medal : m_medals) {
            medal.reload_bitmap_tag();
        }
    }

    void MedalsHandler::set_up_event_listeners() noexcept {
        m_map_load_event_listener = Event::MapLoadEvent::subscribe([this](auto &event) {
            if(event.time == Event::EVENT_TIME_AFTER) {
                update_medals_tag_references();
            }
        });

        m_handle_multiplayer_events_listener = Event::NetworkGameHudMessageEvent::subscribe([this](auto &event) {
            if(event.time == Event::EVENT_TIME_BEFORE) {
                auto &[message_type, causer, victim, local_player] = event.context;
                dispatch_medals(message_type, causer, victim, local_player);
                if(mute_hud_message(message_type)) {
                    event.cancel();
                }
            }
        });

        m_multiplayer_sound_event_listener = Event::NetworkGameMultiplayerSoundEvent::subscribe([this](auto &event) {
            if(event.time == Event::EVENT_TIME_BEFORE) {
                if(mute_multiplayer_sound(event.context.sound)) {
                    event.cancel();
                }
            }
        });
    }

    Medal *MedalsHandler::get_medal(std::string name) noexcept {
        for(auto &medal : m_medals) {
            if(medal.name() == name) {
                return &medal;
            }
        }
        return nullptr;
    }

    void MedalsHandler::add_medal(Medal medal) noexcept {
        m_medals.push_back(medal);
    }

    void MedalsHandler::show_medal(std::string name, std::optional<Engine::PlayerHandle> player) noexcept {
        auto *medal = get_medal(name);
        if(medal) {
            show_medal(medal, player);
        }
    }

    void MedalsHandler::show_medal(Medal *medal, std::optional<Engine::PlayerHandle> player) {
        m_last_medal = medal->name();

        MedalEventContext context = { .medal = medal, .player = player.value_or(Engine::PlayerHandle::null()) };
        MedalEvent event(EVENT_TIME_BEFORE, context);
        event.dispatch();

        if(m_render_queue) {
            m_render_queue->show_medal(medal);
        }

        auto sound = medal->sound_tag_path();
        if(sound) {
            m_sound_queue.enqueue_sound(*sound);
        }

        MedalEvent after_event(EVENT_TIME_AFTER, context);
        after_event.dispatch();
    }

    MedalsStyle MedalsHandler::get_style() const noexcept {
        return m_style;
    }

    void MedalsHandler::set_style(MedalsStyle style) noexcept {
        m_style = style;
        load_style();
    }

    void MedalsHandler::load_style() noexcept {
        if(Balltze::get_balltze_side() == Balltze::BALLTZE_SIDE_DEDICATED_SERVER) {
            return;
        }

        switch(m_style) {
            case STYLE_H4: {
                logger.info("Loading H4 medals style...");
                m_render_queue = std::make_unique<H4RenderQueue>();
                auto medals = get_h4_medals();
                for(auto &medal : medals) {
                    add_medal(medal);
                }
                auto glow = get_medal("glow");
                if(glow) {
                    static_cast<H4RenderQueue *>(m_render_queue.get())->set_glow_sprite(glow);
                }
                break;
            }

            default: {
                logger.error("Unsupported medals style");
                return;
            }
        }

        update_medals_tag_references();
    }

    MedalsHandler::MedalsHandler() noexcept {
        set_up_event_listeners();
    }

    MedalsHandler::~MedalsHandler() noexcept {
        m_map_load_event_listener.remove();
        m_handle_multiplayer_events_listener.remove();
        m_multiplayer_sound_event_listener.remove();
    }

    void set_up_medals() {
        static MedalsHandler medals;

        Balltze::register_command("medals_style", "medals", "Sets the medals style.", "[style: string]", +[](int argc, const char **argv) -> bool {
            auto string_for_style = [](MedalsStyle style) {
                switch(style) {
                    case STYLE_H4:
                        return "h4";
                    default:
                        return "unknown";
                }
            };

            if(argc == 1) {
                std::string style = argv[0];
                if(style == "h4") {
                    medals.set_style(STYLE_H4);
                }
                else {
                    logger.error("Unsupported medals style");
                }
            }
            logger.info("Current medals style: {}", string_for_style(medals.get_style()));
            return true;
        }, true, 0, 1); 

        Balltze::register_command("show_medals", "medals", "", {}, +[](int argc, const char **argv) -> bool {
            std::string name = argv[0];
            int amount = std::stoi(argv[1]);
            for(int i = 0; i < amount; i++) {
                medals.show_medal(name);
            }
            return true;
        }, false, 2, 2, true, false);    
    }
}
