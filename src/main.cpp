#include <iostream>

#include "../Headers/Core/Core.h"

#include "../Headers/Render/Texture2D.h"
#include "../Headers/Render/Sprite.h"

#include "../Headers/Render/Renderer.h"
#include "../Headers/Render/Camera.h"
#include "../Headers/Render/Systems/SpriteRenderer.h"
#include "../Headers/Render/Systems/ShapeRenderer.h"
#include "../Headers/Render/Pipelines/SpritePipeline.h"
#include "../Headers/Render/Pipelines/ShapePipeline.h"

#include "../Headers/Shapes/IShapes.h"
#include "../Headers/Shapes/Rectangle.h"
#include "../Headers/Shapes/Circle.h"

int main()
{
	ZVK::Vec4 randomColour();

	srand((unsigned int)time(0));

	using std::shared_ptr;
	using std::make_shared;
	using ZVK::Sprite;
	using ZVK::Texture2D;
	using ZVK::Vec2;
	using ZVK::Vec2ui;
	using ZVK::Vec3;
	using ZVK::Vec4;
	using ZVK::IShape;
	using ZVK::Rectangle;
	using ZVK::Circle;

	ZVK::Core& core = ZVK::Core::CreateCore();

	// std::vector<ZVK::Sprite> sprites;
	std::vector<shared_ptr<Sprite>> sprites;
	std::vector<shared_ptr<IShape>> shapes;
	std::vector<shared_ptr<IShape>> lights;

	std::string sPath = "resources/shaders/spir-v/";

	try
	{
		core.Init("Example App", 1280, 720);

		// Set sprites
		{
			shared_ptr<Texture2D> textureAtlas = make_shared<Texture2D>("resources/textures/textureAtlas.png");

			Sprite sprite(0.f, 0.f, 64.f, 64.f, textureAtlas, 1.f, 1.f, 1.f, 1.f, 0, 0, 64, 64);

			// x, y, width, height, texture, r, g, b, a, subX, subY, subWidth, subHeight
			sprites.emplace_back(make_shared<Sprite>(
				0.f, 0.f, 64.f, 64.f, textureAtlas, 1.f, 1.f, 1.f, 1.f,
				(uint32_t)0, (uint32_t)0, (uint32_t)64, (uint32_t)64)
			);

			// Pos, Dimensions, Texture, RGBA, SubPos, SubDimensions
			sprites.emplace_back(make_shared<Sprite>(
				Vec2(64.f, 0.f), Vec2(64.f, 64.f), textureAtlas, Vec4(1.f, 1.f, 1.f, 1.f),
				Vec2ui(64, 0), Vec2ui(64, 64))
			);
		}

		// Set Lights
		{
			// Position, dimensions, RGBA
			std::shared_ptr<Circle> light = make_shared<Circle>(Vec3(64.f, 128.f, -0.5f),
				Vec3(64.f, 64.f, 0.f), Vec4(1.f, 1.f, 1.f, 0.3f));

			std::shared_ptr<Rectangle> lightMask = make_shared<Rectangle>(
				Vec3(0.f, 0.f, -0.3f), Vec3(1280.f, 720.f, 0.f), Vec4(0.f, 0.f, 0.f, 0.6f));

			//light->SetFade(0.2f);
			//light->SetThickness(0.5f);

			lights.push_back(light);
			lights.push_back(lightMask);
		}

		// Set shapes
		{

			for (int i = 0; i < 25; ++i)
			{
				float y = (i >= 10) ? (i >= 20) ? 256.f : 192.f : 128.f;

				shared_ptr<Rectangle> rect = make_shared<Rectangle>(
					Vec3((float)(i % 10) * 64.f, y, 0.f), Vec3(64.f, 64.f, 0.f), randomColour());
				shapes.push_back(rect);
			}
		}

		ZVK::SpritePipeline spritePipeline(sPath + "SpriteVert.spv", sPath + "SpriteFrag.spv");
		ZVK::ShapePipeline lightPipeline(sPath + "ShapeVert.spv", sPath + "ShapeFrag.spv");

		ZVK::Camera cam(1280, 720, true);

		ZVK::Renderer renderer;
		ZVK::SpriteRenderer spriteRenderer(renderer, &spritePipeline, "resources/textures/error.png");
		ZVK::ShapeRenderer shapeRenderer(renderer, &lightPipeline);

		sprites[0]->SetRotationZ(ToRadians(35.f));
		sprites[1]->SetScaleX(-1.f);

		float shapeRotation = 0.f;
		while (!core.GetWindow().ShouldClose())
		{
			glfwPollEvents();

			cam.Update();

			for (auto rect : shapes)
				rect->SetRotation(shapeRotation);

			// Make the light follow the mouse cursor
			ZVK::Vec2 lightPos = ZVK::Vec2(
				cam.GetMousePos().x - (lights[0]->GetWidth() / 2.f),
				cam.GetMousePos().y - (lights[0]->GetHeight() / 2.f));

			/* ---- Start Drawing ---- */

			renderer.Begin();

			// Sprites, Camera/MVP, CanUpdateVertices
			spriteRenderer.Draw(sprites, cam.GetPV(), false);

			// Sprites, Camera/MVP, CanUpdateVertices
			shapeRenderer.Draw(shapes, cam, true);
			shapeRenderer.Draw(lights, cam.GetPV());

			renderer.End();


			lights[0]->SetPos(Vec3(lightPos, -0.5f));

			shapeRotation += ToRadians(0.1f);
		}
	}
	catch (std::exception e)
	{
		std::cout << "-------------\n" << e.what() << "\n-------------\n";
	}

	return 0;
}

ZVK::Vec4 randomColour()
{
	return { (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1.f };
}