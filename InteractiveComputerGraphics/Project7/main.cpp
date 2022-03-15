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


// rotation and zoom with mouse
float obj_t_z = -48;
float obj_r_x = -1.5708;
float obj_r_y = 0;

// keeps track of the previous position of the mouse for zoom/rotation direction
int pzy = 0;
int prx = 0;
int pry = 0;
bool alt_down = false;
int mouse_button = -1;

// auto rotation
bool auto_rot_status = false;
float auto_rot = 0;

// obj variables
cyGLSLProgram obj_program;
GLuint obj_program_id;
GLuint obj_vao;
GLuint obj_mvp;
GLuint obj_mv;
GLuint obj_m_shadow;
GLfloat display_width = 800;
GLfloat display_height = 600;
cyTriMesh obj_mesh;
int num_v; // number of vertices in given obj
int num_f; // number of faces in given obj

// plane variables
cyGLSLProgram plane_program;
GLuint plane_program_id;
GLuint plane_vao;
GLuint plane_mvp;
GLuint plane_mv;
GLuint plane_m_shadow;

// shadow map
cyGLSLProgram shadow_program;
GLuint shadow_program_id;
cyGLRenderDepth2D shadow_map;
GLuint shadow_vao;
GLuint shadow_mlp;
int shadow_width = 2048;
int shadow_height = 2048;


// forward declarations
void InitializeGLUT(int argc, char* argv[]);
void CompileProgram();
GLuint* BuildObjBuffers(int argc, const char* filename);
GLuint BuildPlaneBuffers();
GLuint BuildShadowMap();
void SetShaderVariables();
cyMatrix4f GetModelViewProjection();
cyMatrix4f GetModelView();
cyMatrix4f GetModelLightProjection();
cyMatrix4f GetModelShadow();

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
	SetShaderVariables();
	if (auto_rot_status) auto_rot += 0.02;

	// render shadow map
	shadow_map.Bind();
	glUseProgram(shadow_program_id);
	glBindVertexArray(shadow_vao);
	glDrawArrays(GL_TRIANGLES, 0, num_v * sizeof(cyVec3f));
	shadow_map.Unbind();

	glClear(GL_DEPTH_BUFFER_BIT);
	
	// render object
	glUseProgram(obj_program_id);
	glBindVertexArray(obj_vao);
	glDrawArrays(GL_TRIANGLES, 0, num_v * sizeof(cyVec3f));

	// render plane
	glUseProgram(plane_program_id);
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
	
	glutPostRedisplay();
}

#pragma endregion GLUT_Functions

int main(int argc, char* argv[]) {

	// set everything up - initialize glut, read obj and fill vertex buffer, compile and link shaders, set uniform and sttribute variables
	InitializeGLUT(argc, argv);
	CompileProgram();
	GLuint* obj_vbo = BuildObjBuffers(argc, argv[1]);
	GLuint plane_vbo = BuildPlaneBuffers();
	GLuint shadow_vbo = BuildShadowMap();
	
	// start 
	glutMainLoop();

	glDeleteBuffers(1, &obj_vbo[0]);
	glDeleteBuffers(1, &obj_vbo[1]);
	glDeleteBuffers(1, &obj_vbo[2]);
	glDeleteBuffers(1, &plane_vbo);
	glDeleteBuffers(1, &shadow_vbo);

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
	glutCreateWindow("CS 5610 Project 7");

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

	if (!(result = shadow_program.BuildFiles("shadow.vert", "shadow.frag"))) {
		fprintf(stderr, "Error: shadow shader compilation failed");
		exit(1);
	}
	shadow_program_id = shadow_program.GetID();
}

GLuint* BuildObjBuffers(int argc, const char* filename) {
	// input sanitization
	if (argc != 2) {
		fprintf(stderr, "Error: invalid arguments, expected - 2");
		exit(1);
	}

	// read vertex data
	bool r;
	
	if (!(r = obj_mesh.LoadFromFileObj(filename))) {
		fprintf(stderr, "Error: cannot read file");
		exit(1);
	}
	num_v = obj_mesh.NV();
	num_f = obj_mesh.NF();

	cyVec3f* v = new cyVec3f[num_f * 3];
	for (int i = 0; i < num_f; i++) {
		cyTriMesh::TriFace f = obj_mesh.F(i);
		v[3 * i + 0] = obj_mesh.V(f.v[0]);
		v[3 * i + 1] = obj_mesh.V(f.v[1]);
		v[3 * i + 2] = obj_mesh.V(f.v[2]);
	}

	cyVec3f* n = new cyVec3f[num_f * 3];
	for (int i = 0; i < num_f; i++) {
		cyTriMesh::TriFace fn = obj_mesh.FN(i);
		n[3 * i + 0] = obj_mesh.VN(fn.v[0]);
		n[3 * i + 1] = obj_mesh.VN(fn.v[1]);
		n[3 * i + 2] = obj_mesh.VN(fn.v[2]);
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

	glUseProgram(obj_program_id);
	obj_mvp = glGetUniformLocation(obj_program_id, "mvp");
	obj_mv = glGetUniformLocation(obj_program_id, "mv");
	obj_m_shadow = glGetUniformLocation(obj_program_id, "m_shadow");

	return vbo;
}

GLuint BuildPlaneBuffers() {
	static const GLfloat plane[] = { -30,-30,0,  30,-30,0,  -30,30,0,  -30,30,0,  30,-30,0,  30,30,0, };

	GLuint plane_vbo;
	glGenVertexArrays(1, &plane_vao);
	glBindVertexArray(plane_vao);
	glGenBuffers(1, &plane_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, plane_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
	GLuint plane_pos = glGetAttribLocation(plane_program_id, "pos");
	glVertexAttribPointer(plane_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(plane_pos);

	glUseProgram(plane_program_id);
	plane_mvp = glGetUniformLocation(plane_program_id, "mvp");
	plane_mv = glGetUniformLocation(plane_program_id, "mv");
	plane_m_shadow = glGetUniformLocation(plane_program_id, "m_shadow");

	return plane_vbo;
}

GLuint BuildShadowMap() {
	// setup shadowmap
	shadow_map.Initialize(true, shadow_width, shadow_height);
	shadow_map.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);

	cyVec3f* v = new cyVec3f[num_f * 3];
	for (int i = 0; i < num_f; i++) {
		cyTriMesh::TriFace f = obj_mesh.F(i);
		v[3 * i + 0] = obj_mesh.V(f.v[0]);
		v[3 * i + 1] = obj_mesh.V(f.v[1]);
		v[3 * i + 2] = obj_mesh.V(f.v[2]);
	}

	GLuint shadow_vbo;
	glGenVertexArrays(1, &shadow_vao);
	glBindVertexArray(shadow_vao);
	glGenBuffers(1, &shadow_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, shadow_vbo);
	glBufferData(GL_ARRAY_BUFFER, num_v * sizeof(cyVec3f), v, GL_STATIC_DRAW);
	GLuint shadow_pos = glGetAttribLocation(plane_program_id, "pos");
	glVertexAttribPointer(shadow_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(shadow_pos);

	glUseProgram(shadow_program_id);
	shadow_mlp = glGetUniformLocation(plane_program_id, "mlp");

	return shadow_vbo;
}

void SetShaderVariables() {
	float _mvp[16];
	float _mv[16];
	float _mlp[16];
	float _m_shadow[16];
	GetModelViewProjection().Get(_mvp);
	GetModelView().Get(_mv);
	GetModelLightProjection().Get(_mlp);
	GetModelShadow().Get(_m_shadow);

	glUseProgram(obj_program_id);
	glUniformMatrix4fv(obj_mvp, 1, false, _mvp);
	glUniformMatrix4fv(obj_mv, 1, false, _mv);
	glUniformMatrix4fv(obj_m_shadow, 1, false, _m_shadow);

	glUseProgram(plane_program_id);
	glUniformMatrix4fv(plane_mvp, 1, false, _mvp);
	glUniformMatrix4fv(plane_mv, 1, false, _mv);
	glUniformMatrix4fv(plane_m_shadow, 1, false, _m_shadow);

	glUseProgram(shadow_program_id);
	glUniformMatrix4fv(shadow_mlp, 1, false, _mlp);
}

cyMatrix4f GetModelViewProjection() {
	// perspective projection matrix values
	float fov = 3.145 * 40.0 / 180.0;
	float aspect = display_width / display_height;
	float n = 0.1f;
	float f = obj_t_z + obj_t_z;

	// generate perspective projection, translation, and rotation matrices and multiply them
	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, -5, obj_t_z))) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y + auto_rot, 0);
}

cyMatrix4f GetModelView() {
	// generate translation and rotation matrices then multiply them
	return cyMatrix4f::Translation(cyVec3f(0, -5, obj_t_z)) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y + auto_rot, 0);
}

cyMatrix4f GetModelLightProjection() {
	// generate model to light space matrix
	// perspective projection matrix values
	cyVec3f light_pos = cyVec3f(5, 5, 5);
	float fov = 3.145 * 40.0 / 180.0;
	float aspect = shadow_width / shadow_height;
	float n = 0.1f;
	float f = light_pos.Length() * 2;

	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::View(light_pos, cyVec3f(0, 0, 0), cyVec3f(0, 1, 0)));
}
cyMatrix4f GetModelShadow() {
	cyMatrix4f bias(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);

	return bias * GetModelLightProjection();
}



#pragma endregion Helper_Functions