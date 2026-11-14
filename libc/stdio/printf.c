#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

static bool skip_long_modifier(const char* restrict* format) {
	bool is_long = false;
	while (**format == 'l') {
		is_long = true;
		(*format)++;
	}
	return is_long;
}

/* Accepts an optional zero-padding width: %0<width><conv>. */
static bool is_number_spec(const char* format) {
	if (*format == '0')
		while (*format >= '0' && *format <= '9')
			format++;
	while (*format == 'l')
		format++;
	return *format == 'd' || *format == 'i' || *format == 'u' || *format == 'x' || *format == 'X';
}

static unsigned long next_number(va_list* parameters, char spec, bool is_long, int* neg) {
	if (spec == 'd' || spec == 'i') {
		long signed_val = is_long ? va_arg(*parameters, long) : va_arg(*parameters, int);
		if (signed_val < 0) {
			*neg = 1;
			return -(unsigned long) signed_val;
		}
		return signed_val;
	}
	return is_long ? va_arg(*parameters, unsigned long) : va_arg(*parameters, unsigned int);
}

static int print_formatted(const char* restrict format, va_list* parameters) {
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
			char c = (char) va_arg(*parameters, int /* char promotes to int */);
			if (!maxrem) {
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(*parameters, const char*);
			if (str == NULL)
				str = "(null)";
			size_t len = strlen(str);
			if (maxrem < len) {
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else if (is_number_spec(format)) {
			int width = 0;
			if (*format == '0')
				while (*format >= '0' && *format <= '9')
					width = width * 10 + (*format++ - '0');
			bool is_long = skip_long_modifier(&format);
			char spec = *format++;
			char numbuf[32];
			unsigned long val;
			int neg = 0;
			int base = 10;
			const char *digits = "0123456789abcdef";
			if (spec == 'X') digits = "0123456789ABCDEF";
			if (spec == 'x' || spec == 'X') base = 16;
			val = next_number(parameters, spec, is_long, &neg);
			int idx = 31;
			numbuf[idx] = '\0';
			if (val == 0) numbuf[--idx] = '0';
			while (val > 0) { numbuf[--idx] = digits[val % base]; val /= base; }
			if (neg) numbuf[--idx] = '-';
			while (31 - idx < width && idx > 0) numbuf[--idx] = '0';
			const char* numstr = &numbuf[idx];
			size_t len = strlen(numstr);
			if (!print(numstr, len)) return -1;
			written += len;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	return written;
}

int vprintf(const char* restrict format, va_list arguments) {
	va_list parameters;
	va_copy(parameters, arguments);
	int written = print_formatted(format, &parameters);
	va_end(parameters);
	return written;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);
	int written = vprintf(format, parameters);
	va_end(parameters);
	return written;
}
