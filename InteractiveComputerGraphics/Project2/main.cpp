#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <stdlib.h>

#include "cyCore.h"
#include "cyVector.h"
#include "cyMatrix.h"
#include "cyTriMesh.h"
#include "cyGL.h"

#pragma region globals 
int num_v; // number of vertices in given obj

// rotation and zoom with mouse
float t_z = -48;
float r_x = -1;
float r_y = 0;
int mouse_button = -1;

// keeps track of the previous position of the mouse for zoom/rotation direction
int pzy = 0; 
int prx = 0;
int pry = 0;

// auto rotation
bool auto_rot_status = false;
float auto_rot = 0;

// GL variables
GLuint program_id;
GLuint vs_id;
GLuint fs_id;
GLuint mvp;
GLfloat display_width = 800;
GLfloat display_height = 600;

// forward declarations
cyMatrix4f GetModelViewProjection();
void InitializeGLUT(int argc, char* argv[]);
GLuint BuildVertexBuffer(int argc, const char* filename);
void CompileProgram();
void SetShaderVariables();

#pragma endregion globals 

#pragma region GLUT_Functions

void resize(int w, int h) {
	glViewport(0, 0, w, h);
}

void draw() {

	//Clear the viewport
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program_id);

	if (auto_rot_status) auto_rot += 0.02;

	// calculate and set mvp matrix
	float matrix[16];
	GetModelViewProjection().Get(matrix);
	glUniformMatrix4fv(mvp, 1, false, matrix);

	// draw
	glDrawArrays(GL_POINTS, 0, num_v);

	//Swap buffers
	glutSwapBuffers();

	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'r':
		auto_rot_status = !auto_rot_status;
		break;
	case 27: 
		glutLeaveMainLoop();
		break;
	}
}

void mouseInput(int button, int state, int x, int y) {
	if (state == GLUT_DOWN) mouse_button = button;
	else mouse_button = -1;
}

void mouseMotion(int x, int y) {
	if (mouse_button == GLUT_LEFT_BUTTON) { // handle rotation
		if (pry < y) r_x += 0.02;
		else if (pry > y) r_x -= 0.02;
		pry = y;

		if (prx < x) r_y += 0.02;
		else if (prx > x) r_y -= 0.02;
		prx = x;
	}
	else if (mouse_button == GLUT_RIGHT_BUTTON) { // handle zooming
		if (pzy < y) t_z += 0.15; // zooming in 
		else t_z -= 0.15; // zoomng out
		pzy = y;
	}
	glutPostRedisplay();
}


#pragma endregion GLUT_Functions

int main(int argc, char* argv[]) {

	// set everything up - initialize glut, read obj and fill vertex buffer, compile and link shaders, set uniform and sttribute variables
	InitializeGLUT(argc, argv);
	GLuint vbo_id = BuildVertexBuffer(argc, argv[1]);
	CompileProgram();

	SetShaderVariables();

	// start 
	glutMainLoop();

	glDeleteBuffers(1, &vbo_id);

	return 0;
}

#pragma region Helper_Functions

void InitializeGLUT(int argc, char* argv[]) {
	// initialize GLUT
	glutInit(&argc, argv);

	// Glut window setup
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(display_width, display_height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("CS 5610 Project 2");

	// key detection
	glutKeyboardFunc(keyboard);

	// mouse movement/input detection
	glutMouseFunc(mouseInput);
	glutMotionFunc(mouseMotion);

	// display functions initiliazation
	glutReshapeFunc(resize);
	glutDisplayFunc(draw);

	// Linking OpenGL
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW error");
		exit(1);
	}

	program_id = glCreateProgram();

}

GLuint BuildVertexBuffer(int argc, const char* filename) {
	// input sanitization
	if (argc != 2) {
		fprintf(stderr, "Error: invalid arguments, expected - 2");
		exit(1);
	}

	// read vertex data
	bool r;
	cyTriMesh reader;
	if (!(r = reader.LoadFromFileObj(filename, false))) {
		fprintf(stderr, "Error: cannot read file");
		exit(1);
	}
	num_v = reader.NV();

	// fill in array with vertex data
	std::vector<GLfloat> v;
	int i;
	for (i = 0; i < num_v; i++) {
		cyVec3f _v = reader.V(i);
		v.push_back(_v[0]);
		v.push_back(_v[1]);
		v.push_back(_v[2]);
	}

	// create and bind vertex buffer object with vertex data
	GLuint vbo_id;
	glGenBuffers(1, &vbo_id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
	glBufferData(GL_ARRAY_BUFFER, num_v * sizeof(v), v.data(), GL_STATIC_DRAW);

	return vbo_id;
}

void CompileProgram() {
	cyGLSLShader v;
	cyGLSLShader f;
	bool result;

	if (!(result = v.CompileFile("vs.vert", GL_VERTEX_SHADER))) {
		fprintf(stderr, "Error: vertex shader compilation failed");
		//exit(1);
	}
	if (!(result = f.CompileFile("fs.frag", GL_FRAGMENT_SHADER))) {
		fprintf(stderr, "Error: fragment shader compilation failed");
		//exit(1);
	}

	// attach shaders with ID's
	vs_id = v.GetID();
	glAttachShader(program_id, vs_id);
	fs_id = f.GetID();
	glAttachShader(program_id, fs_id);

	glLinkProgram(program_id);

}

void SetShaderVariables() {
	glUseProgram(program_id);

	// set uniform variables
	mvp = glGetUniformLocation(program_id, "mvp");

	// set attribute variables
	GLuint pos = glGetAttribLocation(program_id, "pos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
}

cyMatrix4f GetModelViewProjection() {
	// perspective projection matrix values
	float fov = 3.145 * 40.0 / 180.0;
	float aspect = display_width / display_height;
	float n = t_z - t_z;
	float f = t_z + t_z;

	// generate perspective projection, translation, and rotation matrices and multiply them
	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, -5, t_z))) * cyMatrix4f::RotationXYZ(r_x, r_y + auto_rot, 0);
}

#pragma endregion Helper_Functions