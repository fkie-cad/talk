#pragma once

// __pragam is MSC specific
// _Pragma is similar to the Microsoft-specific __pragma keyword. 
// It was introduced into the C standard in C99, and the C++ standard in C++11. 
// It's available in C only when you specify the /std:c11 or /std:c17 option.
// Unlike #pragma, _Pragma allows you to put pragma directives into a macro definition. 
#define DISABLE_WARNING(_id_) \
    __pragma( warning( disable : _id_) )

#define DISABLE_WARNINGS(...) \
    __pragma( warning( disable : __VA_ARGS__ ) )

#define DEFAULT_WARNING(_id_) \
    __pragma( warning( default : _id_) )

#define DEFAULT_WARNINGS(...) \
    __pragma( warning( default : __VA_ARGS__ ) )
    


// warning C6320: Exception-filter expression is the constant EXCEPTION_EXECUTE_HANDLER

DISABLE_WARNINGS ( 6320 )
