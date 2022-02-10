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
int num_f; // number of faces in given obj

// rotation and zoom with mouse
float t_z = -48;
float r_x = -1.5708;
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
cyGLSLProgram program;
GLuint program_id;
GLuint mvp;
GLuint mvn;
GLfloat display_width = 800;
GLfloat display_height = 600;

// forward declarations
void InitializeGLUT(int argc, char* argv[]);
GLuint* BuildVertexBuffer(int argc, const char* filename);
void CompileProgram();
void SetShaderVariables();
cyMatrix4f GetModelViewProjection();
cyMatrix4f GetModelViewNormal();

#pragma endregion globals 

#pragma region GLUT_Functions

void resize(int w, int h) {
	glViewport(0, 0, w, h);
}

void draw() {

	//Clear the viewport
	glEnable(GL_DEPTH_TEST);
	glUseProgram(program_id);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (auto_rot_status) auto_rot += 0.02;

	// calculate and set mvp matrix
	float matrix[16];
	float _mvn[16];
	GetModelViewProjection().Get(matrix);
	GetModelViewNormal().Get(_mvn);
	glUniformMatrix4fv(mvp, 1, false, matrix);
	glUniformMatrix4fv(mvn, 1, false, _mvn);

	// draw
	glDrawArrays(GL_TRIANGLES, 0, num_f * 3 * sizeof(cyVec3f));

	//Swap buffers
	glutSwapBuffers();
}

void idle() {
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
	CompileProgram();
	GLuint* vbo = BuildVertexBuffer(argc, argv[1]);
	SetShaderVariables();
	
	// start 
	glutMainLoop();

	glDeleteBuffers(1, &vbo[0]);
	glDeleteBuffers(1, &vbo[1]);

	return 0;
}

#pragma region Helper_Functions

void InitializeGLUT(int argc, char* argv[]) {
	// initialize GLUT
	glutInit(&argc, argv);

	// Glut window setup
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitContextFlags(GLUT_DEBUG);
	glutInitWindowSize(display_width, display_height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("CS 5610 Project 3");

	// key detection
	glutKeyboardFunc(keyboard);

	// mouse movement/input detection
	glutMouseFunc(mouseInput);
	glutMotionFunc(mouseMotion);

	// display functions initiliazation
	glutReshapeFunc(resize);
	glutDisplayFunc(draw);
	glutIdleFunc(idle);

	// Linking OpenGL
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW error");
		exit(1);
	}

	program_id = glCreateProgram();

}

void CompileProgram() {
	bool result;
	if (!(result = program.BuildFiles("vs.vert", "fs.frag"))) {
		fprintf(stderr, "Error: program compiliation failed");
		exit(1);
	}
	program_id = program.GetID();
	program.Bind();
}

GLuint* BuildVertexBuffer(int argc, const char* filename) {
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
	num_f = reader.NF();

	cyVec3f* v = new cyVec3f[num_f * 3];
	for (int i = 0; i < num_f; i++) {
		cyTriMesh::TriFace f = reader.F(i);
		v[3 * i + 0] = reader.V(f.v[0]);
		v[3 * i + 1] = reader.V(f.v[1]);
		v[3 * i + 2] = reader.V(f.v[2]);
	}

	cyVec3f* n = new cyVec3f[num_f * 3];
	for (int i = 0; i < num_f; i++) {
		cyTriMesh::TriFace fn = reader.FN(i);
		n[3 * i + 0] = reader.VN(fn.v[0]);
		n[3 * i + 1] = reader.VN(fn.v[1]);
		n[3 * i + 2] = reader.VN(fn.v[2]);
	}

	// create and bind vertex buffer object with vertex data
	GLuint vao, vbo[2];
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(2, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
	
	GLuint pos = glGetAttribLocation(program_id, "pos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), n, GL_STATIC_DRAW);

	GLuint norm = glGetAttribLocation(program_id, "norm");
	glEnableVertexAttribArray(norm);
	glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	return vbo;
}

void SetShaderVariables() {
	glUseProgram(program_id);

	// set uniform variables
	mvp = glGetUniformLocation(program_id, "mvp");	
	mvn = glGetUniformLocation(program_id, "mvn");
}

cyMatrix4f GetModelViewProjection() {
	// perspective projection matrix values
	float fov = 3.145 * 40.0 / 180.0;
	float aspect = display_width / display_height;
	float n = 0.1f;
	float f = t_z + t_z;

	// generate perspective projection, translation, and rotation matrices and multiply them
	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, -5, t_z))) * cyMatrix4f::RotationXYZ(r_x, r_y + auto_rot, 0);
}

cyMatrix4f GetModelViewNormal() {
	// generate translation and rotation matrices then multiply them
	return cyMatrix4f::Translation(cyVec3f(0, -5, t_z)) * cyMatrix4f::RotationXYZ(r_x, r_y + auto_rot, 0);
}

#pragma endregion Helper_Functions