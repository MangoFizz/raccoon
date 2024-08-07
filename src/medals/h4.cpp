// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/command.hpp>
#include <balltze/engine/tag_definitions/tag_collection.hpp>
#include <balltze/engine/tag_definitions/sound.hpp>
#include <balltze/engine/tag.hpp>
#include <balltze/features/tags_handling.hpp>
#include "../resources/resources.hpp"
#include "../logger.hpp"
#include "h4.hpp"

using namespace Balltze;

namespace Raccoon::Medals {
    constexpr const char *h4_medals_tag_collection = "raccoon\\medals\\h4";

    void H4RenderQueue::render() noexcept {
        auto now = std::chrono::steady_clock::now();
        
        for(std::size_t i = m_renders.size(); i < m_max_renders && !m_queue.empty(); i++) {
            auto &[time, medal] = m_renders.emplace_back(std::make_pair(now, m_queue.front()));
            m_sound_queue.push(medal->sound_tag_path());
            m_queue.pop();
            m_last_pushed_medal = now;
        }

        if(m_renders.empty()) {
            return;
        }

        Engine::Point2D position = {8, 358};
        Engine::Point2D offset = {0, 0};
        Engine::Point2D base_offset = {0, 0};
        auto it = m_renders.begin();
        
        auto first_medal_time = it->first;
        auto curve = Math::QuadraticBezier::linear();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - first_medal_time).count();
        auto progress = curve.get_point(elapsed / 60.0f).y;
        
        while(it != m_renders.end()) {
            auto [creation_time, medal] = *it;
            auto local_offset = offset;

            if(elapsed < 60 && creation_time != first_medal_time) {
                local_offset.x = (base_offset.x * progress) + (offset.x - base_offset.x);
            }

            auto state = medal->draw(position + local_offset, creation_time);
            m_glow_sprite->draw(position + local_offset, creation_time);
            offset.x += medal->width();
            if(creation_time == first_medal_time) {
                base_offset.x += medal->width();
            }
            if(state.sequence_finished) {
                it = m_renders.erase(it);
            }
            else {
                it++;
            }
        }
    }

    void H4RenderQueue::set_up_medal_sequences() noexcept {
        auto linear_curve = Math::QuadraticBezier::linear();
        auto flat_curve = Math::QuadraticBezier::flat();

        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 2.0 }, 0);
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 2.0 }, 30);
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 1.5 }, 30); 
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 1.0 }, 30); 
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.2 }, 0);
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.2 }, 30); 
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.6 }, 30); 
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 1.0 }, 30); 
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 1.0 }, 1700); 
        m_medals_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.0 }, 300);

        m_glow_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 1.3 }, 0);
        m_glow_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 1.0 }, 150);
        m_glow_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.65 }, 0);
        m_glow_sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, flat_curve, { .scale = 0.0 }, 130);
    }

    void H4RenderQueue::set_up_event_listeners() noexcept {
        m_map_load_event_listener = MapLoadEvent::subscribe([this](MapLoadEvent &event) {
            if(event.time == Event::EVENT_TIME_AFTER) {
                update_tags_references();
            }
        });
    }

    void H4RenderQueue::update_tags_references() noexcept {
        namespace Engine = Engine;

        if(m_medals.empty()) {
            auto *collection_tag = Engine::get_tag(h4_medals_tag_collection, Engine::TAG_CLASS_TAG_COLLECTION);
            auto *collection_data = reinterpret_cast<Engine::TagDefinitions::TagCollection *>(collection_tag->data);
            for(std::size_t i = 0; i < collection_data->tags.count; i++) {
                auto &tag_ref = collection_data->tags.elements[i].reference;
                if(tag_ref.tag_class == Engine::TAG_CLASS_BITMAP) {
                    std::string path = tag_ref.path;
                    std::string name = path.substr(path.find_last_of("\\") + 1);
                    if(name == "glow") {
                        m_glow_sprite = std::make_unique<Medal>(name, 30, 30, 30, path, "", m_glow_sequence);
                    }
                    else {
                        add_medal(Medal(name, 30, 30, path, "raccoon\\medals\\h4\\sounds\\" + name, m_medals_sequence));
                    }
                }
            }
        }
        else {
            for(auto &medal : m_medals) {
                medal.reload_bitmap_tag();
            }
            m_glow_sprite->reload_bitmap_tag();
        }
    }

    void H4Medals::set_up_event_listeners() noexcept {
        m_handle_multiplayer_events_listener = Event::NetworkGameHudMessageEvent::subscribe([this](Event::NetworkGameHudMessageEvent &event) {
            if(event.time == Event::EVENT_TIME_BEFORE) {
                auto show_medal = [this, &event](std::string name) {
                    m_render_queue.show_medal(name);
                    event.cancel();
                };

                auto [message_type, causer, victim, local_player] = event.context;
                if(message_type == Engine::HUD_MESSAGE_LOCAL_KILLED_PLAYER) {
                    if(causer == local_player) {
                        m_killing_spree_count++;
                        m_dying_spree_count = 0;

                        if(m_last_medal != "kill") {
                            show_medal("kill");
                        }

                        switch(m_killing_spree_count) {
                            case 5:
                                show_medal("killing_spree");
                                break;
                            case 10:
                                show_medal("killing_frenzy");
                                break;
                            case 15:
                                show_medal("running_riot");
                                break;
                            case 20:
                                show_medal("rampage");
                                break;
                            case 25:
                                show_medal("untouchable");
                                break;
                            case 30:
                                show_medal("invincible");
                                break;
                            case 35:
                                show_medal("inconceivable");
                                break;
                            case 40:
                                show_medal("unfriggenbelievable");
                                break;
                        }

                        if(m_multikill_timestamps.find(local_player) == m_multikill_timestamps.end() || !m_multikill_timestamps[local_player]) {
                            m_multikill_timestamps[local_player] = std::chrono::steady_clock::now();
                            m_multikill_counts[local_player] = 1;
                        }
                        else {
                            m_multikill_counts[local_player].value()++;

                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *m_multikill_timestamps[local_player]).count();
                            if(elapsed < 4500 && m_multikill_counts[local_player] <= 10) {
                                switch(*m_multikill_counts[local_player]) {
                                    case 2:
                                        show_medal("double_kill");
                                        break;
                                    case 3:
                                        show_medal("triple_kill");
                                        break;
                                    case 4:
                                        show_medal("overkill");
                                        break;
                                    case 5:
                                        show_medal("killtacular");
                                        break;
                                    case 6:
                                        show_medal("killtrocity");
                                        break;
                                    case 7:
                                        show_medal("killamanjaro");
                                        break;
                                    case 8:
                                        show_medal("killtastrophe");
                                        break;
                                    case 9:
                                        show_medal("killpocalypse");
                                        break;
                                    case 10:
                                        show_medal("killionaire");
                                        break;
                                }
                            }
                            else {
                                m_multikill_counts[local_player] = 1;
                            }

                            m_multikill_timestamps[local_player] = now;
                        }
                    }
                    else {
                        if(m_multikill_timestamps.find(causer) == m_multikill_timestamps.end() || !m_multikill_timestamps[causer]) {
                            m_multikill_counts[causer] = 1;
                        }
                        else {
                            m_multikill_counts[causer].value()++;
                        }
                        m_multikill_timestamps[causer] = std::chrono::steady_clock::now();
                    }

                    if(m_multikill_timestamps.find(victim) != m_multikill_timestamps.end() && m_multikill_timestamps[victim]) {
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *m_multikill_timestamps[victim]).count();
                        if(elapsed <= 700 && victim != local_player) {
                            show_medal("avenger");
                        }

                        if(m_multikill_counts[victim] >= 5) {
                            show_medal("killjoy");
                        }
                    }

                    if(m_last_death_timestamp) {
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *m_last_death_timestamp).count();
                        if(elapsed <= 3000) {
                            show_medal("from_the_grave");
                        }
                    }

                    if(!m_last_killed_by.is_null() && m_last_killed_by == victim) {
                        show_medal("revenge");
                        m_last_killed_by = PlayerHandle::null();
                    }

                    if(local_player == victim) {
                        m_killing_spree_count = 0;
                        m_dying_spree_count++;
                        m_deaths_count++;
                        m_last_death_timestamp = std::chrono::steady_clock::now();
                        m_last_killed_by = causer;
                    }
                    
                    m_multikill_counts[victim] = 0;
                    m_multikill_timestamps[victim] = std::nullopt;
                }

                if(message_type == Engine::HUD_MESSAGE_SUICIDE) {
                    if(local_player == victim) {
                        m_killing_spree_count = 0;
                        m_dying_spree_count++;
                        m_deaths_count++;
                        m_last_death_timestamp = std::nullopt;
                        m_last_killed_by = PlayerHandle::null();
                    }
                    else {
                        m_multikill_counts[victim] = 0;
                        m_multikill_timestamps[victim] = std::nullopt;
                    }
                }

                if(message_type == Engine::HUD_MESSAGE_LOCAL_CTF_SCORE) {
                    show_medal("flag_capture");
                }
            }
        });

        m_multiplayer_sound_event_listener = MultiplayerSoundEvent::subscribe([](MultiplayerSoundEvent &event) {
            if(event.time == Event::EVENT_TIME_BEFORE) {
                switch(event.context.sound) {
                    case Engine::MULTIPLAYER_SOUND_DOUBLE_KILL:
                    case Engine::MULTIPLAYER_SOUND_TRIPLE_KILL:
                    case Engine::MULTIPLAYER_SOUND_KILLTACULAR:
                    case Engine::MULTIPLAYER_SOUND_RUNNING_RIOT:
                    case Engine::MULTIPLAYER_SOUND_KILLING_SPREE:
                        event.cancel();
                }
            }
        });

        m_map_load_event_listener = MapLoadEvent::subscribe([this](MapLoadEvent &event) {
            if(event.time == Event::EVENT_TIME_BEFORE) {
                auto path = Balltze::get_plugin_path() / "raccoon.map";
                Balltze::Features::import_tag_from_map(path, h4_medals_tag_collection, Balltze::Engine::TAG_CLASS_TAG_COLLECTION);
            }
        });
    }

    void H4Medals::show_medal(std::string name) noexcept {
        m_last_medal = name;
        m_render_queue.show_medal(name);
    }

    void set_up_h4_medals() {
        static H4Medals medals;

        register_command("show_medals", "medals", "", {}, +[](int argc, const char **argv) -> bool {
            int amount = std::stoi(argv[0]);
            for(int i = 0; i < amount; i++) {
                medals.show_medal("double_kill");
            }
            return true;
        }, false, 1, 1, true, false);    
    }
}
