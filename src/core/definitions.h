
#include <cstddef>
#define SAF_ASSERT assert

#include <exception>

#define ERR_GUARD_VULKAN(expr)  do { if((expr) < 0) { \
        assert(0 && #expr); \
        throw std::runtime_error(__FILE__ ": VkResult( " #expr " ) < 0"); \
    } } while(false)


namespace saf {

	extern const char* g_EngineName;
	extern const char* g_ApplicationName;

}