#ifndef PROJECTUTIL_H
#define PROJECTUTIL_H

#include <cjson/cJSON.h>

void projectutil_init(cJSON **PCFGo);
void projectutil_build(const cJSON *PCFG);

#endif
