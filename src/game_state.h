#pragma once

#include <ch_stl/types.h>
#include <ch_stl/window.h>

#include "entity.h"
#include "draw.h"


struct World;

struct Game_State {
	ch::Window window;
	World* loaded_world;

	struct Font font;

	f32 dt;
	u32 fps;

    void init();
    void loop();
    void shut_down();

	void process_input();
    void tick_game(f32 dt);
    void draw_game();
};

extern Game_State game_state;