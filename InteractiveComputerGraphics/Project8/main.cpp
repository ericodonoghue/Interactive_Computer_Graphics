#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <stdlib.h>

#include "lodepng.h"
#include "cyCore.h"
#include "cyVector.h"
#include "cyMatrix.h"
#include "cyTriMesh.h"
#include "cyGL.h"

#pragma region globals 

GLfloat display_width = 800;
GLfloat display_height = 600;

// rotation and zoom with mouse
float t_z = -100;
float r_x = 0;
float r_y = 0;

// keeps track of the previous position of the mouse for zoom/rotation direction
int pzy = 0;
int prx = 0;
int pry = 0;
bool alt_down = false;
int mouse_button = -1;

// plane variables
cyGLSLProgram plane_program;
GLuint plane_program_id;
GLuint plane_vao;
GLuint plane_mvp;
GLuint plane_mv;
GLuint plane_tess_level;
GLuint norm_tex;
GLuint disp_tex;
int num_v; // number of vertices in given obj
int num_f; // number of faces in given obj

// wireframe variables
cyGLSLProgram tess_program;
GLuint tess_program_id;
GLuint tess_vao;
GLuint tess_mvp;
GLuint tess_displacement;
GLuint tess_tess_level;
int tess_level = 20;
int displace = 0;
bool wireframe = true;


// forward declarations
void InitializeGLUT(int argc, char* argv[]);
void CompileProgram();
GLuint* BuildTessBuffers();
GLuint* BuildPlaneBuffers();
void SetupTextures(char* argv[]);
void SetUniforms();
cyMatrix4f GetModelViewProjection();
cyMatrix4f GetTessModelViewProjection();
cyMatrix4f GetModelView();

#pragma endregion globals 

#pragma region GLUT_Functions

void resize(int w, int h) {
	display_width = w;
	display_height = h;
	glViewport(0, 0, w, h);
}

void draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	SetUniforms();

	// render plane
	glUseProgram(plane_program_id);

	GLuint loc = glGetUniformLocation(plane_program_id, "norm_tex");
	glUniform1i(loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, norm_tex);

	glBindVertexArray(plane_vao);

	if (!displace) {
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	else {
		GLuint loc = glGetUniformLocation(plane_program_id, "disp_tex");
		glUniform1i(loc, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, disp_tex);

		glPatchParameteri(GL_PATCH_VERTICES, 4);
		glDrawArrays(GL_PATCHES, 0, 4);
	}

	if (wireframe) {
		// render wireframe
		glUseProgram(tess_program_id);

		glBindVertexArray(tess_vao);

		if (!displace) {
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		else {
			GLuint loc = glGetUniformLocation(tess_program_id, "tex");
			glUniform1i(loc, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, disp_tex);

			glPatchParameteri(GL_PATCH_VERTICES, 4);
			glDrawArrays(GL_PATCHES, 0, 4);
		}
	}

	//Swap buffers
	glutSwapBuffers();
}

void idle() {
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 32:
		wireframe = !wireframe;
		break;
	case 27:
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay();
}

void special_keyboard(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_RIGHT:
		if (tess_level < 65) {
			tess_level += 2;
		}
		break;
	case GLUT_KEY_LEFT:
		if (tess_level > 2) {
			tess_level -= 2;
		}
		break;
	}
	glutPostRedisplay();
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
	//displace = 0;
	CompileProgram();
	GLuint* plane_vbo = BuildPlaneBuffers();
	GLuint* tess_vbo = BuildTessBuffers();
	SetupTextures(argv);

	// start 
	glutMainLoop();
	
	glDeleteBuffers(1, &plane_vbo[0]);
	glDeleteBuffers(1, &plane_vbo[1]);
	glDeleteBuffers(1, &tess_vbo[0]);
	glDeleteBuffers(1, &tess_vbo[1]);

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
	glutCreateWindow("CS 5610 Project 8");

	// key detection
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special_keyboard);

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

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Error: invalid arguments, expected - 2 or 3");
		exit(1);
	}
	if (argc == 3) {
		displace = 1;
	}
}

void CompileProgram() {
	bool result;
	if (!displace) {
		if (!(result = plane_program.BuildFiles("plane.vert", "plane.frag"))) {
			fprintf(stderr, "Error: plane shader compilation failed");
			exit(1);
		}
		plane_program_id = plane_program.GetID();
		plane_program.Bind();

		if (!(result = tess_program.BuildFiles("tess.vert", "tess.frag", "tess.geom"))) {
			fprintf(stderr, "Error: wireframe shader compilation failed");
			exit(1);
		}
		tess_program_id = tess_program.GetID();
		tess_program.Bind();
	}
	else {
		if (!(result = plane_program.BuildFiles("d_plane.vert", "d_plane.frag", "d_plane.geom", "d_plane.tesc", "d_plane.tese"))) {
			fprintf(stderr, "Error: plane shader compilation failed");
			exit(1);
		}
		plane_program_id = plane_program.GetID();
		plane_program.Bind();

		if (!(result = tess_program.BuildFiles("tess.vert", "tess.frag", "tess.geom", "tess.tesc", "tess.tese"))) {
			fprintf(stderr, "Error: wireframe shader compilation failed");
			exit(1);
		}
		tess_program_id = tess_program.GetID();
		tess_program.Bind();
	}
}

GLuint* BuildPlaneBuffers() {
	GLuint plane_vbo[2];
	glGenVertexArrays(1, &plane_vao);
	glBindVertexArray(plane_vao);
	glGenBuffers(2, plane_vbo);

	if (!displace) {
		GLfloat plane_v[] = { -30, -30,  0, -30, 30,  0, 30, -30,  0, 30, -30,  0, -30, 30,  0, 30,  30,  0, };
		GLfloat plane_t[] = { 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, };

		glBindBuffer(GL_ARRAY_BUFFER, plane_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_v), plane_v, GL_STATIC_DRAW);
		GLuint plane_pos = glGetAttribLocation(plane_program_id, "pos");
		glVertexAttribPointer(plane_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(plane_pos);

		glBindBuffer(GL_ARRAY_BUFFER, plane_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_t), plane_t, GL_STATIC_DRAW);
		GLuint plane_tcx = glGetAttribLocation(plane_program_id, "tcx");
		glVertexAttribPointer(plane_tcx, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(plane_tcx);
	}
	else {
		GLfloat plane_v[] = { -30, -30,  0, 30, -30,  0, 30,  30,  0, -30,  30,  0, };
		GLfloat plane_t[] = { 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, };

		glBindBuffer(GL_ARRAY_BUFFER, plane_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_v), plane_v, GL_STATIC_DRAW);
		GLuint plane_pos = glGetAttribLocation(plane_program_id, "pos");
		glVertexAttribPointer(plane_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(plane_pos);

		glBindBuffer(GL_ARRAY_BUFFER, plane_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_t), plane_t, GL_STATIC_DRAW);
		GLuint plane_tcx = glGetAttribLocation(plane_program_id, "tcx");
		glVertexAttribPointer(plane_tcx, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(plane_tcx);
	}

	glUseProgram(plane_program_id);
	plane_mvp = glGetUniformLocation(plane_program_id, "mvp");
	plane_mv = glGetUniformLocation(plane_program_id, "mv");
	plane_tess_level = glGetUniformLocation(plane_program_id, "tess_level");

	return plane_vbo;
}

GLuint* BuildTessBuffers() {
	GLuint tess_vbo[2];
	glGenVertexArrays(1, &tess_vao);
	glBindVertexArray(tess_vao);
	glGenBuffers(2, tess_vbo);

	if (!displace) {
		GLfloat plane_v[] = { -30, -30,  0, 30, -30,  0, -30,  30,  0, -30,  30,  0, 30, -30,  0,  30,  30,  0, };
		GLfloat plane_t[] = { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, };

		glBindBuffer(GL_ARRAY_BUFFER, tess_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_v), plane_v, GL_STATIC_DRAW);
		GLuint tess_pos = glGetAttribLocation(tess_program_id, "pos");
		glVertexAttribPointer(tess_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(tess_pos);

		glBindBuffer(GL_ARRAY_BUFFER, tess_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_t), plane_t, GL_STATIC_DRAW);
		GLuint tess_tcx = glGetAttribLocation(tess_program_id, "tcx");
		glVertexAttribPointer(tess_tcx, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(tess_tcx);
	}
	else {
		GLfloat plane_v[] = { -30, -30,  0, 30, -30,  0, 30,  30,  0, -30,  30,  0, };
		GLfloat plane_t[] = { 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, };

		glBindBuffer(GL_ARRAY_BUFFER, tess_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_v), plane_v, GL_STATIC_DRAW);
		GLuint tess_pos = glGetAttribLocation(tess_program_id, "pos");
		glVertexAttribPointer(tess_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(tess_pos);

		glBindBuffer(GL_ARRAY_BUFFER, tess_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_t), plane_t, GL_STATIC_DRAW);
		GLuint tess_tcx = glGetAttribLocation(tess_program_id, "tcx");
		glVertexAttribPointer(tess_tcx, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(tess_tcx);
	}

	glUseProgram(tess_program_id);
	tess_mvp = glGetUniformLocation(tess_program_id, "mvp");
	tess_displacement = glGetUniformLocation(tess_program_id, "displacement");
	tess_tess_level = glGetUniformLocation(tess_program_id, "tess_level");

	return tess_vbo;
}

void SetupTextures(char* argv[]) {
	std::string file(argv[1]);
	std::vector<unsigned char> norm_img;
	unsigned width = 512;
	unsigned height = 512;
	unsigned error = lodepng::decode(norm_img, width, height, file);
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	glGenTextures(1, &norm_tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, norm_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &norm_img[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	if (displace) {
		std::string file(argv[2]);
		std::vector<unsigned char> disp_img;
		unsigned error = lodepng::decode(disp_img, width, height, file);
		if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

		glGenTextures(1, &disp_tex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, disp_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &disp_img[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void SetUniforms() {
	float _mvp[16];
	float _tmvp[16];
	float _mv[16];
	GetModelViewProjection().Get(_mvp);
	GetTessModelViewProjection().Get(_tmvp);
	GetModelView().Get(_mv);

	glUseProgram(plane_program_id);
	glUniformMatrix4fv(plane_mvp, 1, false, _mvp);
	glUniformMatrix4fv(plane_mv, 1, false, _mv);
	glUniform1i(plane_tess_level, tess_level);

	glUseProgram(tess_program_id);
	glUniformMatrix4fv(tess_mvp, 1, false, _tmvp);
	glUniform1i(tess_tess_level, tess_level);
	glUniform1i(tess_displacement, displace);
}

cyMatrix4f GetModelViewProjection() {
	// perspective projection matrix values
	float fov = 3.145 * 40.0 / 180.0;
	float aspect = display_width / display_height;
	float n = 0.1f;
	float f = t_z + t_z;

	// generate perspective projection, translation, and rotation matrices and multiply them
	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, 0, t_z))) * cyMatrix4f::RotationXYZ(r_x, r_y, 0);
}

cyMatrix4f GetTessModelViewProjection() {
	// perspective projection matrix values
	float fov = 3.145 * 40.0 / 180.0;
	float aspect = display_width / display_height;
	float n = 0.1f;
	float f = t_z + t_z;

	// generate perspective projection, translation, and rotation matrices and multiply them
	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, 0, t_z + 0.5))) * cyMatrix4f::RotationXYZ(r_x, r_y, 0);
}

cyMatrix4f GetModelView() {
	// generate translation and rotation matrices then multiply them
	return cyMatrix4f::Translation(cyVec3f(0, -5, t_z)) * cyMatrix4f::RotationXYZ(r_x, r_y, 0);
}


#pragma endregion Helper_Functions