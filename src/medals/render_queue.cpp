// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/command.hpp>
#include <balltze/plugin.hpp>
#include <balltze/events/map_load.hpp>
#include <balltze/events/netgame.hpp>
#include <balltze/features/tags_handling.hpp>
#include <balltze/engine/tag_definitions/tag_collection.hpp>
#include "../logger.hpp"
#include "render_queue.hpp"

namespace Raccoon::Medals {
    using namespace Balltze::Event;

    Medal *RenderQueue::get_medal(std::string name) noexcept {
        for(auto &medal : m_medals) {
            if(medal.name() == name) {
                return &medal;
            }
        }
        return nullptr;
    }

    RenderQueue::RenderQueue(std::size_t max_renders) noexcept
        : m_max_renders(max_renders) {
        m_render_event_listener = UIRenderEvent::subscribe([this](const UIRenderEvent &event) {
            if(event.time == EVENT_TIME_BEFORE) {
                render();
            }
        });
    }

    RenderQueue::~RenderQueue() noexcept {
        m_render_event_listener.remove();
    }

    void RenderQueue::add_medal(Medal medal) noexcept {
        m_medals.push_back(medal);
    }

    void RenderQueue::show_medal(std::string name) {
        try {
            m_queue.push(get_medal(name));
        } 
        catch(std::runtime_error &) {
            throw;
        }
    }

    void H4RenderQueue::render() noexcept {
        for(std::size_t i = m_renders.size(); i < m_max_renders && !m_queue.empty(); i++) {
            m_renders.push_front({ std::chrono::steady_clock::now(), m_queue.front() });
            m_queue.pop();
        }

        Engine::Point2D offset = {8, 358};
        auto it = m_renders.begin();
        while(it != m_renders.end()) {
            auto [creation_time, medal] = *it;
            auto state = medal->draw(offset, creation_time);
            m_glow_sprite->draw(offset, creation_time);
            offset.x += medal->width();
            if(state.sequence_finished) {
                it = m_renders.erase(it);
            }
            else {
                it++;
            }
        }
    }

    void H4RenderQueue::load_medals() noexcept {
        constexpr const char *h4_medals_tag_collection = "raccoon\\medals\\h4";

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

        m_map_load_event_listener = MapLoadEvent::subscribe([this](const Balltze::Event::MapLoadEvent &event) {
            if(event.time == Balltze::Event::EVENT_TIME_BEFORE) {
                auto path = Balltze::get_plugin_path() / "raccoon.map";
                Balltze::Features::import_tag_from_map(path, h4_medals_tag_collection, Balltze::Engine::TAG_CLASS_TAG_COLLECTION);
            }
            else {
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
                                add_medal(Medal(name, 30, 30, path, "", m_medals_sequence));
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
        });
    }

    void set_up_h4_medals() {
        using PlayerHandle = Balltze::Engine::PlayerHandle;
        using TimePoint = std::chrono::steady_clock::time_point;

        static H4RenderQueue render_queue(6);
        static std::size_t killing_spree_count = 0;
        static std::size_t dying_spree_count = 0;
        static std::size_t deaths_count = 0;
        static std::map<PlayerHandle, std::optional<std::size_t>> multikill_counts;
        static std::map<PlayerHandle, std::optional<TimePoint>> multikill_timestamps;
        static std::optional<TimePoint> last_death_timestamp;
        static PlayerHandle last_killed_by = PlayerHandle::null();
        static std::string last_medal;

        auto show_medal = [](std::string name) {
            render_queue.show_medal(name);
            last_medal = name;
        };

        Balltze::Event::NetworkGameHudMessageEvent::subscribe([&show_medal](Balltze::Event::NetworkGameHudMessageEvent &event) {
            if(event.time == Balltze::Event::EVENT_TIME_BEFORE) {
                auto [message_type, causer, victim, local_player] = event.context;
                if(message_type == Balltze::Engine::HUD_MESSAGE_LOCAL_KILLED_PLAYER) {
                    if(causer == local_player) {
                        killing_spree_count++;
                        dying_spree_count = 0;

                        if(last_medal != "kill") {
                            show_medal("kill");
                        }

                        switch(killing_spree_count) {
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

                        if(multikill_timestamps.find(local_player) == multikill_timestamps.end() || !multikill_timestamps[local_player]) {
                            multikill_timestamps[local_player] = std::chrono::steady_clock::now();
                            multikill_counts[local_player] = 1;
                        }
                        else {
                            multikill_counts[local_player].value()++;

                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *multikill_timestamps[local_player]).count();
                            if(elapsed < 4500 && multikill_counts[local_player] <= 10) {
                                switch(*multikill_counts[local_player]) {
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
                                multikill_counts[local_player] = 1;
                            }

                            multikill_timestamps[local_player] = now;
                        }
                    }
                    else {
                        if(multikill_timestamps.find(causer) == multikill_timestamps.end() || !multikill_timestamps[causer]) {
                            multikill_counts[causer] = 1;
                        }
                        else {
                            multikill_counts[causer].value()++;
                        }
                        multikill_timestamps[causer] = std::chrono::steady_clock::now();
                    }

                    if(multikill_timestamps.find(victim) != multikill_timestamps.end() && multikill_timestamps[victim]) {
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *multikill_timestamps[victim]).count();
                        if(elapsed <= 700 && victim != local_player) {
                            show_medal("avenger");
                        }

                        if(multikill_counts[victim] >= 5) {
                            show_medal("killjoy");
                        }
                    }

                    if(last_death_timestamp) {
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *last_death_timestamp).count();
                        if(elapsed <= 3000) {
                            show_medal("from_the_grave");
                        }
                    }

                    if(!last_killed_by.is_null() && last_killed_by == victim) {
                        render_queue.show_medal("revenge");
                        last_killed_by = PlayerHandle::null();
                    }

                    if(local_player == victim) {
                        killing_spree_count = 0;
                        dying_spree_count++;
                        deaths_count++;
                        last_death_timestamp = std::chrono::steady_clock::now();
                        last_killed_by = causer;
                    }
                    
                    multikill_counts[victim] = 0;
                    multikill_timestamps[victim] = std::nullopt;
                }

                if(message_type == Balltze::Engine::HUD_MESSAGE_SUICIDE) {
                    if(local_player == victim) {
                        killing_spree_count = 0;
                        dying_spree_count++;
                        deaths_count++;
                        last_death_timestamp = std::nullopt;
                        last_killed_by = PlayerHandle::null();
                    }
                    else {
                        multikill_counts[victim] = 0;
                        multikill_timestamps[victim] = std::nullopt;
                    }
                }

                if(message_type == Balltze::Engine::HUD_MESSAGE_LOCAL_CTF_SCORE) {
                    render_queue.show_medal("flag_capture");
                }
            }
        });

        Balltze::register_command("show_medals", "medals", "", {}, +[](int argc, const char **argv) -> bool {
            int amount = std::stoi(argv[0]);
            for(int i = 0; i < amount; i++) {
                render_queue.show_medal("double_kill");
            }
            return true;
        }, false, 1, 1, true, false);    
    }
}
