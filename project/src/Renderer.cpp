//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Renderer.h"
#include "DataTypes.h"
#include "BRDF.h"
#include "Texture.h"
#include "Utils.h"

#include <iostream>

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
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);

	//Initialize Camera
	m_Camera.Initialize(45.f, { .0f,5.f,-64.f }, static_cast<float>(m_Width / m_Height));

	//Initialize the vehicle mesh
	Mesh m{};
	//parse the OBJ to load all required data
	Utils::ParseOBJ("resources/vehicle.obj", m.vertices, m.indices);

	//set vehicle textures - should be done through texture manager in bigger project to avoid copies and just maintain a reference the mesh
	m.pDiffuse = std::make_shared<Texture>("resources/vehicle_diffuse.png");
	m.pNormal = std::make_shared<Texture>("resources/vehicle_normal.png");
	m.pSpecular = std::make_shared<Texture>("resources/vehicle_specular.png");
	m.pGloss = std::make_shared<Texture>("resources/vehicle_gloss.png");

	m.primitiveTopology = PrimitiveTopology::TriangleList;

	m.Translate({ 0.f, 0.f, 0.f });
	m_Meshes.push_back(m);
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
	if (m_IsRotating)
	{
		for (auto& m : m_Meshes)
		{
			float constexpr rotation{ 0.05f };
			m.RotateY(rotation);
		}
	}
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	std::fill_n(m_pBackBufferPixels, m_Width * m_Height, 0);

	//clear the background
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//Meshes defined in world space
	//World -> NDC
	for (auto& m : m_Meshes)
	{
		VertexTransformationFunction(m);

		//convert each NDC coordinates to screen space / raster space
		std::vector<Vector2> vertices_screenSpace{};
		for (auto const& vertex : m.vertices_out)
		{
			float const x_screen{ (vertex.position.x + 1) * 0.5f * m_Width };
			float const y_screen{ (1 - vertex.position.y) * 0.5f * m_Height };

			vertices_screenSpace.emplace_back(Vector2{ x_screen, y_screen });
		}

		switch (m.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			for (uint32_t v{ 0 }; v < m.indices.size(); v += 3)
				RenderTriangle(m, vertices_screenSpace, v, false);
			break;
		case PrimitiveTopology::TriangleStrip:
			for (uint32_t v{ 0 }; v < m.indices.size() - 2; ++v)
				RenderTriangle(m, vertices_screenSpace, v, v % 2);
			break;
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(Mesh& mesh) const
{
	//projection stage:
	//model -> world space -> world -> view space 
	auto const m{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
	mesh.vertices_out.clear();
	mesh.vertices_out.reserve(mesh.vertices.size());
	for(auto const& v : mesh.vertices)
	{
		Vertex_Out vOut{};
		vOut.color = v.color;
		vOut.uv = v.uv;

		vOut.position = m.TransformPoint(v.position.ToPoint4() );

		vOut.normal = mesh.worldMatrix.TransformVector(v.normal);
		vOut.tangent = mesh.worldMatrix.TransformVector(v.tangent);
		vOut.viewDirection = vOut.position - m_Camera.origin.ToPoint4();

		//view -> clipping space (NDC)
		float const inverseWComponent{ 1.f / vOut.position.w };
		vOut.position.x *= inverseWComponent;
		vOut.position.y *= inverseWComponent;
		vOut.position.z *= inverseWComponent;
		
		mesh.vertices_out.emplace_back(vOut);
	}
}

void dae::Renderer::RenderTriangle(Mesh const& m, std::vector<Vector2> const& vertices, uint32_t startVertex, bool swapVertex)
{
	//Rasterization stage
	const size_t idx1{ m.indices[startVertex + (2 * swapVertex)] };
	const size_t idx2{ m.indices[startVertex + 1] };
	const size_t idx3{ m.indices[startVertex + (!swapVertex * 2)] };

	if (idx1 == idx2 || idx2 == idx3 || idx3 == idx1)
	{
		return;
	}

	if (m.vertices_out[idx1].position.x < -1.f || m.vertices_out[idx1].position.x > 1.f || m.vertices_out[idx1].position.y < -1.f || m.vertices_out[idx1].position.y > 1.f)
	{
		return;
	}
	if (m.vertices_out[idx2].position.x < -1.f || m.vertices_out[idx2].position.x > 1.f || m.vertices_out[idx2].position.y < -1.f || m.vertices_out[idx2].position.y > 1.f)
	{
		return;
	}
	if (m.vertices_out[idx3].position.x < -1.f || m.vertices_out[idx3].position.x > 1.f || m.vertices_out[idx3].position.y < -1.f || m.vertices_out[idx3].position.y > 1.f)
	{
		return;
	}

	const Vector2 vert0{ vertices[idx1] };
	const Vector2 vert1{ vertices[idx2] };
	const Vector2 vert2{ vertices[idx3] };
	//Bounding boxes logic - only loop over pixels within the smallest possible bounding box
	Vector2 topLeft{ Vector2::Min(vert0,Vector2::Min(vert1,vert2)) - Vector2{1.f, 1.f} };
	Vector2 topRight{ Vector2::Max(vert0,Vector2::Max(vert1,vert2)) + Vector2{1.f, 1.f} };


		topLeft.x = Clamp(topLeft.x, 0.f, static_cast<float>(m_Width));
		topLeft.y = Clamp(topLeft.y, 0.f, static_cast<float>(m_Height));
		topRight.x = Clamp(topRight.x, 0.f, static_cast<float>(m_Width));
		topRight.y = Clamp(topRight.y, 0.f, static_cast<float>(m_Height));

	for (int px{ static_cast<int>(topLeft.x) }; px < static_cast<int>(topRight.x); ++px)
	{
		for (int py{ static_cast<int>(topLeft.y) }; py < static_cast<int>(topRight.y); ++py)
		{
			ColorRGB finalColor{ 1.f, 1.f, 1.f };

			if (m_ShowBoundingBoxes)
			{
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));

				continue;
			}

			Vector2 const pixel{ px + .5f, py + .5f };

			//Calculate barycentric coordinates
			float weight0{ Vector2::Cross((pixel - vert1), (vert1 - vert2)) };
			float weight1{ Vector2::Cross((pixel - vert2), (vert2 - vert0)) };
			float weight2{ Vector2::Cross((pixel - vert0), (vert0 - vert1)) };

			// not in triangle
			if (weight0 < 0.f || weight1 < 0.f || weight2 < 0.f)
				continue;

			// divide by total triangle area && normalize
			float const totalTriangleArea{ Vector2::Cross(vert1 - vert0,vert2 - vert0) };
			float const invTotalTriangleArea{ 1 / totalTriangleArea };
			weight0 *= invTotalTriangleArea;
			weight1 *= invTotalTriangleArea;
			weight2 *= invTotalTriangleArea;

			float const depth0{ m.vertices_out[idx1].position.z };
			float const depth1{ m.vertices_out[idx2].position.z };
			float const depth2{ m.vertices_out[idx3].position.z };
			float const interpolatedDepth{ 1.f / (weight0 * (1.f / depth0) + weight1 * (1.f / depth1) + weight2 * (1.f / depth2)) };

			if (interpolatedDepth < 0.f || interpolatedDepth > 1.f || m_pDepthBufferPixels[px + py * m_Width] < interpolatedDepth)
			{
				continue;
			}
			m_pDepthBufferPixels[px + py * m_Width] = interpolatedDepth;

			//float const remap{ DepthRemap(interpolatedDepth, 0.9975f, 1.0f) };
			//finalColor = ColorRGB(remap, remap, remap);

			//Update Color in Buffer
			Vertex_Out pixelToShade{};
			pixelToShade.position = { float(px), float(py), interpolatedDepth,interpolatedDepth };

			const float r = weight0 * m.vertices_out[idx1].color.r + weight1 * m.vertices_out[idx2].color.r + weight2 * m.vertices_out[idx3].color.r;
			const float g = weight0 * m.vertices_out[idx1].color.g + weight1 * m.vertices_out[idx2].color.g + weight2 * m.vertices_out[idx3].color.g;
			const float b = weight0 * m.vertices_out[idx1].color.b + weight1 * m.vertices_out[idx2].color.b + weight2 * m.vertices_out[idx3].color.b;
			
			finalColor = { r, g, b };
			pixelToShade.color = finalColor;

			pixelToShade.uv = interpolatedDepth * ((weight0 * m.vertices[idx1].uv) / depth0
				+ (weight1 * m.vertices[idx2].uv) / depth1
				+ (weight2 * m.vertices[idx3].uv) / depth2);
			pixelToShade.normal = Vector3{ interpolatedDepth * (weight0 * m.vertices_out[idx1].normal / m.vertices_out[idx1].position.w
															  + weight1 * m.vertices_out[idx2].normal / m.vertices_out[idx2].position.w +
																weight2 * m.vertices_out[idx3].normal / m.vertices_out[idx3].position.w) } / 3;
			pixelToShade.normal.Normalize();
			pixelToShade.tangent = Vector3{ interpolatedDepth * (weight0 * m.vertices_out[idx1].tangent / m.vertices_out[idx1].position.w +
																 weight1 * m.vertices_out[idx2].tangent / m.vertices_out[idx2].position.w +
																 weight2 * m.vertices_out[idx3].tangent / m.vertices_out[idx3].position.w) } / 3;
			pixelToShade.tangent.Normalize();
			pixelToShade.viewDirection = Vector3{ interpolatedDepth * (weight0 * m.vertices_out[idx1].viewDirection / m.vertices_out[idx1].position.w +
																	   weight1 * m.vertices_out[idx2].viewDirection / m.vertices_out[idx2].position.w +
																	   weight2 * m.vertices_out[idx3].viewDirection / m.vertices_out[idx3].position.w) } / 3;
			pixelToShade.viewDirection.Normalize();
			finalColor = PixelShading(m, pixelToShade);

			//Update Color in Buffer
			finalColor.MaxToOne();
			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

ColorRGB dae::Renderer::PixelShading(Mesh const& m, Vertex_Out const& v)
{
	//Global light

	Vector3 const lightDirection{ .577f, -.577f, .577f };
	ColorRGB constexpr ambientColor{ 0.03f, 0.03f, 0.03f };

	ColorRGB result{ v.color };

	float constexpr shininess{ 25.0f };
	float constexpr KD{ 7.f };

	// Normal map
	float observedArea{};

	const Vector3 biNormal = Vector3::Cross(v.normal, v.tangent);
	const Matrix tangentSpaceAxis = { v.tangent, biNormal, v.normal, Vector3::Zero };

	const ColorRGB normalColor = m.pNormal->Sample(v.uv);
	Vector3 sampledNormal = { normalColor.r, normalColor.g, normalColor.b }; // => range [0, 1]
	sampledNormal = 2.f * sampledNormal - Vector3{ 1, 1, 1 }; // => [0, 1] to [-1, 1]
	sampledNormal = tangentSpaceAxis.TransformVector(sampledNormal).Normalized();


	Vector3 normal{ v.normal };

	//calculate normal
	if(m_UseNormalMapping)
	{
		observedArea = Utils::CalculateObservedArea(sampledNormal, lightDirection);
	}
	else
	{
		observedArea = Utils::CalculateObservedArea(normal, lightDirection);
	}
	if (!m_ShowDepthBuffer)
	{
		switch (m_CurrShadingMode)
		{
		case ShadingMode::ObservedArea:
		{
			result = ColorRGB(observedArea, observedArea, observedArea);
			break;
		}
		case ShadingMode::Diffuse:
		{
			auto const lambert{ BRDF::Lambert(KD, m.pDiffuse->Sample(v.uv)) };
			result = lambert * observedArea;
			break;
		}
		case ShadingMode::Specular:
		{
			const ColorRGB specularColor{ m.pSpecular->Sample(v.uv) };
			const float phongExp{ m.pGloss->Sample(v.uv).r * shininess };

			const Vector3 reflect{ Vector3::Reflect(-lightDirection, sampledNormal) };
			float cosAngle{ Vector3::Dot(reflect, v.viewDirection) };
			if (cosAngle < 0.f) cosAngle = 0.f;

			const float specReflection{ 1.f *powf(cosAngle, phongExp) };
			const ColorRGB phong{ specReflection * specularColor };

			result = phong * observedArea;
			break;
		}
		case ShadingMode::Combined:
		{
			auto const lambert{ BRDF::Lambert(KD, m.pDiffuse->Sample(v.uv)) };

			const ColorRGB specularColor{ m.pSpecular->Sample(v.uv) };
			const float phongExp{ m.pGloss->Sample(v.uv).r * shininess };

			const Vector3 reflect{ Vector3::Reflect(-lightDirection, sampledNormal) };
			float cosAngle{ Vector3::Dot(reflect, v.viewDirection) };
			if (cosAngle < 0.f) cosAngle = 0.f;

			const float specReflection{ 1.f * powf(cosAngle, phongExp) };
			const ColorRGB phong{ specReflection * specularColor };

			result =  observedArea * lambert + phong;
			break;
		}
		}

	}
	result += ambientColor;
	return result;
}

float dae::Renderer::DepthRemap(float v, float min, float max)
{
	float const normalizedValue{ (v - min) / (max - min) };
	if (normalizedValue < 0.0f)
		return 0.f;

	return normalizedValue;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
