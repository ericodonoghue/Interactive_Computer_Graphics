#include <GL/freeglut.h>
#include <stdlib.h>

void resize(int w, int h)
{
	glViewport(0, 0, w, h);
}

void display()
{
	//Clear the viewport
	glClear(GL_COLOR_BUFFER_BIT);

	// OpenGL draw calls here

	//Swap buffers
	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {

	switch (key) {
		// Close on ESC
	case 27:
		glutLeaveMainLoop();
		break;
	}
}

int main(int argc, char* argv[]) {

	// initialize GLUT
	glutInit(&argc, argv);

	// GLUT window setup
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("CS 5610 Project 1");

	// setup background color
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.36f, 0.619f, 0.98f, 1.0f);

	// key detection
	glutKeyboardFunc(keyboard);

	// GLUT function initiliazation
	glutDisplayFunc(display);
	glutReshapeFunc(resize);

	// Start display
	glutMainLoop();
	return 0;
}