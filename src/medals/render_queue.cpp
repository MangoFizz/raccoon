// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/events/map_load.hpp>
#include <balltze/command.hpp>
#include "render_queue.hpp"

namespace Raccoon::Medals {
    using namespace Balltze::Event;

    Medal &RenderQueue::get_medal(std::string name) {
        for(auto &medal : m_medals) {
            if(medal.name() == name) {
                return medal;
            }
        }
        throw std::runtime_error("Medal not found");
    }

    RenderQueue::RenderQueue(MedalState initial_state, std::size_t max_renders, MedalSequence sequence) noexcept
        : m_initial_state(initial_state), m_max_renders(max_renders), m_sequence(sequence) {
        m_render_event_listener = UIRenderEvent::subscribe([this](const UIRenderEvent &event) {
            if(event.time == EVENT_TIME_BEFORE) {
                render();
            }
        });
    }

    void RenderQueue::add_medal(Medal medal) noexcept {
        m_medals.push_back(medal);
    }

    void RenderQueue::render() noexcept {
        for(std::size_t i = m_renders.size(); i < m_max_renders && !m_queue.empty(); i++) {
            m_renders.push_front(m_queue.front());
            m_queue.pop();
        }

        Engine::Point2D offset = {100, 100};
        auto it = m_renders.begin();
        while(it != m_renders.end()) {
            auto &medal = *it;
            auto state = medal.draw(offset);
            offset.x += medal.width() * 1.1;
            if(state.sequence_finished) {
                it = m_renders.erase(it);
            }
            else {
                it++;
            }
        }
    }

    void RenderQueue::show_medal(std::string name) {
        try {
            m_queue.push(get_medal(name));
        } 
        catch(std::runtime_error &) {
            throw;
        }
    }

    void set_up_h4_medals() {
        static auto listener = Balltze::Event::MapLoadEvent::subscribe([](const Balltze::Event::MapLoadEvent &event) {
            if(event.time == Balltze::Event::EVENT_TIME_AFTER) {
                static MedalSequence sequence;

                auto curve = Math::QuadraticBezier::linear();

                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, curve, { .scale = 2.0 }, 0);
                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, curve, { .scale = 2.0 }, 30); // 30
                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, curve, { .scale = 1.5 }, 30); // 60
                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, curve, { .scale = 1.0 }, 30); // 90

                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, curve, { .scale = 0.2 }, 0);
                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, curve, { .scale = 0.2 }, 30); // 30
                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, curve, { .scale = 0.6 }, 30); // 90
                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, curve, { .scale = 1.0 }, 30); // 90
                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, curve, { .scale = 1.0 }, 1700); // 1710
                sequence.add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, curve, { .scale = 0.0 }, 300); // 1910

                static RenderQueue queue = RenderQueue(MedalState(), 6, sequence);

                auto bitmaps = Balltze::Engine::find_tags("medals\\h4", Balltze::Engine::TAG_CLASS_BITMAP);
                for(auto &tag : bitmaps) {
                    std::string path = tag->path;
                    std::string name = path.substr(path.find_last_of("\\") + 1);
                    printf("Found medal: %s\n", name.c_str());
                    auto *bitmap = reinterpret_cast<Balltze::Engine::TagDefinitions::Bitmap *>(tag->data);
                    auto &bitmap_data = bitmap->bitmap_data.elements[0];
                    auto medal = Medal(name, bitmap_data, 30, 30, Balltze::Engine::TagHandle::null(), sequence);
                    queue.add_medal(medal);
                }

                Balltze::register_command("show_medals", "medals", "", {}, [](int argc, const char **argv) -> bool {
                    queue.show_medal("double_kill");
                    queue.show_medal("double_kill");
                    queue.show_medal("double_kill");
                    queue.show_medal("double_kill");
                    return true;
                }, false, 0, 0, true, false);
            }
        });
    }
}
