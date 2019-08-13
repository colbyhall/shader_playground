#pragma once

#include <ch_stl/math.h>

struct Render_Command {
	ch::Vector2 position;
	ch::Vector2 size;

	u8 tile_index;
};

struct Tile_Renderer {
	ch::Array<Render_Command> commands;

	Tile_Renderer();

	void push_tile(ch::Vector2 position, ch::Vector2 size, u8 tile_index);
	u32 flush();
};

extern Tile_Renderer tile_renderer;