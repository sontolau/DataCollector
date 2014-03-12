#include "number.h"

DC_number_t *DC_number_with_integer (DC_number_t *num, int val) {
    num->int_value = val;
    return num;
}

DC_number_t *DC_number_with_float (DC_number_t *num, float val) {
    num->float_value = val;
    return num;
}

DC_number_t *DC_number_with_object (DC_number_t *num, void *object) {
    num->object_value = object;
    return num;
}
