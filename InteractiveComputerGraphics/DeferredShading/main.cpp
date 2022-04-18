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
#include "DeferredShading.h"

void InitializeGBuffer();

// Geometry Pass 
cyGLSLProgram geometry_prog;
GLuint geometry_prog_id;
GLuint projection_loc;
GLuint view_loc;
GLuint model_loc;
GLuint diffuse_id;
GLuint diffuse_location;
GLuint specular_id;
GLuint specular_location;
GLuint geometry_vao;
GLuint g_buffer;

// Lighting Pass
cyGLSLProgram lighting_prog;
GLuint lighting_prog_id;
GLuint camera_pos_loc;
GLuint g_pos_id;
GLuint g_pos_location;
GLuint g_normal_id;
GLuint g_normal_location;
GLuint g_diffuse_spec_id;
GLuint g_diffuse_spec_location;
GLuint quad_vao;

// Light Box Pass
cyGLSLProgram cube_prog;
GLuint cube_prog_id;
GLuint cube_mvp_loc;
GLuint cube_color_loc;
GLuint cube_vao;

//Text
cyGLSLProgram text_prog;
GLuint text_prog_id;

std::vector<cyVec3f> object_positions = { 
	cyVec3f(-3.0, -0.5, -3.0), cyVec3f(0.0, -0.5, -3.0), cyVec3f(3.0, -0.5, -3.0), 
	cyVec3f(-3.0, -0.5, 0.0),  cyVec3f(0.0, -0.5, 0.0),  cyVec3f(3.0, -0.5, 0.0), 
	cyVec3f(-3.0, -0.5, 3.0),  cyVec3f(0.0, -0.5, 3.0),  cyVec3f(3.0, -0.5, 3.0) 
};

std::vector<cyVec3f> light_positions;
std::vector<cyVec3f> light_colors; 
int num_lights = 32;

int num_faces;
int num_verts;

float display_width = 800;
float display_height = 600;
float camz = 10;
float camx = 6;
float camy = 2;
bool auto_rot_status = false;
int auto_rot_direction = -1;

int current_mouse_button = -1;
int previous_x = 0;
int previous_y = 0;

void GeometryPass()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(geometry_prog_id);
	glBindVertexArray(geometry_vao);

	float projection[16]; cyMatrix4f::Perspective(0.7, display_width / display_height, 0.1f, 30).Get(projection);
	float view      [16]; cyMatrix4f::View(cyVec3f(camx, camy, camz), cyVec3f(0, 0, 0), cyVec3f(0, 1, 0)).Get(view);
	glUniformMatrix4fv(projection_loc, 1, false, projection);
	glUniformMatrix4fv(view_loc, 1, false, view);

	for (unsigned int i = 0; i < object_positions.size(); i++)
	{
		float model[16]; (cyMatrix4f::Translation(object_positions[i]) * cyMatrix4f::Scale(0.5)).Get(model);
		glUniformMatrix4fv(model_loc, 1, false, model);

		glDrawArrays(GL_TRIANGLES, 0, num_verts * sizeof(cyVec3f));
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightingPass()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(lighting_prog_id);
	glBindVertexArray(quad_vao);
	
	for (unsigned int i = 0; i < light_positions.size(); i++)
	{
		glUniform3f(glGetUniformLocation(lighting_prog_id, ("lights[" + std::to_string(i) + "].position").c_str()), light_positions[i].x, light_positions[i].y, light_positions[i].z);
		glUniform3f(glGetUniformLocation(lighting_prog_id, ("lights[" + std::to_string(i) + "].color").c_str()), light_colors[i].x, light_colors[i].y, light_colors[i].z);
		
		const float constant = 1.0f; 
		const float linear = 0.7f;
		const float quadratic = 1.8f;
		glUniform1f(glGetUniformLocation(lighting_prog_id, ("lights[" + std::to_string(i) + "].linear").c_str()), linear); 
		glUniform1f(glGetUniformLocation(lighting_prog_id, ("lights[" + std::to_string(i) + "].quadratic").c_str()), quadratic);

		const float brightness = std::fmaxf(std::fmaxf(light_colors[i].x, light_colors[i].y), light_colors[i].z);
		float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * brightness))) / (2.0f * quadratic); // quadratic equation
		glUniform1f(glGetUniformLocation(lighting_prog_id, ("lights[" + std::to_string(i) + "].radius").c_str()), radius);
	}
	glUniform3f(camera_pos_loc, camx, camy, camz);

	// finally render quad
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void CubePass()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, display_width, display_height, 0, 0, display_width, display_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(cube_prog_id);
	glBindVertexArray(cube_vao);
	cyMatrix4f projection = cyMatrix4f::Perspective(0.7, display_width / display_height, 0.1f, 30);
	cyMatrix4f view = cyMatrix4f::View(cyVec3f(camx, camy, camz), cyVec3f(0, 0, 0), cyVec3f(0, 1, 0));
	for (unsigned int i = 0; i < light_positions.size(); i++)
	{
		cyMatrix4f model = (cyMatrix4f::Translation(light_positions[i]) * cyMatrix4f::Scale(0.125));
		float mvp[16]; (projection * view * model).Get(mvp);
		glUniformMatrix4fv(cube_mvp_loc, 1, false, mvp);
		glUniform3f(cube_color_loc, light_colors[i].x, light_colors[i].y, light_colors[i].z);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}

void TextPass()
{
	glUseProgram(text_prog_id);
	glColor3f(1.0, 1.0, 1.0); 
	glRasterPos2f(0, 0);
	glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)"'left arrow' - increase lights by 4 & re-seed");
	glRasterPos2f(-0.98, 0.88);
	glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)"'right arrow' - decrease lights by 4 & re-seed");
	glRasterPos2f(-0.98, 0.82);
	glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)"'spacebar' - re-seed random lights");
	glRasterPos2f(-0.98, 0.76);
	glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)"'r' - enable camera auto movement");
	glRasterPos2f(-0.98, 0.70);
	glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)"'e' - reset camera");
}

void Draw() {
	glEnable(GL_DEPTH_TEST);
	GeometryPass();
	LightingPass();
	CubePass();
	//TextPass();
	glutSwapBuffers();
}

void GenerateRandomLights() 
{
	glUniform1i(glGetUniformLocation(lighting_prog_id, "num_lights"), num_lights);
	light_positions.clear();
	light_colors.clear();
	for (unsigned int i = 0; i < num_lights; i++)
	{
		// calculate slightly random offsets
		float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
		float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		light_positions.push_back(cyVec3f(xPos, yPos, zPos));
		// also calculate random color
		float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5);
		float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5);
		float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5);
		light_colors.push_back(cyVec3f(rColor, gColor, bColor));
	}
}

void KeyPress(unsigned char key, int x, int y) {
	switch (key) {
	case 'r':
		auto_rot_status = !auto_rot_status;
		break;
	case 'e':
		auto_rot_status = false;
		auto_rot_direction = -1;
		camz = 10;
		break;
	case 32:
		GenerateRandomLights();
		break;
	case 27:
		glutLeaveMainLoop();
		break;
	}
}

void SpecialKeyPress(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_RIGHT:
		if (num_lights < 128) {
			num_lights += 4;
			GenerateRandomLights();			
		}
		break;
	case GLUT_KEY_LEFT:
		if (num_lights > 4) {
			num_lights -= 4;
			GenerateRandomLights();
		}
		break;
	}
	glutPostRedisplay();
}

void Idle() 
{
	if (auto_rot_status) {
		if (camz <= -10) auto_rot_direction = 1;
		else if (camz >= 10) auto_rot_direction = -1;
		camz += (auto_rot_direction * 0.1);
	}
	glutPostRedisplay();
}

void ChangeViewPort(int w, int h) {
	display_width = w;
	display_height = h;
	glViewport(0, 0, w, h);
	InitializeGBuffer();
}

void InitializeGlutWindow() {
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitContextFlags(GLUT_DEBUG);
	glutInitWindowSize(display_width, display_height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("5610 Final Project - Deferred Shading");
}

void RegisterGlutFunctions() {
	glutReshapeFunc(ChangeViewPort);
	glutDisplayFunc(Draw);
	glutKeyboardFunc(KeyPress);
	glutSpecialFunc(SpecialKeyPress);
	glutIdleFunc(Idle);
}

void CompilePrograms()
{
	// Setup Geometry Pass Program
	if (!geometry_prog.BuildFiles("geometry.vert", "geometry.frag")) {
		fprintf(stderr, "Something went wrong loading the geometry program."); exit(1);
	}
	geometry_prog_id = geometry_prog.GetID();
	geometry_prog.Bind();

	// Setup Lighting Pass Program
	if (!lighting_prog.BuildFiles("lighting.vert", "lighting.frag")) {
		fprintf(stderr, "Something went wrong loading the lighting program."); exit(1);
	}
	lighting_prog_id = lighting_prog.GetID();
	lighting_prog.Bind();

	// Setup Light Box Pass Program
	if (!cube_prog.BuildFiles("cube.vert", "cube.frag")) {
		fprintf(stderr, "Something went wrong loading the cube program."); exit(1);
	}
	cube_prog_id = cube_prog.GetID();
	cube_prog.Bind();

	// Setup Text Pass Program
	if (!text_prog.BuildFiles("text.vert", "text.frag")) {
		fprintf(stderr, "Something went wrong loading the cube program."); exit(1);
	}
	text_prog_id = text_prog.GetID();
	text_prog.Bind();
}

void LoadObjectAndInitializeObjectBuffers()
{
	// Read mesh data for backpack object
	cyTriMesh mesh;
	bool success = mesh.LoadFromFileObj("backpack.obj");
	if (!success) {
		fprintf(stderr, "Something went wrong reading the object file");
		exit(1);
	}
	num_faces = mesh.NF();
	num_verts = num_faces * 3;

	cyVec3f* verts = new cyVec3f[num_verts];
	cyVec3f* norms = new cyVec3f[num_verts];
	cyVec3f* texture = new cyVec3f[num_verts];

	for (int i = 0; i < mesh.NF(); i++) {
		for (int j = 0; j < 3; j++) {
			verts[3 * i + j] = mesh.V(mesh.F(i).v[j]);
			norms[3 * i + j] = mesh.VN(mesh.FN(i).v[j]);
			texture[3 * i + j] = mesh.VT(mesh.FT(i).v[j]);
		}
	}

	glUseProgram(geometry_prog_id);
	// Create and bind vertex buffers
	GLuint geometry_vbo[3];
	glGenVertexArrays(1, &geometry_vao);
	glBindVertexArray(geometry_vao);
	glGenBuffers(3, geometry_vbo);

	GLuint pos = glGetAttribLocation(geometry_prog_id, "pos");
	glBindBuffer(GL_ARRAY_BUFFER, geometry_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, num_verts * sizeof(cyVec3f), verts, GL_STATIC_DRAW);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(pos);

	GLuint norm = glGetAttribLocation(geometry_prog_id, "norm");
	glBindBuffer(GL_ARRAY_BUFFER, geometry_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, num_verts * sizeof(cyVec3f), norms, GL_STATIC_DRAW);
	glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(norm);

	GLuint texpos = glGetAttribLocation(geometry_prog_id, "texpos");
	glBindBuffer(GL_ARRAY_BUFFER, geometry_vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, num_verts * sizeof(cyVec3f), texture, GL_STATIC_DRAW);
	glVertexAttribPointer(texpos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(texpos);

	// Uniform Locations
	projection_loc = glGetUniformLocation(geometry_prog_id, "projection");
	view_loc = glGetUniformLocation(geometry_prog_id, "view");
	model_loc = glGetUniformLocation(geometry_prog_id, "model");
}

void CreateQuadAndInitializeQuadBuffers()
{
	float quad_pos[] = {
		-1.0f,  1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
	};
	float quad_texpos[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};

	glUseProgram(lighting_prog_id);
	GLuint quad_vbo[2];
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);
	glGenBuffers(2, quad_vbo);

	GLuint pos = glGetAttribLocation(lighting_prog_id, "pos");
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_pos), quad_pos, GL_STATIC_DRAW);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(pos);

	GLuint texpos = glGetAttribLocation(lighting_prog_id, "texpos");
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_texpos), quad_texpos, GL_STATIC_DRAW);
	glVertexAttribPointer(texpos, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(texpos);

	camera_pos_loc = glGetAttribLocation(lighting_prog_id, "camera_pos");
}

void CreateCubeAndInitializeCubeBuffers() 
{
	float cube_geometry[] = {
		-1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,  1.0f,  1.0f , 1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,
	};

	glUseProgram(cube_prog_id);
	GLuint cube_vbo[1];
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);
	glGenBuffers(1, cube_vbo);

	GLuint pos = glGetAttribLocation(cube_prog_id, "pos");
	glBindBuffer(GL_ARRAY_BUFFER, cube_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_geometry), cube_geometry, GL_STATIC_DRAW);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(pos);

	cube_mvp_loc = glGetUniformLocation(cube_prog_id, "mvp");
	cube_color_loc = glGetUniformLocation(cube_prog_id, "light_color");
}

void InitializeGBuffer()
{
	//Setup G-BUFFER
	glGenFramebuffers(1, &g_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
	// position color buffer
	glGenTextures(1, &g_pos_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_pos_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, display_width, display_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_pos_id, 0);
	// normal color buffer
	glGenTextures(1, &g_normal_id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_normal_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, display_width, display_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_normal_id, 0);
	// color + specular color buffer
	glGenTextures(1, &g_diffuse_spec_id);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, g_diffuse_spec_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, display_width, display_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_diffuse_spec_id, 0);
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);
	// create and attach depth buffer (renderbuffer)
	GLuint rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, display_width, display_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	g_pos_location = glGetUniformLocation(lighting_prog_id, "g_pos");
	g_normal_location = glGetUniformLocation(lighting_prog_id, "g_normal");
	g_diffuse_spec_location = glGetUniformLocation(lighting_prog_id, "g_diffuse_spec");

	glUseProgram(lighting_prog_id);
	glUniform1i(g_pos_location, 0);
	glUniform1i(g_normal_location, 1);
	glUniform1i(g_diffuse_spec_location, 2);
}

void CreateTexture(std::string filepath, GLuint id, int offset)
{
	//Initialize Texture
	std::string file(filepath);
	std::vector<unsigned char> image;
	unsigned width = 4096;
	unsigned height = 4096;
	unsigned error = lodepng::decode(image, width, height, file);
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0 + offset);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void InitializeObjectTextures()
{
	diffuse_location = glGetUniformLocation(geometry_prog_id, "diffuse");
	specular_location = glGetUniformLocation(geometry_prog_id, "specular");
	glUseProgram(geometry_prog_id);
	glUniform1i(diffuse_location, 3);
	glUniform1i(specular_location, 4);
	CreateTexture("diffuse.png", diffuse_id, 3);
	CreateTexture("specular.png", specular_id, 4);
}

int main(int argc, char* argv[]) {
	srand(13);
	glutInit(&argc, argv);
	InitializeGlutWindow();
	RegisterGlutFunctions();
	if (GLEW_OK != glewInit()) return 1;
	CompilePrograms();
	LoadObjectAndInitializeObjectBuffers();
	InitializeObjectTextures();
	InitializeGBuffer();
	GenerateRandomLights();
	CreateQuadAndInitializeQuadBuffers();
	CreateCubeAndInitializeCubeBuffers();
	glutMainLoop();
	return 0;
}