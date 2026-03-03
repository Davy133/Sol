#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* código feio da porra */
static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else if (*format == '0') {
			/* zero-padding: %0<width><conv> */
			format++;
			int width = 0;
			while (*format >= '0' && *format <= '9')
				width = width * 10 + (*format++ - '0');
			/* fall through to numeric handling below */
			char spec = *format++;
			char numbuf[32];
			unsigned int val;
			int neg = 0;
			int base = 10;
			const char *digits = "0123456789abcdef";
			if (spec == 'X') digits = "0123456789ABCDEF";
			if (spec == 'x' || spec == 'X') base = 16;
			if (spec == 'd' || spec == 'i') {
				int signed_val = va_arg(parameters, int);
				if (signed_val < 0) { neg = 1; val = (unsigned int)(-signed_val); }
				else val = (unsigned int)signed_val;
			} else {
				val = va_arg(parameters, unsigned int);
			}
			int idx = 31;
			numbuf[idx] = '\0';
			if (val == 0) numbuf[--idx] = '0';
			while (val > 0) { numbuf[--idx] = digits[val % base]; val /= base; }
			if (neg) numbuf[--idx] = '-';
			int numlen = 31 - idx;
			while (numlen < width) { numbuf[--idx] = '0'; numlen++; }
			const char* numstr = &numbuf[idx];
			size_t len = strlen(numstr);
			if (!print(numstr, len)) return -1;
			written += len;
		} else if (*format == 'd' || *format == 'i' || *format == 'u' || *format == 'x' || *format == 'X') {
			char spec = *format++;
			char numbuf[32];
			unsigned int val;
			int neg = 0;
			int base = 10;
			const char *digits = "0123456789abcdef";
			if (spec == 'X') digits = "0123456789ABCDEF";
			if (spec == 'x' || spec == 'X') base = 16;
			if (spec == 'd' || spec == 'i') {
				int signed_val = va_arg(parameters, int);
				if (signed_val < 0) { neg = 1; val = (unsigned int)(-signed_val); }
				else val = (unsigned int)signed_val;
			} else {
				val = va_arg(parameters, unsigned int);
			}
			int idx = 31;
			numbuf[idx] = '\0';
			if (val == 0) numbuf[--idx] = '0';
			while (val > 0) { numbuf[--idx] = digits[val % base]; val /= base; }
			if (neg) numbuf[--idx] = '-';
			const char* numstr = &numbuf[idx];
			size_t len = strlen(numstr);
			if (!print(numstr, len)) return -1;
			written += len;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}
