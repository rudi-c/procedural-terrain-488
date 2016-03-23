#version 430

layout(points) in;
layout(points, max_vertices = 5) out;

layout(binding = 0) uniform sampler3D density_map;

uniform int block_size;

out uint z6_y6_x6_edge1_edge2_edge3;

#include "marching_cubes_common.h"

void main() {
    vec3 coords = vec3(gl_in[0].gl_Position);

    ivec3 icoords = ivec3(coords);

    // Use swizzling, avoid typing vector constructors for each corner.
    vec2 offset = vec2(0, 1);

    // More efficient to do vector operations.
    vec4 density0123;
    vec4 density4567;

    density0123.x = density(coords + offset.xxx);
    density0123.y = density(coords + offset.xyx);
    density0123.z = density(coords + offset.yyx);
    density0123.w = density(coords + offset.yxx);
    density4567.x = density(coords + offset.xxy);
    density4567.y = density(coords + offset.xyy);
    density4567.z = density(coords + offset.yyy);
    density4567.w = density(coords + offset.yxy);

    vec4 divider = vec4(0, 0, 0, 0);
    ivec4 ground0123 = ivec4(lessThan(divider, density0123));
    ivec4 ground4567 = ivec4(lessThan(divider, density4567));

    int case_index = (ground0123.x << 0) | (ground0123.y << 1) | (ground0123.z << 2) | (ground0123.w << 3) |
                     (ground4567.x << 4) | (ground4567.y << 5) | (ground4567.z << 6) | (ground4567.w << 7);
    int numpolys = case_to_numpolys[case_index];

    for (int i = 0; i < numpolys; i++) {
        ivec3 edge_index = edge_connect_list[case_index][i];

        // [6 bits: z] [6 bits: y] [6 bits: x]
        // [4 bits: edge1] [4 bits: edge2] [4 bits: edge3]
        uint packed_triangle = (edge_index.x << 0) | (edge_index.y << 4) | (edge_index.z << 8)
                             | (icoords.x << 12)   | (icoords.y << 18)   | (icoords.z << 24);

        z6_y6_x6_edge1_edge2_edge3 = packed_triangle;

        EmitVertex();
        EndPrimitive();
    }
}
