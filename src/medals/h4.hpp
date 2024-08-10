// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__MEDALS__H4_MEDALS_HPP
#define RACCOON__MEDALS__H4_MEDALS_HPP

#include "queue.hpp"

namespace Raccoon::Medals {
    constexpr const char *h4_medals_tag_collection = "raccoon\\medals\\h4";

    class H4RenderQueue : public RenderQueue {
    private:
        std::optional<TimePoint> m_last_pushed_medal;
        double m_slide_duration_ms = 60;
        Medal *m_glow_sprite;
        MedalSequence m_medals_sequence;
        MedalSequence m_glow_sequence;

        void render() noexcept override;

    public:
        void set_glow_sprite(Medal *medal) noexcept;
        H4RenderQueue() : RenderQueue(6) {}
    };

    std::vector<Medal> get_h4_medals() noexcept;
}

#endif
