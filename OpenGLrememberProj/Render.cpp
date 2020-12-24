#include "Render.h"
#include <sstream>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include "MyShaders.h"
#include "ObjLoader.h"
#include "Texture.h"
#include "MyOGL.h"
#include "Camera.h"
#include "Light.h"
#include "Primitives.h"
#include "GUItextRectangle.h"
#include "bass.h"
#pragma comment (lib, "bass.lib")

GLdouble maxHeight = 30.0;
bool operatorMode = false;
bool gameStarted = true;
bool fogMode = true;
bool lightMode = true;
bool textureMode = true;
bool lightLockMode = false;

// Класс для настройки камеры
class CustomCamera : public Camera {
public:
	// Дистанция камеры
	double camDist;
	// Углы поворота камеры
	double fi1, fi2;

	// Значния камеры по умолчанию
	CustomCamera() {
		camDist = maxHeight + 20.0;
		fi1 = M_PI / 180.0 * -90.0;
		fi2 = M_PI / 180.0 * 45.0;
	}

	// Считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera() {
		// Отвечает за поворот камеры мышкой
		lookPoint.setCoords(0.0, 0.0, maxHeight);

		pos.setCoords(camDist * cos(fi2) * cos(fi1),
					  camDist * cos(fi2) * sin(fi1),
					  camDist * sin(fi2));

		if (cos(fi2) <= 0) {
			normal.setCoords(0, 0, -1);
		} else normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt() {
		// Функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}
}  camera; // Создаем объект камеры

// Класс для настройки света
class CustomLight : public Light {
public:
	CustomLight() {
		// начальная позиция света
		pos = Vector3(0.0, 0.0, 0.0);
	}

	// рисует сферу и линии под источником света, вызывается движком
	void DrawLightGhismo() {
		//glDisable(GL_FOG);
		//glDisable(GL_LIGHTING);
		//glDisable(GL_TEXTURE_2D);
		//glColor3d(1.0, 1.0, 0.0);

		//Sphere sphere;
		//sphere.pos = pos;
		//sphere.scale = sphere.scale * 10.0;
		//sphere.Show();

		//fogMode ? glEnable(GL_FOG) : glDisable(GL_FOG);
		//lightMode ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
		//textureMode ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
	}

	void SetUpLight() {
		GLfloat ambient[] = { 0.2, 0.2, 0.2, 0.0 };
		GLfloat diffuse[] = { 1.0, 1.0, 1.0, 0.0 };
		GLfloat specular[] = { 0.7, 0.7, 0.7, 0.0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1.0 };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); // фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); // диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, specular); // зеркально отражаемая составляющая света

		glEnable(GL_LIGHT0);
	}
} light; // Создаем источник света

//===========================================
struct BiomeParams {
	GLuint biomeId;
	GLdouble minAltitude;
	GLdouble maxAltitude;
	GLdouble scale;
};

class TriangularSquare_withNormals {
private:
	void calculateNormal(double *firstPoint, double *secondPoint, double *resultNormal) {
		double x_1 = firstPoint[0];
		double y_1 = firstPoint[1];
		double z_1 = firstPoint[2];

		double x_2 = secondPoint[0];
		double y_2 = secondPoint[1];
		double z_2 = secondPoint[2];

		double x_3 = center[0];
		double y_3 = center[1];
		double z_3 = center[2];

		double line_1[] = { x_2 - x_1, y_2 - y_1, z_2 - z_1 };
		double line_2[] = { x_2 - x_3, y_2 - y_3, z_2 - z_3 };

		double n_x = line_1[1] * line_2[2] - line_2[1] * line_1[2];
		double n_y = line_1[0] * line_2[2] - line_2[0] * line_1[2];
		double n_z = line_1[0] * line_2[1] - line_2[0] * line_1[1];

		double length = sqrt(pow(n_x, 2.0) + pow(n_y, 2.0) + pow(n_z, 2.0));
		resultNormal[0] = n_x / length;
		resultNormal[1] = -n_y / length;
		resultNormal[2] = n_z / length;
	}

public:
	GLdouble lowerLeft[3];
	GLdouble lowerRight[3];
	GLdouble upperRight[3];
	GLdouble upperLeft[3];
	GLdouble center[3];

	GLdouble lowerTriangleNormal[3];
	GLdouble rightTriangleNormal[3];
	GLdouble upperTriangleNormal[3];
	GLdouble leftTriangleNormal[3];

	//GLuint textureId;
	//GLdouble color[3];

	void setLowerLeft(GLdouble x, GLdouble y, GLdouble z) {
		lowerLeft[0] = x;
		lowerLeft[1] = y;
		lowerLeft[2] = z;
	}

	void setLowerRight(GLdouble x, GLdouble y, GLdouble z) {
		lowerRight[0] = x;
		lowerRight[1] = y;
		lowerRight[2] = z;
	}

	void setUpperRight(GLdouble x, GLdouble y, GLdouble z) {
		upperRight[0] = x;
		upperRight[1] = y;
		upperRight[2] = z;
	}

	void setUpperLeft(GLdouble x, GLdouble y, GLdouble z) {
		upperLeft[0] = x;
		upperLeft[1] = y;
		upperLeft[2] = z;
	}

	void calculateCenter() {
		center[0] = (lowerLeft[0] + upperRight[0]) / 2.0;
		center[1] = (lowerLeft[1] + upperRight[1]) / 2.0;
		center[2] = (lowerLeft[2] + lowerRight[2] + upperRight[2] + upperLeft[2]) / 4.0;
	}

	double getCenterCoord_Z() {
		return center[2];
	}

	//void setTexture(GLuint textureId) {
		//this->textureId = textureId;
	//}

	//void setColor(GLdouble red, GLdouble green, GLdouble blue) {
		//color[0] = red;
		//color[1] = green;
		//color[2] = blue;
	//}

	void calculateNormals() {
		calculateNormal(lowerLeft, lowerRight, lowerTriangleNormal);
		calculateNormal(lowerRight, upperRight, rightTriangleNormal);
		calculateNormal(upperRight, upperLeft, upperTriangleNormal);
		calculateNormal(upperLeft, lowerLeft, leftTriangleNormal);
	}
};

class Chunk {
public:
	std::vector <TriangularSquare_withNormals> chunkPolygons;
	std::vector <GLdouble> rightSide_EndCoords;
	std::vector <GLdouble> upperSide_EndCoords;

	std::vector <GLuint> heightIds;
	std::vector <GLuint> textureIds;
	std::vector <std::vector <GLdouble>> colors;

	Chunk() {

	}

	Chunk(std::vector <TriangularSquare_withNormals> chunkPolygons,
		  std::vector <GLdouble> rightSide_EndCoords,
		  std::vector <GLdouble> upperSide_EndCoords) {
		this->chunkPolygons = chunkPolygons;
		this->rightSide_EndCoords = rightSide_EndCoords;
		this->upperSide_EndCoords = upperSide_EndCoords;
	}
};

struct Ring {
	double speed, size, x, y, z;
	DWORD64 deadMSeconds;
};
std::vector<Ring *> rings;

class Biome {
public:
	std::vector <Chunk> chunks;
	std::vector <std::vector <GLdouble>> currentBiome_EndCoords;
	std::vector <std::vector <GLdouble>> previousBiome_EndCoords;
	BiomeParams biomeParams;
};

std::atomic<bool> start_new { true };
std::atomic<bool> thread_done { false };

// Какая-то ***** параметры
ObjFile airPlaneModel, moonModel;
int translateIndexator = 0;
int currentBiomeId = 0;
int currentBiomeLength = 0;
bool manualControl = false;
double backgroundColor = 0.0;
GLdouble prevAirPlaneTranslation[3];
GLdouble airPlaneTranslation[3] = { 0.0, 0.0, maxHeight };

// Параметры отрисовываемой области (лучше не трогать)
const GLint visibleBiomeHeight = 2;
const GLint visibleBiomeWidth = 2 * visibleBiomeHeight + 1;
// Быстрое переключение, работает, если quickMultiplier > 1
// Автоматически определяет значения для:
// polygonSize, polygonNumberPerSide, polygonNumberInChunk, BiomeParams.scale
const GLint quickMultiplier = 7;
GLdouble polygonSize = pow(2, -1);				// HD = -1	: stable = 0	: no_lag = 1
GLint polygonNumberPerSide = (int) pow(2, 8);	// HD = 8	: stable = 7	: no_lag = 6
GLint polygonNumberInChunk = (int) pow(polygonNumberPerSide, 2); // do not touch

// Уровни покровов
const GLdouble waterLevel = 0.0;
const GLdouble groundLevel = 5.0;
const GLdouble stoneLevel = 20.0;
const GLdouble snowLevel = 25.0;

// Быстрое переключение, работает, если quickMultiplier > 3
std::vector<BiomeParams> biomeParams = {
	{ 0, -5.0,			waterLevel,		2 },	// море
	{ 1, waterLevel,	groundLevel,	2 },	// равнина (земляная)
	{ 2, waterLevel,	groundLevel,	2 },	// равнина (снежная)
	{ 3, waterLevel,	groundLevel,	1 },	// равнина (песчанная)
	{ 4, -groundLevel,	groundLevel,	1 },	// болото
	{ 5, waterLevel,	snowLevel,		0 },	// горы
	{ 6, -groundLevel,	snowLevel,		0 },	// горы (с водой)
	{ 7, -groundLevel,	groundLevel,	3 },	// берега (земляные)
	{ 8, -groundLevel,	groundLevel,	3 },	// берега (снежные)
	{ 9, -groundLevel,	groundLevel,	3 },	// берега (песчанные)
	{ 10, -waterLevel,	groundLevel,	0 },	// холмы (земляные)
	{ 11, -waterLevel,	groundLevel,	0 },	// холмы (снежная)
	{ 12, -waterLevel,	groundLevel,	0 }		// холмы (песчанная)
};

// Параметры для текстур (не трогать)
int textureId = 0;
Texture airPlaneTexture, moonTexture;
const std::string textureNames[] = {
	"water.bmp",	// 0
	"grass.bmp",	// 1
	"snow.bmp",		// 2
	"sand.bmp",		// 3
	"stone.bmp"		// 4
};
GLuint textures[sizeof(textureNames) / sizeof(std::string)];

// Цвета
const std::vector <std::vector <GLdouble>> colors = {
	{ 0.42, 0.48, 0.97 },	// 0 - water
	{ 0.47, 0.68, 0.33 },	// 1 - grass
	{ 0.94, 0.98, 0.98 },	// 2 - snow
	{ 0.86, 0.83, 0.63 },	// 3 - sand
	{ 0.49, 0.49, 0.49 }	// 4 - stone
};

// Параметры анимации (можно потрогать)
const double animationSpeed = 0.25; // 0.25
const double animationAirPlaneSpeed = 0.005;
double animationLightMoment = 0.0;
double animationTranslationMoment = 0.0;
double animationAirPlaneMoment = 0.0;


void loadTexture(std::string textureName) {
	RGBTRIPLE *textureArray;
	char *textureCharArray;
	int textureWidth, textureHeight;

	std::string texturePath = "textures\\";
	texturePath.append(textureName);
	OpenGL::LoadBMP(texturePath.c_str(), &textureWidth, &textureHeight, &textureArray);
	OpenGL::RGBtoChar(textureArray, textureWidth, textureHeight, &textureCharArray);

	glBindTexture(GL_TEXTURE_2D, textures[textureId]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, textureCharArray);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	free(textureCharArray);
	free(textureArray);
	textureId++;
}

void initializeTextures() {
	glGenTextures(sizeof(textureNames) / sizeof(std::string), textures);
	for (int i = 0; i < sizeof(textureNames) / sizeof(std::string); i++) loadTexture(textureNames[i]);
}

HCHANNEL channel;
void initializeSound() {
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		MessageBox(NULL, "Ошибка версии BASS.", NULL, 0);
		return;
	}
	if (!BASS_Init(-1, 22050, BASS_DEVICE_3D, 0, NULL)) {
		MessageBox(NULL, "Не удалось инициализировать BASS.", NULL, 0);
		return;
	}

	HSAMPLE samp;
	char filename[] = "sound/hit.mp3";
	samp = BASS_SampleLoad(FALSE, filename, 0, 0, 1, BASS_SAMPLE_3D);
	if (!samp) {
		MessageBox(NULL, "Беды с семплом.", NULL, 0);
		return;
	}

	channel = BASS_SampleGetChannel(samp, FALSE);
	if (!channel) {
		MessageBox(NULL, "Беды с каналом.", NULL, 0);
		return;
	}
}


void animationController() {
	animationLightMoment += animationSpeed;
	animationTranslationMoment += animationSpeed;
	animationAirPlaneMoment += animationAirPlaneSpeed;

	if (camera.fi1 < 0.0) camera.fi1 += 2.0 * M_PI;
	if (camera.fi1 > 2.0 * M_PI) camera.fi1 -= 2.0 * M_PI;

	if (animationLightMoment > polygonSize * polygonNumberPerSide * visibleBiomeWidth) animationLightMoment = 0.0;
	if (animationTranslationMoment > polygonSize * polygonNumberPerSide * visibleBiomeHeight - 2.0 * polygonSize) {
		animationTranslationMoment = 0.0;
		translateIndexator++;
		if (translateIndexator > 2) translateIndexator = 0;
	}
	if (animationAirPlaneMoment > 2.0 * M_PI) animationAirPlaneMoment = 0.0;
}

void lightController() {
	if (lightLockMode) animationLightMoment = polygonSize * polygonNumberPerSide * visibleBiomeWidth / 4.0;
	float i = animationLightMoment / (polygonSize * polygonNumberPerSide * visibleBiomeWidth);

	float angle = 2.0 * M_PI * i;
	float dx = visibleBiomeWidth * polygonNumberPerSide * polygonSize * cosf(angle);
	float dz = visibleBiomeWidth * polygonNumberPerSide * polygonSize * sinf(angle);

	light.pos = Vector3(dx, 0.0, dz);

	glDisable(GL_FOG);

	glPushMatrix();
	GLfloat ambient[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat specular[] = { 0.7, 0.7, 0.7, 1.0 };
	GLfloat shininess = 0.0 * 128;

	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);

	double moonSize = polygonNumberPerSide * polygonSize / 4;
	glTranslated(-dx / visibleBiomeWidth, polygonNumberPerSide * polygonSize * visibleBiomeHeight + moonSize, -dz / visibleBiomeWidth);
	glRotated(90.0, 1.0, 0.0, 0.0);
	glRotated(-45.0, 0.0, 0.0, 1.0);
	glScaled(moonSize / 64.0, moonSize / 64.0, moonSize / 64.0);
	glDepthMask(false);
	moonTexture.bindTexture();
	moonModel.DrawObj();
	glDepthMask(true);
	glPopMatrix();

	fogMode ? glEnable(GL_FOG) : glDisable(GL_FOG);
}

void airPlaneControl() {
	double x = 25.0 * (sqrt(2) * cos(animationAirPlaneMoment)) / (1 + pow(sin(animationAirPlaneMoment), 2));
	double z = 10.0 * (sqrt(2) * cos(animationAirPlaneMoment) * sin(animationAirPlaneMoment)) / (1 + pow(sin(animationAirPlaneMoment), 2));
	double y = 10.0 * exp(sin(animationAirPlaneMoment));

	prevAirPlaneTranslation[0] = airPlaneTranslation[0];
	prevAirPlaneTranslation[1] = airPlaneTranslation[1];
	prevAirPlaneTranslation[2] = airPlaneTranslation[2];

	airPlaneTranslation[0] = x;
	airPlaneTranslation[1] = y;
	airPlaneTranslation[2] = maxHeight + 5.0 + z;
}


double getRandomNumber(double lowerBorder, double upperBorder) {
	return lowerBorder + static_cast <double> (rand()) / (static_cast <double> (RAND_MAX / (upperBorder - lowerBorder)));
}

double getRandomHeight(int count, int range, double avg, BiomeParams biomeParams) {
	double biomeBorder_max = biomeParams.maxAltitude * count - avg;
	double biomeBorder_min = biomeParams.minAltitude * count - avg;

	double randomBorder_max = double(range) / biomeParams.scale;
	double randomBorder_min = -randomBorder_max;

	double lowerBorder = max(biomeBorder_min, randomBorder_min);
	double upperBorder = min(biomeBorder_max, randomBorder_max);

	return (avg + getRandomNumber(lowerBorder, upperBorder)) / count;
}


void diamondSquareAlgorithm_SquareStep(std::vector <std::vector <double>> *points,
									   int x, int y, int reach, BiomeParams biomeParams) {
	int count = 0;
	double avg = 0.0;

	if (x - reach >= 0 && y - reach >= 0) {
		avg += (*points)[x - reach][y - reach];
		count++;
	}

	if (x - reach >= 0 && y + reach < polygonNumberPerSide) {
		avg += (*points)[x - reach][y + reach];
		count++;
	}

	if (x + reach < polygonNumberPerSide && y - reach >= 0) {
		avg += (*points)[x + reach][y - reach];
		count++;
	}

	if (x + reach < polygonNumberPerSide && y + reach < polygonNumberPerSide) {
		avg += (*points)[x + reach][y + reach];
		count++;
	}

	(*points)[x][y] = getRandomHeight(count, reach, avg, biomeParams);
}

void diamondSquareAlgorithm_DiamondStep(std::vector <std::vector <double>> *points,
										int x, int y, int reach, BiomeParams biomeParams,
										std::vector <double> rightSide_EndCoords,
										std::vector <double> upperSide_EndCoords) {
	if (!rightSide_EndCoords.empty()) (*points)[0][y] = rightSide_EndCoords[y];
	if (!upperSide_EndCoords.empty()) (*points)[x][0] = upperSide_EndCoords[x];

	int count = 0;
	double avg = 0.0;

	if (x - reach >= 0) {
		avg += (*points)[x - reach][y];
		count++;
	}
	if (x + reach < polygonNumberPerSide) {
		avg += (*points)[x + reach][y];
		count++;
	}
	if (y - reach >= 0) {
		avg += (*points)[x][y - reach];
		count++;
	}
	if (y + reach < polygonNumberPerSide) {
		avg += (*points)[x][y + reach];
		count++;
	}

	(*points)[x][y] = getRandomHeight(count, reach, avg, biomeParams);
}

void callDiamondSquareAlgorithm(std::vector <std::vector <double>> *points,
								int size, BiomeParams biomeParams,
								std::vector <double> rightSide_EndCoords,
								std::vector <double> upperSide_EndCoords) {
	int half = size / 2;
	if (half < 1) return;

	// Square steps
	for (int y = half; y < polygonNumberPerSide; y += size) {
		for (int x = half; x < polygonNumberPerSide; x += size) {
			diamondSquareAlgorithm_SquareStep(points, x % polygonNumberPerSide, y % polygonNumberPerSide, half, biomeParams);
		}
	}

	// Diamond steps
	int column = 0;
	for (int y = 0; y < polygonNumberPerSide; y += half) {
		column++;
		// If this is an odd column.
		if (column % 2 == 1) {
			for (int x = half; x < polygonNumberPerSide; x += size) {
				diamondSquareAlgorithm_DiamondStep(points, y % polygonNumberPerSide, x % polygonNumberPerSide, half,
												   biomeParams, rightSide_EndCoords, upperSide_EndCoords);
			}
		} else {
			for (int x = 0; x < polygonNumberPerSide; x += size) {
				diamondSquareAlgorithm_DiamondStep(points, y % polygonNumberPerSide, x % polygonNumberPerSide, half,
												   biomeParams, rightSide_EndCoords, upperSide_EndCoords);
			}
		}
	}
	callDiamondSquareAlgorithm(points, size / 2, biomeParams, rightSide_EndCoords, upperSide_EndCoords);
}


void drawAirPlane() {
	glPushMatrix();
	glTranslated(airPlaneTranslation[0], airPlaneTranslation[1], airPlaneTranslation[2]);
	double multiplier_X = manualControl ? 45.0 : 180.0;
	double multiplier_Y = manualControl ? 15.0 : 60.0;
	glRotated((airPlaneTranslation[0] - prevAirPlaneTranslation[0]) * multiplier_X, 0.0, 1.0, 0.0);
	glRotated((airPlaneTranslation[1] - prevAirPlaneTranslation[1]) * multiplier_Y, 1.0, 0.0, 0.0);
	glColor3d(backgroundColor, backgroundColor, backgroundColor);
	glScaled(0.5, 0.5, 0.5);

	airPlaneTexture.bindTexture();
	airPlaneModel.DrawObj();
	glPopMatrix();
}


std::mutex cout_guard;

void set_Id_Texture_Color_forPolygon(Chunk *chunk, GLint firstId, GLint textureAndColorId) {
	chunk->heightIds.push_back(firstId);
	chunk->colors.push_back(colors[textureAndColorId]);
	chunk->textureIds.push_back(textures[textureAndColorId]);
}

struct between {
	between(GLdouble upperBorder, GLdouble lowerBorder = -100.0) :
		_upperBorder(upperBorder), _lowerBorder(lowerBorder) { }

	bool operator()(TriangularSquare_withNormals val) {
		return val.getCenterCoord_Z() > _lowerBorder && val.getCenterCoord_Z() <= _upperBorder;
	}

	GLdouble _lowerBorder, _upperBorder;
};

bool comp(TriangularSquare_withNormals triangularSquare_withNormals_1, TriangularSquare_withNormals triangularSquare_withNormals_2) {
	return triangularSquare_withNormals_1.center[2] < triangularSquare_withNormals_2.center[2];
}

void calculateBiome(Biome *biome) {
	std::lock_guard<std::mutex> lock(cout_guard);
	srand(time(NULL));
	biome->chunks.clear();

	GLdouble chunkSize = polygonNumberPerSide * polygonSize;
	GLdouble chunkOffset_Y = -chunkSize / 2;

	for (int index = 0; index < visibleBiomeHeight; index++) {
		GLdouble chunkOffset_X = -(visibleBiomeWidth / 2.0) * chunkSize;// +chunkSize / 2.0 + polygonSize / 2.0;

		for (int jndex = 0; jndex < visibleBiomeWidth; jndex++) {
			std::vector <std::vector <double>> points;
			for (int i = 0; i < polygonNumberPerSide; i++) {
				std::vector <double> points_X(polygonNumberPerSide);
				points.push_back(points_X);
			}

			// Связывание текущего чанка и биома с предыдущими
			//============================================================================================
			std::vector <double> prevBiome_upperSide_EndCoords =
				biome->previousBiome_EndCoords.empty() ?
				std::vector <GLdouble>() : biome->previousBiome_EndCoords[jndex];
			std::vector <double> prevChunk_rightSide_EndCoords;
			std::vector <double> prevChunk_upperSide_EndCoords;

			if (jndex - 1 >= 0) prevChunk_rightSide_EndCoords = 
				biome->chunks[index * visibleBiomeWidth + jndex - 1].rightSide_EndCoords;
			if (index - 1 >= 0) prevChunk_upperSide_EndCoords = 
				biome->chunks[(index - 1) * visibleBiomeWidth + jndex].upperSide_EndCoords;

			if (!prevBiome_upperSide_EndCoords.empty() && index == 0) {
				callDiamondSquareAlgorithm(&points, polygonNumberPerSide, biome->biomeParams,
										   prevChunk_rightSide_EndCoords, prevBiome_upperSide_EndCoords);
			} else {
				callDiamondSquareAlgorithm(&points, polygonNumberPerSide, biome->biomeParams,
										   prevChunk_rightSide_EndCoords, prevChunk_upperSide_EndCoords);
			}
			//============================================================================================

			// Сохранение высот правой и верхней границы чанка
			//============================================================================================
			std::vector <double> rightSide_EndCoords;
			std::vector <double> upperSide_EndCoords;

			for (int i = 0; i < polygonNumberPerSide; i++) {
				rightSide_EndCoords.push_back(points[polygonNumberPerSide - 1][i]);
				upperSide_EndCoords.push_back(points[i][polygonNumberPerSide - 1]);
			}
			//============================================================================================

			// Создание полигонов чанка
			//============================================================================================
			std::vector <TriangularSquare_withNormals> chunkPolygons;
			for (int i = 0; i < polygonNumberPerSide - 1; i++) {
				for (int j = 0; j < polygonNumberPerSide - 1; j++) {
					TriangularSquare_withNormals triangularSquare_withNormals;
					triangularSquare_withNormals.setLowerLeft((i + 0.0) * polygonSize, (j + 0.0) * polygonSize, points[i + 0][j + 0]);
					triangularSquare_withNormals.setLowerRight((i + 0.0) * polygonSize, (j + 1.0) * polygonSize, points[i + 0][j + 1]);
					triangularSquare_withNormals.setUpperRight((i + 1.0) * polygonSize, (j + 1.0) * polygonSize, points[i + 1][j + 1]);
					triangularSquare_withNormals.setUpperLeft((i + 1.0) * polygonSize, (j + 0.0) * polygonSize, points[i + 1][j + 0]);
					triangularSquare_withNormals.calculateCenter();
					triangularSquare_withNormals.calculateNormals();
					chunkPolygons.push_back(triangularSquare_withNormals);
				}
			}
			//============================================================================================

			Chunk chunk(chunkPolygons, rightSide_EndCoords, upperSide_EndCoords);

			// Translate
			//============================================================================================
			for (int i = 0; i < (int) chunk.chunkPolygons.size(); i++) {
				chunk.chunkPolygons[i].lowerLeft[0] += chunkOffset_X;
				chunk.chunkPolygons[i].lowerRight[0] += chunkOffset_X;
				chunk.chunkPolygons[i].upperRight[0] += chunkOffset_X;
				chunk.chunkPolygons[i].upperLeft[0] += chunkOffset_X;
				chunk.chunkPolygons[i].center[0] += chunkOffset_X;

				chunk.chunkPolygons[i].lowerLeft[1] += chunkOffset_Y;
				chunk.chunkPolygons[i].lowerRight[1] += chunkOffset_Y;
				chunk.chunkPolygons[i].upperRight[1] += chunkOffset_Y;
				chunk.chunkPolygons[i].upperLeft[1] += chunkOffset_Y;
				chunk.chunkPolygons[i].center[1] += chunkOffset_Y;
			}
			//============================================================================================

			std::sort(chunk.chunkPolygons.begin(), chunk.chunkPolygons.end(), comp);
			// ground
			std::vector<TriangularSquare_withNormals>::iterator groundIterator =
				std::find_if(chunk.chunkPolygons.begin(), chunk.chunkPolygons.end(), between(groundLevel));
			// stone
			std::vector<TriangularSquare_withNormals>::iterator stoneIterator =
				std::find_if(chunk.chunkPolygons.begin(), chunk.chunkPolygons.end(), between(stoneLevel, groundLevel));
			// snow
			std::vector<TriangularSquare_withNormals>::iterator snowIterator =
				std::find_if(chunk.chunkPolygons.begin(), chunk.chunkPolygons.end(), between(snowLevel, stoneLevel));

			if (groundIterator != chunk.chunkPolygons.end()) {
				GLuint textureId = 1;
				if (biome->biomeParams.biomeId == 2 || biome->biomeParams.biomeId == 8 || biome->biomeParams.biomeId == 11) {
					textureId = 2;
				} else if (biome->biomeParams.biomeId == 3 || biome->biomeParams.biomeId == 9 || biome->biomeParams.biomeId == 12) {
					textureId = 3;
				}
				set_Id_Texture_Color_forPolygon(&chunk, std::distance(chunk.chunkPolygons.begin(), groundIterator), textureId);
			}
			if (stoneIterator != chunk.chunkPolygons.end()) {
				set_Id_Texture_Color_forPolygon(&chunk, std::distance(chunk.chunkPolygons.begin(), stoneIterator), 4);
			}
			if (snowIterator != chunk.chunkPolygons.end()) {
				set_Id_Texture_Color_forPolygon(&chunk, std::distance(chunk.chunkPolygons.begin(), snowIterator), 2);
			}
			biome->chunks.push_back(chunk);
			chunkOffset_X += chunkSize - polygonSize;
			if (index == visibleBiomeHeight - 1) biome->currentBiome_EndCoords.push_back(chunk.upperSide_EndCoords);
		}
		chunkOffset_Y += chunkSize - polygonSize;
	}
	biome->previousBiome_EndCoords.clear();
}

void generateLists(GLuint *biomeLists, Biome biome) {
	for (int index = 0; index < visibleBiomeHeight; index++) {
		for (int jndex = 0; jndex < visibleBiomeWidth; jndex++) {
			glNewList(biomeLists[index * visibleBiomeWidth + jndex], GL_COMPILE);
			Chunk currentChunk = biome.chunks[index * visibleBiomeWidth + jndex];

			int k = 0;
			for (int i = 0; i < (int) currentChunk.chunkPolygons.size(); i++) {
				if (k < currentChunk.heightIds.size() && currentChunk.heightIds[k] == i) {
					if (k > 0) glEnd();
					glColor3dv(&currentChunk.colors[k][0]);
					glBindTexture(GL_TEXTURE_2D, currentChunk.textureIds[k]);
					glBegin(GL_TRIANGLES);
					k++;
				}
				TriangularSquare_withNormals currentPolygon = currentChunk.chunkPolygons[i];

				glNormal3dv(currentPolygon.lowerTriangleNormal);
				glTexCoord2d(0.0, 0.0);
				glVertex3dv(currentPolygon.lowerLeft);
				glTexCoord2d(0.0, 1.0);
				glVertex3dv(currentPolygon.lowerRight);
				glTexCoord2d(0.5, 0.5);
				glVertex3dv(currentPolygon.center);

				glNormal3dv(currentPolygon.rightTriangleNormal);
				glTexCoord2d(0.0, 1.0);
				glVertex3dv(currentPolygon.lowerRight);
				glTexCoord2d(1.0, 1.0);
				glVertex3dv(currentPolygon.upperRight);
				glTexCoord2d(0.5, 0.5);
				glVertex3dv(currentPolygon.center);

				glNormal3dv(currentPolygon.upperTriangleNormal);
				glTexCoord2d(1.0, 1.0);
				glVertex3dv(currentPolygon.upperRight);
				glTexCoord2d(1.0, 0.0);
				glVertex3dv(currentPolygon.upperLeft);
				glTexCoord2d(0.5, 0.5);
				glVertex3dv(currentPolygon.center);

				glNormal3dv(currentPolygon.leftTriangleNormal);
				glTexCoord2d(1.0, 0.0);
				glVertex3dv(currentPolygon.upperLeft);
				glTexCoord2d(0.0, 0.0);
				glVertex3dv(currentPolygon.lowerLeft);
				glTexCoord2d(0.5, 0.5);
				glVertex3dv(currentPolygon.center);
			}
			glEnd();
			glEndList();
		}
	}
}

void startThread(Biome *biome, Biome *nextBiome) {
	calculateBiome(biome);
	nextBiome->previousBiome_EndCoords = biome->currentBiome_EndCoords;
	biome->currentBiome_EndCoords.clear();
	thread_done = true;
}


// Биомы
Biome biome_0, biome_1, biome_2;
// Массив водички
GLuint waterLevel_lists[visibleBiomeWidth * visibleBiomeHeight];
// Листы текущего биома
GLuint biome_0_lists[visibleBiomeWidth * visibleBiomeHeight];
// Листы будущего биома
GLuint biome_1_lists[visibleBiomeWidth * visibleBiomeHeight];
// Листы поточного биома
GLuint biome_2_lists[visibleBiomeWidth * visibleBiomeHeight];

void createWaterLists() {
	GLdouble polygonNumberPerSide_water = polygonNumberPerSide / 16;
	GLdouble polygonSize_water = polygonSize * 16;

	GLdouble chunkSize = polygonNumberPerSide * polygonSize;
	GLdouble chunkOffset_Y = -chunkSize / 2;

	waterLevel_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	for (int i = 1; i < visibleBiomeWidth * visibleBiomeHeight; i++) {
		waterLevel_lists[i] = waterLevel_lists[i - 1] + 1;
	}

	for (int i = 0; i < visibleBiomeHeight; i++) {
		GLdouble chunkOffset_X = -(visibleBiomeWidth / 2.0) * chunkSize;

		for (int j = 0; j < visibleBiomeWidth; j++) {
			glNewList(waterLevel_lists[i * visibleBiomeWidth + j], GL_COMPILE);
			glColor3dv(&colors[0][0]);
			glNormal3d(0.0, 0.0, 1.0);
			glBindTexture(GL_TEXTURE_2D, textures[0]);

			glBegin(GL_QUADS);
			for (int polygonNumber_Y = 0; polygonNumber_Y < polygonNumberPerSide_water; polygonNumber_Y++) {
				for (int polygonNumber_X = 0; polygonNumber_X < polygonNumberPerSide_water; polygonNumber_X++) {
					GLint resultOffset_Y = chunkOffset_Y + polygonNumber_Y * polygonSize_water;
					GLint resultOffset_X = chunkOffset_X + polygonNumber_X * polygonSize_water;

					glTexCoord2d(0.0, 0.0);
					glVertex3d(resultOffset_X, resultOffset_Y, waterLevel);
					glTexCoord2d(0.0, 1.0);
					glVertex3d(resultOffset_X, resultOffset_Y + GLdouble(polygonSize_water), waterLevel);
					glTexCoord2d(1.0, 1.0);
					glVertex3d(resultOffset_X + GLdouble(polygonSize_water), resultOffset_Y + GLdouble(polygonSize_water), waterLevel);
					glTexCoord2d(1.0, 0.0);
					glVertex3d(resultOffset_X + GLdouble(polygonSize_water), resultOffset_Y, waterLevel);
				}
			}
			glEnd();
			glEndList();
			chunkOffset_X += chunkSize;
		}
		chunkOffset_Y += chunkSize;
	}
}


void transController() {
	double translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;

	switch (translateIndexator) {
		case 0:
			glPushMatrix();
			glTranslated(0.0, -animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_0_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslated(0.0, translationSize - animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_1_lists);
			glPopMatrix();
			break;
		case 1:
			glPushMatrix();
			glTranslated(0.0, -animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_1_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslated(0.0, translationSize - animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_2_lists);
			glPopMatrix();
			break;
		case 2:
			glPushMatrix();
			glTranslated(0.0, -animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_2_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslated(0.0, translationSize - animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_0_lists);
			glPopMatrix();
			break;
	}
}

int getBiomeId() {
	if (currentBiomeLength == 0) {
		currentBiomeLength = rand() % 2 + 1;
		currentBiomeId = rand() % biomeParams.size();
	} else currentBiomeLength--;
	return currentBiomeId;
}


GLuint torusList;
static void torus(int numc, int numt) {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glColor3d(1.0, 1.0, 0.0);

	for (int i = 0; i < numc; i++) {
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= numt; j++) {
			for (int k = 1; k >= 0; k--) {
				GLdouble s = (i + k) % numc + 0.5;
				GLdouble t = j % numt;

				GLdouble x = (1 + 0.1 * cos(s * 2.0 * PI / numc)) * cos(t * 2.0 * PI / numt);
				GLdouble y = (1 + 0.1 * cos(s * 2.0 * PI / numc)) * sin(t * 2.0 * PI / numt);
				GLdouble z = 0.1 * sin(s * 2.0 * PI / numc);
				glVertex3d(x, y, z);
			}
		}
		glEnd();
	}

	lightMode ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
	textureMode ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
}

double distance(double x1, double y1, double z1, double x2, double y2, double z2) {
	return sqrt(pow((x1 - x2), 2) + pow((y1 - y2), 2) + pow((z1 - z2), 2));
}


void myInitRender() {
	srand(time(NULL));
	initializeSound();
	initializeTextures();
	loadModel("models\\airplane.obj", &airPlaneModel);
	airPlaneTexture.loadTextureFromFile("textures\\airplane.bmp");
	loadModel("models\\moon.obj", &moonModel);
	moonTexture.loadTextureFromFile("textures\\moon.bmp");

	if (quickMultiplier > 1) {
		polygonNumberPerSide = (int) pow(2, quickMultiplier);
		polygonSize = 128.0 / GLdouble(polygonNumberPerSide);
		polygonNumberInChunk = (int) pow(polygonNumberPerSide, 2);
	}

	if (quickMultiplier > 1) {
		for (int i = 0; i < (int) biomeParams.size(); i++) {
			biomeParams[i].scale = pow(2, biomeParams[i].scale + quickMultiplier - 8);
		}
	} else {
		for (int i = 0; i < (int) biomeParams.size(); i++) {
			biomeParams[i].scale = pow(2, biomeParams[i].scale);
		}
	}

	createWaterLists();
	biome_0_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	biome_1_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	biome_2_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);

	for (int i = 1; i < visibleBiomeWidth * visibleBiomeHeight; i++) {
		biome_0_lists[i] = biome_0_lists[i - 1] + 1;
		biome_1_lists[i] = biome_1_lists[i - 1] + 1;
		biome_2_lists[i] = biome_2_lists[i - 1] + 1;
	}

	biome_0.biomeParams = biomeParams[getBiomeId()];
	biome_1.biomeParams = biomeParams[getBiomeId()];
	biome_2.biomeParams = biomeParams[getBiomeId()];

	std::thread myThread1(startThread, &biome_0, &biome_1);
	myThread1.join();

	std::thread myThread2(startThread, &biome_1, &biome_2);
	myThread2.join();

	std::thread myThread3(startThread, &biome_2, &biome_0);
	myThread3.join();

	generateLists(biome_0_lists, biome_0);
	generateLists(biome_1_lists, biome_1);
	generateLists(biome_2_lists, biome_2);

	if (gameStarted) {
		torusList = glGenLists(1);
		glNewList(torusList, GL_COMPILE);
		torus(60, 60);
		glEndList();
	}
}

void myRender() {
	std::this_thread::sleep_for(std::chrono::microseconds(16000));

	GLfloat color[4] = { backgroundColor, backgroundColor, backgroundColor, 1.0 };
	glFogfv(GL_FOG_COLOR, color);

	animationController();
	lightController();
	if (!manualControl)	airPlaneControl();

	GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
	GLfloat diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat specular[] = { 0.8, 0.8, 0.8, 1.0 };
	GLfloat shininess = 0.1 * 128;

	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);

	glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, waterLevel_lists);

	if (animationTranslationMoment == 0.0 && start_new) {
		start_new = false;
		thread_done = false;
		BiomeParams newBiome = biomeParams[getBiomeId()];

		if (translateIndexator == 0) {
			biome_2.biomeParams = newBiome;
			std::thread myThread(startThread, &biome_2, &biome_0);
			myThread.detach();
		} else if (translateIndexator == 1) {
			biome_0.biomeParams = newBiome;
			std::thread myThread(startThread, &biome_0, &biome_1);
			myThread.detach();
		} else {
			biome_1.biomeParams = newBiome;
			std::thread myThread(startThread, &biome_1, &biome_2);
			myThread.detach();
		}
	} else if (thread_done && !start_new) {
		switch (translateIndexator) {
			case 0:
				generateLists(biome_2_lists, biome_2);
				break;
			case 1:
				generateLists(biome_0_lists, biome_0);
				break;
			case 2:
				generateLists(biome_1_lists, biome_1);
				break;
		}
		start_new = true;
	}

	transController();

	if (gameStarted) {
	// При попадании в кольцо - оно удаляется из вектора, соответ. необходимо генерировать новое
		int ringNeed = 100;
		if (rings.size() != ringNeed) {
			for (int i = 0; i < (ringNeed - rings.size()); i++) {
				// noob : double speed = 1;
				// default : double speed = getRandomNumber(0.9, 1.2);
				// master : double speed = getRandomNumber(0.6, 4);
				double speed = 1.0;// getRandomNumber(0.6, 4.0);
				double size = getRandomNumber(3.5, 4.5);

				double offset_X = getRandomNumber(-10.0, 10.0);
				double offset_Y = getRandomNumber(0.0, 500.0);
				double offset_Z = maxHeight;// + (maxHeight / 2) - getRandomNumber(10.0, 11.0);

				/*
				double xoff = -5 + getRandomNumber(0, 10);
				double yoff = getRandomNumber(0, 500);
				double zoff = maxHeight + (maxHeight / 2) - getRandomNumber(10, 15);
				*/

				double x = offset_X;
				double y = offset_Y;
				double z = offset_Z;

				// Проверка на дистанцию, если рядом с сгенерируемой координатой в дистанции имеется объект - игнорим добавление 
				bool distancePassed = true;
				for (int i = 0; i < rings.size() && distancePassed; i++) {
					Ring *ring = rings[i];
					if (ring != NULL) {
						if (distance(ring->x, ring->y, ring->z, x, y, z) < (ring->size + size) * 5)
							distancePassed = false;
					}
				}

				// Также проверим чтобы спавн не происходил вблизи/за самолетом
				double translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;
				double yPosReal = y - animationTranslationMoment * speed;
				if (abs(airPlaneTranslation[1] - yPosReal) < 100) distancePassed = false;


				if (distancePassed) {
					Ring *r = new Ring({ speed, size, x, y, z, (DWORD64) -1 });
					rings.push_back(r);
				}
			}
		}

		// отрисовка колец
		for (int i = 0; i < rings.size(); i++) {
			Ring *ring = rings[i];
			if (ring != NULL) {
				double translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;
				double yPosReal = ring->y - animationTranslationMoment * ring->speed;
				glPushMatrix();
				glTranslated(ring->x, yPosReal, ring->z);
				glRotated(90.0, 1.0, 0.0, 0.0);
				glScaled(ring->size, ring->size, ring->size);
				glCallList(torusList);
				glPopMatrix();
			}
		}

		DWORD64 currentMseconds = GetTickCount64();
		// проверка позиций колец
		for (int i = 0; i < rings.size(); i++) {
			Ring *ring = rings[i];
			if (ring != NULL) {
				double translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;
				double yPosReal = ring->y - animationTranslationMoment * ring->speed;
				if (distance(ring->x, yPosReal, ring->z, airPlaneTranslation[0], airPlaneTranslation[1], airPlaneTranslation[2]) < ring->size + 4) {
					// дистанция очень близкая к самолету
					// назначим кол-во милиссикунд после которого кольцо пропадет если это еще не сделано
					if (ring->deadMSeconds == -1) {
						ring->deadMSeconds = currentMseconds + 1000; // через 1 сек после попадания - кольцо пропадет
						BASS_ChannelPlay(channel, TRUE);
					}
				}
				if (ring->deadMSeconds <= currentMseconds) rings.erase(rings.begin() + i);
				// Тут нужна проверка - если кольцо улетело далеко - его надо удалять из вектора, чтобы тот пополнялся новыми кольцами
			}
		}
	}

	drawAirPlane();
}
//===========================================

int prevMouseCoord_X = 0, prevMouseCoord_Y = 0; // Старые координаты мыши

void mouseEvent(OpenGL *ogl, int mouseCoord_X, int mouseCoord_Y) {
	if (operatorMode) {
		int dx = prevMouseCoord_X - mouseCoord_X;
		int dy = prevMouseCoord_Y - mouseCoord_Y;
		prevMouseCoord_X = mouseCoord_X;
		prevMouseCoord_Y = mouseCoord_Y;

		// Меняем углы камеры при нажатой правой кнопке мыши
		if (OpenGL::isKeyPressed(VK_RBUTTON)) {
			camera.fi1 += 0.01 * dx;
			camera.fi2 += -0.01 * dy;
		}
	}

	if (manualControl && !OpenGL::isKeyPressed(VK_LBUTTON)) {
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);
		double z = airPlaneTranslation[2];

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0) {
			k = 0;
		} else k = (z - r.origin.Z()) / r.direction.Z();

		x = k * r.direction.X() + r.origin.X();
		y = k * r.direction.Y() + r.origin.Y();

		if (y > 0.0) {
			x = airPlaneTranslation[0];
			y = 0.0;
		}

		prevAirPlaneTranslation[0] = airPlaneTranslation[0];
		prevAirPlaneTranslation[1] = airPlaneTranslation[1];
		prevAirPlaneTranslation[2] = airPlaneTranslation[2];

		airPlaneTranslation[0] = x;
		airPlaneTranslation[1] = y;
		airPlaneTranslation[2] = z;
	}
}

void mouseWheelEvent(OpenGL *ogl, int delta) {
	if (operatorMode) {
		if (delta < 0 && camera.camDist <= 1) return;
		if (delta > 0 && camera.camDist >= 100)	return;

		camera.camDist += 0.01 * delta;
	}
}

void keyDownEvent(OpenGL *ogl, int key) {
	// if (key == 'G') gameStarted = !gameStarted;

	if (key == 'O') {
		operatorMode = !operatorMode;
		if (!operatorMode) {
			fogMode = true;
			lightMode = true;
			textureMode = true;
			camera.camDist = maxHeight + 20.0;
			camera.fi1 = M_PI / 180.0 * -90.0;
			camera.fi2 = M_PI / 180.0 * 45.0;
		}
	}

	if (key == 'M') {
		manualControl = !manualControl;
		airPlaneTranslation[0] = 0.0;
		airPlaneTranslation[1] = 0.0;
		if (manualControl) {
			airPlaneTranslation[2] = maxHeight;
		} else airPlaneTranslation[2] = maxHeight + 5.0;
	}

	if (operatorMode) {
		if (key == 'F') fogMode = !fogMode;
		if (key == 'L') lightMode = !lightMode;
		if (key == 'T') textureMode = !textureMode;
		if (key == 'J') lightLockMode = !lightLockMode;
		if (key == 'R') {
			fogMode = true;
			lightMode = true;
			textureMode = true;
			camera.camDist = maxHeight + 20.0;
			camera.fi1 = M_PI / 180.0 * -90.0;
			camera.fi2 = M_PI / 180.0 * 45.0;
			animationLightMoment = 0.0;
		}
	}
}

void keyUpEvent(OpenGL *ogl, int key) {

}


// Выполняется перед первым рендером
void initRender(OpenGL *ogl) {
	myInitRender();

	// камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	glEnable(GL_NORMALIZE); // нормализация нормалей
	glEnable(GL_LINE_SMOOTH); // устранение ступенчатости для линий
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	int end = polygonNumberPerSide * polygonSize * 1.5;

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 0.35);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogf(GL_FOG_START, end / 5.0);
	glFogf(GL_FOG_END, end);
}

void drawMessage(OpenGL *ogl) {
	// Сообщение вверху экрана
	glMatrixMode(GL_PROJECTION);	// Делаем активной матрицу проекций. 
									// (всек матричные операции, будут ее видоизменять.)
	glPushMatrix();											// сохраняем текущую матрицу проецирования (которая описывает перспективную проекцию) в стек 				    
	glLoadIdentity();										// Загружаем единичную матрицу
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);	// врубаем режим ортогональной проекции

	glMatrixMode(GL_MODELVIEW);								// переключаемся на модел-вью матрицу
	glPushMatrix();											// сохраняем текущую матрицу в стек (положение камеры, фактически)
	glLoadIdentity();										// сбрасываем ее в дефолт

	glDisable(GL_LIGHTING);

	GuiTextRectangle rec;			// классик моего авторства для удобной работы с рендером текста.
	rec.setSize(300, 300);
	rec.setPosition(10, ogl->getHeight() - 300 - 10);

	std::stringstream ss;

	double angle = 360.0 * animationLightMoment / (polygonSize * polygonNumberPerSide * visibleBiomeWidth);
	int hours = (int) round(angle) / 15 + 6;
	int minutes = (angle - (hours - 6.0) * 15.0) * 4 + 2;
	if (hours >= 24) hours = hours - 24;
	ss << "Time: " << hours << ":" << minutes << std::endl;

	if (angle <= 30.0) {
		backgroundColor = angle / 30.0;
	} else if (angle > 30.0 && angle < 150.0) {
		backgroundColor = 1.0;
	} else if (angle >= 150.0 && angle < 180.0) {
		backgroundColor = (180.0 - angle) / 30.0;
	}
	glClearColor(backgroundColor, backgroundColor, backgroundColor, 1.0);

	ss << "Biome: ";
	int biomeId = 0;
	switch (translateIndexator) {
		case 0:
			biomeId = biome_0.biomeParams.biomeId;
			break;
		case 1:
			biomeId = biome_1.biomeParams.biomeId;
			break;
		case 2:
			biomeId = biome_2.biomeParams.biomeId;
			break;
	}

	switch (biomeId) {
		case 0:
			ss << "Ocean" << std::endl;
			break;
		case 1:
			ss << "Flat (Grass)" << std::endl;
			break;
		case 2:
			ss << "Flat (Snow)" << std::endl;
			break;
		case 3:
			ss << "Flat (Sand)" << std::endl;
			break;
		case 4:
			ss << "Swamp" << std::endl;
			break;
		case 5:
			ss << "Mountains" << std::endl;
			break;
		case 6:
			ss << "Mountains (with pond)" << std::endl;
			break;
		case 7:
			ss << "Beach (Grass)" << std::endl;
			break;
		case 8:
			ss << "Beach (Snow)" << std::endl;
			break;
		case 9:
			ss << "Beach (Sand)" << std::endl;
			break;
		case 10:
			ss << "Hills (Grass)" << std::endl;
			break;
		case 11:
			ss << "Hills (Snow)" << std::endl;
			break;
		case 12:
			ss << "Hills (Sand)" << std::endl;
			break;
	}
	std::string manualStatus = manualControl ? " [вкл]" : " [выкл]";
	ss << "M - вкл/выкл режима пилота" << manualStatus << std::endl << std::endl;

	if (operatorMode) {
		std::string textureStatus = textureMode ? " [вкл]" : " [выкл]";
		std::string lightStatus = lightMode ? " [вкл]" : " [выкл]";
		std::string lightLockStatus = lightLockMode ? " [вкл]" : " [выкл]";

		ss << "РЕЖИМ ОПЕРАТОРА" << std::endl << std::endl;
		ss << "T - вкл/выкл текстур" << textureStatus << std::endl;
		ss << "L - вкл/выкл освещение" << lightStatus << std::endl;
		ss << "J - блокировка смены суток" << lightLockStatus << std::endl << std::endl;
		ss << "animationTranslationMoment: " << animationTranslationMoment << std::endl;
		ss << "animationLightMoment: " << animationLightMoment << std::endl;
		ss << "start_new: " << start_new << std::endl;
		ss << "thread_done: " << thread_done << std::endl;
	}
	//ss << "Коорд. света: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
	//ss << "Коорд. камеры: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
	//ss << "Cam Coords: (" << camera.lookPoint.X() << ", " << camera.lookPoint.Y() << ", " << camera.lookPoint.Z() << ")" << std::endl;
	//ss << "Cam Param: R=" << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;

	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION); // восстанавливаем матрицы проекции и модел-вью обратьно из стека.
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void Render(OpenGL *ogl) {
	glEnable(GL_DEPTH_TEST);

	fogMode ? glEnable(GL_FOG) : glDisable(GL_FOG);
	lightMode ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
	textureMode ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);

	// Чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH);
	//===================================
	// Прогать тут
	myRender();
	//===================================
	drawMessage(ogl);
}