
#include <glm/glm.hpp>
#include <glm/common.hpp>

/*

namespace saf {

	class Camera3D
	{
	public:
		Camera3D(glm::vec3 pos, glm::vec3 angles, float ar = 16.f / 9.f, float zmin = 0.1f, float zmax = 100.f)
			: m_Pos(pos), m_Angles(angles), m_AR(ar), m_ZMin(zmin), m_ZMax(zmax), m_ProjectionMatrix(glm::mat4(1.f))
		{

		}

		inline const glm::vec3& GetPos() const { return m_Pos; }
		inline const glm::vec3& GetAngles() const { return m_Angles; }
		inline const glm::vec3 GetDir() const;

		virtual void SetView(const glm::vec3& pos, const glm::vec3& angles) = 0;
		virtual void SetAngles(const glm::vec3& angles) { m_Angles = angles; }
		virtual void SetPos(const glm::vec3& pos) { m_Pos = pos; }
		virtual void MoveView(const glm::vec3& posoffs, const glm::vec3& angoffs) = 0;
		virtual void MoveLocalView(const glm::vec3& localoffs, const glm::vec3& angoffs) = 0;
		virtual const glm::mat4 GetViewMatrix() const = 0;
		virtual const glm::mat4 GetViewMatrixInv() const = 0;

		inline const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		inline glm::mat4 GetVP() const { return m_ProjectionMatrix * GetViewMatrix(); }
	protected:
		float m_AR;
		float m_ZMin, m_ZMax;

		glm::vec3 m_Angles;
		//glm::vec3 m_Forward, m_Side, m_Up;

		glm::vec3 m_Pos;

		glm::mat4 m_ProjectionMatrix;
	};

	class PerspectiveCamera : public Camera3D
	{
	public:
		PerspectiveCamera(glm::vec3 pos, glm::vec3 angles, float fov = glm::half_pi<float>(), float ar = 16.f / 9.f, float znear = 0.1f, float zfar = 1000.f);
		PerspectiveCamera(const PerspectiveCamera& camera);

		virtual void SetView(const glm::vec3& pos, const glm::vec3& angles) override;
		virtual void MoveView(const glm::vec3& posoffs, const glm::vec3& angoffs) override;
		virtual void MoveLocalView(const glm::vec3& localoffs, const glm::vec3& angoffs) override;
		virtual const glm::mat4 GetViewMatrix() const override;
		virtual const glm::mat4 GetViewMatrixInv() const override;

		void SetZoom(float zoom);

		friend class PerspectiveCameraController;
	private:
		float m_FOV;
		float m_Zoom = 1.f;
	};

	class OrthographicCamera : public Camera3D
	{
	public:
		OrthographicCamera(glm::vec3 pos, glm::vec3 angles, float xmin = -50.f, float xmax = 50.f, float ymin = -50.f, float ymax = 50.f, float zmin = -50.f, float zmax = 20.f);

		virtual void SetView(const glm::vec3& pos, const glm::vec3& angles) override;
		virtual void MoveView(const glm::vec3& posoffs, const glm::vec3& angoffs) override;
		virtual void MoveLocalView(const glm::vec3& localoffs, const glm::vec3& angoffs) override;
		virtual const glm::mat4 GetViewMatrix() const override;
		virtual const glm::mat4 GetViewMatrixInv() const override;
	private:
		float m_XMin, m_XMax;
		float m_YMin, m_YMax;
	};

}

*/