
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace saf {


	template <class T>
	concept valid_vertex = requires(T t)
	{
		t.pos;
		t.print();
	};

	template <class T>
	concept has_uv = requires(T t) { t.uv; };

	template <class T>
	concept has_normal = requires(T t) { t.normal; };

	template <class T>
	concept has_color = requires(T t) { t.color; };

	template <class T>
	concept has_texid = requires(T t) { t.texid; };

	template <class T, typename K> requires valid_vertex<T> && std::is_integral<K>::value
	struct Mesh
	{
		using ThisMesh = Mesh<T, K>;
		Mesh(int vertices, int indices, bool filled = true)
			: m_Vertices(vertices), m_Indices(indices)
		{
			m_VertexCount = 0U;
			m_IndexCount = 0U;

			if (filled)
			{
				m_VertexCount = m_Vertices.size();
				m_IndexCount = m_Indices.size();
			}
		}
		std::vector<T> m_Vertices;
		std::vector<K> m_Indices;
		uint32_t m_VertexCount;
		uint32_t m_IndexCount;

		inline ThisMesh& Transform(const glm::mat4 transform)
		{
			for (auto& v : m_Vertices)
			{
				if constexpr(std::is_same<decltype(v.pos), glm::vec4>()) v.pos = transform * v.pos;
				if constexpr(std::is_same<decltype(v.pos), glm::vec3>()) v.pos = glm::vec3(transform * glm::vec4(v.pos, 1.f));
				if constexpr(std::is_same<decltype(v.pos), glm::vec2>()) v.pos = glm::vec2(transform * glm::vec4(v.pos, 0.f, 1.f));
			}

			return *this;
		}

		inline ThisMesh& Insert(const ThisMesh& other, uint32_t vertexoffs = 0U, uint32_t indexoffs = 0U)
		{
			uint32_t max = m_Vertices.capacity();

			T* verts = m_Vertices.data();

			if (!pre_alloc)
			{
				if (m_Vertices.capacity() < vertexoffs + other.m_Vertices.size()) m_Vertices.resize(vertexoffs + other.m_Vertices.size());
				for (int i = 0; i < other.m_Vertices.size(); ++i)
				{
					m_Vertices[vertexoffs + i] = other.m_Vertices[i];
				}
			}
			else
			{
				if (m_Vertices.capacity() < other.m_Vertices.size()) m_Vertices.resize(other.m_Vertices.size());
				for (int i = 0; i < other.m_Vertices.size(); ++i)
				{
					m_Vertices[vertexoffs + i] = other.m_Vertices[i];
				}
			}

			uint32_t isize = m_Indices.size();
			if (!pre_alloc)
			{
				if (m_Indices.capacity() < isize + other.m_Indices.size()) m_Indices.resize(indexoffs + other.m_Indices.size());
				for (int i = 0; i < other.m_Indices.size(); ++i)
				{
					m_Indices[indexoffs + i] = vertexoffs + other.m_Indices[i];
				}
			}
			else
			{
				if (m_Indices.capacity() < other.m_Indices.size()) m_Indices.resize(other.m_Indices.size());
				for (int i = 0; i < other.m_Indices.size(); ++i)
				{
					m_Indices[indexoffs + i] = vertexoffs + other.m_Indices[i];
				}
			}

			return *this;
		}
	
		inline ThisMesh& Append(const ThisMesh& other)
	};

	const glm::ivec2 tvertices[]
	{
		glm::ivec2(0, 0),
		glm::ivec2(0, 1),
		glm::ivec2(1, 1),
		glm::ivec2(1, 0)
	};

	const int32_t tindices[]
	{
		0, 3, 1, 1, 3, 2
	};

	template <class T, typename K> requires valid_vertex<T> && std::is_integral<K>::value
	class MeshFactory
	{
	public:
		using ThisMesh = Mesh<T, K>;

		static auto GridCenter(glm::vec3 xdir, glm::vec3 ydir, const int xdetail, const int ydetail, glm::vec4 colour = { 1.f, 1.f, 1.f, 1.f })
		{
			ThisMesh mesh(xdetail * ydetail, (xdetail - 1) * (ydetail - 1) * 6);

			using namespace glm;

			for (int y = 0; y < ydetail; y++)
			{
				vec3 v = ((float)y / (ydetail - 1) - 0.5f) * ydir;
				for (int x = 0; x < xdetail; x++)
				{
					vec3 u = ((float)x / (xdetail - 1) - 0.5f) * xdir;

					auto& vert = mesh.m_Vertices[x + y * xdetail];
					vert.pos = u + v;
					if constexpr (has_color<T>) vert.color = colour;
					if constexpr (has_normal<T>) vert.normal = glm::normalize(glm::cross(xdir, ydir));
					if constexpr (has_texid<T>) vert.texid = -1.f;
					if constexpr (has_uv<T>) vert.uv = glm::vec2((float)x / (xdetail - 1.f), (float)y / (ydetail - 1.f));
				}
			}

			for (int v = 0; v < ydetail - 1; v++)
			{
				for (int u = 0; u < xdetail - 1; u++)
				{
					for (int n = 0; n < 6; n++)
					{
						glm::ivec2 vert = tvertices[tindices[n]];
						mesh.m_Indices[(u + v * (xdetail - 1)) * 6 + n] = (vert.x + u) + (vert.y + v) * xdetail;
					}
				}
			}

			return mesh;
		}

		static ThisMesh CubeSphere(const float& radius, const int& detail, glm::vec4 colour = { 1.f, 1.f, 1.f, 1.f }, std::vector<int>* strides = nullptr)
		{
			using namespace glm;
			ThisMesh mesh = GridCubeCenter(1.f, detail, colour, strides);

			for (int i = 0; i < mesh.m_Vertices.size(); i++)
			{
				auto v = mesh.m_Vertices[i];
				auto n = glm::normalize(v.pos);
				v.pos = n * radius;
				if constexpr (has_normal<T>) v.a_Normal = n;
			}

			return mesh;
		}

		static ThisMesh GridCubeCenter(float size, const int detail, glm::vec4 colour = { 1.f, 1.f, 1.f, 1.f }, std::vector<int>* strides = nullptr)
		{
			return GridRectCenter(size, size, size, detail, detail, detail, colour, strides);
		}

		static ThisMesh GridRectCenter(float width, float height, float depth, const int wdetail, const int hdetail, int ddetail, glm::vec4 colour = { 1.f, 1.f, 1.f, 1.f }, std::vector<int>* strides = nullptr)
		{
			using namespace glm;

			float wd = (float)(wdetail - 3) / (float)(wdetail - 1);
			float hd = (float)(hdetail - 3) / (float)(hdetail - 1);

			ThisMesh bottom = GridCenter(vec3(-width, 0.f, 0.f), vec3(0.f, depth, 0.f), wdetail, ddetail, colour);
			ThisMesh top = GridCenter(vec3(width, 0.f, 0.f), vec3(0.f, depth, 0.f), wdetail, ddetail, colour);
			ThisMesh right = GridCenter(vec3(0.f, depth, 0.f), vec3(0.f, 0.f, height * hd), ddetail, hdetail - 2, colour);
			ThisMesh left = GridCenter(vec3(0.f, -depth, 0.f), vec3(0.f, 0.f, height * hd), ddetail, hdetail - 2, colour);
			ThisMesh back = GridCenter(vec3(-width * wd, 0.f, 0.f), vec3(0.f, 0.f, height * hd), wdetail - 2, hdetail - 2, colour);
			ThisMesh front = GridCenter(vec3(width * wd, 0.f, 0.f), vec3(0.f, 0.f, height * hd), wdetail - 2, hdetail - 2, colour);
			
			bottom.Transform(glm::translate(mat4(1.f), vec3(0.f, 0.f, -height / 2.f)));
			top   .Transform(glm::translate(mat4(1.f), vec3(0.f, 0.f, height / 2.f)));
			right .Transform(glm::translate(mat4(1.f), vec3(width / 2.f, 0.f, 0.f)));
			left  .Transform(glm::translate(mat4(1.f), vec3(-width / 2.f, 0.f, 0.f)));
			back  .Transform(glm::translate(mat4(1.f), vec3(0.f, depth / 2.f, 0.f)));
			front .Transform(glm::translate(mat4(1.f), vec3(0.f, -depth / 2.f, 0.f)));

			int vertoffs = 0;
			int bottomvertoffs =	vertoffs;
			int topvertoffs =		vertoffs += bottom.m_Vertices.size();
			int rightvertoffs =		vertoffs += top.m_Vertices.size();
			int leftvertoffs =		vertoffs += right.m_Vertices.size();
			int backvertoffs =		vertoffs += left.m_Vertices.size();
			int frontvertoffs =		vertoffs += back.m_Vertices.size();

			if (strides)
			{
				strides->push_back(bottomvertoffs);
				strides->push_back(topvertoffs);
				strides->push_back(rightvertoffs);
				strides->push_back(leftvertoffs);
				strides->push_back(backvertoffs);
				strides->push_back(frontvertoffs);
			}

			ThisMesh gaps(0, (ddetail - 1) * 6 * 4 + (wdetail - 1) * 6 * 4 + (hdetail - 2 - 1) * 6 * 4);
			int gindeoffs = 0;

			//for (int i = 0; i < (ddetail - 1) * 6; i++) gaps->m_Indices[gindeoffs++] = ;

			gaps.m_Indices[gindeoffs++] = bottomvertoffs + wdetail - 2;
			gaps.m_Indices[gindeoffs++] = leftvertoffs + ddetail - 1;
			gaps.m_Indices[gindeoffs++] = bottomvertoffs + wdetail - 1; //FRONT BOTTOM LEFT
			gaps.m_Indices[gindeoffs++] = bottomvertoffs + wdetail - 2;
			gaps.m_Indices[gindeoffs++] = frontvertoffs;
			gaps.m_Indices[gindeoffs++] = leftvertoffs + ddetail - 1;
				
			gaps.m_Indices[gindeoffs++] = bottomvertoffs;
			gaps.m_Indices[gindeoffs++] = rightvertoffs;
			gaps.m_Indices[gindeoffs++] = frontvertoffs + wdetail - 3; // FRONT BOTTOM RIGHT
			gaps.m_Indices[gindeoffs++] = bottomvertoffs;
			gaps.m_Indices[gindeoffs++] = frontvertoffs + wdetail - 3;
			gaps.m_Indices[gindeoffs++] = bottomvertoffs + 1;
				
			gaps.m_Indices[gindeoffs++] = frontvertoffs + (hdetail - 3) * (wdetail - 2); // FRONT TOP LEFT
			gaps.m_Indices[gindeoffs++] = topvertoffs;
			gaps.m_Indices[gindeoffs++] = leftvertoffs + ddetail - 1 + (hdetail - 3) * ddetail;
			gaps.m_Indices[gindeoffs++] = frontvertoffs + (hdetail - 3) * (wdetail - 2);
			gaps.m_Indices[gindeoffs++] = topvertoffs + 1;
			gaps.m_Indices[gindeoffs++] = topvertoffs;
				
			gaps.m_Indices[gindeoffs++] = rightvertoffs + (hdetail - 3) * ddetail; // FRONT TOP RIGHT
			gaps.m_Indices[gindeoffs++] = topvertoffs + wdetail - 1;
			gaps.m_Indices[gindeoffs++] = topvertoffs + wdetail - 2;
			gaps.m_Indices[gindeoffs++] = rightvertoffs + (hdetail - 3) * ddetail;
			gaps.m_Indices[gindeoffs++] = topvertoffs + wdetail - 2;
			gaps.m_Indices[gindeoffs++] = frontvertoffs + +(hdetail - 3) * (wdetail - 2) + wdetail - 3;

			for (int i = 0; i < wdetail - 3; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = y * (frontvertoffs + i + x) + (1 - y) * (wdetail - (bottomvertoffs + i + 1 + x) - 1);
				}

			for (int i = 0; i < wdetail - 3; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = (1 - y) * (frontvertoffs + i + x + (hdetail - 3) * (wdetail - 2)) + y * (topvertoffs + i + 1 + x);
				}

			for (int i = 0; i < hdetail - 3; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = x * (frontvertoffs + (i + y) * (wdetail - 2)) + (1 - x) * (leftvertoffs + (i + y) * ddetail + ddetail - 1);
				}

			for (int i = 0; i < hdetail - 3; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = (1 - x) * (frontvertoffs + (i + y) * (wdetail - 2) + wdetail - 3) + x * (rightvertoffs + (i + y) * ddetail);
				}


			for (int i = 0; i < ddetail - 1; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = (1 - y) * (leftvertoffs + ddetail - (i + x) - 1) + y * (bottomvertoffs + (i + x) * wdetail + wdetail - 1);
				}

			for (int i = 0; i < ddetail - 1; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = y * (leftvertoffs + ddetail - (i + x) - 1 + (hdetail - 3) * ddetail) + (1 - y) * (topvertoffs + (i + x) * wdetail);
				}

			for (int i = 0; i < ddetail - 1; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = y * (rightvertoffs + i + x) + (1 - y) * (bottomvertoffs + (i + x) * wdetail);
				}

			for (int i = 0; i < ddetail - 1; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = (1 - y) * (rightvertoffs + i + x + (hdetail - 3) * ddetail) + y * (topvertoffs + (i + x) * wdetail + wdetail - 1);
				}


			gaps.m_Indices[gindeoffs++] = bottomvertoffs + wdetail - 1 + (ddetail - 1) * wdetail; //BACK BOTTOM LEFT
			gaps.m_Indices[gindeoffs++] = backvertoffs + wdetail - 3;
			gaps.m_Indices[gindeoffs++] = bottomvertoffs + wdetail - 2 + (ddetail - 1) * wdetail;
			gaps.m_Indices[gindeoffs++] = bottomvertoffs + wdetail - 1 + (ddetail - 1) * wdetail;
			gaps.m_Indices[gindeoffs++] = leftvertoffs;
			gaps.m_Indices[gindeoffs++] = backvertoffs + wdetail - 3;
				
			gaps.m_Indices[gindeoffs++] = bottomvertoffs + (ddetail - 1) * wdetail; //FRONT BOTTOM RIGHT
			gaps.m_Indices[gindeoffs++] = bottomvertoffs + 1 + (ddetail - 1) * wdetail;
			gaps.m_Indices[gindeoffs++] = rightvertoffs + ddetail - 1;
			gaps.m_Indices[gindeoffs++] = bottomvertoffs + 1 + (ddetail - 1) * wdetail;
			gaps.m_Indices[gindeoffs++] = backvertoffs;
			gaps.m_Indices[gindeoffs++] = rightvertoffs + ddetail - 1;
				
			gaps.m_Indices[gindeoffs++] = backvertoffs + (hdetail - 3) * (wdetail - 2);
			gaps.m_Indices[gindeoffs++] = topvertoffs + +wdetail - 1 + (ddetail - 1) * wdetail;
			gaps.m_Indices[gindeoffs++] = rightvertoffs + (hdetail - 3) * ddetail + ddetail - 1;
			gaps.m_Indices[gindeoffs++] = backvertoffs + (hdetail - 3) * (wdetail - 2);
			gaps.m_Indices[gindeoffs++] = topvertoffs + wdetail - 2 + (ddetail - 1) * wdetail;
			gaps.m_Indices[gindeoffs++] = topvertoffs + +wdetail - 1 + (ddetail - 1) * wdetail; //BACK TOP LEFT
				
			gaps.m_Indices[gindeoffs++] = backvertoffs + (hdetail - 3) * (wdetail - 2) + wdetail - 3;
			gaps.m_Indices[gindeoffs++] = leftvertoffs + (hdetail - 3) * ddetail;
			gaps.m_Indices[gindeoffs++] = topvertoffs + (ddetail - 1) * wdetail + 1;
			gaps.m_Indices[gindeoffs++] = leftvertoffs + (hdetail - 3) * ddetail;
			gaps.m_Indices[gindeoffs++] = topvertoffs + (ddetail - 1) * wdetail;
			gaps.m_Indices[gindeoffs++] = topvertoffs + (ddetail - 1) * wdetail + 1;



			for (int i = 0; i < wdetail - 3; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = y * (backvertoffs + i + x) + (1 - y) * (bottomvertoffs + i + 1 + x + wdetail * (ddetail - 1));
				}

			for (int i = 0; i < wdetail - 3; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = (1 - y) * (backvertoffs + i + x + (hdetail - 3) * (wdetail - 2)) + y * (topvertoffs + wdetail - (i + 1 + x) - 1 + wdetail * (ddetail - 1));
				}

			for (int i = 0; i < hdetail - 3; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = x * (backvertoffs + (i + y) * (wdetail - 2)) + (1 - x) * (rightvertoffs + (i + y) * ddetail + ddetail - 1);
				}

			for (int i = 0; i < hdetail - 3; i++)
				for (int n = 0; n < 6; n++)
				{
					const int& x = tvertices[tindices[n]].x;
					const int& y = tvertices[tindices[n]].y;
					gaps.m_Indices[gindeoffs++] = (1 - x) * (backvertoffs + (i + y) * (wdetail - 2) + wdetail - 3) + x * (leftvertoffs + (i + y) * ddetail);
				}

			int indeoffs = 0;
			ThisMesh mesh(wdetail * ddetail * 2 + ddetail * (hdetail - 2) * 2 + (wdetail - 2) * (hdetail - 2) * 2, (wdetail - 1) * (ddetail - 1) * 12 + (hdetail - 1) * (ddetail - 1) * 12 + (wdetail - 1) * (hdetail - 1) * 12);
			mesh.Insert(bottom, bottomvertoffs, indeoffs, true);
			mesh.Insert(top, topvertoffs, indeoffs += bottom.m_Indices.size(), true);
			mesh.Insert(right, rightvertoffs, indeoffs += top.m_Indices.size(), true);
			mesh.Insert(left, leftvertoffs, indeoffs += right.m_Indices.size(), true);
			mesh.Insert(back, backvertoffs, indeoffs += left.m_Indices.size(), true);
			mesh.Insert(front, frontvertoffs, indeoffs += back.m_Indices.size(), true);

			mesh.Insert(gaps, 0U, indeoffs += front.m_Indices.size(), true);

			return mesh;
		}
	};

}