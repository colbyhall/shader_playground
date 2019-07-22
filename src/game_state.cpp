#include "game_state.h"
#include "draw.h"
#include "collision.h"

#include <ch_stl/window.h>
#include <ch_stl/opengl.h>
#include <ch_stl/filesystem.h>
#include <ch_stl/time.h>

Game_State g_game_state;
Input_State g_input_state;

const tchar* window_title = CH_TEXT("pandora_project");

void Input_State::bind(ch::Window* window) {
	window->on_exit_requested = [](const ch::Window& window) {
		g_input_state.exit_requested = true;
	};

	window->on_key_pressed = [](const ch::Window& window, u8 key) {
		g_input_state.keys_down[key] = true;
		g_input_state.keys_pressed[key] = true;
	};

	window->on_key_released = [](const ch::Window& window, u8 key) {
		g_input_state.keys_down[key] = false;
	};
}

void Game_State::init() {
    assert(ch::load_gl());
    {
        const u32 width = 1280;
        const u32 height = (u32)((f32)width * (9.f / 16.f));
        assert(ch::create_gl_window(window_title, width, height, 0, &window));
    }
    assert(ch::make_current(window));

    window.on_sizing = [](const ch::Window& window) {
        g_game_state.draw_game();
    };

    window.set_visibility(true);

	g_input_state.bind(&window);

#if CH_PLATFORM_WINDOWS
	wglSwapIntervalEXT(!BUILD_DEBUG);
#endif
	draw_init();

	loaded_world = ch_new World;

	Camera* cam = loaded_world->spawn_entity<Camera>(0.f);
	cam->set_to_current();
	{
		Block* b = loaded_world->spawn_entity<Block>(ch::Vector2(0.f, -200.f));
		b->size = ch::Vector2(10000.f, 100.f);
	}
	{
		Block* b = loaded_world->spawn_entity<Block>(ch::Vector2(200.f, -100.f));
		b->size = 100.f;
	}
	{
		Block* b = loaded_world->spawn_entity<Block>(ch::Vector2(300.f, -50.f));
		b->size = ch::Vector2(100.f, 200.f);
	}
	Player* p = loaded_world->spawn_entity<Player>(ch::Vector2(0.f, 100.f));
	player_id = p->id;
}

void Game_State::loop() {
	f64 last_frame_time = ch::get_ms_time();
    while (!g_input_state.exit_requested) {
		f64 current_frame_time = ch::get_ms_time();
		const f32 dt = (f32)(current_frame_time - last_frame_time);
		last_frame_time = current_frame_time;
        process_inputs();
        tick_game(dt);
        draw_game();
    }
}

void Game_State::shut_down() {

}

void Game_State::process_inputs() {
	ch::mem_set(g_input_state.keys_pressed, sizeof(g_input_state.keys_pressed), 0);

    ch::poll_events();

	ch::Vector2 mouse_pos;
	if (!window.get_mouse_position(&mouse_pos)) {
		mouse_pos.ux = 0;
		mouse_pos.uy = 0;
	}
	g_input_state.current_mouse_position = ch::Vector2((f32)mouse_pos.ux, (f32)mouse_pos.uy);
}

void Game_State::tick_game(f32 dt) {
	if (loaded_world) loaded_world->tick(dt);
}

void Game_State::draw_game() {
	draw_frame_begin();

	if (loaded_world) loaded_world->draw();

	{
		const ch::Vector2 viewport_size = window.get_viewport_size();
		const ch::Vector2 mouse_pos = g_input_state.current_mouse_position;
		const ch::Vector2 cam_pos = loaded_world->current_camera->position.xy;

		const f32 width = (f32)viewport_size.ux;
		const f32 height = (f32)viewport_size.uy;

		const f32 x = (2.f * mouse_pos.x) / width - 1.f;
		const f32 y = 1.f - (2.f * mouse_pos.y) / height;

		const ch::Vector4 clip_coords(x, y, -1.f, 1.f);
		ch::Vector4 eye_coords = view_to_projection.inverse() * clip_coords;
		eye_coords.z = -1.f;
		eye_coords.w = 0.f;

		const ch::Vector4 ray_world = world_to_view.inverse() * eye_coords;
		const ch::Vector2 world = ray_world.xy;

		draw_quad(world + cam_pos, 10.f, ch::white);
	}

	draw_frame_end();
}
