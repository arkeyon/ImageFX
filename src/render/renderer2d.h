#pragma once

#include "shader.h"

namespace saf {

	class Renderer2D
	{
	public:

		void BeginScene(Shader shader);
		void Submit();
		void EndScene();

	private:




	};

}