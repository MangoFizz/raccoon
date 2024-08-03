// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/engine/user_interface.hpp>
#include "../logger.hpp"
#include "medal.hpp"

namespace Raccoon::Medals {
    long MedalSequence::duration() noexcept {
        if(!m_max_duration_buffer.has_value()) {
            m_max_duration_buffer = 0;
            for (const auto& [property, transitions] : m_properties_sequences) {
                long duration = 0;
                for (const auto& transition : transitions) {
                    duration += transition.duration;
                }
                if (duration > m_max_duration_buffer.value()) {
                    m_max_duration_buffer = duration;
                }
            }
        }
        return m_max_duration_buffer.value();
    }

    void MedalSequence::add_property_keyframe(MedalStateProperty property, Math::QuadraticBezier curve, Keyframe::Value value, long duration) noexcept {
        m_properties_sequences[property].push_back({ .duration = duration, .curve = curve, .value = value });
        m_max_duration_buffer.reset();
    }

    MedalState MedalSequence::get_state_at(std::chrono::milliseconds elapsed) noexcept {
        MedalState state;
        if(elapsed.count() > duration()) {
            state.sequence_finished = true;
        }

        for (const auto &[property, transitions] : m_properties_sequences) {
            for(auto &transition : transitions) {
                if (transition.duration > 0 && elapsed.count() < transition.duration) {
                    auto curve_point = transition.curve.get_point(static_cast<float>(elapsed.count()) / transition.duration);
                    auto progress = curve_point.y;
                    switch (property) {
                        case MEDAL_STATE_PROPERTY_POSITION_X:
                            state.position.x += (transition.value.position - state.position.x) * progress;
                            break;
                        case MEDAL_STATE_PROPERTY_POSITION_Y:
                            state.position.y += (transition.value.position - state.position.y) * progress;
                            break;
                        case MEDAL_STATE_PROPERTY_SCALE:
                            state.scale += (transition.value.scale - state.scale) * progress;
                            break;
                        case MEDAL_STATE_PROPERTY_OPACITY:
                            state.color_mask.alpha += (transition.value.scale * 255 - state.color_mask.alpha) * progress;
                            break;
                        case MEDAL_STATE_PROPERTY_ROTATION:
                            state.rotation += (transition.value.scale - state.rotation) * progress;
                            break;
                    }
                    break;
                }
                else {
                    switch (property) {
                        case MEDAL_STATE_PROPERTY_POSITION_X:
                            state.position.x += transition.value.position;
                            break;
                        case MEDAL_STATE_PROPERTY_POSITION_Y:
                            state.position.y += transition.value.position;
                            break;
                        case MEDAL_STATE_PROPERTY_SCALE:
                            state.scale = transition.value.scale;
                            break;
                        case MEDAL_STATE_PROPERTY_OPACITY:
                            state.color_mask.alpha = static_cast<std::uint8_t>(transition.value.scale * 255);
                            break;
                        case MEDAL_STATE_PROPERTY_ROTATION:
                            state.rotation = transition.value.scale;
                            break;
                    }
                    elapsed -= std::chrono::milliseconds(transition.duration);
                }
            }
        }
        return state;
    }

    static void rotate_rectangle(Engine::Rectangle2D &rect, Engine::Point2D center, float angle) {
        auto rotate_point = [&center, &angle](Engine::Point2D &point) {
            float x = point.x - center.x;
            float y = point.y - center.y;
            point.x = center.x + x * std::cos(angle) - y * std::sin(angle);
            point.y = center.y + x * std::sin(angle) + y * std::cos(angle);
        };

        Engine::Point2D points[4] = {
            {rect.left, rect.top},
            {rect.right, rect.top},
            {rect.right, rect.bottom},
            {rect.left, rect.bottom}
        };

        for(auto &point : points) {
            rotate_point(point);
        }

        rect.left = points[0].x;
        rect.top = points[0].y;
        rect.right = points[0].x;
        rect.bottom = points[0].y;

        for(auto &point : points) {
            rect.left = std::min(rect.left, static_cast<std::int16_t>(point.x));
            rect.top = std::min(rect.top, static_cast<std::int16_t>(point.y));
            rect.right = std::max(rect.right, static_cast<std::int16_t>(point.x));
            rect.bottom = std::max(rect.bottom, static_cast<std::int16_t>(point.y));
        }
    }

    MedalState Medal::draw(Engine::Point2D offset) noexcept {
        if(!m_creation_time.has_value()) {
            m_creation_time = std::chrono::steady_clock::now();
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - *m_creation_time);
        auto state = m_sequence->get_state_at(elapsed);
        Engine::Rectangle2D draw_rect;
        draw_rect.left = state.position.x + offset.x - (m_width * (state.scale - 1.0)) / 2;
        draw_rect.top = state.position.y + offset.y - (m_height * (state.scale - 1.0)) / 2;
        draw_rect.right = draw_rect.left + m_width * state.scale;
        draw_rect.bottom = draw_rect.top + m_height * state.scale;

        Engine::Point2D center = {
            draw_rect.left + static_cast<float>(m_width * state.scale) / 2,
            draw_rect.top + static_cast<float>(m_height * state.scale) / 2
        };

        if(state.rotation != 0) {
            rotate_rectangle(draw_rect, center, state.rotation);
        }

        auto color_mask = state.color_mask;

        Engine::draw_bitmap_in_rect(m_bitmap, draw_rect, color_mask);

        return state;
    }

    std::string &Medal::name() noexcept {
        return m_name;
    }

    std::uint16_t Medal::width() noexcept {
        return m_width;
    }

    std::uint16_t Medal::height() noexcept {
        return m_height;
    }

    BitmapData *Medal::bitmap() noexcept {
        return m_bitmap;
    }

    Engine::TagHandle Medal::sound_tag() noexcept {
        return m_sound_tag;
    }

    MedalState &Medal::state() noexcept {
        return m_state;
    }

    TimePoint Medal::creation_time() noexcept {
        return *m_creation_time;
    }

    MedalSequence const *Medal::sequence() noexcept {
        return m_sequence;
    }
}
