// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/command.hpp>
#include <balltze/plugin.hpp>
#include <balltze/events/map_load.hpp>
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
        static H4RenderQueue render_queue(6);    

        Balltze::register_command("show_medals", "medals", "", {}, +[](int argc, const char **argv) -> bool {
            int amount = std::stoi(argv[0]);
            for(int i = 0; i < amount; i++) {
                render_queue.show_medal("double_kill");
            }
            return true;
        }, false, 1, 1, true, false);    
    }
}
