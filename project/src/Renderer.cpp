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

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(static_cast<float>(m_Width / m_Height), 60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
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



	//vertices in world space
	std::vector<Vertex> triangle
	{
		{ {0.f, 4.f, 2.f}, {1,0,0} },
		{ {3.f, -2.f, 2.f}, {0,1,0} },
		{ {-3.f, -2.f, 2.f}, {0,0,1} }
	};

	//for each triangle: 
	//World -> NDC
	std::vector<Vertex_Out> triangleNDC{};
		VertexTransformationFunction(triangle, triangleNDC);

	//convert each NDC coordinates to screen space / raster space
	std::vector<Vector2> vertices_screenSpace{};
	for (auto const& vertex : triangleNDC)
	{
		float const x_screen{ (vertex.position.x + 1) * 0.5f * m_Width };
		float const y_screen{ (1 - vertex.position.y) * 0.5f * m_Height };

		vertices_screenSpace.emplace_back(Vector2{ x_screen, y_screen });
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
				const Vector2 vert0{ vertices_screenSpace[0] };
				const Vector2 vert1{ vertices_screenSpace[1] };
				const Vector2 vert2{ vertices_screenSpace[2] };

				float weight0, weight1, weight2;
				weight0 = Vector2::Cross((pixel - vert1), (vert1 - vert2));
				weight1 = Vector2::Cross((pixel - vert2), (vert2 - vert0));
				weight2 = Vector2::Cross((pixel - vert0), (vert0 - vert1));
				// divide by total triangle area
				const float totalTriangleArea{ Vector2::Cross(vert1 - vert0,vert2 - vert0) };
				const float invTotalTriangleArea{ 1 / totalTriangleArea };
				weight0 *= invTotalTriangleArea;
				weight1 *= invTotalTriangleArea;
				weight2 *= invTotalTriangleArea;

				const float depth0{ triangleNDC[0].position.z };
				const float depth1{ triangleNDC[1].position.z };
				const float depth2{ triangleNDC[2].position.z };
				const float interpolatedDepth{ 1.f / (weight0 * (1.f / depth0) + weight1 * (1.f / depth1) + weight2 * (1.f / depth2)) };

				const float r = weight0 * triangleNDC[0].color.r + weight1 * triangleNDC[1].color.r + weight2 * triangleNDC[2].color.r;
				const float g = weight0 * triangleNDC[0].color.g + weight1 * triangleNDC[1].color.g + weight2 * triangleNDC[2].color.g;
				const float b = weight0 * triangleNDC[0].color.b + weight1 * triangleNDC[1].color.b + weight2 * triangleNDC[2].color.b;

				finalColor = { r, g, b };
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

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const
{
	//projection stage:

	//mesh .worldmatrix * 
	//model -> world space todo later

	//world -> view space 
	auto const m{ m_Camera.viewMatrix * m_Camera.projectionMatrix };
	
	vertices_out.reserve(vertices_in.size());
	for(auto const& v : vertices_in)
	{
		Vertex_Out vOut{};
		vOut.color = v.color;

		vOut.position = m.TransformPoint({ v.position, 1.f });

		//view -> clipping space (NDC)
		float const inverseWComponent{ 1.f / vOut.position.w };
		vOut.position.x *= inverseWComponent;
		vOut.position.y *= inverseWComponent;
		vOut.position.z *= inverseWComponent;
	
		vertices_out.emplace_back(vOut);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
