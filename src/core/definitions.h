
#include <cstddef>
#define SAF_ASSERT assert

#ifdef _MSC_VER
#define VKCHECK(x) (SAF_ASSERT(x == VK_SUCCESS, #x))
#endif

#ifdef __clang__
#define VKCHECK(x) (SAF_ASSERT(x == VK_SUCCESS))
#endif


namespace saf {

	extern const char* g_EngineName;
	extern const char* g_ApplicationName;

}