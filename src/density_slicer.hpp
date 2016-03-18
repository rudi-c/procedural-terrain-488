#pragma once

#include <string>

#include "cs488-framework/ShaderProgram.hpp"
#include "constants.hpp"
#include "plane.hpp"

class DensitySlicer
{
public:
    DensitySlicer();

    void init(std::string dir);
    void draw(glm::mat4 P, glm::mat4 V, glm::mat4 M, float period);

	GLint P_uni;    // Uniform location for Projection matrix.
	GLint V_uni;    // Uniform location for View matrix.
	GLint M_uni;    // Uniform location for Model matrix.

private:
    ShaderProgram density_shader;

    Plane xy_grid;
    Plane yz_grid;
    Plane xz_grid;
    GLint period_uni;
};