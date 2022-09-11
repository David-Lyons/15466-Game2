#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <algorithm>
#include <string>
#include <time.h>

// Based on starter code
GLuint sandwich_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > sandwich_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("sandwich.pnct"));
	sandwich_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > sandwich_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("sandwich.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = sandwich_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = sandwich_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*sandwich_scene) {
	// Pointers to the main transforms
	for (auto &transform : scene.transforms) {
		if (transform.name == "Bread1") bread1_transform = &transform;
		else if (transform.name == "Bread2") bread2_transform = &transform;
		else if (transform.name == "Meat") meat_transform = &transform;
		else if (transform.name == "Cheese") cheese_transform = &transform;
		else if (transform.name == "Lettuce") lettuce_transform = &transform;
		else if (transform.name == "Tomato") tomato_transform = &transform;
	}

	// Set initial positions
	bread1_origin = bread1_transform->position; 
	bread2_origin = bread2_transform->position;
	meat_origin = meat_transform->position;
	cheese_origin = cheese_transform->position;
	lettuce_origin = lettuce_transform->position;
	tomato_origin = tomato_transform->position;

	// Set enums to their default
	guesses = Correct::NA;
	animation = Animation::INACTIVE;

	// Get pointer to camera
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// Randomize the order
	order.push_back(1);
	order.push_back(2);
	order.push_back(3);
	order.push_back(4);
	order.push_back(5);
	order.push_back(6);
	// Shuffle from https://stackoverflow.com/questions/6926433/how-to-shuffle-a-stdvector
	// Seed generation from my game1 code
	srand((unsigned int)time(NULL));
	unsigned seed = rand();
	std::shuffle(order.begin(), order.end(), std::default_random_engine(seed));
	// printf("Game: %d %d %d %d %d %d\n", order[0], order[1], order[2], order[3], order[4], order[5]);
	// Enable this print if you'd like to have the option to give up.

	// Initialize our guess vector to be size 6
	my_order.push_back(0);
	my_order.push_back(0);
	my_order.push_back(0);
	my_order.push_back(0);
	my_order.push_back(0);
	my_order.push_back(0);
	my_order_index = 0;

	// Nothing has been selected
	bread1_pressed = false;
	bread2_pressed = false;
	meat_pressed = false;
	cheese_pressed = false;
	lettuce_pressed = false;
	tomato_pressed = false;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	// Ignore key presses if animation is going or if the game is over
	if (end_game || animation != Animation::INACTIVE) {
		return false;
	}
	// When an ingredient is chosen, advance the index and begin the animation
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_1) {
			if (!bread1_pressed) {
				bread1_pressed = true;
				my_order[my_order_index] = 1;
				my_order_index++;
				animation = Animation::BREAD1;
				return true;
			}
		} else if (evt.key.keysym.sym == SDLK_2) {
			if (!bread2_pressed) {
				bread2_pressed = true;
				my_order[my_order_index] = 2;
				my_order_index++;
				animation = Animation::BREAD2;
				return true;
			}
		} else if (evt.key.keysym.sym == SDLK_3) {
			if (!meat_pressed) {
				meat_pressed = true;
				my_order[my_order_index] = 3;
				my_order_index++;
				animation = Animation::MEAT;
				return true;
			}
		} else if (evt.key.keysym.sym == SDLK_4) {
			if (!cheese_pressed) {
				cheese_pressed = true;
				my_order[my_order_index] = 4;
				my_order_index++;
				animation = Animation::CHEESE;
				return true;
			}
		} else if (evt.key.keysym.sym == SDLK_5) {
			if (!lettuce_pressed) {
				lettuce_pressed = true;
				my_order[my_order_index] = 5;
				my_order_index++;
				animation = Animation::LETTUCE;
				return true;
			}
		} else if (evt.key.keysym.sym == SDLK_6) {
			if (!tomato_pressed) {
				tomato_pressed = true;
				my_order[my_order_index] = 6;
				my_order_index++;
				animation = Animation::TOMATO;
				return true;
			}
		}
	}
	return false;
}

// Find out how many guesses were correct
uint8_t compare_vectors(std::vector<uint8_t> first, std::vector<uint8_t> second) {
	uint8_t result = 0;
	for (uint8_t i = 0; i < 6; i++) {
		if (first[i] == second[i]) {
			result++;
		} else if ((first[i] == 1 && second[i] == 2) || (first[i] == 2 && second[i] == 1)) {
			result++;
		}
	}
	return result;
}

// Just in case there are rounding errors
bool vec3_equal(glm::vec3 first, glm::vec3 second) {
	return abs(first.x - second.x) < 0.01 && abs(first.y - second.y) < 0.01 && abs(first.z - second.z) < 0.01;
}

void PlayMode::update(float elapsed) {
	if (my_order_index == 6 && animation == Animation::INACTIVE) {
		total_guesses++;
		uint8_t num_correct = compare_vectors(order, my_order);
		// Tell DrawLine how many are correct
		if (num_correct == 0) {
			guesses = Correct::ZERO;
		} else if (num_correct == 1) {
			guesses = Correct::ONE;
		} else if (num_correct == 2) {
			guesses = Correct::TWO;
		} else if (num_correct == 3) {
			guesses = Correct::THREE;
		} else if (num_correct == 4) {
			guesses = Correct::FOUR;
		} else if (num_correct == 6) {
			guesses = Correct::SIX;
			// Stop everything
			end_game = true;
		}

		// Reset all variables
		my_order_index = 0;
		my_order[0] = 0;
		my_order[1] = 0;
		my_order[2] = 0;
		my_order[3] = 0;
		my_order[4] = 0;
		my_order[5] = 0;

		bread1_pressed = false;
		bread2_pressed = false;
		meat_pressed = false;
		cheese_pressed = false;
		lettuce_pressed = false;
		tomato_pressed = false;
		
		if (!end_game) {
			// Don't move the sandwich ingredients back if the game is over
			bread1_transform->position = bread1_origin;
			bread2_transform->position = bread2_origin;
			meat_transform->position = meat_origin;
			cheese_transform->position = cheese_origin;
			lettuce_transform->position = lettuce_origin;
			tomato_transform->position = tomato_origin;
		}
	}
	
	// A bit of math here. Basically my goal is that the animation takes about a second.
	// So I take the difference between the origin and destination and call that the velocity.
	// Multiply that by elapsed to get distance traveled. If the difference between us and the
	// destination is less, then go to the end.
	// Figure out the z offset based on which index we are.
	float height = vertical_offset * float(my_order_index - 1);
	glm::vec3 velocity;
	float x_diff, y_diff, z_diff;
	switch (animation) {
	case INACTIVE:
		break;
	case BREAD1:
		velocity = sandwich_destination - bread1_origin;
		x_diff = std::max(sandwich_destination.x - bread1_transform->position.x, velocity.x * elapsed);
		y_diff = std::max(sandwich_destination.y - bread1_transform->position.y, velocity.y * elapsed);
		z_diff = std::min(height - bread1_transform->position.z, 2.0f * height * elapsed);
		bread1_transform->position += glm::vec3(x_diff, y_diff, z_diff);
		if (vec3_equal(bread1_transform->position, sandwich_destination + glm::vec3(0.0f, 0.0f, height))) {
			animation = Animation::INACTIVE;
		}
		break;
	case BREAD2:
		velocity = sandwich_destination - bread2_origin;
		x_diff = std::max(sandwich_destination.x - bread2_transform->position.x, velocity.x * elapsed);
		y_diff = std::max(sandwich_destination.y - bread2_transform->position.y, velocity.y * elapsed);
		z_diff = std::min(height - bread2_transform->position.z, 2.0f * height * elapsed);
		bread2_transform->position += glm::vec3(x_diff, y_diff, z_diff);
		if (vec3_equal(bread2_transform->position, sandwich_destination + glm::vec3(0.0f, 0.0f, height))) {
			animation = Animation::INACTIVE;
		}
		break;
	case MEAT:
		velocity = sandwich_destination - meat_origin;
		x_diff = std::max(sandwich_destination.x - meat_transform->position.x, velocity.x * elapsed);
		z_diff = std::min(height - meat_transform->position.z, 2.0f * height * elapsed);
		meat_transform->position += glm::vec3(x_diff, 0.0f, z_diff);
		if (vec3_equal(meat_transform->position, sandwich_destination + glm::vec3(0.0f, 0.0f, height))) {
			animation = Animation::INACTIVE;
		}
		break;
	case CHEESE:
		velocity = sandwich_destination - cheese_origin;
		x_diff = std::max(sandwich_destination.x - cheese_transform->position.x, velocity.x * elapsed);
		z_diff = std::min(height - cheese_transform->position.z, 2.0f * height * elapsed);
		cheese_transform->position += glm::vec3(x_diff, 0.0f, z_diff);
		if (vec3_equal(cheese_transform->position, sandwich_destination + glm::vec3(0.0f, 0.0f, height))) {
			animation = Animation::INACTIVE;
		}
		break;
	case LETTUCE:
		velocity = sandwich_destination - lettuce_origin;
		x_diff = std::max(sandwich_destination.x - lettuce_transform->position.x, velocity.x * elapsed);
		y_diff = std::min(sandwich_destination.y - lettuce_transform->position.y, velocity.y * elapsed);
		z_diff = std::min(height - lettuce_transform->position.z, 2.0f * height * elapsed);
		lettuce_transform->position += glm::vec3(x_diff, y_diff, z_diff);
		if (vec3_equal(lettuce_transform->position, sandwich_destination + glm::vec3(0.0f, 0.0f, height))) {
			animation = Animation::INACTIVE;
		}
		break;
	case TOMATO:
		velocity = sandwich_destination - tomato_origin;
		x_diff = std::max(sandwich_destination.x - tomato_transform->position.x, velocity.x * elapsed);
		y_diff = std::min(sandwich_destination.y - tomato_transform->position.y, velocity.y * elapsed);
		z_diff = std::min(height - tomato_transform->position.z, 2.0f * height * elapsed);
		tomato_transform->position += glm::vec3(x_diff, y_diff, z_diff);
		if (vec3_equal(tomato_transform->position, sandwich_destination + glm::vec3(0.0f, 0.0f, height))) {
			animation = Animation::INACTIVE;
		}
		break;
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text (from starter code with text modified):
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.2f;
		float offset = 50.0f / drawable_size.y;
		switch (guesses) {
		case NA:
			break;
		case ZERO:
			lines.draw_text("Your guess had no ingredients correct.",
				glm::vec3(-aspect + 0.1f * H + offset, -1.0 + + 0.1f * H + offset, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			break;
		case ONE:
			lines.draw_text("Your guess had 1 ingredient correct.",
				glm::vec3(-aspect + 0.1f * H + offset, -1.0 + + 0.1f * H + offset, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			break;
		case TWO:
			lines.draw_text("Your guess had 2 ingredients correct.",
				glm::vec3(-aspect + 0.1f * H + offset, -1.0 + + 0.1f * H + offset, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			break;
		case THREE:
			lines.draw_text("Your guess had 3 ingredients correct.",
				glm::vec3(-aspect + 0.1f * H + offset, -1.0 + + 0.1f * H + offset, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			break;
		case FOUR:
			lines.draw_text("Your guess had 4 ingredients correct.",
				glm::vec3(-aspect + 0.1f * H + offset, -1.0 + + 0.1f * H + offset, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			break;
		case SIX:
			lines.draw_text("You won! Congrats! " + std::to_string(total_guesses) + " guesses.",
				glm::vec3(-aspect + 0.1f * H + offset, -1.0 + + 0.1f * H + offset, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			break;
		}
	}
}
