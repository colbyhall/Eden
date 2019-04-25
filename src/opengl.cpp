#include "opengl.h"

#define DEFINE_GL_FUNCTIONS(type, func) type func = NULL;
GL_BINDINGS(DEFINE_GL_FUNCTIONS);
#undef DEFINE_GL_FUNCTIONS