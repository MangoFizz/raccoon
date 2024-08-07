// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/engine/user_interface.hpp>
#include <balltze/engine/tag.hpp>
#include "../logger.hpp"
#include "base.hpp"

using namespace Balltze;

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
            {static_cast<float>(rect.left), static_cast<float>(rect.top)},
            {static_cast<float>(rect.right), static_cast<float>(rect.top)},
            {static_cast<float>(rect.right), static_cast<float>(rect.bottom)},
            {static_cast<float>(rect.left), static_cast<float>(rect.bottom)}
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

    MedalState Medal::draw(Engine::Point2D offset, std::optional<TimePoint> creation_time) noexcept {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - *creation_time);
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

        std::uint8_t frame = 0;
        if(m_fps > 0) {
            frame = round(elapsed.count() / (1000 / m_fps) % m_bitmaps.size());
        }

        Engine::draw_bitmap_in_rect(m_bitmaps[frame], draw_rect, color_mask);

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

    std::string Medal::sound_tag_path() noexcept {
        return m_sound_tag_path;
    }

    std::string Medal::bitmap_tag_path() noexcept {
        return m_bitmap_tag_path;
    }

    MedalSequence const *Medal::sequence() noexcept {
        return m_sequence;
    }

    void Medal::reload_bitmap_tag() {
        auto *bitmap_tag = Engine::get_tag(m_bitmap_tag_path, Engine::TAG_CLASS_BITMAP);
        if(!bitmap_tag) {
            throw std::runtime_error("Bitmap tag not found");
        }

        auto *bitmap = reinterpret_cast<Engine::TagDefinitions::Bitmap *>(bitmap_tag->data);
        m_bitmaps.clear();
        for(std::size_t i = 0; i < bitmap->bitmap_data.count; i++) {
            Balltze::Engine::load_bitmap_data_texture(bitmap->bitmap_data.elements + i, true, true);
            m_bitmaps.push_back(bitmap->bitmap_data.elements + i);
        }
    }

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
            if(event.time == Event::EVENT_TIME_BEFORE) {
                render();
            }
        });

        m_tick_event_listener = TickEvent::subscribe([this](TickEvent &event) {
            if(event.time == Event::EVENT_TIME_AFTER) {
                if(m_current_playing_sound_start) {
                    if(m_current_playing_sound_duration) {
                        auto now = std::chrono::steady_clock::now();
                        auto current_playing_sound_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - *m_current_playing_sound_start).count();
                        if(current_playing_sound_elapsed >= m_current_playing_sound_duration && !m_sound_queue.empty()) {
                            m_sound_queue.pop();
                            m_current_playing_sound_start = std::nullopt;
                            m_current_playing_sound = nullptr;
                        }
                    }
                }
                else {
                    if(!m_sound_queue.empty()) {
                        auto *sound_tag = Engine::get_tag(m_sound_queue.front(), Engine::TAG_CLASS_SOUND);
                        if(sound_tag) {
                            m_current_playing_sound_start = std::chrono::steady_clock::now();
                            Engine::play_sound(sound_tag->handle);
                            m_current_playing_sound = reinterpret_cast<Engine::TagDefinitions::Sound *>(sound_tag->data);
                        }
                        else {
                            logger.debug("Medal sound tag not found: {}", m_sound_queue.front());
                            m_sound_queue.pop();
                        }
                    }
                }
            }
        });

        m_sound_playback_event_listener = SoundPlaybackEvent::subscribe([this](const SoundPlaybackEvent &event) {
            if(event.time == Event::EVENT_TIME_AFTER && !event.cancelled()) {
                auto &[sound, permutation] = event.context;
                if(!m_current_playing_sound_duration && m_current_playing_sound && m_current_playing_sound == sound) {
                    auto duration = Engine::get_sound_permutation_samples_duration(permutation);
                    m_current_playing_sound_duration = duration.count();
                }
            }
        }, Event::EVENT_PRIORITY_HIGHEST);
    }

    RenderQueue::~RenderQueue() noexcept {
        m_render_event_listener.remove();
    }

    void RenderQueue::add_medal(Medal medal) noexcept {
        m_medals.push_back(medal);
    }

    void RenderQueue::show_medal(std::string name) {
        auto *medal = get_medal(name);
        if(!medal) {
            logger.debug("Medal not found: {}", name);
            return;
        }
        m_queue.push(medal);
    }
}
