#ifndef CONVERTER_H
#define CONVERTER_H

#include <windows.h>



ULONG parsePlainBytes(const char* raw, BYTE** buffer, ULONG size);
int parseUint8(const char* arg, BYTE* value, BYTE base);
int parseUint64(const char* arg, ULONGLONG* value, BYTE base);



ULONG parsePlainBytes(const char* raw, BYTE** buffer, ULONG size)
{
	ULONG i, j;
	SIZE_T arg_ln = strnlen(raw, MAX_BUFFER_LN);
	BYTE* p;
	char byte[3] = {0};
	ULONG buffer_ln;
	int s = 0;
    
	buffer_ln = (ULONG) (arg_ln / 2);

	if ( arg_ln % 2 != 0 || arg_ln == 0 )
	{
		printf("Error: Buffer data is not byte aligned!\n");
		return 0;
	}

	if ( buffer_ln != size )
	{
		printf("Error: Buffer data has the wrong size %u != %u!\n", buffer_ln, size);
		return 0;
	}

	p = (BYTE*) malloc(arg_ln/2);
	if ( p == NULL )
	{
		printf("ERROR (0x%08x): Allocating memory failed!\n", GetLastError());
		return 0;
	}

	for ( i = 0, j = 0; i < arg_ln; i += 2 )
	{
		byte[0] = raw[i];
		byte[1] = raw[i + 1];

		 s = parseUint8(byte, &p[j++], 16);
		 if ( s != 0 )
		 {
		 	free(p);
		 	return 0;
		 }
	}
    
	*buffer = p;
	return buffer_ln;
}

int parseUint8(const char* arg, BYTE* value, BYTE base)
{
	ULONGLONG result;
	int s = parseUint64(arg, &result, base);
	if ( s != 0 ) return s;
	if ( s > (BYTE)-1 )
	{
		fprintf(stderr, "Error: %s could not be converted to a byte: Out of range!\n", arg);
		return 5;
	}

	*value = (BYTE) result;
	return 0;
}

int parseUint64(const char* arg, ULONGLONG* value, BYTE base)
{
	char* endptr;
	int err_no = 0;
	errno = 0;
	ULONGLONG result;

	if ( base != 10 && base != 16 && base != 0 )
	{
		fprintf(stderr, "Error: Unsupported base %u!\n", base);
		return 1;
	}

	if ( arg[0] ==  '-' )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: is negative!\n", arg);
		return 2;
	}

#if defined(_WIN32)
	result = strtoull(arg, &endptr, base);
#else
	result = strtoul(arg, &endptr, base);
#endif
	err_no = errno;

	if ( endptr == arg )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Not a number!\n", arg);
		return 3;
	}
	if ( result == (ULONGLONG)-1 && err_no == ERANGE )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Out of range!\n", arg);
		return 4;
	}

	*value = result;
	return 0;
}

#endif
