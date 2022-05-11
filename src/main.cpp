#include <vector>
#include <memory>

#include "Core/Headers/Core.h"
#include "Render/Headers/Texture2D.h"

#include "Render/Headers/Renderer.h"
#include "Render/Headers/Camera.h"
#include "Render/Systems/Headers/SpriteRenderer.h"
#include "Render/Pipelines/Headers/SpritePipeline.h"

// Example
int main()
{
	using ZVK::Texture2D;

	ZVK::Core& core = ZVK::Core::GetCore();

	try
	{
		// Title, window width, window height
		core.Init("Test App", 1280, 720);

		std::vector<ZVK::Sprite> sprites;
		
		std::shared_ptr<Texture2D> texture1 = std::make_shared<Texture2D>("resources/textures/textureAtlas.png");

		// x, y, width, height, texture, r, g, b, a, subX, subY, subWidth, subHeight
		sprites.emplace_back(0.f, 0.f, 64.f, 64.f, texture1, 1.f, 1.f, 1.f, 1.f, 0, 0, 64, 64);
	
		// Vertex shader, fragment shader
		ZVK::SpritePipeline spritePipeline("resources/shaders/spir-v/SpriteVert.spv", 
			"resources/shaders/spir-v/SpriteFrag.spv");

		// window width, window height, can use mouse scroll event for zooming
		ZVK::Camera cam(1280, 720, true);

		ZVK::Renderer renderer;

		// Pipeline, error texture path(in case a nullptr is passed to the sprite for a texture)
		ZVK::SpriteRenderer spriteRenderer(&spritePipeline, "resources/textures/error.png");

		while (!core.GetWindow().ShouldClose())
		{
			glfwPollEvents();

			cam.Update();

			renderer.Begin();
			spriteRenderer.Draw(renderer, sprites, cam);
			renderer.End();
		}

	}
	catch (std::exception e)
	{
		std::cout << "-------------\n" << e.what() << "\n-------------\n";
	}

	return 0;
}