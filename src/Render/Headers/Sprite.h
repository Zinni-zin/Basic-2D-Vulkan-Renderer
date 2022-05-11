#pragma once

#include <memory>

#include "DrawableObject.h"
#include "Texture2D.h"

namespace ZVK
{
	class Sprite : public IDrawableObject
	{
	public:
		Sprite(std::shared_ptr<Texture2D> texture = nullptr, Vec4 colour = { 1.f, 1.f, 1.f, 1.f },
			Vec2ui subPos = { 1, 1 }, Vec2ui subDimensions = { 0, 0 });

		Sprite(Vec3 pos, Vec3 dimensions, std::shared_ptr<Texture2D> texture = nullptr,
			Vec4 colour = { 1.f, 1.f, 1.f, 1.f }, 
			Vec2ui subPos = { 1, 1 }, Vec2ui subDimensions = { 0, 0 });

		// No z and depth (vec2 pos and dimension)
		Sprite(Vec2 pos, Vec2 dimensions, std::shared_ptr<Texture2D> texture = nullptr,
			Vec4 colour = { 1.f, 1.f, 1.f, 1.f }, 
			Vec2ui subPos = { 1, 1 }, Vec2ui subDimensions = { 0, 0 });
		
		// Uses the texture's width and height
		Sprite(Vec3 pos, std::shared_ptr<Texture2D> texture,
			Vec4 colour = { 1.f, 1.f, 1.f, 1.f }, 
			Vec2ui subPos = { 1, 1 }, Vec2ui subDimensions = { 0, 0 });

		// No z and depth(vec2 pos and dimension) and uses the texture's width and height
		Sprite(Vec2 pos, std::shared_ptr<Texture2D> texture,
			Vec4 colour = { 1.f, 1.f, 1.f, 1.f }, 
			Vec2ui subPos = { 1, 1 }, Vec2ui subDimensions = { 0, 0 });

		Sprite(float x, float y, float z, float width, float height, float depth,
			std::shared_ptr<Texture2D> texture = nullptr, 
			float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f, uint32_t subX = 1.f, uint32_t subY = 1.f,
			uint32_t subWidth = 0, uint32_t subHeight = 0);
		
		// No z and depth
		Sprite(float x, float y, float width, float height, 
			std::shared_ptr<Texture2D> texture = nullptr, 
			float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f, uint32_t subX = 1.f, uint32_t subY = 1.f,
			uint32_t subWidth = 0, uint32_t subHeight = 0);

		// Uses the texture's width and height
		Sprite(float x, float y, float z, std::shared_ptr<Texture2D> texture,
			float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f, uint32_t subX = 1.f, uint32_t subY = 1.f,
			uint32_t subWidth = 0, uint32_t subHeight = 0);

		// No z and depth and uses the texture's width and height
		Sprite(float x, float y, std::shared_ptr<Texture2D> texture,
			float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f,
			uint32_t subX = 1.f, uint32_t subY = 1.f, 
			uint32_t subWidth = 0, uint32_t subHeight = 0);

		inline Vec2ui GetSubPos() const { return m_subPos; }
		inline int GetSubX() const { return m_subPos.x; }
		inline int GetSubY() const { return m_subPos.y; }

		inline Vec2ui GetSubDimensions() const { return m_subDimensions; }
		inline int GetSubWidth() const { return m_subDimensions.x; }
		inline int GetSubHeight() const { return m_subDimensions.y; }

		inline Vec4 GetUVInfo() const { return m_uvInfo; }

		inline std::shared_ptr<Texture2D> GetTexture() const { return p_texture; }
		inline int GetTextureRangedID() const { return p_texture->GetRangedID(); }
		inline int GetTextureID() const { return p_texture->GetID(); }

		void SetSubPos(Vec2ui pos);
		void SetSubX(uint32_t subX);
		void SetSubY(uint32_t subY);

		void SetSubDimensions(Vec2ui dimensions);
		void SetSubWidth(uint32_t width);
		void SetSubHeight(uint32_t height);

		inline void SetTexture(std::shared_ptr<Texture2D> texture) { p_texture = texture; }

		inline bool operator==(const Sprite& rhs) { return this == &rhs; }

	private:
		
		void setUVInfo();

	private:
		Vec2ui m_subPos;
		Vec2ui m_subDimensions;

		Vec4 m_uvInfo;

		std::shared_ptr<Texture2D> p_texture;
	};
}