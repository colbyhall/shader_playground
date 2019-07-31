#include "entity.h"
#include "draw.h"
#include "game_state.h"
#include "world.h"

#include <ch_stl/time.h>
#include <ch_stl/input.h>

void Entity::draw() {
#if BUILD_DEBUG
	if (collision_enabled && g_game_state.debug_collision) {
		get_bounds().debug_draw();
	}
#endif
}

Camera::Camera() : Super() {
	collision_enabled = false;
}

void Camera::tick(f32 dt) {
	Player* player = get_world()->find_entity<Player>(g_game_state.player_id);

	if (player->position.x > position.x + 96.f) {
		position.x = player->position.x - 96.f;
	}

	if (player->position.x < position.x - 96.f) {
		position.x = player->position.x + 96.f;
	}
}

void Camera::draw() {
	Super::draw();
	const ch::Vector2 render_pos(ch::round(position.x), ch::round(position.y));
	Imm_Draw::render_from_pos(position.xy, ((f32)Imm_Draw::back_buffer_height / 2.f));
}

void Camera::set_to_current() {
	World* world = get_world();

	if (world) {
		world->current_camera = this;
	}
}

ch::Vector2 Camera::get_mouse_position_in_world() const {
	// @HACK(Chall): Find a better way to do this
	Imm_Draw::render_from_pos(position.xy, (f32)Imm_Draw::back_buffer_height / 2.f);

	const ch::Vector2 viewport_size = g_game_state.window.get_viewport_size();
	const ch::Vector2 mouse_pos = g_input_state.current_mouse_position;

	const f32 width = (f32)viewport_size.ux;
	const f32 height = (f32)viewport_size.uy;

	const f32 x = (2.f * mouse_pos.x) / width - 1.f;
	const f32 y = 1.f - (2.f * mouse_pos.y) / height;

	const ch::Vector4 clip_coords(x, y, -1.f, 1.f);
	ch::Vector4 eye_coords = Imm_Draw::view_to_projection.inverse() * clip_coords;
	eye_coords.z = -1.f;
	eye_coords.w = 0.f;

	const ch::Vector4 ray_world = Imm_Draw::world_to_view.inverse() * eye_coords;
	const ch::Vector2 world = ray_world.xy;

	return position.xy + world;
}

Block::Block() : Super() {
	tick_enabled = false;
}

void Block::draw() {
	const ch::Color color = ch::white;
	Imm_Draw::draw_textured_quad(position.xy, size, color, Imm_Draw::test);
	Super::draw();
}

void Player::tick(f32 dt) {
	size = ch::Vector2(12.f, 32.f);

	// velocity.y = ch::max(velocity.y, -(980.f * 980.f));

	if (on_wall && is_falling()) {
		velocity.y = -16.f * 2.f;
		num_jumps = 0;
	} else {
		velocity.y -= 16.f * 9.8f * dt;
	}

	velocity.x = 0.f;

	if (num_jumps < max_jumps &&  g_input_state.was_key_pressed(' ')) {
		if (velocity.y > 0) {
			velocity.y += jump_y_velocity;
		} else {
			velocity.y = jump_y_velocity;
		}
		num_jumps += 1;
	}

	const bool is_sprinting = g_input_state.is_key_down(16);

	const f32 speed = is_sprinting ? sprint_speed : walk_speed;

	if (g_input_state.is_key_down('A')) {
		velocity.x = -speed;
	}

	if (g_input_state.is_key_down('D')) {
		velocity.x = speed;
	}

	if (g_input_state.was_mouse_button_pressed(CH_MOUSE_LEFT)) {
		const Camera* current_camera = get_world()->current_camera;
		const ch::Vector2 mouse_pos = current_camera->get_mouse_position_in_world();

		Block* bobby_b = get_world()->spawn_entity<Block>(mouse_pos);
		bobby_b->size = 16.f;
	}

	collision_tick(dt);

	get_world()->current_camera->tick(dt);
}

void Player::draw() {
	const ch::Vector2 draw_size(16.f, 32.f);
	const ch::Vector2 draw_pos(ch::round(position.x), ch::round(position.y));
	Imm_Draw::draw_textured_quad(position.xy, draw_size, ch::white, Imm_Draw::character);


	Super::draw();
}

void Player::collision_tick(f32 dt) {
	ch::Vector2 new_velocity = velocity;
	ch::Vector2 new_position = position.xy + velocity * dt;

	Trace_Details td;
	td.e_to_ignore.push(id);

	const ch::Vector2 start = position.xy;
	const ch::Vector2 end = new_position;

	const bool was_on_wall = on_wall;

	on_ground = false;
	on_wall = false;

	ch::Array<Hit_Result> results;
	if (get_world()->aabb_multi_sweep(&results, start, end, size, td)) {
		ch::Vector2 best_pos = 0.f;
		for (Hit_Result& result : results) {
			const ch::Vector2 d = ch::abs(result.impact - end);

			const ch::Vector2 possible_pos = result.normal * d;

			if (ch::abs(best_pos.x) < ch::abs(possible_pos.x)) best_pos.x = possible_pos.x;
			if (ch::abs(best_pos.y) < ch::abs(possible_pos.y)) best_pos.y = possible_pos.y;

			const ch::Vector2 up(0.f, 1.f);
			const ch::Vector2 right(1.f, 0.f);
			const f32 dot_up = up.dot(result.normal);
			const f32 dot_right = right.dot(result.normal);

			new_velocity -= ch::abs(result.normal) * new_velocity;

			if (dot_up > 0.7f) {
				on_ground = true;
				num_jumps = 0;
			}

			if (ch::abs(dot_right) > 0.7f && velocity.x != 0.f) {
				on_wall = true;
			}
		}
		new_position += best_pos;
	}

	position.xy = new_position;
	velocity = new_velocity;

}
