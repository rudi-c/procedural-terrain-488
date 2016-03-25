#include "water.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cs488-framework/GlErrorCheck.hpp"

using namespace glm;
using namespace std;

Water::Water()
: water_plane(1.0f)
{
}

void Water::init(string dir)
{
    water_shader.generateProgramObject();
	water_shader.attachVertexShader((dir + "WaterShader.vs").c_str());
	water_shader.attachFragmentShader((dir + "WaterShader.fs").c_str());
	water_shader.link();

	P_uni = water_shader.getUniformLocation("P");
	V_uni = water_shader.getUniformLocation("V");
	M_uni = water_shader.getUniformLocation("M");
	alpha_uni = water_shader.getUniformLocation("alpha");

    water_plane.init(water_shader, translate(mat4(), vec3(0.5f, 0.0f, 0.5f)));

	CHECK_GL_ERRORS;
}

void Water::draw(mat4 P, mat4 V, mat4 M, float alpha)
{
    water_shader.enable();

    glUniformMatrix4fv(P_uni, 1, GL_FALSE, value_ptr(P));
    glUniformMatrix4fv(V_uni, 1, GL_FALSE, value_ptr(V));
    glUniformMatrix4fv(M_uni, 1, GL_FALSE, value_ptr(M));

    glUniform1f(alpha_uni, alpha);

    glBindVertexArray(water_plane.getVertices());
    glDrawArrays(GL_TRIANGLES, 0, 6);

    water_shader.disable();

	CHECK_GL_ERRORS;
}
