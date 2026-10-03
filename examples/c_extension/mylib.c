#if defined(_WIN32) || defined(_MSC_VER)
#define MYLIB_EXPORT __declspec(dllexport)
#else
#define MYLIB_EXPORT __attribute__((visibility("default")))
#endif

MYLIB_EXPORT int c_custom_calc(int a, int b)
{
	return a * 2 + b * 3;
}
