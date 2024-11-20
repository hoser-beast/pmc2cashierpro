#ifndef UTILS
#define UTILS

#include <stdint.h>

#if DEBUG
#	define assert(expr, msg) if(!(expr)) { printf("Assert failed! %s(%d).\n", __FILE__, __LINE__); __debugbreak(); }
#else
#	define assert(expr, msg) ""
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t	 i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef enum {false, true} bool;

inline u32 TruncateUnsignedI64(u64 value)
{
	// @TODO: define for maximum value
	assert(value <= 0xFFFFFFFF, "File is greater than 4GiB, giving up.\n");
	return (u32)value;
}

void FreeFileMemory(void* memory)
{
	if (memory)
	{
		VirtualFree(memory, 0, MEM_RELEASE);
	}
}

char* ReadEntireFile(char* file_name)
{
	void* data = NULL;

	HANDLE file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	LARGE_INTEGER file_size;
	if (GetFileSizeEx(file_handle, &file_size))
	{
		u32 file_size_32bits = TruncateUnsignedI64(file_size.QuadPart);
		data = (char*)VirtualAlloc(0, file_size_32bits, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (data)
		{
			DWORD bytes_read = 0;
			if (ReadFile(file_handle, data, file_size_32bits, &bytes_read, 0) && (file_size_32bits == bytes_read))
			{
				// success
			}
			else
			{
				FreeFileMemory(data);
				data = NULL;
			}
		}

		CloseHandle(file_handle);
	}

	return data;
}

void PrintSubstring(const char* start, size_t length)
{
	for (int index = 0; index < length; index++)
	{
		putchar(start[index]);
	}
}

size_t FillTextFieldAndTrim(char* field, char* start, size_t length)
{
	if ((field == NULL) || (start == NULL))
	{
		return 0;
	}

	// Trim excess whitespace from the beginning.
	char* beginning = start;
	while (isspace((unsigned char)* beginning) && ((size_t)(beginning - start) < length))
	{
		beginning++;
	}

	// Trim excess whitespace from the end.
	char* end = start + length - 1;
	if ((beginning == end) && isspace((unsigned char)* beginning)) // @TODO had to add this extra isspace() check to fix bug in strings like "          1". Can this be avoided?
	{
		// Field is empty (all whitespace).
		return 0;
	}

	while (isspace((unsigned char)* end) && (end > beginning))
	{
		end--;
	}

	// Fill field by copying remaining characters.
	size_t index;
	for (index = 0; index < (size_t)(end - beginning + 1); index++)
	{
		field[index] = *(beginning + index);
	}
	field[index] = '\0';

	return index + 1;
}

inline i32 FindCharInString(char* data, char character)
{
	i32 index = 0;
	while (data[index] != '\0')
	{
		if (data[index] == character)
			return index;

		index++;
	}
	return -1;
}

#endif
