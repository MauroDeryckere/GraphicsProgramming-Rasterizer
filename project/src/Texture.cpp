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
		uint32_t const x{ static_cast<uint32_t>(uv.x * m_pSurface->w) };
		uint32_t const y{ static_cast<uint32_t>(uv.y * m_pSurface->h) };

		uint32_t const pixel{ m_pSurfacePixels[x + y * m_pSurface->w] };

		Uint8 r{};
		Uint8 g{};
		Uint8 b{};
		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

		static constexpr float normalizedFactor{ 1 / 255.f };
		return { r * normalizedFactor, g * normalizedFactor, b * normalizedFactor };
	}
}