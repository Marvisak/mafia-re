#include "main.h"

#include <cstdlib>

float EXPORT RandFloat()
{
    return rand() * (1 / RAND_MAX);
}
