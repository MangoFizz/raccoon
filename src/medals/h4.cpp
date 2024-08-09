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
    void H4RenderQueue::render() noexcept {
        auto now = std::chrono::steady_clock::now();
        
        for(std::size_t i = m_renders.size(); i < m_max_renders && !m_queue.empty(); i++) {
            auto &[time, medal] = m_renders.emplace_front(std::make_pair(now, m_queue.front()));
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

    void H4RenderQueue::set_glow_sprite(Medal *medal) noexcept {
        m_glow_sprite = medal;
    }

    std::vector<Medal> get_h4_medals() noexcept {
        auto linear_curve = Math::QuadraticBezier::linear();
        auto flat_curve = Math::QuadraticBezier::flat();

        static std::optional<MedalSequence> medals_sequence;
        if(!medals_sequence) {
            medals_sequence = std::make_optional<MedalSequence>();
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 2.0 }, 0);
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 2.0 }, 30);
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 1.5 }, 30); 
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 1.0 }, 30); 
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.2 }, 0);
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.2 }, 30); 
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.6 }, 30); 
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 1.0 }, 30); 
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 1.0 }, 1700); 
            medals_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.0 }, 300);
        }

        static std::optional<MedalSequence> glow_sequence;
        if(!glow_sequence) {
            glow_sequence = std::make_optional<MedalSequence>();
            glow_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 1.3 }, 0);
            glow_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_SCALE, linear_curve, { .scale = 1.0 }, 150);
            glow_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, linear_curve, { .scale = 0.65 }, 0);
            glow_sequence->add_property_keyframe(MEDAL_STATE_PROPERTY_OPACITY, flat_curve, { .scale = 0.0 }, 130);
        }

        auto *collection_tag = Engine::get_tag(h4_medals_tag_collection, Engine::TAG_CLASS_TAG_COLLECTION);
        if(!collection_tag) {
            logger.error("Failed to load H4 medals tag collection. Medals will not be available.");
            return {};
        }

        auto *collection_data = reinterpret_cast<Engine::TagDefinitions::TagCollection *>(collection_tag->data);
        std::map<std::string, std::pair<std::string, std::optional<std::string>>> medals_data;
        for(std::size_t i = 0; i < collection_data->tags.count; i++) {
            auto &tag_ref = collection_data->tags.elements[i].reference;
            std::string path = tag_ref.path;
            std::string name = path.substr(path.find_last_of("\\") + 1);
            medals_data.try_emplace(name, std::make_pair("", std::nullopt));
            if(tag_ref.tag_class == Engine::TAG_CLASS_BITMAP) {
                medals_data[name].first = path;
            }
            else if(tag_ref.tag_class == Engine::TAG_CLASS_SOUND) {
                medals_data[name].second = path;
            }
        }

        std::vector<Medal> medals;
        for(auto &[name, data] : medals_data) {
            auto &[bitmap, sound] = data;
            if(bitmap.empty()) {
                continue;
            }
            if(name == "glow") {
                medals.emplace_back(name, 30, 30, bitmap, sound, *glow_sequence);
            }
            else {
                medals.emplace_back(name, 30, 30, 30, bitmap, sound, *medals_sequence);
            }
        }

        return std::move(medals);
    }
}
