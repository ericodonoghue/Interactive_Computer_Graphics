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
int num_v; // number of vertices in given obj
int num_f; // number of faces in given obj
unsigned img_width = 512;
unsigned img_height = 512;

// rotation and zoom with mouse
float obj_t_z = -48;
float obj_r_x = -1.5708;
float obj_r_y = 0;

float plane_t_z = -4;
float plane_r_x = -1;
float plane_r_y = 0.5;

// keeps track of the previous position of the mouse for zoom/rotation direction
int pzy = 0;
int prx = 0;
int pry = 0;
bool alt_down = false;
int mouse_button = -1;

// auto rotation
bool auto_rot_status = false;
float auto_rot = 0;

// GL variables
cyGLSLProgram obj_program;
cyGLSLProgram plane_program;
GLuint obj_program_id;
GLuint plane_program_id;
GLuint obj_vao;
GLuint plane_vao;
GLuint obj_mvp;
GLuint obj_mvn;
GLuint obj_tex_id;
GLuint plane_mvp;
GLfloat display_width = 800;
GLfloat display_height = 600;
cyTriMesh reader;

// render to texture
cyGLRenderTexture2D renderBuffer;
int rb_width = 600;
int rb_height = 600;


// forward declarations
void InitializeGLUT(int argc, char* argv[]);
GLuint* BuildObjBuffers(int argc, const char* filename);
GLuint BuildPlaneBuffers();
void CompileProgram();
void InitializeTexture();
void SetShaderVariables();
void BuildRenderBuffer();
cyMatrix4f GetModelViewProjection();
cyMatrix4f GetPlaneModelViewProjection();
cyMatrix4f GetModelViewNormal();

#pragma endregion globals 

#pragma region GLUT_Functions

void resize(int w, int h) {
	display_width = w;
	display_height = h;
	glViewport(0, 0, w, h);
}

void draw() {
	// obj render to texture
	glUseProgram(obj_program_id);
	renderBuffer.Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (auto_rot_status) auto_rot += 0.02;

	// calculate and set mvp/mvn matrix
	float matrix[16];
	float _mvn[16];
	GetModelViewProjection().Get(matrix);
	GetModelViewNormal().Get(_mvn);
	glUniformMatrix4fv(obj_mvp, 1, false, matrix);
	glUniformMatrix4fv(obj_mvn, 1, false, _mvn);

	glBindVertexArray(obj_vao);

	GLuint tex_loc = glGetUniformLocation(obj_program_id, "tex");
	glUniform1i(tex_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, obj_tex_id);

	// draw obj
	glDrawArrays(GL_TRIANGLES, 0, num_f * 3 * sizeof(cyVec3f));

	renderBuffer.Unbind();


	// render buffer texture on plane
	glUseProgram(plane_program_id);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float _p_mvp[16];
	GetPlaneModelViewProjection().Get(_p_mvp);
	glUniformMatrix4fv(plane_mvp, 1, false, _p_mvp);

	GLuint rb_tex_loc = glGetUniformLocation(plane_program_id, "render_tex");
	glUniform1i(rb_tex_loc, 0);
	renderBuffer.BindTexture(0);

	glBindVertexArray(plane_vao);

	glDrawArrays(GL_TRIANGLES, 0, 6);

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
	int modifiers = glutGetModifiers();

	if (modifiers == GLUT_ACTIVE_ALT) {
		if (mouse_button == GLUT_LEFT_BUTTON) { // handle rotation
			if (pry < y) plane_r_x += 0.02;
			else if (pry > y) plane_r_x -= 0.02;
			pry = y;

			if (prx < x) plane_r_y += 0.02;
			else if (prx > x) plane_r_y -= 0.02;
			prx = x;
		}
		else if (mouse_button == GLUT_RIGHT_BUTTON) { // handle zooming
			if (pzy < y) plane_t_z += 0.15; // zooming in 
			else plane_t_z -= 0.15; // zoomng out
			pzy = y;
		}
	}
	else {
		if (mouse_button == GLUT_LEFT_BUTTON) { // handle rotation
			if (pry < y) obj_r_x += 0.02;
			else if (pry > y) obj_r_x -= 0.02;
			pry = y;

			if (prx < x) obj_r_y += 0.02;
			else if (prx > x) obj_r_y -= 0.02;
			prx = x;
		}
		else if (mouse_button == GLUT_RIGHT_BUTTON) { // handle zooming
			if (pzy < y) obj_t_z += 0.15; // zooming in 
			else obj_t_z -= 0.15; // zoomng out
			pzy = y;
		}
	}
	
	glutPostRedisplay();
}

#pragma endregion GLUT_Functions

int main(int argc, char* argv[]) {

	// set everything up - initialize glut, read obj and fill vertex buffer, compile and link shaders, set uniform and sttribute variables
	InitializeGLUT(argc, argv);
	CompileProgram();
	GLuint* obj_vbo = BuildObjBuffers(argc, argv[1]);
	GLuint plane_vbo = BuildPlaneBuffers();
	InitializeTexture();
	SetShaderVariables();
	BuildRenderBuffer();
	
	// start 
	glutMainLoop();

	glDeleteBuffers(1, &obj_vbo[0]);
	glDeleteBuffers(1, &obj_vbo[1]);
	glDeleteBuffers(1, &obj_vbo[2]);
	glDeleteBuffers(1, &plane_vbo);

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
	glutCreateWindow("CS 5610 Project 5");

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

	obj_program_id = glCreateProgram();

}

void CompileProgram() {
	bool result;
	if (!(result = obj_program.BuildFiles("obj.vert", "obj.frag"))) {
		fprintf(stderr, "Error: object shader compilation failed");
		exit(1);
	}
	obj_program_id = obj_program.GetID();

	if (!(result = plane_program.BuildFiles("plane.vert", "plane.frag"))) {
		fprintf(stderr, "Error: plane shader compilation failed");
		exit(1);
	}
	plane_program_id = plane_program.GetID();
}

GLuint* BuildObjBuffers(int argc, const char* filename) {
	// input sanitization
	if (argc != 2) {
		fprintf(stderr, "Error: invalid arguments, expected - 2");
		exit(1);
	}

	// read vertex data
	bool r;
	
	if (!(r = reader.LoadFromFileObj(filename))) {
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

	cyVec3f* t = new cyVec3f[num_f * 3];
	for (int i = 0; i < num_f; i++) {
		cyTriMesh::TriFace ft = reader.FT(i);
		t[3 * i + 0] = reader.VT(ft.v[0]);
		t[3 * i + 1] = reader.VT(ft.v[1]);
		t[3 * i + 2] = reader.VT(ft.v[2]);
	}

	// create and bind vertex buffer object with vertex data
	GLuint vbo[3];
	glGenVertexArrays(1, &obj_vao);
	glBindVertexArray(obj_vao);
	glGenBuffers(3, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
	GLuint pos = glGetAttribLocation(obj_program_id, "pos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);


	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), n, GL_STATIC_DRAW);
	GLuint norm = glGetAttribLocation(obj_program_id, "norm");
	glEnableVertexAttribArray(norm);
	glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, num_f * 3 * sizeof(cyVec3f), t, GL_STATIC_DRAW);
	GLuint tcx = glGetAttribLocation(obj_program_id, "tcx");
	glVertexAttribPointer(tcx, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(tcx);

	return vbo;
}

GLuint BuildPlaneBuffers() {
	static const GLfloat plane[] = { -1,-1,0,  1,-1,0,  -1,1,0,  -1,1,0,  1,-1,0,  1,1,0, };

	GLuint plane_vbo;
	glGenVertexArrays(1, &plane_vao);
	glBindVertexArray(plane_vao);
	glGenBuffers(1, &plane_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, plane_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
	GLuint plane_pos = glGetAttribLocation(plane_program_id, "pos");
	glVertexAttribPointer(plane_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(plane_pos);

	return plane_vbo;
}

void InitializeTexture() {
	std::string file(reader.M(0).map_Kd);
	std::vector<unsigned char> image;
	unsigned error = lodepng::decode(image, img_width, img_height, file);
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	
	glGenTextures(1, &obj_tex_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, obj_tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void SetShaderVariables() {
	glUseProgram(obj_program_id);

	// set uniform variables location
	obj_mvp = glGetUniformLocation(obj_program_id, "mvp");	
	obj_mvn = glGetUniformLocation(obj_program_id, "mvn");

	glUseProgram(plane_program_id);
	plane_mvp = glGetUniformLocation(plane_program_id, "p_mvp");
}

void BuildRenderBuffer() {
	// Initialize Render Buffer
	renderBuffer.Initialize(true, 3, rb_width, rb_height);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "Error: render buffer incorrect");
		exit(1);
	}
	renderBuffer.BuildTextureMipmaps();
}

cyMatrix4f GetModelViewProjection() {
	// perspective projection matrix values
	float fov = 3.145 * 40.0 / 180.0;
	float aspect = rb_width / rb_height;
	float n = 0.1f;
	float f = obj_t_z + obj_t_z;

	// generate perspective projection, translation, and rotation matrices and multiply them
	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, -5, obj_t_z))) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y + auto_rot, 0);
}

cyMatrix4f GetModelViewNormal() {
	// generate translation and rotation matrices then multiply them
	return cyMatrix4f::Translation(cyVec3f(0, -5, obj_t_z)) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y + auto_rot, 0);
}

cyMatrix4f GetPlaneModelViewProjection() {
	// perspective projection matrix values
	float fov = 0.7;
	float aspect = display_width / display_height;
	float n = 0.1f;
	float f = plane_t_z * 2;

	// generate perspective projection, translation, and rotation matrices and multiply them
	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, 0, plane_t_z))) * cyMatrix4f::RotationXYZ(plane_r_x, plane_r_y, 0);
}

#pragma endregion Helper_Functions