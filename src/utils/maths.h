
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/*
namespace saf::maths {

	inline glm::mat4 ScreenInverse(const unsigned int& width, const unsigned int& height)
	{
		float w = (float)width;
		float h = (float)height;

		glm::mat4 screen(1.f);
		screen[0] = glm::vec4(2.f / w, 0.f, 0.f, 0.f);
		screen[1] = glm::vec4(0.f, 2.f / h, 0.f, 0.f);
		screen[3] = glm::vec4(-1.f, -1.f, 0.f, 1.f);
		return screen;
	}

	inline void AngleVectors(const glm::vec3& angles, glm::vec3* forward = nullptr, glm::vec3* side = nullptr, glm::vec3* up = nullptr)
	{
		float cospitch = cosf(angles.x);
		float sinpitch = sinf(angles.x);
		float cosyaw = cosf(angles.y);
		float sinyaw = sinf(angles.y);

		if (forward)*forward = glm::vec3(cosyaw * cospitch, sinyaw * cospitch, -sinpitch);
		if (side)*side = glm::vec3(sinyaw, -cosyaw, 0.f);
		if (up)*up = glm::vec3(cosyaw * sinpitch, sinyaw * sinpitch, cospitch);
	}

	inline void NormalizeAngles(glm::vec3& angles)
	{

		while (angles.x > glm::pi<float>()) angles.x -= glm::two_pi<float>(); //pitch
		while (angles.x <= -glm::pi<float>()) angles.x += glm::two_pi<float>();

		while (angles.y > glm::pi<float>()) angles.y -= glm::two_pi<float>();
		while (angles.y <= -glm::pi<float>()) angles.y += glm::two_pi<float>(); //yaw

		while (angles.z > glm::pi<float>()) angles.z -= glm::two_pi<float>();
		while (angles.z <= -glm::pi<float>()) angles.z += glm::two_pi<float>(); //roll
	}

	//static glm::mat4 FPViewMatrix(const glm::vec3& pos, const glm::vec3& angles, glm::vec3* forward = nullptr, glm::vec3* side = nullptr, glm::vec3* up = nullptr)
	//{
	//	glm::vec3 f, s, u;
	//
	//	AngleVectors(angles, &f, &s, &u);
	//
	//	if (forward)* forward = f;
	//	if (side)* side = s;
	//	if (up)* up = u;
	//
	//	glm::mat4 Result(1.f);
	//	Result[0][0] = s.x;
	//	Result[1][0] = s.y;
	//	Result[2][0] = s.z;
	//	Result[3][0] = -glm::dot(s, pos);
	//	Result[0][1] = u.x;
	//	Result[1][1] = u.y;
	//	Result[2][1] = u.z;
	//	Result[3][1] = -glm::dot(u, pos);
	//	Result[0][2] = -f.x;
	//	Result[1][2] = -f.y;
	//	Result[2][2] = -f.z;
	//	Result[3][2] = glm::dot(f, pos);
	//
	//	return Result;
	//}

	/*

#define space glm::mat4(glm::vec4(0.f, 0.f, -1.f, 0.f), glm::vec4(-1.f, 0.f, 0.f, 0.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec4(0.f, 0.f, 0.f, 1.f))

	inline glm::mat4 getCameraRotationInv(const glm::vec3& angles) { return mat4_cast(glm::angleAxis(angles.x, glm::vec3(0.f, 1.f, 0.f)) * glm::angleAxis(angles.z, glm::vec3(1.f, 0.f, 0.f)) * glm::angleAxis(angles.y, glm::vec3(0.f, 0.f, 1.f))); }
	inline glm::mat4 getCameraRotation(const glm::vec3& angles) { return mat4_cast(glm::angleAxis(angles.y, glm::vec3(0.f, 0.f, 1.f)) * glm::angleAxis(angles.z, glm::vec3(1.f, 0.f, 0.f)) * glm::angleAxis(angles.x, glm::vec3(0.f, 1.f, 0.f))); }

	inline glm::mat4 FPViewMatrix(const glm::vec3& pos, const glm::vec3& angles, glm::vec3* forward = nullptr, glm::vec3* side = nullptr, glm::vec3* up = nullptr)
	{
		using namespace glm;
		return space * getCameraRotationInv(-angles) * translate(mat4(1.f), -pos);
	}

	inline glm::mat4 FPViewMatrixInv(const glm::vec3& pos, const glm::vec3& angles, glm::vec3* forward = nullptr, glm::vec3* side = nullptr, glm::vec3* up = nullptr)
	{
		using namespace glm;
		return translate(mat4(1.f), pos) * getCameraRotation(angles) * glm::inverse(space);
	}

}

*/