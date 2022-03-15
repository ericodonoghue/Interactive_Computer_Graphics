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
unsigned img_width = 2048;
unsigned img_height = 2048;

// rotation and zoom with mouse
float obj_t_z = -72;
float obj_r_x = 0.1;
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
GLuint obj_f_mv;
GLfloat display_width = 800;
GLfloat display_height = 600;
cyTriMesh obj_mesh;
int obj_num_v; // number of vertices in given obj
int obj_num_f; // number of faces in given obj

// cubemap variables
cyGLSLProgram cube_program;
GLuint cube_program_id;
GLuint cube_vao;
GLuint cube_mvp;
cyTriMesh cube_mesh;
cyGLTextureCubeMap envmap;
int cube_num_v;
int cube_num_f; 

// forward declarations
void InitializeGLUT(int argc, char* argv[]);
GLuint* BuildObjBuffers(int argc, const char* filename);
GLuint* BuildCubeBuffers();
void CompileProgram();
void SetShaderVariables();
void InitializeCubemap();
cyMatrix4f GetModelViewProjection();
cyMatrix4f GetModelView();
cyMatrix4f GetCubeModelViewProjection();

#pragma endregion globals 

#pragma region GLUT_Functions

void resize(int w, int h) {
	display_width = w;
	display_height = h;
	glViewport(0, 0, w, h);
}

void draw() {
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (auto_rot_status) auto_rot += 0.02;

	// cubemap render
	glUseProgram(cube_program_id);

	glDepthFunc(GL_ALWAYS);
	
	float _cmvp[16];
	GetCubeModelViewProjection().Get(_cmvp);
	glUniformMatrix4fv(cube_mvp, 1, false, _cmvp);

	glBindVertexArray(cube_vao);

	// draw cubemap
	glDrawArrays(GL_TRIANGLES, 0, cube_num_f * 3);
	glDepthFunc(GL_LESS);


	glClear(GL_DEPTH_BUFFER_BIT);

	// obj render
	glUseProgram(obj_program_id);
	
	// calculate and set mvp/mvn matrix
	float _mvp[16];
	float _mv[16];
	GetModelViewProjection().Get(_mvp);
	GetModelView().Get(_mv);
	glUniformMatrix4fv(obj_mvp, 1, false, _mvp);
	glUniformMatrix4fv(obj_mv, 1, false, _mv);
	//glUniformMatrix4fv(obj_f_mv, 1, false, _mv);

	glBindVertexArray(obj_vao);

	// draw obj
	glDrawArrays(GL_TRIANGLES, 0, obj_num_f * 3 * sizeof(cyVec3f));

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
	GLuint* cube_vbo = BuildCubeBuffers();
	SetShaderVariables();
	InitializeCubemap();
	
	// start 
	glutMainLoop();

	glDeleteBuffers(1, &obj_vbo[0]);
	glDeleteBuffers(1, &obj_vbo[1]);
	glDeleteBuffers(1, &obj_vbo[2]);
	glDeleteBuffers(1, &cube_vbo[0]);

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
	glutCreateWindow("CS 5610 Project 6");

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

	if (!(result = cube_program.BuildFiles("cube.vert", "cube.frag"))) {
		fprintf(stderr, "Error: cube shader compilation failed");
		exit(1);
	}
	cube_program_id = cube_program.GetID();
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
		fprintf(stderr, "Error: cannot obj read file");
		exit(1);
	}
	obj_num_v = obj_mesh.NV();
	obj_num_f = obj_mesh.NF();

	cyVec3f* v = new cyVec3f[obj_num_f * 3];
	for (int i = 0; i < obj_num_f; i++) {
		cyTriMesh::TriFace f = obj_mesh.F(i);
		v[3 * i + 0] = obj_mesh.V(f.v[0]);
		v[3 * i + 1] = obj_mesh.V(f.v[1]);
		v[3 * i + 2] = obj_mesh.V(f.v[2]);
	}

	cyVec3f* n = new cyVec3f[obj_num_f * 3];
	for (int i = 0; i < obj_num_f; i++) {
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
	glBufferData(GL_ARRAY_BUFFER, obj_num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
	GLuint pos = glGetAttribLocation(obj_program_id, "pos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);


	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, obj_num_f * 3 * sizeof(cyVec3f), n, GL_STATIC_DRAW);
	GLuint norm = glGetAttribLocation(obj_program_id, "norm");
	glEnableVertexAttribArray(norm);
	glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);


	return vbo;
}

GLuint* BuildCubeBuffers() {
	bool r;
	if (!(r = cube_mesh.LoadFromFileObj("cube.obj"))) {
		fprintf(stderr, "Error: cannot read cube file");
		exit(1);
	}
	cube_num_v = cube_mesh.NV();
	cube_num_f = cube_mesh.NF();

	cyVec3f* v = new cyVec3f[cube_num_f * 3];
	for (int i = 0; i < cube_num_f; i++) {
		cyTriMesh::TriFace f = cube_mesh.F(i);
		v[3 * i + 0] = cube_mesh.V(f.v[0]);
		v[3 * i + 1] = cube_mesh.V(f.v[1]);
		v[3 * i + 2] = cube_mesh.V(f.v[2]);
	}

	// create and bind vertex buffer object with vertex data
	GLuint vbo[1];
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);
	glGenBuffers(1, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, cube_num_f * 3 * sizeof(cyVec3f), v, GL_STATIC_DRAW);
	GLuint pos = glGetAttribLocation(cube_program_id, "pos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	return vbo;
}

void SetShaderVariables() {
	glUseProgram(obj_program_id);
	obj_mvp = glGetUniformLocation(obj_program_id, "mvp");	
	obj_mv = glGetUniformLocation(obj_program_id, "mv");

	glUseProgram(cube_program_id);
	cube_mvp = glGetUniformLocation(cube_program_id, "mvp");
}

void InitializeCubemap() {
	envmap.Initialize();
	const char* files[6] = { "cubemap_posx.png",
							 "cubemap_negx.png",
							 "cubemap_posy.png",
							 "cubemap_negy.png", 
							 "cubemap_posz.png",
							 "cubemap_negz.png" };

	for (int i = 0; i < 6; i++) {
		std::string file(files[i]);
		std::vector<unsigned char> image;
		unsigned error = lodepng::decode(image, img_width, img_height, file);
		if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

		// in order to call SetImageRGBA the image data must be a GL type
		// uses too much stack memory must allocate on heap
		/*GLubyte _image [2048 * 2048 * 4] = {};
		for (int i = 0; i < image.size(); i++) {
			_image[i] = image[i];
		}*/

		GLubyte* _image = new GLubyte[img_width * img_width * 4]; // allocate memory
		for (int i = 0; i < image.size(); i++) { _image[i] = image[i]; }

		// set image data
		envmap.SetImageRGBA((cyGLTextureCubeMapSide)i, _image, img_width, img_width);

		delete[] _image; // free memory
	}
	envmap.BuildMipmaps();
	envmap.SetSeamless();
	envmap.Bind(0);
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

cyMatrix4f GetCubeModelViewProjection() {
	// perspective projection matrix values
	float fov = 3.145 * 40.0 / 180.0;
	float aspect = display_width / display_height;
	float n = 0.1f;
	float f = obj_t_z + obj_t_z;

	// generate perspective projection, translation, and rotation matrices and multiply them
	return (cyMatrix4f::Perspective(fov, aspect, n, f) * cyMatrix4f::Translation(cyVec3f(0, 0, 0))) * cyMatrix4f::RotationXYZ(obj_r_x, obj_r_y + auto_rot, 0);
}

#pragma endregion Helper_Functions