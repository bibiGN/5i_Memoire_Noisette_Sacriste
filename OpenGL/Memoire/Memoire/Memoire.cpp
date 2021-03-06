// ---------------------------------------------------------------------------
//
// ESGI OpenGL (ES) 2.0 Framework
// Malek Bengougam, 2012							malek.bengougam@gmail.com
//
// ---------------------------------------------------------------------------

// --- Includes --------------------------------------------------------------
#define _USE_MATH_DEFINES

#include "../../EsgiGL/EsgiGL.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>

#include "../../EsgiGL/Common/vector.h"
#include "../../EsgiGL/Common/matrix.h"
#include "../../EsgiGL/EsgiShader.h"

#include "PMC.h"

using namespace std;

// --- Globales --------------------------------------------------------------

static vector<vec3> listMousePoints;

static int sizeWindowX = 600;
static int sizeWindowY = 600;
static int nbAnglesNeed = 10;
static int typeForm = 0.0f; // 0 - Carre, 1 - Cercle, 2 - Ligne, 3 - Z, 4 - U, 5 - 8, 6 - S, 7 - Triangle, 8 - Etoile, 9 - escargot
static bool inputMode = true;
static bool eraseFileInput = true;
PMC * perceptron;

EsgiShader shader;

struct EsgiCamera
{
	vec3 position;
	vec3 target;
	vec3 orientation;
};
EsgiCamera camera;

// --- Fonctions -------------------------------------------------------------

// ---

void Update(float elapsedTime)
{
}

void Draw()
{
	// efface le color buffer et le depth buffer
	glClearColor(1.f, 1.f, 1.f, 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

#ifndef ESGI_GLES_20
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

	// ---
	GLuint programObject = shader.GetProgram();
	glUseProgram(programObject);
	// alternativement
	// shader.Bind();
	// ---

	GLint position_attrib = glGetAttribLocation(programObject, "a_Position");
	glVertexAttribPointer(position_attrib, 3, GL_FLOAT, false, 0, listMousePoints.data());
	
	GLint color_uniform = glGetUniformLocation(programObject, "u_Color");
	glUniform4f(color_uniform, 0.f, 0.f, 0.f, 1.f);
	
	mat4 projectionMatrix = esgiPerspective(45, (float)sizeWindowX/sizeWindowY, 0.1f, 1000.f);
	GLint projectionUniform = glGetUniformLocation(programObject, "u_ProjectionMatrix");
	glUniformMatrix4fv(projectionUniform, 1, false, &projectionMatrix.I.x);
	
	mat4 cameraMatrix = esgiLookAt(camera.position, camera.target, vec3(0.0, 1.0, 0.0));
	mat4 viewMatrix = esgiMultiplyMatrix(cameraMatrix, esgiRotateY(camera.orientation.y));
	GLuint viewUniform = glGetUniformLocation(programObject, "u_ViewMatrix");
	glUniformMatrix4fv(viewUniform, 1, false, &viewMatrix.I.x);

	mat4 worldMatrix;
	worldMatrix.Identity();
	worldMatrix.T.set(0.f, 0.f, -700.f, 1.f);
	GLint worldUniform = glGetUniformLocation(programObject, "u_WorldMatrix");
	glUniformMatrix4fv(worldUniform, 1, 0, &worldMatrix.I.x);

	glEnableVertexAttribArray(position_attrib);
	glDrawArrays(GL_POINTS, 0, listMousePoints.size());
	glDisableVertexAttribArray(position_attrib);

	// alternativement
	// shader.Unbind();
}

//
// Initialise les shader & mesh
//
bool Setup()
{
	shader.LoadVertexShader("basic.vert");
	shader.LoadFragmentShader("basic.frag");
	shader.Create();

	camera.position = vec3(0.f, 0.f, 10.f);
	camera.orientation = vec3(0.f, 0.f, 0.f);
	camera.target = vec3(0.f, 0.f, 0.f);

	int myints[] = {10,50,50,10};
	std::vector<int> sizePMC (myints, myints + sizeof(myints) / sizeof(int) );
	perceptron = new PMC(sizePMC, 5000, 0.25, 0.9, true);
	perceptron->LaunchLearning("inputs10_essais_10angles.txt");
	
	return true;
}

//
// Libere la memoire occupee
//
void Clean()
{
	shader.Destroy();
}

//
// Fonction d'event de la souris
//
void ActiveMouse(int mousex, int mousey)
{
	float tempX = mousex - sizeWindowX/2, tempY = (mousey - sizeWindowY/2) * -1;
	//cout << "tempX " << tempX << " tempY " << tempY << endl;
	listMousePoints.push_back(vec3(tempX, tempY, 0));
}
void PassiveMouse(int mousex, int mousey)
{
}

double orientedAngle(const vec3 & p1, const vec3 & p2, const vec3 & p3)
{
   vec3 v1 = p2 - p1;
   vec3 v2 = p3 - p1;

   double angle = (atan2f(v1.y, v1.x) - atan2f(v2.y, v2.x)) * 180 / M_PI;
   if (angle < 0)
	   angle += 360;
   return angle/360;
}

void Keyboard(unsigned char key, int mx, int my)
{
	switch(key)
	{
	case 'c':
	case 'C':
		//cout << "Clear" << endl;
		listMousePoints.clear();
		break;
	case 'i':
	case 'I':
		inputMode = !inputMode;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		{
			char arr[] = "X";
			arr[0] = key;
			typeForm = atoi(arr);
		} break;
	case 's':
	case 'S':
		if (listMousePoints.size() >= (unsigned int)(nbAnglesNeed + 2)) // Need 2 points more for to have nbAnglesNeed
		{
			double offsetPoint = (double)(listMousePoints.size()) / (double)(nbAnglesNeed + 1);
			vector<vec3> listPointsSelected;
			for (double i = 0; i < listMousePoints.size(); i += offsetPoint)
			{
				listPointsSelected.push_back(listMousePoints.at((int)i));
			}
			if (listPointsSelected.size() < nbAnglesNeed + 2)
				listPointsSelected.push_back(listMousePoints.at(listMousePoints.size() - 1));

			ofstream myfile;
			if (eraseFileInput)
				myfile.open ("inputs.txt", ios::out | ios::trunc);
			else
				myfile.open ("inputs.txt", ios::out | ios::app);
			for (unsigned int i = 1; i < listPointsSelected.size() - 1; ++i)
			{
				myfile << orientedAngle(listPointsSelected.at(i), listPointsSelected.at(i-1), listPointsSelected.at(i+1)) << " ";
			}

			if (!inputMode)
			{
				for (int i = 0; i < 10; ++i)
				{
					if (i == typeForm)
						myfile << 1;
					else
						myfile << 0;
					if (i < 10 - 1)
						myfile << " ";
					else
						myfile << "\n";
				}
			}
			myfile.close();
		} break;
	case 't':
	case 'T':
		{
			perceptron->Evaluate("inputs.txt");
		} break;
	case 'e':
	case 'E':
		{
			eraseFileInput = !eraseFileInput;
		} break;
	}
}

// 
//
//
int main(int argc, char *argv[])
{
	EsgiGLApplication esgi;
    
	esgi.InitWindowPosition(0, 0);
	esgi.InitWindowSize(sizeWindowX, sizeWindowY);
	esgi.InitDisplayMode(ESGI_WINDOW_RGBA|ESGI_WINDOW_DEPTH|ESGI_WINDOW_DOUBLEBUFFER);
	esgi.CreateWindow("M�moire", ESGI_WINDOW_CENTERED);
	
    esgi.IdleFunc(&Update);
	esgi.DisplayFunc(&Draw);
    esgi.InitFunc(&Setup);
    esgi.CleanFunc(&Clean);
	esgi.MotionFunc(&ActiveMouse);
	esgi.PassiveMotionFunc(&PassiveMouse);
	esgi.KeyboardFunction(&Keyboard);
    
	esgi.MainLoop();
    
    return 0;
}