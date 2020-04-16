#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vector>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		
#include <GL/freeglut.h>	
#endif

const unsigned int windowWidth = 600, windowHeight = 600;

int majorVersion = 3, minorVersion = 3;

void getErrorInfo(unsigned int handle) {
	int logLen;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0) {
		char * log = new char[logLen];
		int written;
		glGetShaderInfoLog(handle, logLen, &written, log);
		printf("Shader log:\n%s", log);
		delete log;
	}
}

void checkShader(unsigned int shader, char * message) {
	int OK;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
	if (!OK) { printf("%s!\n", message); getErrorInfo(shader); }
}

void checkLinking(unsigned int program) {
	int OK;
	glGetProgramiv(program, GL_LINK_STATUS, &OK);
	if (!OK) { printf("Failed to link shader program!\n"); getErrorInfo(program); }
}

const char * vertexSource = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			

	layout(location = 0) in vec2 vertexPosition;	
	layout(location = 1) in vec3 vertexColor;	    
	out vec3 color;									

	void main() {
		color = vertexColor;														
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		
	}
)";

const char * fragmentSource = R"(
	#version 330
    precision highp float;

	in vec3 color;				
	out vec4 fragmentColor;		

	void main() {
		fragmentColor = vec4(color, 1); 
	}
)";

struct mat4 {
	float m[4][4];
public:
	mat4() {}
	mat4(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33) {
		m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
		m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
		m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
		m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
	}

	mat4 operator*(const mat4& right) {
		mat4 result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.m[i][j] = 0;
				for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
			}
		}
		return result;
	}
	operator float*() { return &m[0][0]; }
};

struct vec4 {
	float x, y, z, w;
	vec4(float _x = 0, float _y = 0, float _z = 0, float _w = 1) { x = _x; y = _y; z = _z; w = _w; }
	vec4 operator*(const mat4& mat) {
		return vec4(x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0] + w * mat.m[3][0],
			x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1] + w * mat.m[3][1],
			x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2] + w * mat.m[3][2],
			x * mat.m[0][3] + y * mat.m[1][3] + z * mat.m[2][3] + w * mat.m[3][3]);
	}
};

float dot(const vec4& left, const vec4& right) {
	return left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w;
}

struct Camera {
	float wCx, wCy;	
	float wWx, wWy;	
public:
	Camera() {
		Animate(0);
	}

	mat4 V() { 
		return mat4(1,    0, 0, 0,
			        0,    1, 0, 0,
			        0,    0, 1, 0,
			     -wCx, -wCy, 0, 1);
	}

	mat4 P() { 
		return mat4(2/wWx,    0, 0, 0,
			        0,    2/wWy, 0, 0,
			        0,        0, 1, 0,
			        0,        0, 0, 1);
	}

	mat4 Vinv() { 
		return mat4(1,     0, 0, 0,
				    0,     1, 0, 0,
			        0,     0, 1, 0,
			        wCx, wCy, 0, 1);
	}

	mat4 Pinv() { 
		return mat4(wWx/2, 0,    0, 0,
			           0, wWy/2, 0, 0,
			           0,  0,    1, 0,
			           0,  0,    0, 1);
	}

	void Animate(float t) {
		wCx = 0; 
		wCy = 0;
		wWx = 20;
		wWy = 20;
	}
};

Camera camera;

unsigned int shaderProgram;

class Triangle {
	unsigned int vao;	
	float sx, sy;		
	float wTx, wTy;		
public:
	Triangle() { Animate(0); }

	void Create() {
		glGenVertexArrays(1, &vao);	
		glBindVertexArray(vao);		

		unsigned int vbo[2];		
		glGenBuffers(2, &vbo[0]);	

		
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); 
		float vertexCoords[] = { -8, -8, -6, 10, 8, -2 };	
		glBufferData(GL_ARRAY_BUFFER,      
			         sizeof(vertexCoords), 
					 vertexCoords,		   
					 GL_STATIC_DRAW);	   
		
		glEnableVertexAttribArray(0); 
		
		glVertexAttribPointer(0,			
			                  2, GL_FLOAT,  
							  GL_FALSE,		
							  0, NULL);     

		
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); 
		float vertexColors[] = { 1, 0, 0,  0, 1, 0,  0, 0, 1 };	
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);	

		
		glEnableVertexAttribArray(1);  
		
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); 
	}

	void Animate(float t) {
		sx = 1; 
		sy = 1; 
		wTx = 0; 
		wTy = 0; 
	}

	void Draw() {
		mat4 Mscale(sx, 0, 0, 0,
			        0, sy, 0, 0,
			        0, 0, 0, 0,
			        0, 0, 0, 1); 

		mat4 Mtranslate(1,   0,  0, 0,
			            0,   1,  0, 0,
			            0,   0,  0, 0,
			          wTx, wTy,  0, 1); 

		mat4 MVPTransform = Mscale * Mtranslate * camera.V() * camera.P();

		
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVPTransform); 
		else printf("uniform MVP cannot be set\n");

		glBindVertexArray(vao);	
		glDrawArrays(GL_TRIANGLES, 0, 3);	
	}
};

class LineStrip {
	GLuint vao, vbo;        
	float  vertexData[100]; 
	int    nVertices;       
public:
	LineStrip() {
		nVertices = 0;
	}
	void Create() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo); 
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		
		glEnableVertexAttribArray(0);  
		glEnableVertexAttribArray(1);  
		
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0)); 
		
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
	}

	void AddPoint(float cX, float cY) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		if (nVertices >= 20) return;

		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		
		vertexData[5 * nVertices]     = wVertex.x;
		vertexData[5 * nVertices + 1] = wVertex.y;
		vertexData[5 * nVertices + 2] = 1; 
		vertexData[5 * nVertices + 3] = 1; 
		vertexData[5 * nVertices + 4] = 0; 
		nVertices++;
		
		glBufferData(GL_ARRAY_BUFFER, nVertices * 5 * sizeof(float), vertexData, GL_DYNAMIC_DRAW);
	}

	void Draw() {
		if (nVertices > 0) {
			mat4 VPTransform = camera.V() * camera.P();

			int location = glGetUniformLocation(shaderProgram, "MVP");
			if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, VPTransform);
			else printf("uniform MVP cannot be set\n");

			glBindVertexArray(vao);
			glDrawArrays(GL_LINE_STRIP, 0, nVertices);
		}
	}
};

Triangle triangle;
LineStrip lineStrip;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	
	lineStrip.Create();
	triangle.Create();

	
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertexShader) {
		printf("Error in vertex shader creation\n");
		exit(1);
	}
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	checkShader(vertexShader, "Vertex shader error");

	
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fragmentShader) {
		printf("Error in fragment shader creation\n");
		exit(1);
	}
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	checkShader(fragmentShader, "Fragment shader error");

	
	shaderProgram = glCreateProgram();
	if (!shaderProgram) {
		printf("Error in shader program creation\n");
		exit(1);
	}
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");	

	
	glLinkProgram(shaderProgram);
	checkLinking(shaderProgram);
	
	glUseProgram(shaderProgram);
}

void onExit() {
	glDeleteProgram(shaderProgram);
	printf("exit");
}

void onDisplay() {
	glClearColor(0, 0, 0, 0);							
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	triangle.Draw();
	lineStrip.Draw();
	glutSwapBuffers();									
}

void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         
}

void onKeyboardUp(unsigned char key, int pX, int pY) {

}

void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  
		float cX = 2.0f * pX / windowWidth - 1;	
		float cY = 1.0f - 2.0f * pY / windowHeight;
		lineStrip.AddPoint(cX, cY);
		glutPostRedisplay();     
	}
}

void onMouseMotion(int pX, int pY) {
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); 
	float sec = time / 1000.0f;				
	camera.Animate(sec);					
	triangle.Animate(sec);					
	glutPostRedisplay();					
}


int main(int argc, char * argv[]) {
	glutInit(&argc, argv);
#if !defined(__APPLE__)
	glutInitContextVersion(majorVersion, minorVersion);
#endif
	glutInitWindowSize(windowWidth, windowHeight);				
	glutInitWindowPosition(100, 100);							
#if defined(__APPLE__)
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_3_CORE_PROFILE);  
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutCreateWindow(argv[0]);

#if !defined(__APPLE__)
	glewExperimental = true;	
	glewInit();
#endif

	printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
	printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
	printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
	printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	onInitialization();

	glutDisplayFunc(onDisplay);                
	glutMouseFunc(onMouse);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutKeyboardUpFunc(onKeyboardUp);
	glutMotionFunc(onMouseMotion);

	glutMainLoop();
	onExit();
	return 1;
}

