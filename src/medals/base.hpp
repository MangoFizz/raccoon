// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__MEDALS__BASE_HPP
#define RACCOON__MEDALS__BASE_HPP

#include <map>
#include <chrono>
#include <queue>
#include <deque>
#include <optional>
#include <cstdint>
#include <balltze/events/render.hpp>
#include <balltze/math.hpp>
#include <balltze/engine/data_types.hpp>
#include <balltze/engine/tag_definitions/bitmap.hpp>
#include <balltze/engine/tag_definitions/sound.hpp>

namespace Raccoon::Medals {
    namespace Event = Balltze::Event;
    namespace Engine = Balltze::Engine;
    namespace Math = Balltze::Math;

    using TimePoint = std::chrono::steady_clock::time_point;

    struct MedalState {
        Engine::Point2D position = {0.0f, 0.0f};
        float scale = 1.0f;
        Engine::ColorARGBInt color_mask = {255, 255, 255, 255};
        float rotation = 0.0f;
        bool sequence_finished = false;
    };

    enum MedalStateProperty {
        MEDAL_STATE_PROPERTY_POSITION_X,
        MEDAL_STATE_PROPERTY_POSITION_Y,
        MEDAL_STATE_PROPERTY_SCALE,
        MEDAL_STATE_PROPERTY_OPACITY,
        MEDAL_STATE_PROPERTY_ROTATION
    };

    class MedalSequence {
    private:
        struct Keyframe {
            long duration;
            Math::QuadraticBezier curve;
            union Value {
                Engine::Point position;
                float scale;
            } value;
        };

        std::map<MedalStateProperty, std::deque<Keyframe>> m_properties_sequences;
        std::optional<long> m_max_duration_buffer;

    public:
        long duration() noexcept;
        void add_property_keyframe(MedalStateProperty property, Math::QuadraticBezier curve, Keyframe::Value value, long duration) noexcept;
        MedalState get_state_at(std::chrono::milliseconds elapsed) noexcept;
        MedalSequence() = default;
    };

    class Medal {
    private:
        std::string m_name;
        std::uint16_t m_width;
        std::uint16_t m_height;
        std::uint8_t m_fps;
        std::vector<Engine::TagDefinitions::BitmapData *> m_bitmaps;
        std::string m_bitmap_tag_path;
        std::optional<std::string> m_sound_tag_path;
        MedalSequence *m_sequence;

    public:
        const std::string &name() const noexcept;
        std::uint16_t width() const noexcept;
        std::uint16_t height() const noexcept;
        std::optional<std::string> sound_tag_path() const noexcept;
        std::string bitmap_tag_path() const noexcept;
        const MedalSequence *sequence() const noexcept;
        MedalState draw(Engine::Point2D offset, std::optional<TimePoint> creation_time) const noexcept;
        void reload_bitmap_tag() noexcept;

        Medal(std::string name, std::uint16_t width, std::uint16_t height, std::string bitmap_tag_path, std::optional<std::string> sound_tag_path, MedalSequence &sequence) 
          : m_name(name), m_width(width), m_height(height), m_bitmap_tag_path(bitmap_tag_path), m_sound_tag_path(sound_tag_path), m_sequence(&sequence) {
            m_fps = 0;
            reload_bitmap_tag();
        }

        Medal(std::string name, std::uint16_t width, std::uint16_t height, std::uint8_t fps, std::string bitmap_tag_path, std::optional<std::string> sound_tag_path, MedalSequence &sequence) 
          : m_name(name), m_width(width), m_height(height), m_fps(fps), m_bitmap_tag_path(bitmap_tag_path), m_sound_tag_path(sound_tag_path), m_sequence(&sequence) {
            reload_bitmap_tag();            
        }
    };

    class SoundPlaybackQueue {
    private:
        std::optional<std::chrono::steady_clock::time_point> m_current_playing_sound_start;
        std::optional<std::int64_t> m_current_playing_sound_duration;
        Engine::TagDefinitions::Sound *m_current_playing_sound = nullptr;
        std::queue<std::string> m_queue;
        Event::TickEvent::ListenerHandle m_tick_event_listener;
        Event::SoundPlaybackEvent::ListenerHandle m_sound_playback_event_listener;

    public:
        SoundPlaybackQueue() noexcept;
        ~SoundPlaybackQueue() noexcept;
        void enqueue_sound(std::string sound_tag_path) noexcept;
    };

    class RenderQueue {
    protected:
        std::size_t m_max_renders;
        std::deque<std::pair<TimePoint, const Medal *>> m_renders;
        std::queue<const Medal *> m_queue;
        Event::UIRenderEvent::ListenerHandle m_render_event_listener;
        
        virtual void render() noexcept = 0;

    public:
        RenderQueue(std::size_t max_renders) noexcept;
        ~RenderQueue() noexcept;
        void show_medal(const Medal *medal) noexcept;
    };
}

#endif
