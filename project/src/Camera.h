#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{ }


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float aspectRatio{};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		float movementSpeed{ 3.f };
		float rotationSpeed{ .02f };

		float nearPlane{ 0.1f };
		float farPlane{ 100.f };

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 const& _origin = {0.f,0.f,0.f}, float _aspectRatio = 19.f / 6.f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			aspectRatio = _aspectRatio;

			CalculateProjectionMatrix();
		}

	#pragma region CameraSettings
		void UpdateCameraSettings(float _fovAngle = 90.f, Vector3 const& _origin = { 0.f, 0.f, 0.f }, float _aspectRatio = 19.f / 6.f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			aspectRatio = _aspectRatio;

			CalculateProjectionMatrix();
		}

		void UpdateFov(float angle)
		{
			fovAngle = angle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);
			CalculateProjectionMatrix();
		}
		void UpdateAspectRatio(float ratio)
		{
			aspectRatio = ratio;
			CalculateProjectionMatrix();
		}
		void UpdateOrigin(Vector3 const& pos)
		{
			origin = pos;
		}
	#pragma endregion

		void CalculateViewMatrix()
		{
			right = Vector3::Cross(Vector3::UnitY, forward);
			up = Vector3::Cross(forward, right);

			invViewMatrix = { right, up, forward, origin };
			viewMatrix = Matrix::Inverse(invViewMatrix);


			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			float const deltaTime{ pTimer->GetElapsed() };

			//Keyboard Input
			uint8_t const* pKeyboardState{ SDL_GetKeyboardState(nullptr) };
			
			Vector3 movementDir{ };

			if (pKeyboardState[SDL_SCANCODE_W])
			{
				movementDir += forward;
			}
			if (pKeyboardState[SDL_SCANCODE_S])
			{
				movementDir -= forward;
			}

			if (pKeyboardState[SDL_SCANCODE_A])
			{
				movementDir -= right;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				movementDir += right;
			}

			if (movementDir != Vector3::Zero)
			{
				movementDir.Normalize();
				origin += (movementDir * movementSpeed * deltaTime);
			}

			//Mouse Input
			int mouseX{};
			int mouseY{};
			uint32_t const mouseState{ SDL_GetRelativeMouseState(&mouseX, &mouseY) };

			if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				forward = Matrix::CreateRotationY(mouseX * rotationSpeed).TransformVector(forward);
				forward = Matrix::CreateRotationX(-mouseY * rotationSpeed).TransformVector(forward);
			}


			//Update Matrices
			CalculateViewMatrix();
		}
	};
}
