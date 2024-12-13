#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	struct Vertex_Out;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void VertexTransformationFunction(Mesh& mesh) const;

	#pragma region Settings
		void ToggleBoundingBoxes() noexcept
		{
			m_ShowBoundingBoxes = !m_ShowBoundingBoxes;
		}

		void ToggleDepthBuffer() noexcept
		{
			m_ShowDepthBuffer = !m_ShowDepthBuffer;
		}

		void ToggleRotation() noexcept
		{
			m_IsRotating = !m_IsRotating;
		}

		void ToggleNormalMapping() noexcept
		{
			m_UseNormalMapping = !m_UseNormalMapping;
		}

		void CycleShadingMode() noexcept
		{
			auto curr{ static_cast<uint8_t>(m_CurrShadingMode) };
			++curr %= static_cast<uint8_t>(ShadingMode::Count);

			m_CurrShadingMode = static_cast<ShadingMode>(curr);
		}
	#pragma endregion

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		//Settings variables
		bool m_ShowBoundingBoxes{ false };

		bool m_ShowDepthBuffer{ false };
		bool m_IsRotating{ true };
		bool m_UseNormalMapping{ true };


		enum class ShadingMode : uint8_t
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined,
			Count
		};
		ShadingMode m_CurrShadingMode{ ShadingMode::Combined };

		std::vector<Mesh> m_Meshes;

		void RenderTriangle(Mesh const& m, std::vector<Vector2> const& vertices, uint32_t startVertex, bool swapVertex);

		ColorRGB PixelShading(Mesh const& m, Vertex_Out const& v);
		float DepthRemap(float v, float min, float max);
	};
}
