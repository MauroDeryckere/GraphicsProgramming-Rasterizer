#pragma once
#include <SDL_surface.h>
#include <filesystem>
#include <string>
#include "ColorRGB.h"

namespace dae
{
	struct Vector2;

	class Texture
	{
	public:
		Texture(std::filesystem::path const& path);
		~Texture();

		ColorRGB Sample(const Vector2& uv) const;

	private:

		SDL_Surface* m_pSurface{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };
	};
}