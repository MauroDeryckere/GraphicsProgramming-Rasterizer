#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <cassert>

namespace dae
{
	Texture::Texture(std::filesystem::path const& path)
	{
		assert(std::filesystem::exists(path));

		m_pSurface = IMG_Load(path.string().c_str());
		if(!m_pSurface)
			throw std::runtime_error("Failed to load texture from path: " + path.string());
		
		m_pSurfacePixels = reinterpret_cast<uint32_t*>(m_pSurface->pixels);
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		float const u{ std::clamp(uv.x, 0.f, 1.f) };
		float const v{ std::clamp(uv.y, 0.f, 1.f) };
		uint32_t const x{ static_cast<uint32_t>(u * m_pSurface->w) };
		uint32_t const y{ static_cast<uint32_t>(v * m_pSurface->h) };

		uint8_t r{};
		uint8_t g{};
		uint8_t b{};
		SDL_GetRGB(m_pSurfacePixels[(y * m_pSurface->w) + x], m_pSurface->format, &r, &g, &b);
		static constexpr float normalizedFactor{ 1 / 255.f };
		return { r * normalizedFactor, g * normalizedFactor, b * normalizedFactor };
	}
}