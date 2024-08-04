// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__MEDALS__RENDER_QUEUE_HPP
#define RACCOON__MEDALS__RENDER_QUEUE_HPP

#include <deque>
#include <queue>
#include <vector>
#include <utility>
#include <cstdint>
#include <balltze/events/render.hpp>
#include "medal.hpp"

namespace Raccoon::Medals {
    class RenderQueue {
    protected:
        using UIRenderEvent = Balltze::Event::UIRenderEvent;
        using UIRenderEventListenerHandle = Balltze::Event::EventListenerHandle<UIRenderEvent>;

        std::size_t m_max_renders;
        std::deque<std::pair<TimePoint, Medal *>> m_renders;
        std::queue<Medal *> m_queue;
        std::vector<Medal> m_medals;
        UIRenderEventListenerHandle m_render_event_listener;

        virtual void render() noexcept = 0;

    public:
        RenderQueue(std::size_t max_renders) noexcept;
        ~RenderQueue() noexcept;
        Medal *get_medal(std::string name) noexcept;
        void add_medal(Medal medal) noexcept;
        void show_medal(std::string name);
    };

    class H4RenderQueue : public RenderQueue {
    private:
        using MapLoadEvent = Balltze::Event::MapLoadEvent;
        using MapLoadEventListenerHandle = Balltze::Event::EventListenerHandle<MapLoadEvent>;

        std::unique_ptr<Medal> m_glow_sprite;
        MedalSequence m_medals_sequence;
        MedalSequence m_glow_sequence;
        MapLoadEventListenerHandle m_map_load_event_listener;

        void render() noexcept override;
        void load_medals() noexcept;

    public:
        H4RenderQueue(std::size_t max_renders) noexcept : RenderQueue(max_renders) {
            load_medals();
        }
    };
}

#endif
