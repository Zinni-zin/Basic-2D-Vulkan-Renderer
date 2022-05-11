#include "Headers/Sprite.h"
#include <iostream>

#include <cassert>

namespace ZVK
{
	/* ---- Constructors ---- */

	Sprite::Sprite(std::shared_ptr<Texture2D> texture, Vec4 colour,
		Vec2ui subPos, Vec2ui subDimensions)
		: IDrawableObject(Vec3(0.f, 0.f, 0.f), Vec3(32.f, 32.f, 0.f), colour),
		p_texture(texture), m_subPos(subPos), m_subDimensions(subDimensions),
		m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{
		if (subDimensions.x > 0 && subDimensions.y > 0)
		{
			SetSubPos(subPos);
			SetSubDimensions(subDimensions);
			setUVInfo();
		}
	}

	Sprite::Sprite(Vec3 pos, Vec3 dimensions, std::shared_ptr<Texture2D> texture, 
		Vec4 colour, Vec2ui subPos, Vec2ui subDimensions)
		: IDrawableObject(Vec3(pos), Vec3(dimensions), colour), 
		p_texture(texture), m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{
		if (subDimensions.x > 0 && subDimensions.y > 0)
		{
			SetSubPos(subPos);
			SetSubDimensions(subDimensions);
			setUVInfo();
		}
	}

	// No z and depth(vec2 pos and dimension)
	Sprite::Sprite(Vec2 pos, Vec2 dimensions, std::shared_ptr<Texture2D> texture,
		Vec4 colour, Vec2ui subPos, Vec2ui subDimensions)
		: IDrawableObject(Vec3(pos.x, pos.y, 0.f), 
			Vec3(dimensions.x, dimensions.y, 0.f), colour), 
		p_texture(texture), m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{ 
		if (subDimensions.x > 0 && subDimensions.y > 0)
		{
			SetSubPos(subPos);
			SetSubDimensions(subDimensions);
			setUVInfo();
		}
	}

	// Uses the texture's width and height
	Sprite::Sprite(Vec3 pos, std::shared_ptr<Texture2D> texture, Vec4 colour, 
		Vec2ui subPos, Vec2ui subDimensions)
		: IDrawableObject(Vec3(pos), Vec3(0.f, 0.f, 0.f), colour),
		p_texture(texture), m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{
		assert(texture && "Error in sprite constructor: no width and height specified can't use the texture's width and height because the texture is null");
		
		m_dimensions.x = (float)texture->GetWidth();
		m_dimensions.y = (float)texture->GetHeight();

		if (subDimensions.x > 0 && subDimensions.y > 0)
		{
			SetSubPos(subPos);
			SetSubDimensions(subDimensions);
			setUVInfo();
		}
	}

	// No z and depth(vec2 pos and dimension) and uses the texture's width and height
	Sprite::Sprite(Vec2 pos, std::shared_ptr<Texture2D> texture, Vec4 colour, 
		Vec2ui subPos, Vec2ui subDimensions)
		: IDrawableObject(Vec3(pos, 0.f), Vec3(0.f, 0.f, 0.f), colour),
		p_texture(texture), m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{
		assert(texture && "Error in sprite constructor: no width and height specified can't use the texture's width and height because the texture is null");

		m_dimensions.x = (float)texture->GetWidth();
		m_dimensions.y = (float)texture->GetHeight();

		if (subDimensions.x > 0 && subDimensions.y > 0)
		{
			SetSubPos(subPos);
			SetSubDimensions(subDimensions);
			setUVInfo();
		}
	}

	Sprite::Sprite(float x, float y, float z, float width, float height, float depth,
		std::shared_ptr<Texture2D> texture, float r, float g, float b, float a,
		uint32_t subX, uint32_t subY, uint32_t subWidth, uint32_t subHeight)
		: IDrawableObject(Vec3(x, y, z), Vec3(width, height, depth), 
			Vec4(r, g, b, a)), p_texture(texture), m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{ 
		if (subWidth > 0 && subHeight > 0)
		{
			SetSubPos(Vec2ui(subX, subY));
			SetSubDimensions(Vec2ui(subWidth, subHeight));
			setUVInfo();
		}
	}

	// No z and depth
	Sprite::Sprite(float x, float y, float width, float height,
		std::shared_ptr<Texture2D> texture, float r, float g, float b, float a,
		uint32_t subX, uint32_t subY, uint32_t subWidth, uint32_t subHeight)
		: IDrawableObject(Vec3(x, y, 0.f), Vec3(width, height, 0.f), 
			Vec4(r, g, b, a)), p_texture(texture), m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{ 
		if (subWidth > 0 && subHeight > 0)
		{
			SetSubPos(Vec2ui(subX, subY));
			SetSubDimensions(Vec2ui(subWidth, subHeight));
			setUVInfo();
		}
	}

	// Uses the texture's width and height
	Sprite::Sprite(float x, float y, float z, std::shared_ptr<Texture2D> texture,
		float r, float g, float b, float a, uint32_t subX, uint32_t subY, 
		uint32_t subWidth, uint32_t subHeight)
		: IDrawableObject(Vec3(x, y, z), Vec3(0.f, 0.f, 0.f), Vec4(r, g, b, a)),
		p_texture(texture), m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{
		assert(texture && "Error in sprite constructor: no width and height specified can't use the texture's width and height because the texture is null");

		m_dimensions.x = (float)texture->GetWidth();
		m_dimensions.y = (float)texture->GetHeight();

		if (subWidth > 0 && subHeight > 0)
		{
			SetSubPos(Vec2ui(subX, subY));
			SetSubDimensions(Vec2ui(subWidth, subHeight));
			setUVInfo();
		}
	}

	// No z and depth and uses the texture's width and height
	Sprite::Sprite(float x, float y, std::shared_ptr<Texture2D> texture,
		float r, float g, float b, float a, uint32_t subX, uint32_t subY,
		uint32_t subWidth, uint32_t subHeight)
		: IDrawableObject(Vec3(x, y, 0.f), Vec3(0.f, 0.f, 0.f), Vec4(r, g, b, a)),
		p_texture(texture), m_uvInfo(1.f, 1.f, 0.f, 0.f)
	{
		assert(texture && "Error in sprite constructor: no width and height specified can't use the texture's width and height because the texture is null");

		m_dimensions.x = (float)texture->GetWidth();
		m_dimensions.y = (float)texture->GetHeight();

		if (subWidth > 0 && subHeight > 0)
		{
			SetSubPos(Vec2ui(subX, subY));
			SetSubDimensions(Vec2ui(subWidth, subHeight));
			setUVInfo();
		}
	}

	/* ---- Set Sub Texture Info ---- */

	void Sprite::SetSubPos(Vec2ui pos) 
	{ 
		SetSubX(pos.x);
		SetSubY(pos.y);
	}

	void Sprite::SetSubX(uint32_t subX) 
	{ 
		if (p_texture && subX + m_subDimensions.y < (uint32_t)p_texture->GetWidth())
		{
			m_subPos.x = subX;
			setUVInfo();
		}
		else
		{
			std::cout << "Error SetSubX() in sprite(" << this << "): Texture is null or " << 
				"subX + width is greater than the texture's width\n";
		}
	}

	void Sprite::SetSubY(uint32_t subY)
	{
		if (p_texture && subY + m_subDimensions.y < (uint32_t)p_texture->GetHeight())
		{
			m_subPos.y = subY;
			setUVInfo();
		}
		else
		{
			std::cout << "Error SetSubY() in sprite(" << this << "): Texture is null or " <<
				"subY + height is greater than the texture's height\n";
		}
	}
		 
	void Sprite::SetSubDimensions(Vec2ui dimensions) 
	{ 
		SetSubWidth(dimensions.x);
		SetSubHeight(dimensions.y);
	}

	void Sprite::SetSubWidth(uint32_t width)
	{ 
		if (p_texture && m_subPos.x + width < (uint32_t)p_texture->GetWidth())
		{
			m_subDimensions.x = width;
			setUVInfo();
		}
		else
		{
			std::cout << "Error SetSubWidth() in sprite(" << this << "): Texture is null or " <<
				"subX + width is greater than the texture's width\n";
		}
	}

	void Sprite::SetSubHeight(uint32_t height)
	{ 
		if (p_texture && m_subPos.y + height < (uint32_t)p_texture->GetHeight())
		{
			m_subDimensions.y = height;
			setUVInfo();
		}
		else
		{
			std::cout << "Error SetSubHeight() in sprite(" << this << "): Texture is null or " <<
				"subY + height is greater than the texture's height\n";
		}
	}

	void Sprite::setUVInfo()
	{
		if (m_subDimensions.x == 0 || m_subDimensions.y == 0) return;

		if (p_texture && m_subDimensions.x != p_texture->GetWidth())
		{
			m_uvInfo.x = (float)(m_subPos.x + m_subDimensions.x) / (float)p_texture->GetWidth();
			m_uvInfo.z = (float)m_subPos.x / (float)p_texture->GetWidth();
		}

		if (p_texture && m_subDimensions.y != p_texture->GetHeight())
		{
			m_uvInfo.y = (float)(m_subPos.y + m_subDimensions.y) / (float)p_texture->GetHeight();
			m_uvInfo.w = (float)m_subPos.y / (float)p_texture->GetHeight();
		}
	}
}
