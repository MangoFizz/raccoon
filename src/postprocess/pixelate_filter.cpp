// SPDX-License-Identifier: GPL-3.0-only

#include <chrono>
#include <d3dx9.h>
#include <balltze/events/map_load.hpp>
#include <balltze/events/d3d9.hpp>
#include <balltze/events/render.hpp>
#include <balltze/engine/tag.hpp>
#include <balltze/engine/rasterizer.hpp>
#include <balltze/helpers/d3d9.hpp>
#include <balltze/helpers/resources.hpp>
#include <balltze/memory.hpp>
#include <balltze/hook.hpp>
#include "../logger.hpp"
#include "../resources.hpp"

using namespace Balltze;

namespace Raccoon::PostProcess {
    static IDirect3DPixelShader9 *pixelate_pixel_shader = nullptr;
    static IDirect3DTexture9 *pixelate_render_target_texture = nullptr;
    static IDirect3DSurface9 *pixelate_render_target_surface = nullptr;
    static IDirect3DSurface9 *backbuffer_surface = nullptr;
    static IDirect3DDevice9 *device = nullptr;
    static Sprite pixelate_sprite;
    static Engine::RenderTarget *render_targets = nullptr;
    static void(*render_text_function_1)() = nullptr;
    static void(*render_text_function_2)() = nullptr;
    static std::uint32_t ui_render_player_index;
    static Event::EventListenerHandle<Event::TickEvent> tick_event_listener_handle;

    static HRESULT load_pixelate_pixel_shader(IDirect3DDevice9 *device, IDirect3DPixelShader9 **shader) {
        auto shader_data = load_resource_data(get_current_module(), MAKEINTRESOURCEW(ID_PIXELATE_PIXEL_SHADER), L"CSO");
        if(!shader_data) {
            return E_FAIL;
        }
        HRESULT hr = device->CreatePixelShader(reinterpret_cast<const DWORD *>(shader_data->data()), shader);
        return hr;
    }

    static void hack_text_render_functions() noexcept {
        // Get Chimera and Balltze's render text functions and then restore the original code
        auto *text_hook_sig = Memory::get_signature("text_hook");
        auto *text_hook_cave = reinterpret_cast<std::byte *>(Memory::follow_32bit_jump(text_hook_sig->data()));
        render_text_function_1 = reinterpret_cast<void(*)()>(Memory::follow_32bit_jump(text_hook_cave + 0x2));
        render_text_function_2 = reinterpret_cast<void(*)()>(Memory::follow_32bit_jump(Memory::follow_32bit_jump(text_hook_cave + 0x12) + 2));
        const_cast<Memory::Signature *>(text_hook_sig)->restore();
    }
    
    static void set_pixelate_pixel_shader() {
        constexpr float target_res[2] = {640.0f, 240.0f};
        auto &backbuffer = render_targets[0];
        float res[2] = {static_cast<float>(backbuffer.width), static_cast<float>(backbuffer.height)};
        if(!pixelate_pixel_shader) {
            load_pixelate_pixel_shader(device, &pixelate_pixel_shader);
        }
        device->SetPixelShader(pixelate_pixel_shader);
        device->SetPixelShaderConstantF(0, res, 1);
        device->SetPixelShaderConstantF(1, target_res, 1);
    }

    static void apply_pixelate_postprocess() {
        auto &backbuffer = render_targets[0];
        pixelate_sprite.update_texture(pixelate_render_target_texture);
        pixelate_sprite.begin();
        set_pixelate_pixel_shader();        
        pixelate_sprite.draw(0, 0, backbuffer.width, backbuffer.height);
        pixelate_sprite.end();
    }

    static void draw_pixelate_frame() {
        auto &backbuffer = render_targets[0];
        device->SetRenderTarget(0, backbuffer_surface);
        pixelate_sprite.update_texture(pixelate_render_target_texture);
        pixelate_sprite.begin();
        pixelate_sprite.draw(0, 0, backbuffer.width, backbuffer.height);
        pixelate_sprite.end();
    }

    static void on_d3d9_begin_scene(Event::D3D9BeginSceneEvent &event) {
        if(event.time == Event::EVENT_TIME_AFTER) {
            return;
        }

        device = event.context.device;
        if(!device) {
            return;
        }

        // Get the back buffer
        auto &backbuffer_render_target = render_targets[0];

        // Create the pixelate sprite
        if(!pixelate_render_target_texture) {
            device->CreateTexture(backbuffer_render_target.width, backbuffer_render_target.height, 1, D3DUSAGE_RENDERTARGET, backbuffer_render_target.format, D3DPOOL_DEFAULT, &pixelate_render_target_texture, NULL);
            pixelate_render_target_texture->GetSurfaceLevel(0, &pixelate_render_target_surface);
        }

        // Force it to render into the pixelate render target
        backbuffer_surface = backbuffer_render_target.surface;
        backbuffer_render_target.surface = pixelate_render_target_surface;
    }

    static void on_d3d9_end_scene(Event::D3D9EndSceneEvent &event) {
        if(event.time == Event::EVENT_TIME_AFTER || !device) {
            return;
        }

        // Apply the pixelate post process to the render target texture
        apply_pixelate_postprocess();

        // Force it to render the HUD text into our render target
        auto render_target_2 = render_targets[1];
        auto render_target_2_surface = render_target_2.surface;
        render_target_2.surface = pixelate_render_target_surface;
        Engine::render_player_hud();
        render_target_2.surface = render_target_2_surface;
        device->SetRenderTarget(0, pixelate_render_target_surface);

        // Render post carnage report
        auto asd = reinterpret_cast<std::uint16_t *>(0x006B4C00);
        auto asd2 = reinterpret_cast<bool *>(0x006B4C00 + 2);
        if(*asd2 == false && *asd == 0xFFFF) {
            Engine::render_netgame_post_carnage_report();
        }
        
        // Render ui stuff
        Engine::render_user_interface_widgets(ui_render_player_index);
        render_text_function_1();
        render_text_function_2();

        // Draw the pixelated frame to the back buffer and restore the original render target
        draw_pixelate_frame();
        render_targets[0].surface = backbuffer_surface;
    }

    static void on_ui_render_event(Event::UIRenderEvent &event) {
        if(event.time == Event::EVENT_TIME_BEFORE) {
            ui_render_player_index = event.context.player_index;
            event.cancel();
            return;
        }
    }

    static void on_hud_render_event(Event::HUDRenderEvent &event) {
        if(event.time == Event::EVENT_TIME_BEFORE) {
            event.cancel();
            return;
        }
    }

    void set_up_pixelate_shader() {
        tick_event_listener_handle = Event::TickEvent::subscribe([](Event::TickEvent &event) {
            hack_text_render_functions();
            render_targets = Engine::get_render_target();
            Event::D3D9BeginSceneEvent::subscribe(on_d3d9_begin_scene, Event::EVENT_PRIORITY_DEFAULT);
            Event::D3D9EndSceneEvent::subscribe(on_d3d9_end_scene, Event::EVENT_PRIORITY_DEFAULT);
            Event::UIRenderEvent::subscribe(on_ui_render_event, Event::EVENT_PRIORITY_HIGHEST);
            Event::HUDRenderEvent::subscribe(on_hud_render_event, Event::EVENT_PRIORITY_HIGHEST);
            tick_event_listener_handle.remove();
        }, Event::EVENT_PRIORITY_LOWEST);
    }
}
