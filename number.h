#ifndef _DC_NUMBER_H
#define _DC_NUMBER_H

typedef struct DC_number {
    union {
        int   int_value;
        float float_value;
        void  *object_value;
    };
} DC_number_t;

extern DC_number_t *DC_number_with_integer (DC_number_t *num, int val);

extern DC_number_t *DC_number_with_float (DC_number_t *num, float val);

extern DC_number_t *DC_number_with_object (DC_number_t *num, void *obj);


#define DC_NUM_INTEGER(val) \
    DC_number_with_integer(((DC_number_t*)malloc(sizeof(DC_number_t))), val)

#define DC_NUM_FLOAT(val) \
    DC_number_with_float(((DC_number_t*)malloc(sizeof(DC_number_t))), val)

#define DC_NUM_OBJECT(val) \
    DC_number_with_object(((DC_number_t*)malloc(sizeof(DC_number_t))), val)

#endif
