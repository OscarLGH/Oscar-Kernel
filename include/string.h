#ifndef _STRING_H_
#define _STRING_H_

unsigned long strlen(const char * str);
char *strcpy(char *dest, const char *src);
int strncasecmp(const char *s1, const char *s2, unsigned long len);
int strcasecmp(const char *s1, const char *s2);
char *strncpy(char *dest, const char *src, unsigned int count);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, unsigned long count);
int strcmp(const char *cs, const char *ct);
int strncmp(const char *cs, const char *ct, unsigned long count);
char *skip_spaces(const char *str);
unsigned long strnlen(const char *s, unsigned long count);
char *strstr(const char *s1, const char *s2);
int findstr(unsigned char *data, int size, const char *str, int len);
static inline char tolower(char c);
static inline char toupper(char c);
static inline char isspace(char c);

static inline char isspace(char c)
{
	if (c == ' ')
		return 1;
	return 0;
}

static inline char tolower(char c)
{
	return c - 0x20;
}

static inline char toupper(char c)
{
	return c + 0x20;
}

void memcpy(void *dst,const void *src,int size);
void memset(void *dst,char value,int size);
int memcmp(const void *s1,const void *s2,int size);
int hex_dump(void *addr, u64 len);
int long_int_print(void *addr, u64 len);
void unicode_to_ascii(u16 *unicode_str, char *ascii_str);


#endif
