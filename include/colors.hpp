
#if GDPM_ENABLE_COLORS == 1
	#define GDPM_COLOR_BLACK "\033[0;30m"
	#define GDPM_COLOR_BLUE "\033[0;34m"
	#define GDPM_COLOR_GREEN "\033[0;32m"
	#define GDPM_COLOR_CYAN "\033[0;36m"
	#define GDPM_COLOR_RED "\033[0;31m"
	#define GDPM_COLOR_PURPLE "\033[0;35m"
	#define GDPM_COLOR_BROWN "\033[0;33m"
	#define GDPM_COLOR_GRAY "\033[0;37m"
	#define GDPM_COLOR_DARK_GRAY "\033[0;30m"
	#define GDPM_COLOR_LIGHT_BLUE "\033[0;34m"
	#define GDPM_COLOR_LIGHT_GREEN "\033[0;32m"
	#define GDPM_COLOR_LIGHT_CYAN "\033[0;36m"
	#define GDPM_COLOR_LIGHT_RED "\033[0;31m"
	#define GDPM_COLOR_LIGHT_PURPLE "\033[0;35m"
	#define GDPM_COLOR_YELLOW "\033[0;33m"
	#define GDPM_COLOR_WHITE "\033[0;37m"

#else
	#define GDPM_COLOR_BLACK
	#define GDPM_COLOR_BLUE
	#define GDPM_COLOR_GREEN
	#define GDPM_COLOR_CYAN
	#define GDPM_COLOR_RED
	#define GDPM_COLOR_PURPLE
	#define GDPM_COLOR_BROWN
	#define GDPM_COLOR_GRAY
	#define GDPM_COLOR_DARK_GRAY
	#define GDPM_COLOR_LIGHT_BLUE
	#define GDPM_COLOR_LIGHT_GREEN
	#define GDPM_COLOR_LIGHT_CYAN
	#define GDPM_COLOR_LIGHT_RED
	#define GDPM_COLOR_LIGHT_PURPLE
	#define GDPM_COLOR_YELLOW
	#define GDPM_COLOR_WHITE


#endif

#define GDPM_COLOR_LOG_RESET GDPM_COLOR_WHITE
#define GDPM_COLOR_LOG_INFO GDPM_COLOR_LOG_RESET
#define GDPM_COLOR_LOG_ERROR GDPM_COLOR_RED
#define GDPM_COLOR_LOG_DEBUG GDPM_COLOR_YELLOW
#define GDPM_COLOR_LOG_WARNING GDPM_COLOR_YELLOW