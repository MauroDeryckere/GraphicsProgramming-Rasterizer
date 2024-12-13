//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Renderer.h"
#include "DataTypes.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(static_cast<float>(m_Width / m_Height), 60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);


	//for each triange: 

	//vertices in world space
	//clockwise order!
	std::vector<Vertex> triangle
	{
		Vector3{0.f, 4.f, 2.f},
		Vector3{3.f, -2.f, 2.f },
		Vector3{ -3.f, -2.f, 2.f }
	};

	std::vector<Vertex> triangleNDC{};
	 VertexTransformationFunction(triangle, triangleNDC);

	//convert each NDC coordinates to screen space / raster space
	std::vector<Vector2> vertices_screenSpace{};
	for (auto const& vertex : triangleNDC)
	{
		float const x_screen{ (vertex.position.x + 1) * 0.5f * m_Width };
		float const y_screen{ (1 - vertex.position.y) * 0.5f * m_Height };

		vertices_screenSpace.emplace_back(x_screen, y_screen);
	}

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{};

			//rasterization stage: 
			Vector2 const pixel{ px + .5f, py + .5f };
			if(Utils::IsPixelInTriangle(pixel, vertices_screenSpace))
			{
				finalColor = { 1.f, 1.f, 1.f };
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//projection stage:

	//mesh .worldmatrix * 
	//model -> world space todo later

	//world -> view space 
	auto m = m_Camera.viewMatrix * m_Camera.projectionMatrix;
	
	vertices_out.resize(3);

	for(uint32_t i = 0; i < vertices_in.size(); ++i)
	{

		vertices_out[i].position = m.TransformPoint({ vertices_in[i].position, 1.f });

		//view -> clipping space (NDC)
		float const inverseWComponent{ 1.f / vertices_out[i].position.z };
		vertices_out[i].position.x *= inverseWComponent;
		vertices_out[i].position.y *= inverseWComponent;
		vertices_out[i].position.z *= inverseWComponent;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
