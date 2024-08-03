// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__MEDALS__RENDER_QUEUE_HPP
#define RACCOON__MEDALS__RENDER_QUEUE_HPP

#include <deque>
#include <queue>
#include <vector>
#include <cstdint>
#include <balltze/events/render.hpp>
#include "medal.hpp"

namespace Raccoon::Medals {
    class RenderQueue {
    private:
        using UIRenderEvent = Balltze::Event::UIRenderEvent;
        using UIRenderEventListenerHandle = Balltze::Event::EventListenerHandle<UIRenderEvent>;

        MedalState m_initial_state;
        std::size_t m_max_renders;
        MedalSequence m_sequence;
        std::deque<Medal> m_renders;
        std::queue<Medal> m_queue;
        std::vector<Medal> m_medals;
        UIRenderEventListenerHandle m_render_event_listener;

        Medal &get_medal(std::string name);

    public:
        RenderQueue(MedalState initial_state, std::size_t max_renders, MedalSequence sequence) noexcept;
        void add_medal(Medal medal) noexcept;
        void render() noexcept;
        void show_medal(std::string name);
    };
}

#endif
