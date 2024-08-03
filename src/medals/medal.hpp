// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__MEDALS__MEDALS_HPP
#define RACCOON__MEDALS__MEDALS_HPP

#include <map>
#include <chrono>
#include <deque>
#include <optional>
#include <cstdint>
#include <balltze/math.hpp>
#include <balltze/engine/data_types.hpp>
#include <balltze/engine/tag_definitions/bitmap.hpp>
#include <balltze/engine/tag_definitions/sound.hpp>

namespace Raccoon::Medals {
    namespace Engine = Balltze::Engine;
    namespace Math = Balltze::Math;
    using TimePoint = std::chrono::steady_clock::time_point;
    using BitmapData = Engine::TagDefinitions::BitmapData;

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
        /**
         * Get the duration of the whole animation (in milliseconds)
         */
        long duration() noexcept;

        /**
         * Set a property of the animation
         * @param property      Animation property
         * @param curve         The quadratic bezier used to generate the property frames
         * @param value         The value of the property
         * @param duration      Duration of the animation for this property in milliseconds
         */
        void add_property_keyframe(MedalStateProperty property, Math::QuadraticBezier curve, Keyframe::Value value, long duration) noexcept;

        /**
         * Apply the current state of the animation transform to a render state
         * @param state     Reference to render state
         */
        MedalState get_state_at(std::chrono::milliseconds elapsed) noexcept;

        /**
         * Default constructor for animation
         */
        MedalSequence() = default;
    };

    class Medal {
    private:
        std::string m_name;
        std::uint16_t m_width = 0;
        std::uint16_t m_height = 0;
        BitmapData *m_bitmap;
        Engine::TagHandle m_sound_tag;
        MedalSequence *m_sequence;
        MedalState m_state;
        std::optional<TimePoint> m_creation_time;

    public:
        /**
         * Get the name of the medal
         */
        std::string &name() noexcept;

        /**
         * Get the width of the medal
         */
        std::uint16_t width() noexcept;

        /**
         * Get the height of the medal
         */
        std::uint16_t height() noexcept;

        /**
         * Get the bitmap of the medal
         */
        BitmapData *bitmap() noexcept;

        /**
         * Get the sound of the medal
         */
        Engine::TagHandle sound_tag() noexcept;

        /**
         * Get the current state of the medal
         */
        MedalState &state() noexcept;

        /**
         * Get the creation time of the medal
         */
        TimePoint creation_time() noexcept;

        /**
         * Get the animation sequence of the medal
         */
        MedalSequence const *sequence() noexcept;

        /**
         * Set the bitmap of the medal
         * @param bitmap    Bitmap data
         */
        MedalState draw(Engine::Point2D offset) noexcept;

        /**
         * Default constructor for medal
         */
        Medal(std::string name, BitmapData &bitmap, std::uint16_t width, std::uint16_t height, Engine::TagHandle sound_tag, MedalSequence &sequence) 
          : m_name(name), m_bitmap(&bitmap), m_width(width), m_height(height), m_sound_tag(sound_tag), m_sequence(&sequence) {
            m_state = MedalState();
            m_creation_time = std::nullopt;
        }
    };
}

#endif
