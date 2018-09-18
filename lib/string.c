#include <types.h>
#include <stdarg.h>
#include <string.h>
#include <spinlock.h>


void memcpy(void *dst,const void *src,int size)
{
	if(dst == 0 || src == 0)
		return;
	int i = 0;
	
	size--;
	if(dst > src) {
		while(size >= 0) {
			((char *)dst)[size] = ((char *)src)[size];
			size--;
		}
	} else {
		while(i <= size) {
			((char *)dst)[i] = ((char *)src)[i];
			i++;
		}
	}
}

void memset(void *dst,char value,int size)
{
	if(!dst)
		return;
	int i = 0;
	while(i<size)
		*((char *)dst + i++) = value;
}

int memcmp(const void *s1,const void *s2,int size)
{
	int i;
	for(i=0; i<size; i++) {
		if(*(char *)(s1 + i) != *(char *)(s2 + i))
			return *(char *)(s1 + i) - *(char *)(s2 + i);
	}
	return 0;
}


unsigned long strlen(const char *s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

char *strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}

int strncasecmp(const char *s1, const char *s2, unsigned long len)
{
	/* Yes, Virginia, it had better be unsigned */
	unsigned char c1, c2;

	if (!len)
		return 0;

	do {
		c1 = *s1++;
		c2 = *s2++;
		if (!c1 || !c2)
			break;
		if (c1 == c2)
			continue;
		c1 = tolower(c1);
		c2 = tolower(c2);
		if (c1 != c2)
			break;
	} while (--len);
	return (int)c1 - (int)c2;
}

int strcasecmp(const char *s1, const char *s2)
{
	int c1, c2;

	do {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} while (c1 == c2 && c1 != 0);
	return c1 - c2;
}

char *strncpy(char *dest, const char *src, unsigned int count)
{
	char *tmp = dest;

	while (count) {
		if ((*tmp = *src) != 0)
			src++;
		tmp++;
		count--;
	}
	return dest;
}

char *strchr(const char *s, int c)
{
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			return NULL;
	return (char *)s;
}

char *strrchr(const char *s, int c)
{
	const char *last = NULL;
	do {
		if (*s == (char)c)
			last = s;
	} while (*s++);
	return (char *)last;
}

char *strcat(char *dest, const char *src)
{
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;
	return tmp;
}

char *strncat(char *dest, const char *src, unsigned long count)
{
	char *tmp = dest;

	if (count) {
		while (*dest)
			dest++;
		while ((*dest++ = *src++) != 0) {
			if (--count == 0) {
				*dest = '\0';
				break;
			}
		}
	}
	return tmp;
}

int strcmp(const char *cs, const char *ct)
{
	unsigned char c1, c2;

	while (1) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}
	return 0;
}

int strncmp(const char *cs, const char *ct, unsigned long count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
		count--;
	}
	return 0;
}

char *skip_spaces(const char *str)
{
	while (isspace(*str))
		++str;
	return (char *)str;
}

unsigned long strnlen(const char *s, unsigned long count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

char *strstr(const char *s1, const char *s2)
{
	unsigned long l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *)s1;
	l1 = strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1, s2, l2))
			return (char *)s1;
		s1++;
	}
	return NULL;
}

int findstr(unsigned char *data, int size, const char *str, int len)
{
	int i, j;

	for (i = 0; i <= (size - len); i++) 
	{
		for (j = 0; j < len; j++)
			if ((char)data[i + j] != str[j])
				break;
		if (j == len)
			return i;
	}

	return 0;
}


char *i2a(u64 val, u64 base, char ** ps)
{
	u64 m = val % base;
	u64 q = val / base;
	
	if (q) 
		i2a(q, base, ps);
	
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'a');

	return *ps;
}

char *i2a_full(u64 val, u64 base, u64 digit, char ** ps)
{
	u64 m;
	u64 q = val;
	u64 i = 0;
	(*ps) += (digit - 1);
	
	while(q) {
		m = q % base;
		q /= base;
		*(*ps)-- = (m < 10) ? (m + '0') : (m - 10 + 'a');
		i++;
	}
	
	for(; i<digit; i++) {
		*(*ps)-- = '0';
	}
	
	return *ps;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
	char*	p;
	long long m;

	char	inner_buf[256] = {0};
	char	cs;
	int	align_nr;

	for (p = buf; *fmt; fmt++) {
		if (*fmt != '%') {
			*p++ = *fmt;
			continue;
		}
		else {		/* a format string begins */
			align_nr = 0;
		}

		fmt++;

		if (*fmt == '%') {
			*p++ = *fmt;
			continue;
		} else if (*fmt == '0') {
			cs = '0';
			fmt++;
		} else {
			cs = ' ';
		}
		
		while (((unsigned char)(*fmt) >= '0') && ((unsigned char)(*fmt) <= '9')) {
			align_nr *= 10;
			align_nr += *fmt - '0';
			fmt++;
		}
		
		char * q = inner_buf;
		memset(q, 0, sizeof(inner_buf));

		switch (*fmt) {
			case 'c':
				*q++ = va_arg(args, long long);
				//p_next_arg += 8;
				break;
			case 'x':
				m = va_arg(args, long long);
				i2a(m, 16, &q);
				//p_next_arg += 8;
				break;
			case 'd':
				m = va_arg(args, long long);
				if (m < 0) {
					m = m * (-1);
					*q++ = '-';
				}
				i2a(m, 10, &q);
				break;
			case 's':
				strcpy(q, va_arg(args, char *));
				q += strlen(q);
				break;
			default:
				break;
		}
		int k;
		for (k = 0; k < ((align_nr > strlen(inner_buf)) ? (align_nr - strlen(inner_buf)) : 0); k++) {
			*p++ = cs;
		}
		q = inner_buf;
		while (*q) {
			*p++ = *q++;
		}
	}

	*p = 0;

	return (p - buf);
}

#include <console.h>

#include <in_out.h>
u64 lock = 0;
int write_console(u32 chr);

int printk(const char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	long long i,j;
	char buf[256] = {0};
	
	i = vsprintf(buf, fmt, ap);
	va_end(ap);
	buf[i] = 0;
	spin_lock(&lock);
	for (j = 0; j < strlen(buf); j++) {
		
		write_console(buf[j]);

		//early qemu debug purpose.remove when boot code is ready.
		if (buf[j] == '\n')
			out8(0x3f8, '\r');
		out8(0x3f8, buf[j]);
		//uart_out
	}
	spin_unlock(&lock);
	return i;
}

