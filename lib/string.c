#include "string.h"
#include "../kernel/debug.h"
#include "../kernel/global.h"
#include "user/assert.h"

#ifdef USER_DEBUG
#undef ASSERT
#define ASSERT(CONDITION) assert(CONDITION)
#endif

void memcpy(void *dst, const void *src, size_t size) {
	ASSERT(dst && src);
	while (size--) {
		*(char *)(dst++) = *(char *)(src++);
	}
}

void memset(void *dst, uint8_t value, size_t size) {
	ASSERT(dst);
	while (size--) {
		*(char *)(dst++) = value;
	}
}

int memcmp(const void *a, const void *b, size_t size) {
	ASSERT(a != NULL || b != NULL);
	while (size-- > 0) {
		if (*(char *)a != *(char *)b) {
			return *(char *)a - *(char *)b;
		}
		++a;
		++b;
	}
	return 0;
}

char *strcpy(char *dst, const char *src) {
	ASSERT(dst && src);
	char *ret = dst;

	while ((*dst++ = *src++)) {}

	return ret;
}

size_t strlen(const char *str) {
	ASSERT(str);
	size_t len = 0;
	while (str[len++]) {}
	return len - 1;
}

char *strcat(char *dst, char *src) {
	ASSERT(dst && src);
	char *str = dst;

	while (*str++) {}

	--str;
	while ((*str++ = *src++)) {}
	return dst;
}

int strcmp(const char *a, const char *b) {
	ASSERT(a && b);
	while (*a != 0 && *a == *b) {
		++a;
		++b;
	}

	return *a - *b;
}

char *strrchr(const char *str, const char ch) {
	ASSERT(str != NULL);
	const char *last_char = NULL;
	while (*str != 0) {
		if (*str == ch) {
			last_char = str;
		}
		++str;
	}
	return (char *)last_char;
}

char *strchr(const char *str, const char ch) {
	ASSERT(str != NULL);
	while (*str != 0) {
		if (*str == ch) {
			return (char *)str;
		}
		++str;
	}
	return NULL;
}
