#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	enum Correct {
		NA,
		ZERO,
		ONE,
		TWO,
		THREE,
		FOUR, // No FIVE, mathematically impossible
		SIX
	} guesses;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// sandwich transforms:
	Scene::Transform *bread1_transform = nullptr;
	Scene::Transform *bread2_transform = nullptr;
	Scene::Transform *meat_transform = nullptr;
	Scene::Transform *cheese_transform = nullptr;
	Scene::Transform *lettuce_transform = nullptr;
	Scene::Transform *tomato_transform = nullptr;

	// Sandwich booleans
	bool bread1_pressed;
	bool bread2_pressed;
	bool meat_pressed;
	bool cheese_pressed;
	bool lettuce_pressed;
	bool tomato_pressed;
	uint8_t my_order_index;

	// Transforms to center
	glm::vec3 bread1_offset = glm::vec3(-7, -2.75, 0);
	glm::vec3 bread2_offset = glm::vec3(-4, -2.75, 0);
	glm::vec3 meat_offset = glm::vec3(-7, 0, -0.1);
	glm::vec3 cheese_offset = glm::vec3(-4, 0, 0);
	glm::vec3 lettuce_offset = glm::vec3(-7, 2.75, 0);
	glm::vec3 tomato_offset = glm::vec3(-4, 2.75, 0);
	glm::vec3 vertical_offset = glm::vec3(0, 0, 0.25);

	// Game variables
	std::vector<uint8_t> order;
	std::vector<uint8_t> my_order;
	bool end_game = false;
	bool animation_active = false;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
