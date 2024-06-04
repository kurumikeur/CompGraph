
#include "Render.h"

#include <sstream>
#include <iostream>
#include <vector>
#include <math.h>

#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "GUItextRectangle.h"

bool textureMode = true;
bool lightMode = true;
bool changetexture = true;

//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera()
	{
		//отвечает за поворот камеры мышкой
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		//функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		glDisable(GL_LIGHTING);

		
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();
		
		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}

	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света





//старые координаты мыши
int mouseX = 0, mouseY = 0;

void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}

	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

void mouseWheelEvent(OpenGL *ogl, int delta)
{

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;

}

void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}
	if (key == 'Z') 
	{
		changetexture = !changetexture;
		
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}
}

void keyUpEvent(OpenGL *ogl, int key)
{
	
}

GLuint texId;
RGBTRIPLE* texarray, * texarray1;
char* texCharArray;
char* texCharArray1;
int texW, texH, texW1, texH1;

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	

	//массив трехбайтных элементов  (R G B)
	OpenGL::LoadBMP("texture.bmp", &texW, &texH, &texarray);
	OpenGL::RGBtoChar(texarray, texW, texH, &texCharArray);

	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

	OpenGL::LoadBMP("maxwell.bmp", &texW1, &texH1, &texarray1);
	OpenGL::RGBtoChar(texarray1, texW1, texH1, &texCharArray1);

	//генерируем ИД для текстуры
	//биндим айдишник, все что будет происходить с текстурой, будте происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texId);
	//загружаем текстуру в видеопямять, в оперативке нам больше  она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW1, texH1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray1);
	glBindTexture(GL_TEXTURE_2D, texId);


	//наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	camera.fi1 = -1.3;
	camera.fi2 = 0.8;
}


void Center(double center[3], const double A[3], const double B[3], const double C[3], const double D[3]) {
	for (int i = 0; i < 2; i++) {
		center[i] = (A[i] + B[i] + C[i] + D[i]) / 4;
	}
	center[2] = 0;
}
float alpha(const double A[3], const double B[3]) {
	float x1 = A[1] - B[1];
	float x2 = 1;
	float y1 = A[0] - B[0];
	float y2 = 0;
	return (180 / 3.14) * acos((x1 * x2 + y1 * y2) / (sqrt(x1 * x1 + y1 * y1) * sqrt(x2 * x2 + y2 * y2)));
}
void VectorSet(double *vec, const double a[3], const double b[3]) {
	for (int i = 0; i < 3; i++) {
		vec[i] = b[i] - a[i];
	}
}
void VectorMult(double *vec, double vec1[3], double vec2[3]) {
	vec[1] = vec1[0] * vec2[2] - vec2[0] * vec1[2];
	vec[0] = -vec1[1] * vec2[2] + vec2[1] * vec1[2];
	vec[2] = vec1[1] * vec2[0] - vec2[1] * vec1[0];
}
void VectorNormalize(double *vec) {
	float length = sqrt(pow(vec[0], 2) + pow(vec[1], 2) + pow(vec[2], 2));
	vec[0] = vec[0] / length;
	vec[1] = vec[1] / length;
	vec[2] = vec[2] / length;
}
void DrawQuad(const double A[3], const double A1[3], const double B1[3], const double B[3]) {
	double* vec1 { new double[3] };
	double *vec2 { new double[3] };
	double *vec3 { new double[3] };
	glBegin(GL_QUADS);
	VectorSet(vec1, A, A1);
	VectorSet(vec2, A, B);
	VectorMult(vec3, vec2, vec1);
	VectorNormalize(vec3);

	glNormal3dv(vec3);
	glTexCoord2d(0, 0);
	glVertex3dv(A);
	glTexCoord2d(1, 0);
	glVertex3dv(A1);
	glTexCoord2d(1, 1);
	glVertex3dv(B1);
	glTexCoord2d(0, 1);
	glVertex3dv(B);
	glEnd();

}
void DrawQuadSph(int step, double offset, const double A[3], const double A1[3], const double B1[3], const double B[3]) {
	double* vec1{ new double[3] };
	double* vec2{ new double[3] };
	double* vec3{ new double[3] };
	glBegin(GL_QUADS);
	VectorSet(vec1, A, A1);
	VectorSet(vec2, A, B);
	VectorMult(vec3, vec2, vec1);
	VectorNormalize(vec3);

	glNormal3dv(vec3);
	glTexCoord2d(0, offset * (step + 1));
	glVertex3dv(A1);
	glTexCoord2d(1, offset * (step + 1));
	glVertex3dv(A);
	glTexCoord2d(1, offset * step);
	glVertex3dv(B);
	glTexCoord2d(0, offset * step);
	glVertex3dv(B1);
	glEnd();

}
void Sph(int step, double r, const double start[3], const double start1[3], const double end[3], const double end1[3], float a, double center[3], double vec1[3], double vec2[3]) {
	double A[] = { 0, 0, 0 };
	double B[] = { 0, 0, 0 };
	double C[] = { 0, 0, 0 };
	double D[] = { 0, 0, 0 };
	double* vec3{ new double[3] };
	float angle_offset = 3.14 / step;
	double offset = 1 / (double)step;

	glPushMatrix();
	glTranslated(0.5, 3, 0);
	glTranslated(17, 4, 0);
	glRotated(a, 0, 0, 1);
	glTranslated(-17, -4, 0);
	glBegin(GL_QUADS);
	glColor3d(0.5, 0.5, 0.5);
	B[2] = start1[2];
	for (int i = 0; i < 3; i++) {
		C[i] = start[i];
		D[i] = start1[i];
	}


	/*Построение выпуклости*/
	for (int j = 0; j < step; j++) {
		A[1] = start[1] + r * sin(3.14 / 2) * cos(angle_offset * j);
		A[0] = start[0] + r * sin(3.14 / 2) * 1 / 2 * sin(angle_offset * j);
		B[1] = A[1];
		B[0] = A[0];

		C[1] = start[1] + r * sin(3.14 / 2) * cos(angle_offset * (j + 1));
		C[0] = start[0] + r * sin(3.14 / 2) * 1 / 2 * sin(angle_offset * (j + 1));
		D[1] = C[1];
		D[0] = C[0];

		DrawQuadSph(j, offset, D, C, A, B);

	}
	glEnd();

	/*Построение нижней части*/
	glBegin(GL_POLYGON);
	VectorMult(vec3, vec2, vec1);
	VectorNormalize(vec3);
	glNormal3dv(vec3);
	glVertex3dv(start);
	for (int j = 0; j <= step; j++) {
		A[1] = start[1] + r * sin(3.14 / 2) * cos(angle_offset * j);
		A[0] = start[0] + r * sin(3.14 / 2) * 1 / 2 * sin(angle_offset * j);

		glVertex3dv(A);
	}
	glEnd();
	/*Построене верхней части*/
	glBegin(GL_POLYGON);
	VectorMult(vec3, vec1, vec2);
	VectorNormalize(vec3);
	glNormal3dv(vec3);
	glVertex3dv(start1);
	for (int j = 0; j <= step; j++) {
		B[1] = start[1] + r * sin(3.14 / 2) * cos(angle_offset * j);
		B[0] = start[0] + r * sin(3.14 / 2) * 1 / 2 * sin(angle_offset * j);


		glVertex3dv(B);
	}
	glVertex3dv(start1);
	glEnd();
	glPopMatrix();
}
void сircle(int step, double r) {
	double A[] = { 0, 0, 0 };
	double B[] = { 0, 0, 0 };
	double C[] = { -3, -3, 0 };				//Центр круга
	float angle_offset = 3.14 / step;
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2d(0.5, 0.5);
	glVertex3dv(C);
	for (int j = 0; j < step; j++) {		//Первая половина круга
		A[1] = C[1] + r * cos(angle_offset * j);
		A[0] = C[0] + r * sin(angle_offset * j);
		B[1] = C[1] + r * cos(angle_offset * (j + 1));
		B[0] = C[0] + r * sin(angle_offset * (j + 1));
		glTexCoord2d(0.5 - sin(angle_offset * j) / 2, 0.5 - cos(angle_offset * j) / 2);
		glVertex3dv(A);
		glTexCoord2d(0.5 - sin(angle_offset * (j + 1)) / 2, 0.5 - cos(angle_offset * (j + 1)) / 2);
		glVertex3dv(B);

	}
	for (int j = 0; j < step; j++) {		//Вторая половина круга
		A[1] = C[1] - r * cos(angle_offset * j);
		A[0] = C[0] - r * sin(angle_offset * j);
		B[1] = C[1] - r * cos(angle_offset * (j + 1));
		B[0] = C[0] - r * sin(angle_offset * (j + 1));
		glTexCoord2d(0.5 + sin(angle_offset * j) / 2, 0.5 + cos(angle_offset * j) / 2);
		glVertex3dv(A);
		glTexCoord2d(0.5 + sin(angle_offset * (j + 1)) / 2, 0.5 + cos(angle_offset * (j + 1)) / 2);
		glVertex3dv(B);
	}
	glEnd();
}

void Render(OpenGL *ogl)
{
	const double A[] = { 4, 4, 0 };
	const double B[] = { 9, 10, 0 };
	const double C[] = { 7, 14, 0 };
	const double D[] = { 13, 17, 0 };
	const double E[] = { 12, 11, 0 };
	const double F[] = { 18, 10, 0 };
	const double G[] = { 17, 4, 0 };
	const double H[] = { 10, 8, 0 };
	const double A1[] = { 4, 4, 5 };
	const double B1[] = { 9, 10, 5 };
	const double C1[] = { 7, 14, 5 };
	const double D1[] = { 13, 17, 5 };
	const double E1[] = { 12, 11, 5 };
	const double F1[] = { 18, 10, 5 };
	const double G1[] = { 17, 4, 5 };
	const double H1[] = { 10, 8, 5 };
	double center[] = { 0, 0, 0 };

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (changetexture) {
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW1, texH1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray1);
		glBindTexture(GL_TEXTURE_2D, texId);
	}
	else {
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);
		glBindTexture(GL_TEXTURE_2D, texId);
	}

	if (lightMode)
		glEnable(GL_LIGHTING);




	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;


	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	glShadeModel(GL_SMOOTH);


	//Начало рисования призмы
	double* vec1{ new double[3] };
	double* vec2{ new double[3] };
	VectorSet(vec2, A1, H1);
	VectorSet(vec1, A1, B1);
	float a = alpha(F, G);
	Center(center, F, G, F1, G1);
	double Length = sqrt(pow(F[0] - G[0], 2) + pow(F[1] - G[1], 2));

	Sph(10, Length / 2, G, G1, F, F1, -a, center, vec1, vec2);
	сircle(10, 2);
		//Боковые грани призмы
		DrawQuad(A, A1, B1, B);
		DrawQuad(B, B1, C1, C);
		DrawQuad(C, C1, D1, D);
		DrawQuad(D, D1, E1, E);
		glColor3d(0.3, 0.3, 0.3);
		DrawQuad(E, E1, F1, F);
		DrawQuad(G, G1, H1, H);
		DrawQuad(H, H1, A1, A);

		glColor3d(0, 0, 0);
		//Нижняя грань призмы
		DrawQuad(A, B, E, H);
		DrawQuad(H, E, F, G);
		DrawQuad(C, D, E, B);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(1, 0, 0, 0.3);
		//Верхняя грань призмы
		DrawQuad(H1, E1, B1, A1);
		DrawQuad(G1, F1, E1, H1);
		DrawQuad(B1, E1, D1, C1);

	//конец рисования призмы


   //Сообщение вверху экрана

	
	glMatrixMode(GL_PROJECTION);	//Делаем активной матрицу проекций. 
	                                
	glPushMatrix();   //сохраняем текущую матрицу проецирования (которая описывает перспективную проекцию) в стек 				    
	glLoadIdentity();	  //Загружаем единичную матрицу
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);	 //врубаем режим ортогональной проекции

	glMatrixMode(GL_MODELVIEW);		//переключаемся на модел-вью матрицу
	glPushMatrix();			  //сохраняем текущую матрицу в стек 
	glLoadIdentity();		  

	glDisable(GL_LIGHTING);



	GuiTextRectangle rec;		   
	rec.setSize(300, 150);
	rec.setPosition(10, ogl->getHeight() - 150 - 10);


	std::stringstream ss;
	ss << "T - вкл/выкл текстур" << std::endl;
	ss << "Z - сменить текстуру" << std::endl;
	ss << "L - вкл/выкл освещение" << std::endl;
	ss << "F - Свет из камеры" << std::endl;
	ss << "G - двигать свет по горизонтали" << std::endl;
	ss << "G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << "Коорд. света: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
	ss << "Коорд. камеры: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
	ss << "Параметры камеры: R="  << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;
	
	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION);	  
	glPopMatrix();


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}