
#define SAF_ASSERT assert
#define VKCHECK(x) (SAF_ASSERT(x == VK_SUCCESS, #x))

namespace saf {

	extern const char* g_EngineName;
	extern const char* g_ApplicationName;

}