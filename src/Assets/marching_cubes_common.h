layout(binding = 0) uniform sampler3D density_map;

uniform int block_size;

// first 3 components are the world coordinate
// 4th is the texture coordinate, should be 1, 2 or 4
uniform ivec4 block_index;

// The marching cubes algorithm consists of 256 cases of triangle configurations (each
// corner can be on or off).

// Tables by Ryan Geiss.

// Lookup table for many polygons for each of the 256 cases.
int case_to_numpolys[256] = {
    0, 1, 1, 2, 1, 2, 2, 3,  1, 2, 2, 3, 2, 3, 3, 2,  1, 2, 2, 3, 2, 3, 3, 4,  2, 3, 3, 4, 3, 4, 4, 3,
    1, 2, 2, 3, 2, 3, 3, 4,  2, 3, 3, 4, 3, 4, 4, 3,  2, 3, 3, 2, 3, 4, 4, 3,  3, 4, 4, 3, 4, 5, 5, 2,
    1, 2, 2, 3, 2, 3, 3, 4,  2, 3, 3, 4, 3, 4, 4, 3,  2, 3, 3, 4, 3, 4, 4, 5,  3, 4, 4, 5, 4, 5, 5, 4,
    2, 3, 3, 4, 3, 4, 2, 3,  3, 4, 4, 5, 4, 5, 3, 2,  3, 4, 4, 3, 4, 5, 3, 2,  4, 5, 5, 4, 5, 2, 4, 1,
    1, 2, 2, 3, 2, 3, 3, 4,  2, 3, 3, 4, 3, 4, 4, 3,  2, 3, 3, 4, 3, 4, 4, 5,  3, 2, 4, 3, 4, 3, 5, 2,
    2, 3, 3, 4, 3, 4, 4, 5,  3, 4, 4, 5, 4, 5, 5, 4,  3, 4, 4, 3, 4, 5, 5, 4,  4, 3, 5, 2, 5, 4, 2, 1,
    2, 3, 3, 4, 3, 4, 4, 5,  3, 4, 4, 5, 2, 3, 3, 2,  3, 4, 4, 5, 4, 5, 5, 2,  4, 3, 5, 4, 3, 2, 4, 1,
    3, 4, 4, 5, 4, 5, 3, 4,  4, 5, 5, 2, 3, 4, 2, 1,  2, 3, 3, 2, 3, 4, 2, 1,  3, 2, 4, 1, 2, 1, 1, 0
};

vec3 edge_start[12] = {
    vec3(0, 0, 0), vec3(0, 1, 0), vec3(1, 0, 0), vec3(0, 0, 0),
    vec3(0, 0, 1), vec3(0, 1, 1), vec3(1, 0, 1), vec3(0, 0, 1),
    vec3(0, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(1, 0, 0)
};

vec3 edge_dir[12] = {
    vec3(0, 1, 0), vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 0, 0),
    vec3(0, 1, 0), vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 0, 0),
    vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1)
};

// Equal to edge_start + edge_dir
vec3 edge_end[12] = {
    vec3(0, 1, 0), vec3(1, 1, 0), vec3(1, 1, 0), vec3(1, 0, 0),
    vec3(0, 1, 1), vec3(1, 1, 1), vec3(1, 1, 1), vec3(1, 0, 1),
    vec3(0, 0, 1), vec3(0, 1, 1), vec3(1, 1, 1), vec3(1, 0, 1)
};

vec3 edge_axis[12] = {
    vec3(1, 0, 0), vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 0, 0),
    vec3(1, 0, 0), vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 0, 0),
    vec3(2, 0, 0), vec3(2, 0, 0), vec3(2, 0, 0), vec3(2, 0, 0)
};

// Lookup table for the triangles in each case.
// Each case can have up to 5 triangles.
// Each triangle is defined by the index of the 3 edges in which its vertices are located.
ivec3 edge_connect_list[256][5] = {
    { ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  1,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  8,  3), ivec3( 9,  8,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8,  3), ivec3( 1,  2, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  2, 10), ivec3( 0,  2,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2,  8,  3), ivec3( 2, 10,  8), ivec3(10,  9,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3, 11,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0, 11,  2), ivec3( 8, 11,  0), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  9,  0), ivec3( 2,  3, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1, 11,  2), ivec3( 1,  9, 11), ivec3( 9,  8, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3, 10,  1), ivec3(11, 10,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0, 10,  1), ivec3( 0,  8, 10), ivec3( 8, 11, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  9,  0), ivec3( 3, 11,  9), ivec3(11, 10,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  8, 10), ivec3(10,  8, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  7,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  3,  0), ivec3( 7,  3,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  1,  9), ivec3( 8,  4,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  1,  9), ivec3( 4,  7,  1), ivec3( 7,  3,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2, 10), ivec3( 8,  4,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  4,  7), ivec3( 3,  0,  4), ivec3( 1,  2, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  2, 10), ivec3( 9,  0,  2), ivec3( 8,  4,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2, 10,  9), ivec3( 2,  9,  7), ivec3( 2,  7,  3), ivec3( 7,  9,  4), ivec3(-1, -1, -1) },
    { ivec3( 8,  4,  7), ivec3( 3, 11,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(11,  4,  7), ivec3(11,  2,  4), ivec3( 2,  0,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  0,  1), ivec3( 8,  4,  7), ivec3( 2,  3, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  7, 11), ivec3( 9,  4, 11), ivec3( 9, 11,  2), ivec3( 9,  2,  1), ivec3(-1, -1, -1) },
    { ivec3( 3, 10,  1), ivec3( 3, 11, 10), ivec3( 7,  8,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1, 11, 10), ivec3( 1,  4, 11), ivec3( 1,  0,  4), ivec3( 7, 11,  4), ivec3(-1, -1, -1) },
    { ivec3( 4,  7,  8), ivec3( 9,  0, 11), ivec3( 9, 11, 10), ivec3(11,  0,  3), ivec3(-1, -1, -1) },
    { ivec3( 4,  7, 11), ivec3( 4, 11,  9), ivec3( 9, 11, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  5,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  5,  4), ivec3( 0,  8,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  5,  4), ivec3( 1,  5,  0), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 8,  5,  4), ivec3( 8,  3,  5), ivec3( 3,  1,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2, 10), ivec3( 9,  5,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  0,  8), ivec3( 1,  2, 10), ivec3( 4,  9,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5,  2, 10), ivec3( 5,  4,  2), ivec3( 4,  0,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2, 10,  5), ivec3( 3,  2,  5), ivec3( 3,  5,  4), ivec3( 3,  4,  8), ivec3(-1, -1, -1) },
    { ivec3( 9,  5,  4), ivec3( 2,  3, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0, 11,  2), ivec3( 0,  8, 11), ivec3( 4,  9,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  5,  4), ivec3( 0,  1,  5), ivec3( 2,  3, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2,  1,  5), ivec3( 2,  5,  8), ivec3( 2,  8, 11), ivec3( 4,  8,  5), ivec3(-1, -1, -1) },
    { ivec3(10,  3, 11), ivec3(10,  1,  3), ivec3( 9,  5,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  9,  5), ivec3( 0,  8,  1), ivec3( 8, 10,  1), ivec3( 8, 11, 10), ivec3(-1, -1, -1) },
    { ivec3( 5,  4,  0), ivec3( 5,  0, 11), ivec3( 5, 11, 10), ivec3(11,  0,  3), ivec3(-1, -1, -1) },
    { ivec3( 5,  4,  8), ivec3( 5,  8, 10), ivec3(10,  8, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  7,  8), ivec3( 5,  7,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  3,  0), ivec3( 9,  5,  3), ivec3( 5,  7,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  7,  8), ivec3( 0,  1,  7), ivec3( 1,  5,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  5,  3), ivec3( 3,  5,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  7,  8), ivec3( 9,  5,  7), ivec3(10,  1,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  1,  2), ivec3( 9,  5,  0), ivec3( 5,  3,  0), ivec3( 5,  7,  3), ivec3(-1, -1, -1) },
    { ivec3( 8,  0,  2), ivec3( 8,  2,  5), ivec3( 8,  5,  7), ivec3(10,  5,  2), ivec3(-1, -1, -1) },
    { ivec3( 2, 10,  5), ivec3( 2,  5,  3), ivec3( 3,  5,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 7,  9,  5), ivec3( 7,  8,  9), ivec3( 3, 11,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  5,  7), ivec3( 9,  7,  2), ivec3( 9,  2,  0), ivec3( 2,  7, 11), ivec3(-1, -1, -1) },
    { ivec3( 2,  3, 11), ivec3( 0,  1,  8), ivec3( 1,  7,  8), ivec3( 1,  5,  7), ivec3(-1, -1, -1) },
    { ivec3(11,  2,  1), ivec3(11,  1,  7), ivec3( 7,  1,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  5,  8), ivec3( 8,  5,  7), ivec3(10,  1,  3), ivec3(10,  3, 11), ivec3(-1, -1, -1) },
    { ivec3( 5,  7,  0), ivec3( 5,  0,  9), ivec3( 7, 11,  0), ivec3( 1,  0, 10), ivec3(11, 10,  0) },
    { ivec3(11, 10,  0), ivec3(11,  0,  3), ivec3(10,  5,  0), ivec3( 8,  0,  7), ivec3( 5,  7,  0) },
    { ivec3(11, 10,  5), ivec3( 7, 11,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  6,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8,  3), ivec3( 5, 10,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  0,  1), ivec3( 5, 10,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  8,  3), ivec3( 1,  9,  8), ivec3( 5, 10,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  6,  5), ivec3( 2,  6,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  6,  5), ivec3( 1,  2,  6), ivec3( 3,  0,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  6,  5), ivec3( 9,  0,  6), ivec3( 0,  2,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5,  9,  8), ivec3( 5,  8,  2), ivec3( 5,  2,  6), ivec3( 3,  2,  8), ivec3(-1, -1, -1) },
    { ivec3( 2,  3, 11), ivec3(10,  6,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(11,  0,  8), ivec3(11,  2,  0), ivec3(10,  6,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  1,  9), ivec3( 2,  3, 11), ivec3( 5, 10,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5, 10,  6), ivec3( 1,  9,  2), ivec3( 9, 11,  2), ivec3( 9,  8, 11), ivec3(-1, -1, -1) },
    { ivec3( 6,  3, 11), ivec3( 6,  5,  3), ivec3( 5,  1,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8, 11), ivec3( 0, 11,  5), ivec3( 0,  5,  1), ivec3( 5, 11,  6), ivec3(-1, -1, -1) },
    { ivec3( 3, 11,  6), ivec3( 0,  3,  6), ivec3( 0,  6,  5), ivec3( 0,  5,  9), ivec3(-1, -1, -1) },
    { ivec3( 6,  5,  9), ivec3( 6,  9, 11), ivec3(11,  9,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5, 10,  6), ivec3( 4,  7,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  3,  0), ivec3( 4,  7,  3), ivec3( 6,  5, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  9,  0), ivec3( 5, 10,  6), ivec3( 8,  4,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  6,  5), ivec3( 1,  9,  7), ivec3( 1,  7,  3), ivec3( 7,  9,  4), ivec3(-1, -1, -1) },
    { ivec3( 6,  1,  2), ivec3( 6,  5,  1), ivec3( 4,  7,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2,  5), ivec3( 5,  2,  6), ivec3( 3,  0,  4), ivec3( 3,  4,  7), ivec3(-1, -1, -1) },
    { ivec3( 8,  4,  7), ivec3( 9,  0,  5), ivec3( 0,  6,  5), ivec3( 0,  2,  6), ivec3(-1, -1, -1) },
    { ivec3( 7,  3,  9), ivec3( 7,  9,  4), ivec3( 3,  2,  9), ivec3( 5,  9,  6), ivec3( 2,  6,  9) },
    { ivec3( 3, 11,  2), ivec3( 7,  8,  4), ivec3(10,  6,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5, 10,  6), ivec3( 4,  7,  2), ivec3( 4,  2,  0), ivec3( 2,  7, 11), ivec3(-1, -1, -1) },
    { ivec3( 0,  1,  9), ivec3( 4,  7,  8), ivec3( 2,  3, 11), ivec3( 5, 10,  6), ivec3(-1, -1, -1) },
    { ivec3( 9,  2,  1), ivec3( 9, 11,  2), ivec3( 9,  4, 11), ivec3( 7, 11,  4), ivec3( 5, 10,  6) },
    { ivec3( 8,  4,  7), ivec3( 3, 11,  5), ivec3( 3,  5,  1), ivec3( 5, 11,  6), ivec3(-1, -1, -1) },
    { ivec3( 5,  1, 11), ivec3( 5, 11,  6), ivec3( 1,  0, 11), ivec3( 7, 11,  4), ivec3( 0,  4, 11) },
    { ivec3( 0,  5,  9), ivec3( 0,  6,  5), ivec3( 0,  3,  6), ivec3(11,  6,  3), ivec3( 8,  4,  7) },
    { ivec3( 6,  5,  9), ivec3( 6,  9, 11), ivec3( 4,  7,  9), ivec3( 7, 11,  9), ivec3(-1, -1, -1) },
    { ivec3(10,  4,  9), ivec3( 6,  4, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4, 10,  6), ivec3( 4,  9, 10), ivec3( 0,  8,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  0,  1), ivec3(10,  6,  0), ivec3( 6,  4,  0), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 8,  3,  1), ivec3( 8,  1,  6), ivec3( 8,  6,  4), ivec3( 6,  1, 10), ivec3(-1, -1, -1) },
    { ivec3( 1,  4,  9), ivec3( 1,  2,  4), ivec3( 2,  6,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  0,  8), ivec3( 1,  2,  9), ivec3( 2,  4,  9), ivec3( 2,  6,  4), ivec3(-1, -1, -1) },
    { ivec3( 0,  2,  4), ivec3( 4,  2,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 8,  3,  2), ivec3( 8,  2,  4), ivec3( 4,  2,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  4,  9), ivec3(10,  6,  4), ivec3(11,  2,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8,  2), ivec3( 2,  8, 11), ivec3( 4,  9, 10), ivec3( 4, 10,  6), ivec3(-1, -1, -1) },
    { ivec3( 3, 11,  2), ivec3( 0,  1,  6), ivec3( 0,  6,  4), ivec3( 6,  1, 10), ivec3(-1, -1, -1) },
    { ivec3( 6,  4,  1), ivec3( 6,  1, 10), ivec3( 4,  8,  1), ivec3( 2,  1, 11), ivec3( 8, 11,  1) },
    { ivec3( 9,  6,  4), ivec3( 9,  3,  6), ivec3( 9,  1,  3), ivec3(11,  6,  3), ivec3(-1, -1, -1) },
    { ivec3( 8, 11,  1), ivec3( 8,  1,  0), ivec3(11,  6,  1), ivec3( 9,  1,  4), ivec3( 6,  4,  1) },
    { ivec3( 3, 11,  6), ivec3( 3,  6,  0), ivec3( 0,  6,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 6,  4,  8), ivec3(11,  6,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 7, 10,  6), ivec3( 7,  8, 10), ivec3( 8,  9, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  7,  3), ivec3( 0, 10,  7), ivec3( 0,  9, 10), ivec3( 6,  7, 10), ivec3(-1, -1, -1) },
    { ivec3(10,  6,  7), ivec3( 1, 10,  7), ivec3( 1,  7,  8), ivec3( 1,  8,  0), ivec3(-1, -1, -1) },
    { ivec3(10,  6,  7), ivec3(10,  7,  1), ivec3( 1,  7,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2,  6), ivec3( 1,  6,  8), ivec3( 1,  8,  9), ivec3( 8,  6,  7), ivec3(-1, -1, -1) },
    { ivec3( 2,  6,  9), ivec3( 2,  9,  1), ivec3( 6,  7,  9), ivec3( 0,  9,  3), ivec3( 7,  3,  9) },
    { ivec3( 7,  8,  0), ivec3( 7,  0,  6), ivec3( 6,  0,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 7,  3,  2), ivec3( 6,  7,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2,  3, 11), ivec3(10,  6,  8), ivec3(10,  8,  9), ivec3( 8,  6,  7), ivec3(-1, -1, -1) },
    { ivec3( 2,  0,  7), ivec3( 2,  7, 11), ivec3( 0,  9,  7), ivec3( 6,  7, 10), ivec3( 9, 10,  7) },
    { ivec3( 1,  8,  0), ivec3( 1,  7,  8), ivec3( 1, 10,  7), ivec3( 6,  7, 10), ivec3( 2,  3, 11) },
    { ivec3(11,  2,  1), ivec3(11,  1,  7), ivec3(10,  6,  1), ivec3( 6,  7,  1), ivec3(-1, -1, -1) },
    { ivec3( 8,  9,  6), ivec3( 8,  6,  7), ivec3( 9,  1,  6), ivec3(11,  6,  3), ivec3( 1,  3,  6) },
    { ivec3( 0,  9,  1), ivec3(11,  6,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 7,  8,  0), ivec3( 7,  0,  6), ivec3( 3, 11,  0), ivec3(11,  6,  0), ivec3(-1, -1, -1) },
    { ivec3( 7, 11,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 7,  6, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  0,  8), ivec3(11,  7,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  1,  9), ivec3(11,  7,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 8,  1,  9), ivec3( 8,  3,  1), ivec3(11,  7,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  1,  2), ivec3( 6, 11,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2, 10), ivec3( 3,  0,  8), ivec3( 6, 11,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2,  9,  0), ivec3( 2, 10,  9), ivec3( 6, 11,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 6, 11,  7), ivec3( 2, 10,  3), ivec3(10,  8,  3), ivec3(10,  9,  8), ivec3(-1, -1, -1) },
    { ivec3( 7,  2,  3), ivec3( 6,  2,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 7,  0,  8), ivec3( 7,  6,  0), ivec3( 6,  2,  0), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2,  7,  6), ivec3( 2,  3,  7), ivec3( 0,  1,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  6,  2), ivec3( 1,  8,  6), ivec3( 1,  9,  8), ivec3( 8,  7,  6), ivec3(-1, -1, -1) },
    { ivec3(10,  7,  6), ivec3(10,  1,  7), ivec3( 1,  3,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  7,  6), ivec3( 1,  7, 10), ivec3( 1,  8,  7), ivec3( 1,  0,  8), ivec3(-1, -1, -1) },
    { ivec3( 0,  3,  7), ivec3( 0,  7, 10), ivec3( 0, 10,  9), ivec3( 6, 10,  7), ivec3(-1, -1, -1) },
    { ivec3( 7,  6, 10), ivec3( 7, 10,  8), ivec3( 8, 10,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 6,  8,  4), ivec3(11,  8,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  6, 11), ivec3( 3,  0,  6), ivec3( 0,  4,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 8,  6, 11), ivec3( 8,  4,  6), ivec3( 9,  0,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  4,  6), ivec3( 9,  6,  3), ivec3( 9,  3,  1), ivec3(11,  3,  6), ivec3(-1, -1, -1) },
    { ivec3( 6,  8,  4), ivec3( 6, 11,  8), ivec3( 2, 10,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2, 10), ivec3( 3,  0, 11), ivec3( 0,  6, 11), ivec3( 0,  4,  6), ivec3(-1, -1, -1) },
    { ivec3( 4, 11,  8), ivec3( 4,  6, 11), ivec3( 0,  2,  9), ivec3( 2, 10,  9), ivec3(-1, -1, -1) },
    { ivec3(10,  9,  3), ivec3(10,  3,  2), ivec3( 9,  4,  3), ivec3(11,  3,  6), ivec3( 4,  6,  3) },
    { ivec3( 8,  2,  3), ivec3( 8,  4,  2), ivec3( 4,  6,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  4,  2), ivec3( 4,  6,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  9,  0), ivec3( 2,  3,  4), ivec3( 2,  4,  6), ivec3( 4,  3,  8), ivec3(-1, -1, -1) },
    { ivec3( 1,  9,  4), ivec3( 1,  4,  2), ivec3( 2,  4,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 8,  1,  3), ivec3( 8,  6,  1), ivec3( 8,  4,  6), ivec3( 6, 10,  1), ivec3(-1, -1, -1) },
    { ivec3(10,  1,  0), ivec3(10,  0,  6), ivec3( 6,  0,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  6,  3), ivec3( 4,  3,  8), ivec3( 6, 10,  3), ivec3( 0,  3,  9), ivec3(10,  9,  3) },
    { ivec3(10,  9,  4), ivec3( 6, 10,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  9,  5), ivec3( 7,  6, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8,  3), ivec3( 4,  9,  5), ivec3(11,  7,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5,  0,  1), ivec3( 5,  4,  0), ivec3( 7,  6, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(11,  7,  6), ivec3( 8,  3,  4), ivec3( 3,  5,  4), ivec3( 3,  1,  5), ivec3(-1, -1, -1) },
    { ivec3( 9,  5,  4), ivec3(10,  1,  2), ivec3( 7,  6, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 6, 11,  7), ivec3( 1,  2, 10), ivec3( 0,  8,  3), ivec3( 4,  9,  5), ivec3(-1, -1, -1) },
    { ivec3( 7,  6, 11), ivec3( 5,  4, 10), ivec3( 4,  2, 10), ivec3( 4,  0,  2), ivec3(-1, -1, -1) },
    { ivec3( 3,  4,  8), ivec3( 3,  5,  4), ivec3( 3,  2,  5), ivec3(10,  5,  2), ivec3(11,  7,  6) },
    { ivec3( 7,  2,  3), ivec3( 7,  6,  2), ivec3( 5,  4,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  5,  4), ivec3( 0,  8,  6), ivec3( 0,  6,  2), ivec3( 6,  8,  7), ivec3(-1, -1, -1) },
    { ivec3( 3,  6,  2), ivec3( 3,  7,  6), ivec3( 1,  5,  0), ivec3( 5,  4,  0), ivec3(-1, -1, -1) },
    { ivec3( 6,  2,  8), ivec3( 6,  8,  7), ivec3( 2,  1,  8), ivec3( 4,  8,  5), ivec3( 1,  5,  8) },
    { ivec3( 9,  5,  4), ivec3(10,  1,  6), ivec3( 1,  7,  6), ivec3( 1,  3,  7), ivec3(-1, -1, -1) },
    { ivec3( 1,  6, 10), ivec3( 1,  7,  6), ivec3( 1,  0,  7), ivec3( 8,  7,  0), ivec3( 9,  5,  4) },
    { ivec3( 4,  0, 10), ivec3( 4, 10,  5), ivec3( 0,  3, 10), ivec3( 6, 10,  7), ivec3( 3,  7, 10) },
    { ivec3( 7,  6, 10), ivec3( 7, 10,  8), ivec3( 5,  4, 10), ivec3( 4,  8, 10), ivec3(-1, -1, -1) },
    { ivec3( 6,  9,  5), ivec3( 6, 11,  9), ivec3(11,  8,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  6, 11), ivec3( 0,  6,  3), ivec3( 0,  5,  6), ivec3( 0,  9,  5), ivec3(-1, -1, -1) },
    { ivec3( 0, 11,  8), ivec3( 0,  5, 11), ivec3( 0,  1,  5), ivec3( 5,  6, 11), ivec3(-1, -1, -1) },
    { ivec3( 6, 11,  3), ivec3( 6,  3,  5), ivec3( 5,  3,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2, 10), ivec3( 9,  5, 11), ivec3( 9, 11,  8), ivec3(11,  5,  6), ivec3(-1, -1, -1) },
    { ivec3( 0, 11,  3), ivec3( 0,  6, 11), ivec3( 0,  9,  6), ivec3( 5,  6,  9), ivec3( 1,  2, 10) },
    { ivec3(11,  8,  5), ivec3(11,  5,  6), ivec3( 8,  0,  5), ivec3(10,  5,  2), ivec3( 0,  2,  5) },
    { ivec3( 6, 11,  3), ivec3( 6,  3,  5), ivec3( 2, 10,  3), ivec3(10,  5,  3), ivec3(-1, -1, -1) },
    { ivec3( 5,  8,  9), ivec3( 5,  2,  8), ivec3( 5,  6,  2), ivec3( 3,  8,  2), ivec3(-1, -1, -1) },
    { ivec3( 9,  5,  6), ivec3( 9,  6,  0), ivec3( 0,  6,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  5,  8), ivec3( 1,  8,  0), ivec3( 5,  6,  8), ivec3( 3,  8,  2), ivec3( 6,  2,  8) },
    { ivec3( 1,  5,  6), ivec3( 2,  1,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  3,  6), ivec3( 1,  6, 10), ivec3( 3,  8,  6), ivec3( 5,  6,  9), ivec3( 8,  9,  6) },
    { ivec3(10,  1,  0), ivec3(10,  0,  6), ivec3( 9,  5,  0), ivec3( 5,  6,  0), ivec3(-1, -1, -1) },
    { ivec3( 0,  3,  8), ivec3( 5,  6, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  5,  6), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(11,  5, 10), ivec3( 7,  5, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(11,  5, 10), ivec3(11,  7,  5), ivec3( 8,  3,  0), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5, 11,  7), ivec3( 5, 10, 11), ivec3( 1,  9,  0), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(10,  7,  5), ivec3(10, 11,  7), ivec3( 9,  8,  1), ivec3( 8,  3,  1), ivec3(-1, -1, -1) },
    { ivec3(11,  1,  2), ivec3(11,  7,  1), ivec3( 7,  5,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8,  3), ivec3( 1,  2,  7), ivec3( 1,  7,  5), ivec3( 7,  2, 11), ivec3(-1, -1, -1) },
    { ivec3( 9,  7,  5), ivec3( 9,  2,  7), ivec3( 9,  0,  2), ivec3( 2, 11,  7), ivec3(-1, -1, -1) },
    { ivec3( 7,  5,  2), ivec3( 7,  2, 11), ivec3( 5,  9,  2), ivec3( 3,  2,  8), ivec3( 9,  8,  2) },
    { ivec3( 2,  5, 10), ivec3( 2,  3,  5), ivec3( 3,  7,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 8,  2,  0), ivec3( 8,  5,  2), ivec3( 8,  7,  5), ivec3(10,  2,  5), ivec3(-1, -1, -1) },
    { ivec3( 9,  0,  1), ivec3( 5, 10,  3), ivec3( 5,  3,  7), ivec3( 3, 10,  2), ivec3(-1, -1, -1) },
    { ivec3( 9,  8,  2), ivec3( 9,  2,  1), ivec3( 8,  7,  2), ivec3(10,  2,  5), ivec3( 7,  5,  2) },
    { ivec3( 1,  3,  5), ivec3( 3,  7,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8,  7), ivec3( 0,  7,  1), ivec3( 1,  7,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  0,  3), ivec3( 9,  3,  5), ivec3( 5,  3,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9,  8,  7), ivec3( 5,  9,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5,  8,  4), ivec3( 5, 10,  8), ivec3(10, 11,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 5,  0,  4), ivec3( 5, 11,  0), ivec3( 5, 10, 11), ivec3(11,  3,  0), ivec3(-1, -1, -1) },
    { ivec3( 0,  1,  9), ivec3( 8,  4, 10), ivec3( 8, 10, 11), ivec3(10,  4,  5), ivec3(-1, -1, -1) },
    { ivec3(10, 11,  4), ivec3(10,  4,  5), ivec3(11,  3,  4), ivec3( 9,  4,  1), ivec3( 3,  1,  4) },
    { ivec3( 2,  5,  1), ivec3( 2,  8,  5), ivec3( 2, 11,  8), ivec3( 4,  5,  8), ivec3(-1, -1, -1) },
    { ivec3( 0,  4, 11), ivec3( 0, 11,  3), ivec3( 4,  5, 11), ivec3( 2, 11,  1), ivec3( 5,  1, 11) },
    { ivec3( 0,  2,  5), ivec3( 0,  5,  9), ivec3( 2, 11,  5), ivec3( 4,  5,  8), ivec3(11,  8,  5) },
    { ivec3( 9,  4,  5), ivec3( 2, 11,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2,  5, 10), ivec3( 3,  5,  2), ivec3( 3,  4,  5), ivec3( 3,  8,  4), ivec3(-1, -1, -1) },
    { ivec3( 5, 10,  2), ivec3( 5,  2,  4), ivec3( 4,  2,  0), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3, 10,  2), ivec3( 3,  5, 10), ivec3( 3,  8,  5), ivec3( 4,  5,  8), ivec3( 0,  1,  9) },
    { ivec3( 5, 10,  2), ivec3( 5,  2,  4), ivec3( 1,  9,  2), ivec3( 9,  4,  2), ivec3(-1, -1, -1) },
    { ivec3( 8,  4,  5), ivec3( 8,  5,  3), ivec3( 3,  5,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  4,  5), ivec3( 1,  0,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 8,  4,  5), ivec3( 8,  5,  3), ivec3( 9,  0,  5), ivec3( 0,  3,  5), ivec3(-1, -1, -1) },
    { ivec3( 9,  4,  5), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4, 11,  7), ivec3( 4,  9, 11), ivec3( 9, 10, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  8,  3), ivec3( 4,  9,  7), ivec3( 9, 11,  7), ivec3( 9, 10, 11), ivec3(-1, -1, -1) },
    { ivec3( 1, 10, 11), ivec3( 1, 11,  4), ivec3( 1,  4,  0), ivec3( 7,  4, 11), ivec3(-1, -1, -1) },
    { ivec3( 3,  1,  4), ivec3( 3,  4,  8), ivec3( 1, 10,  4), ivec3( 7,  4, 11), ivec3(10, 11,  4) },
    { ivec3( 4, 11,  7), ivec3( 9, 11,  4), ivec3( 9,  2, 11), ivec3( 9,  1,  2), ivec3(-1, -1, -1) },
    { ivec3( 9,  7,  4), ivec3( 9, 11,  7), ivec3( 9,  1, 11), ivec3( 2, 11,  1), ivec3( 0,  8,  3) },
    { ivec3(11,  7,  4), ivec3(11,  4,  2), ivec3( 2,  4,  0), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(11,  7,  4), ivec3(11,  4,  2), ivec3( 8,  3,  4), ivec3( 3,  2,  4), ivec3(-1, -1, -1) },
    { ivec3( 2,  9, 10), ivec3( 2,  7,  9), ivec3( 2,  3,  7), ivec3( 7,  4,  9), ivec3(-1, -1, -1) },
    { ivec3( 9, 10,  7), ivec3( 9,  7,  4), ivec3(10,  2,  7), ivec3( 8,  7,  0), ivec3( 2,  0,  7) },
    { ivec3( 3,  7, 10), ivec3( 3, 10,  2), ivec3( 7,  4, 10), ivec3( 1, 10,  0), ivec3( 4,  0, 10) },
    { ivec3( 1, 10,  2), ivec3( 8,  7,  4), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  9,  1), ivec3( 4,  1,  7), ivec3( 7,  1,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  9,  1), ivec3( 4,  1,  7), ivec3( 0,  8,  1), ivec3( 8,  7,  1), ivec3(-1, -1, -1) },
    { ivec3( 4,  0,  3), ivec3( 7,  4,  3), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 4,  8,  7), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9, 10,  8), ivec3(10, 11,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  0,  9), ivec3( 3,  9, 11), ivec3(11,  9, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  1, 10), ivec3( 0, 10,  8), ivec3( 8, 10, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  1, 10), ivec3(11,  3, 10), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  2, 11), ivec3( 1, 11,  9), ivec3( 9, 11,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  0,  9), ivec3( 3,  9, 11), ivec3( 1,  2,  9), ivec3( 2, 11,  9), ivec3(-1, -1, -1) },
    { ivec3( 0,  2, 11), ivec3( 8,  0, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 3,  2, 11), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2,  3,  8), ivec3( 2,  8, 10), ivec3(10,  8,  9), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 9, 10,  2), ivec3( 0,  9,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 2,  3,  8), ivec3( 2,  8, 10), ivec3( 0,  1,  8), ivec3( 1, 10,  8), ivec3(-1, -1, -1) },
    { ivec3( 1, 10,  2), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 1,  3,  8), ivec3( 9,  1,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  9,  1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3( 0,  3,  8), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
    { ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1), ivec3(-1, -1, -1) },
};

// 32 random rays on sphere with Poisson distribution, table from Ryan Geiss.
vec3 random_rays[32] = {
    vec3( 0.286582,  0.257763, -0.922729),
    vec3(-0.171812, -0.888079,  0.426375),
    vec3( 0.440764, -0.502089, -0.744066),
    vec3(-0.841007, -0.428818, -0.329882),
    vec3(-0.380213, -0.588038, -0.713898),
    vec3(-0.055393, -0.207160, -0.976738),
    vec3(-0.901510, -0.077811,  0.425706),
    vec3(-0.974593,  0.123830, -0.186643),
    vec3( 0.208042, -0.524280,  0.825741),
    vec3( 0.258429, -0.898570, -0.354663),
    vec3(-0.262118,  0.574475, -0.775418),
    vec3( 0.735212,  0.551820,  0.393646),
    vec3( 0.828700, -0.523923, -0.196877),
    vec3( 0.788742,  0.005727, -0.614698),
    vec3(-0.696885,  0.649338, -0.304486),
    vec3(-0.625313,  0.082413, -0.776010),
    vec3( 0.358696,  0.928723,  0.093864),
    vec3( 0.188264,  0.628978,  0.754283),
    vec3(-0.495193,  0.294596,  0.817311),
    vec3( 0.818889,  0.508670, -0.265851),
    vec3( 0.027189,  0.057757,  0.997960),
    vec3(-0.188421,  0.961802, -0.198582),
    vec3( 0.995439,  0.019982,  0.093282),
    vec3(-0.315254, -0.925345, -0.210596),
    vec3( 0.411992, -0.877706,  0.244733),
    vec3( 0.625857,  0.080059,  0.775818),
    vec3(-0.243839,  0.866185,  0.436194),
    vec3(-0.725464, -0.643645,  0.243768),
    vec3( 0.766785, -0.430702,  0.475959),
    vec3(-0.446376, -0.391664,  0.804580),
    vec3(-0.761557,  0.562508,  0.321895),
    vec3( 0.344460,  0.753223, -0.560359)
};

float density(vec3 coord)
{
    // Should be block_size, not block_resolution even though we're sampling
    // from a texture that's block_resolution x block_resolution.
    // The reason is that if resolution = 2, then we want (0, 1) / 1 = (0, 1)
    return texture(density_map, coord / block_size).x;
}
