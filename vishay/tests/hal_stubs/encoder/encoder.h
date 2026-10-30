#include <stdbool.h>
void encoder_set_custom_callbacks(float (*read)(void), bool (*fault)(void), char *(*info)(void));
