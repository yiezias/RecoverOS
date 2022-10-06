#ifndef __LIB_USER_ASSERT_H
#define __LIB_USER_ASSERT_H


void user_spin(const char *file, int line, const char *func,
	       const char *condition);

#define panic(...) user_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NDEBUG
#define assert ((void)0)
#else
#define assert(CONDITION)          \
	if (CONDITION)             \
		;                  \
	else {                     \
		panic(#CONDITION); \
	}
#endif


#endif
