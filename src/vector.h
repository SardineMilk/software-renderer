// vector.h
#ifndef VECTOR_H
#define VECTOR_H


#include <math.h>

typedef struct {
    float x;
    float y;
    float z;
} Vec3;


static inline Vec3 add(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline Vec3 sub(Vec3 a, Vec3 b) {
    return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline Vec3 scale(Vec3 v, float s) {
    return (Vec3){v.x * s, v.y * s, v.z * s};
}

static inline Vec3 vector_floor(Vec3 v) {
    return (Vec3){floorf(v.x), floorf(v.y), floorf(v.z)};
}

static inline float length(Vec3 v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline float sqlength(Vec3 v) {
    return v.x*v.x + v.y*v.y + v.z*v.z;
}


#endif // VECTOR_H