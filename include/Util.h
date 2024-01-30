#pragma once

#define Assert(E) ((E) || (fprintf(stderr,"[ASSERT %s:%d]: %s\n",__FILE__, __LINE__, #E), *(bool*)nullptr))