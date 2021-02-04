#ifndef ARGS_H
#define ARGS_H

#include <windows.h>



#define PARAM_IDENTIFIER '/'



BOOL isAskForHelp(INT argc, CHAR** argv);
BOOL isArgOfType(const CHAR* arg, const CHAR* type);
BOOL hasValue(const char* type, int i, int end_i);




BOOL isAskForHelp(INT argc, CHAR** argv)
{
    int i = 1;
    if ( argc < i+1 )
        return FALSE;

    return isArgOfType(argv[i], "h") || isArgOfType(argv[i], "?");
}

BOOL isArgOfType(const CHAR* arg, const CHAR* type)
{
    size_t type_ln;

    type_ln = strlen(type);

    return arg[0] == PARAM_IDENTIFIER && 
        strnlen(&arg[1], 10) == type_ln && 
        strncmp(&arg[1], type, type_ln) == 0;
}

BOOL hasValue(const char* type, int i, int last_i)
{
    if ( i >= last_i )
    {
        printf("INFO: Arg \"%c%s\" has no value! Skipped!\n", PARAM_IDENTIFIER, type);
        return FALSE;
    }

    return TRUE;
}

#endif
