#pragma once
#include "Maths.h"
#include "Texture.h"
#include "vector"
#include <memory>

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		ColorRGB color{colors::White};
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};

		Vertex() = default;
		Vertex(Vector3 const& p, ColorRGB const& c = {}) :
		position{p},
		color{c}
		{}
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};

		Vertex_Out() = default;
		Vertex_Out(Vector4 const& p) :
			position{ p } {}
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	struct Mesh
	{
		//textures
		std::shared_ptr<Texture> pGloss{ nullptr };
		std::shared_ptr<Texture> pDiffuse{ nullptr };
		std::shared_ptr<Texture> pNormal{ nullptr };
		std::shared_ptr<Texture> pSpecular{ nullptr };

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

		std::vector<Vertex_Out> vertices_out{};
		Matrix worldMatrix{};

		Mesh() = default;
		~Mesh() = default;

		//translate the worldMatrix
		void Translate(Vector3 const& t) noexcept
		{
			worldMatrix = Matrix::CreateTranslation(t.x, t.y, t.z) * worldMatrix;
		}

		void RotateY(float r)
		{
			worldMatrix = worldMatrix * Matrix::CreateRotationY(r);
		}
	};
}
