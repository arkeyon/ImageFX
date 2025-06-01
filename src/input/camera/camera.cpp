#include "safpch.h"
#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include "utils/maths.h"

/*

namespace saf {

	inline const glm::vec3 Camera3D::GetDir() const
	{
		glm::vec3 forward;
		maths::AngleVectors(m_Angles, &forward);
		return forward;
	}

	PerspectiveCamera::PerspectiveCamera(glm::vec3 pos, glm::vec3 angles, float fov, float ar, float znear, float zfar)
		: Camera3D(pos, angles, ar, znear, zfar), m_FOV(fov), m_Zoom(1.f)
	{
		m_ProjectionMatrix = glm::perspective(m_FOV, m_AR, m_ZMin, m_ZMax);
	}

	PerspectiveCamera::PerspectiveCamera(const PerspectiveCamera& camera)
		: Camera3D(camera.m_Pos, camera.m_Angles, camera.m_AR, camera.m_ZMin, camera.m_ZMax), m_FOV(camera.m_FOV), m_Zoom(camera.m_Zoom)
	{
		m_ProjectionMatrix = glm::perspective(m_FOV, m_AR, m_ZMin, m_ZMax);
	}

	void PerspectiveCamera::SetView(const glm::vec3& pos, const glm::vec3& angles)
	{
		m_Pos = pos;
		m_Angles = angles;
		maths::NormalizeAngles(m_Angles);
	}

	void PerspectiveCamera::MoveView(const glm::vec3& posoffs, const glm::vec3& angoffs)
	{
		m_Pos += posoffs;
		m_Angles += angoffs;
		maths::NormalizeAngles(m_Angles);
	}

	void PerspectiveCamera::MoveLocalView(const glm::vec3& localoffs, const glm::vec3& angoffs)
	{
		m_Angles += angoffs;
		maths::NormalizeAngles(m_Angles);

		glm::vec3 forward, side, up;

		maths::AngleVectors(m_Angles, &forward, &side, &up);

		m_Pos += localoffs.z * forward + localoffs.x * side + localoffs.y * up;
	}

	void PerspectiveCamera::SetZoom(float zoom)
	{
		m_Zoom = zoom;
		m_ProjectionMatrix = glm::perspective(m_FOV / m_Zoom, m_AR, m_ZMin, m_ZMax);
	}

	const glm::mat4 PerspectiveCamera::GetViewMatrix() const
	{
		return maths::FPViewMatrix(m_Pos, m_Angles);
	}

	const glm::mat4 PerspectiveCamera::GetViewMatrixInv() const
	{
		return maths::FPViewMatrixInv(m_Pos, m_Angles);
	}

	OrthographicCamera::OrthographicCamera(glm::vec3 pos, glm::vec3 angles, float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
		: Camera3D(pos, angles, 1.f, zmin, zmax), m_XMin(xmin), m_XMax(xmax), m_YMin(ymin), m_YMax(ymax)
	{
		m_ProjectionMatrix = glm::ortho(m_XMin, m_XMax, m_YMin, m_YMax, m_ZMin, m_ZMax);
	}

	void OrthographicCamera::SetView(const glm::vec3& pos, const glm::vec3& angles)
	{
		m_Pos = pos;
		m_Angles = angles;
		maths::NormalizeAngles(m_Angles);
	}

	void OrthographicCamera::MoveView(const glm::vec3& posoffs, const glm::vec3& angoffs)
	{
		m_Pos += posoffs;
		m_Angles += angoffs;
		maths::NormalizeAngles(m_Angles);
	}

	void OrthographicCamera::MoveLocalView(const glm::vec3& localoffs, const glm::vec3& angoffs)
	{
		m_Angles += angoffs;
		glm::vec3 forward, side, up;
		maths::AngleVectors(m_Angles, &forward, &side, &up);

		m_Pos += localoffs.z * forward + localoffs.x * side + localoffs.y * up;
	}

	const glm::mat4 OrthographicCamera::GetViewMatrix() const
	{
		return glm::mat4(1.f);
	}

	const glm::mat4 OrthographicCamera::GetViewMatrixInv() const
	{
		return glm::mat4(1.f);
	}

}

*/