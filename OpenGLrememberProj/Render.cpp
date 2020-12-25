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

GLfloat maxHeight = 30.0;
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
	float camDist;
	// Углы поворота камеры
	float fi1, fi2;

	// Значния камеры по умолчанию
	CustomCamera() {
		camDist = maxHeight + 20.0f;
		fi1 = M_PI / 180.0f * -90.0f;
		fi2 = M_PI / 180.0f * 45.0f;
	}

	// Считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera() {
		if (cos(fi2) <= 0.0f) {
			normal.setCoords(0.0f, 0.0f, -1.0f);
		} else normal.setCoords(0.0f, 0.0f, 1.0f);

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
		pos = Vector3(0.0, 0.0, 0.0);
	}

	void DrawLightGhismo() {

	}

	void SetUpLight() {
		GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		GLfloat specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };

		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

		glEnable(GL_LIGHT0);
	}
} light; // Создаем источник света

//===========================================
struct Material {
	GLfloat ambient;
	GLfloat diffuse;
	GLfloat specular;
	GLfloat shininess;
};

struct BiomeParams {
	GLuint biomeId;
	GLfloat minAltitude;
	GLfloat maxAltitude;
	GLfloat scale;
};

class TriangularSquare_withNormals {
private:
	void calculateNormal(float *firstPoint, GLfloat *secondPoint, GLfloat *resultNormal) {
		float x_1 = firstPoint[0];
		float y_1 = firstPoint[1];
		float z_1 = firstPoint[2];

		float x_2 = secondPoint[0];
		float y_2 = secondPoint[1];
		float z_2 = secondPoint[2];

		float x_3 = center[0];
		float y_3 = center[1];
		float z_3 = center[2];

		float line_1[] = { x_2 - x_1, y_2 - y_1, z_2 - z_1 };
		float line_2[] = { x_2 - x_3, y_2 - y_3, z_2 - z_3 };

		float n_x = line_1[1] * line_2[2] - line_2[1] * line_1[2];
		float n_y = line_1[0] * line_2[2] - line_2[0] * line_1[2];
		float n_z = line_1[0] * line_2[1] - line_2[0] * line_1[1];

		float length = sqrt(pow(n_x, 2.0) + pow(n_y, 2.0) + pow(n_z, 2.0));
		resultNormal[0] = n_x / length;
		resultNormal[1] = -n_y / length;
		resultNormal[2] = n_z / length;
	}

public:
	GLfloat lowerLeft[3];
	GLfloat lowerRight[3];
	GLfloat upperRight[3];
	GLfloat upperLeft[3];
	GLfloat center[3];

	GLfloat lowerTriangleNormal[3];
	GLfloat rightTriangleNormal[3];
	GLfloat upperTriangleNormal[3];
	GLfloat leftTriangleNormal[3];

	void setLowerLeft(GLfloat x, GLfloat y, GLfloat z) {
		lowerLeft[0] = x;
		lowerLeft[1] = y;
		lowerLeft[2] = z;
	}

	void setLowerRight(GLfloat x, GLfloat y, GLfloat z) {
		lowerRight[0] = x;
		lowerRight[1] = y;
		lowerRight[2] = z;
	}

	void setUpperRight(GLfloat x, GLfloat y, GLfloat z) {
		upperRight[0] = x;
		upperRight[1] = y;
		upperRight[2] = z;
	}

	void setUpperLeft(GLfloat x, GLfloat y, GLfloat z) {
		upperLeft[0] = x;
		upperLeft[1] = y;
		upperLeft[2] = z;
	}

	void calculateCenter() {
		center[0] = (lowerLeft[0] + upperRight[0]) / 2.0f;
		center[1] = (lowerLeft[1] + upperRight[1]) / 2.0f;
		center[2] = (lowerLeft[2] + lowerRight[2] + upperRight[2] + upperLeft[2]) / 4.0f;
	}

	float getCenterCoord_Z() {
		return center[2];
	}

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
	std::vector <GLfloat> rightSide_EndCoords;
	std::vector <GLfloat> upperSide_EndCoords;

	std::vector <GLuint> heightIds;
	std::vector <GLuint> textureIds;
	std::vector <Material> materialIds;
	std::vector <std::vector <GLfloat>> colors;

	Chunk() {

	}

	Chunk(std::vector <TriangularSquare_withNormals> chunkPolygons,
		  std::vector <GLfloat> rightSide_EndCoords,
		  std::vector <GLfloat> upperSide_EndCoords) {
		this->chunkPolygons = chunkPolygons;
		this->rightSide_EndCoords = rightSide_EndCoords;
		this->upperSide_EndCoords = upperSide_EndCoords;
	}
};

struct Ring {
	float speed, size, x, y, z;
	DWORD64 deadMSeconds;
};
std::vector<Ring *> rings;

class Biome {
public:
	std::vector <Chunk> chunks;
	std::vector <std::vector <GLfloat>> currentBiome_EndCoords;
	std::vector <std::vector <GLfloat>> previousBiome_EndCoords;
	BiomeParams biomeParams;
};

// Параметры для потока
std::atomic<bool> start_new { true };		// Указывает, когда начинать генерировать новый биом
std::atomic<bool> thread_done { false };	// Указывает, когда новый биом сгенерирован

// Параметры контроля и управления самолетом
GLint torusScore = 0;					// счетчик собранных колец								(Do not touch)
GLint viewMode = 2;						// вид на самолет: от кабины, с хвоста, по умолчанию	(Do not touch)
GLfloat pointToFly[2];					// точка, к которой должен лететь самолет				(Do not touch)
GLfloat savedAnimStatus = 0.0;			// сохраняет статус анимации							(Do not touch)
GLboolean isControlSwitched = false;	// определение переключения режима управления			(Do not touch)
GLboolean isReadyToControl = true;		// уведомляет о готовности передать контроль			(Do not touch)
GLboolean manualControl = false;		// режим управления самолетом							(Do not touch)
GLfloat airPlaneTranslation[3];			// текущие координаты самолета							(Do not touch)
GLfloat prevAirPlaneTranslation[3];		// предыдущие координаты самолета						(Do not touch)

// Дополнительные параметры
ObjFile airPlaneModel, moonModel;	// Do not touch
bool isMusicStoped = false;			// Do not touch
int translateIndexator;				// Do not touch
int currentBiomeId;					// ID текущего биома (Do not touch)
int currentBiomeLength;				// Количество биомов данного типа (Do not touch)
float backgroundColor;				// Do not touch
float sunColor[3];					// Do not touch

// Параметры отрисовываемой области (лучше не трогать)
const GLint visibleBiomeHeight = 2;							// Default: 2
const GLint visibleBiomeWidth = 2 * visibleBiomeHeight + 1; // Default: 2 * visibleBiomeHeight + 1
// Быстрое переключение, работает, если quickMultiplier > 1
// Автоматически определяет значения для:
// polygonSize, polygonNumberPerSide, polygonNumberInChunk, BiomeParams.scale
const GLint quickMultiplier = 7;
GLfloat polygonSize = pow(2, -1);				// HD = -1	: stable = 0	: no_lag = 1
GLint polygonNumberPerSide = (int) pow(2, 8);	// HD = 8	: stable = 7	: no_lag = 6
GLint polygonNumberInChunk = (int) pow(polygonNumberPerSide, 2); // Do not touch

// Уровни покровов (можно потрогать)
const GLfloat waterLevel = 0.0;		// Уровень воды
const GLfloat groundLevel = 5.0;	// Уровень земли (трава, песок, снег)
const GLfloat stoneLevel = 20.0;	// Уровень камня
const GLfloat snowLevel = 25.0;		// Уровень снежных шапок

// Быстрое переключение, работает, если quickMultiplier > 3 (можно потрогать)
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

// Цвета (можно потрогать)
const std::vector <std::vector <GLfloat>> colors = {
	{ 0.42, 0.48, 0.97 },	// 0 - water, Default { 0.42, 0.48, 0.97 }
	{ 0.47, 0.68, 0.33 },	// 1 - grass, Default { 0.47, 0.68, 0.33 }
	{ 0.94, 0.98, 0.98 },	// 2 - snow,  Default { 0.94, 0.98, 0.98 }
	{ 0.86, 0.83, 0.63 },	// 3 - sand,  Default { 0.86, 0.83, 0.63 }
	{ 0.49, 0.49, 0.49 }	// 4 - stone, Default { 0.49, 0.49, 0.49 }
};

// Материалы (можно потрогать)
const std::vector <Material> materials = {
	{ 0.06, 0.95, 0.90, 0.50 },	// 0 - water, Default { 0.06, 0.95, 0.90, 0.50 }
	{ 0.32, 0.90, 0.10, 0.05 },	// 1 - grass, Default { 0.32, 0.90, 0.10, 0.05 }
	{ 0.87, 0.82, 0.35, 0.35 },	// 2 - snow,  Default { 0.87, 0.82, 0.35, 0.35 }
	{ 0.18, 0.90, 0.15, 0.10 },	// 3 - sand,  Default { 0.18, 0.90, 0.15, 0.10 }
	{ 0.40, 0.92, 0.05, 0.02 }	// 4 - stone, Default { 0.40, 0.92, 0.05, 0.02 }
};

// Параметры анимации (можно потрогать)
const GLfloat animationSpeed = 0.25;			// Default: 0.25
const GLfloat animationAirPlaneSpeed = 0.005;	// Default: 0.005
GLfloat animationLightMoment;					// Do not touch
GLfloat animationTranslationMoment;				// Do not touch
GLfloat animationAirPlaneMoment;				// Do not touch


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

HSTREAM streamHit;
HSTREAM streamMusic;

void initializeSound() {
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		MessageBox(NULL, "Ошибка версии BASS", NULL, 0);
		return;
	}
	if (!BASS_Init(-1, 22050, BASS_DEVICE_3D, 0, NULL)) {
		MessageBox(NULL, "Не удалось инициализировать BASS", NULL, 0);
		return;
	}

	char sampleFilepath[] = "sounds/hit.mp3";
	streamHit = BASS_StreamCreateFile(FALSE, sampleFilepath, 0, 0, BASS_SAMPLE_3D);
	if (!streamHit) {
		MessageBox(NULL, "Не удалось загрузить звук", NULL, 0);
		return;
	}

	char streamFilepath[] = "sounds/music.mp3";
	streamMusic = BASS_StreamCreateFile(FALSE, streamFilepath, 0, 0, BASS_SAMPLE_3D);
	if (!streamMusic) {
		MessageBox(NULL, "Не удалось загрузить музыку", NULL, 0);
		return;
	}

	if (BASS_SampleGetChannel(streamMusic, FALSE)) {
		MessageBox(NULL, "Не удалось обнаружить звуковой канал", NULL, 0);
		return;
	}

	BASS_ChannelSetAttribute(streamHit, BASS_ATTRIB_VOL, 0.5);
	BASS_ChannelSetAttribute(streamMusic, BASS_ATTRIB_VOL, 0.1);
	BASS_ChannelPlay(streamMusic, TRUE);
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

	float sunAngle = 2.0 * M_PI * i;
	float sun_dx = visibleBiomeWidth * polygonNumberPerSide * polygonSize * cosf(sunAngle);
	float sun_dz = visibleBiomeWidth * polygonNumberPerSide * polygonSize * sinf(sunAngle);
	float sunPosition[3] = { sun_dx, 0.0, sun_dz };

	glDisable(GL_FOG);

	glPushMatrix();
	float moonColor = 1.4f - sunColor[0];
	float blueColorReducer = (sunColor[0] + sunColor[1]) / (255.0f + sunColor[2]);
	if (blueColorReducer > 1.0f) blueColorReducer -= 1.0f - blueColorReducer;
	GLfloat ambient[] = { moonColor, moonColor, moonColor, 1.0f };
	GLfloat diffuse[] = { sunColor[1] / 255.0f, sunColor[0] / 255.0f, blueColorReducer, 1.0f };
	GLfloat specular[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLfloat shininess = 0.3f * 128.0f;

	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);

	float moonSize = polygonNumberPerSide * polygonSize / 4;
	float moonAngle = sunAngle + 166.0;
	float moon_dx = moon_dx = polygonNumberPerSide * polygonSize * cosf(moonAngle);
	float moon_dz = moon_dz = polygonNumberPerSide * polygonSize * sinf(moonAngle);

	glTranslatef(moon_dx, polygonNumberPerSide * polygonSize * visibleBiomeHeight + moonSize, moon_dz);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	glRotatef(-45.0, 0.0, 0.0, 1.0);
	glScalef(moonSize / 64.0, moonSize / 64.0, moonSize / 64.0);
	glDepthMask(false);
	moonTexture.bindTexture();
	moonModel.DrawObj();
	glDepthMask(true);
	glPopMatrix();

	fogMode ? glEnable(GL_FOG) : glDisable(GL_FOG);
}

void airPlaneAutoManualController() {
	GLfloat speed = animationAirPlaneSpeed * 200.0 * (0.005 / animationAirPlaneSpeed);
	GLfloat inaccuracy = 0.25;

	GLfloat acceleration_X = pow(max(pointToFly[0], airPlaneTranslation[0]) -
								 min(pointToFly[0], airPlaneTranslation[0]), 2) / 100.0;
	GLfloat acceleration_Y = pow(max(pointToFly[1], airPlaneTranslation[1]) -
								 min(pointToFly[1], airPlaneTranslation[1]), 2) / 100.0;

	acceleration_X = acceleration_X > 1.0 ? acceleration_X : 1.0;
	acceleration_Y = acceleration_Y > 1.0 ? acceleration_Y : 1.0;
	acceleration_X = acceleration_X < 20.0 ? acceleration_X : 20.0;
	acceleration_Y = acceleration_Y < 20.0 ? acceleration_Y : 20.0;

	GLfloat vectorLength = sqrt(pow(pointToFly[0], 2) + pow(pointToFly[1], 2));
	if (vectorLength == 0.0) vectorLength = 1.0;
	GLfloat speed_X = speed * acceleration_X * (max(pointToFly[0], airPlaneTranslation[0]) - min(pointToFly[0], airPlaneTranslation[0])) / vectorLength;
	GLfloat speed_Y = speed * acceleration_Y * (max(pointToFly[1], airPlaneTranslation[1]) - min(pointToFly[1], airPlaneTranslation[1])) / vectorLength;

	prevAirPlaneTranslation[0] = airPlaneTranslation[0];
	prevAirPlaneTranslation[1] = airPlaneTranslation[1];
	prevAirPlaneTranslation[2] = airPlaneTranslation[2];

	if (airPlaneTranslation[0] > pointToFly[0] + inaccuracy) {
		airPlaneTranslation[0] -= speed_X;
	} else if (airPlaneTranslation[0] < pointToFly[0] - inaccuracy) airPlaneTranslation[0] += speed_X;

	if (airPlaneTranslation[1] > pointToFly[1] + inaccuracy) {
		airPlaneTranslation[1] -= speed_Y;
	} else if (airPlaneTranslation[1] < pointToFly[1] - inaccuracy) airPlaneTranslation[1] += speed_Y;
}

void airPlaneAutoFlyController() {
	GLfloat x = 25.0 * (sqrt(2.0) * cos(animationAirPlaneMoment)) / (1.0 + pow(sin(animationAirPlaneMoment), 2.0));
	GLfloat y = 10.0 * exp(sin(animationAirPlaneMoment));
	GLfloat z = 10.0 * (sqrt(2.0) * cos(animationAirPlaneMoment) * sin(animationAirPlaneMoment)) /
		(1.0 + pow(sin(animationAirPlaneMoment), 2.0)) + maxHeight + 5.0;

	prevAirPlaneTranslation[0] = airPlaneTranslation[0];
	prevAirPlaneTranslation[1] = airPlaneTranslation[1];
	prevAirPlaneTranslation[2] = airPlaneTranslation[2];

	airPlaneTranslation[0] = x;
	airPlaneTranslation[1] = y;
	airPlaneTranslation[2] = z;
}

void airPlaneControlSwitchController() {
	prevAirPlaneTranslation[0] = airPlaneTranslation[0];
	prevAirPlaneTranslation[1] = airPlaneTranslation[1];
	prevAirPlaneTranslation[2] = airPlaneTranslation[2];

	GLfloat speed = animationAirPlaneSpeed * 50.0;
	GLfloat inaccuracy = 2.0 * speed;

	if (manualControl) {
		GLfloat acceleration_X = pow(airPlaneTranslation[0], 2) / 100.0;
		GLfloat acceleration_Y = pow(-10.0 - airPlaneTranslation[1], 2) / 100.0;
		GLfloat acceleration_Z = pow(maxHeight - airPlaneTranslation[2], 2) / 100.0;

		acceleration_X = acceleration_X > 0.5 * (0.005 / animationAirPlaneSpeed) ? acceleration_X : 0.5 * (0.005 / animationAirPlaneSpeed);
		acceleration_Y = acceleration_Y > 0.5 * (0.005 / animationAirPlaneSpeed) ? acceleration_Y : 0.5 * (0.005 / animationAirPlaneSpeed);
		acceleration_Z = acceleration_Z > 0.5 * (0.005 / animationAirPlaneSpeed) ? acceleration_Z : 0.5 * (0.005 / animationAirPlaneSpeed);

		if (airPlaneTranslation[0] > inaccuracy || airPlaneTranslation[0] < -inaccuracy ||
			airPlaneTranslation[1] > -10.0 + inaccuracy || airPlaneTranslation[1] < -10.0 - inaccuracy ||
			airPlaneTranslation[2] > maxHeight + inaccuracy ||
			airPlaneTranslation[2] < maxHeight - inaccuracy) {
			if (airPlaneTranslation[0] > inaccuracy) {
				airPlaneTranslation[0] -= speed * acceleration_X;
			} else if (airPlaneTranslation[0] < -inaccuracy) {
				airPlaneTranslation[0] += speed * acceleration_X;
			}

			if (airPlaneTranslation[1] > -10.0 + inaccuracy) {
				airPlaneTranslation[1] -= speed * acceleration_Y;
			} else if (airPlaneTranslation[1] < -10.0 - inaccuracy) {
				airPlaneTranslation[1] += speed * acceleration_Y;
			}

			if (airPlaneTranslation[2] > maxHeight + inaccuracy) {
				airPlaneTranslation[2] -= speed * acceleration_Z;
			} else if (airPlaneTranslation[2] < maxHeight - inaccuracy) {
				airPlaneTranslation[2] += speed * acceleration_Z;
			}
		} else {
			isReadyToControl = true;
			isControlSwitched = false;
		}
	} else {
		GLfloat animOffset = 0.75;
		if (savedAnimStatus + animOffset > 2.0 * M_PI) animOffset = 2.0 * M_PI - savedAnimStatus;

		GLfloat x = 25.0 * (sqrt(2.0) * cos(savedAnimStatus + animOffset)) / (1.0 + pow(sin(savedAnimStatus + animOffset), 2.0));
		GLfloat y = 10.0 * exp(sin(savedAnimStatus + animOffset));
		GLfloat z = 10.0 * (sqrt(2.0) * cos(savedAnimStatus + animOffset) * sin(savedAnimStatus + animOffset)) /
			(1.0 + pow(sin(savedAnimStatus + animOffset), 2.0)) + maxHeight + 5.0;

		GLfloat acceleration_X = pow(x - airPlaneTranslation[0], 2) / 100.0;
		GLfloat acceleration_Y = pow(y - airPlaneTranslation[1], 2) / 100.0;
		GLfloat acceleration_Z = pow(z - airPlaneTranslation[2], 2) / 100.0;

		acceleration_X = acceleration_X > 0.5 * (0.005 / animationAirPlaneSpeed) ? acceleration_X : 0.5 * (0.005 / animationAirPlaneSpeed);
		acceleration_Y = acceleration_Y > 0.5 * (0.005 / animationAirPlaneSpeed) ? acceleration_Y : 0.5 * (0.005 / animationAirPlaneSpeed);
		acceleration_Z = acceleration_Z > 0.5 * (0.005 / animationAirPlaneSpeed) ? acceleration_Z : 0.5 * (0.005 / animationAirPlaneSpeed);

		if (airPlaneTranslation[0] > x + inaccuracy || airPlaneTranslation[0] < x - inaccuracy ||
			airPlaneTranslation[1] > y + inaccuracy || airPlaneTranslation[1] < y - inaccuracy ||
			airPlaneTranslation[2] > z + inaccuracy || airPlaneTranslation[2] < z - inaccuracy ||
			animationAirPlaneMoment > savedAnimStatus + animOffset + animationAirPlaneSpeed ||
			animationAirPlaneMoment < savedAnimStatus + animOffset - animationAirPlaneSpeed) {
			if (airPlaneTranslation[0] > x + inaccuracy) {
				airPlaneTranslation[0] -= speed * acceleration_X;
			} else if (airPlaneTranslation[0] < x - inaccuracy) {
				airPlaneTranslation[0] += speed * acceleration_X;
			}

			if (airPlaneTranslation[1] > y + inaccuracy) {
				airPlaneTranslation[1] -= speed * acceleration_Y;
			} else if (airPlaneTranslation[1] < y - inaccuracy) {
				airPlaneTranslation[1] += speed * acceleration_Y;
			}

			if (airPlaneTranslation[2] > z + inaccuracy) {
				airPlaneTranslation[2] -= speed * acceleration_Z;
			} else if (airPlaneTranslation[2] < z - inaccuracy) {
				airPlaneTranslation[2] += speed * acceleration_Z;
			}
		} else {
			isReadyToControl = true;
			isControlSwitched = false;
		}
	}
}


float getRandomNumber(float lowerBorder, GLfloat upperBorder) {
	return lowerBorder + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (upperBorder - lowerBorder)));
}

float getRandomHeight(GLint count, GLint range, GLfloat avg, BiomeParams biomeParams) {
	float biomeBorder_max = biomeParams.maxAltitude * count - avg;
	float biomeBorder_min = biomeParams.minAltitude * count - avg;

	float randomBorder_max = float(range) / biomeParams.scale;
	float randomBorder_min = -randomBorder_max;

	float lowerBorder = max(biomeBorder_min, randomBorder_min);
	float upperBorder = min(biomeBorder_max, randomBorder_max);

	return (avg + getRandomNumber(lowerBorder, upperBorder)) / count;
}

void calculateNormal(const GLfloat *firstPoint, const GLfloat *secondPoint, const GLfloat *generalPoint, GLfloat *resultNormal) {
	float x_1 = firstPoint[0];
	float y_1 = firstPoint[1];
	float z_1 = firstPoint[2];

	float x_2 = secondPoint[0];
	float y_2 = secondPoint[1];
	float z_2 = secondPoint[2];

	float x_3 = generalPoint[0];
	float y_3 = generalPoint[1];
	float z_3 = generalPoint[2];

	float line_1[] = { x_2 - x_1, y_2 - y_1, z_2 - z_1 };
	float line_2[] = { x_2 - x_3, y_2 - y_3, z_2 - z_3 };

	float n_x = line_1[1] * line_2[2] - line_2[1] * line_1[2];
	float n_y = line_1[0] * line_2[2] - line_2[0] * line_1[2];
	float n_z = line_1[0] * line_2[1] - line_2[0] * line_1[1];

	float length = sqrt(pow(n_x, 2.0) + pow(n_y, 2.0) + pow(n_z, 2.0));
	resultNormal[0] = n_x / length;
	resultNormal[1] = -n_y / length;
	resultNormal[2] = n_z / length;
}


void diamondSquareAlgorithm_SquareStep(std::vector <std::vector <GLfloat>> *points,
									   GLint x, GLint y, GLint reach, BiomeParams biomeParams) {
	int count = 0;
	float avg = 0.0;

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

void diamondSquareAlgorithm_DiamondStep(std::vector <std::vector <GLfloat>> *points,
										GLint x, GLint y, GLint reach, BiomeParams biomeParams,
										std::vector <GLfloat> rightSide_EndCoords,
										std::vector <GLfloat> upperSide_EndCoords) {
	if (!rightSide_EndCoords.empty()) (*points)[0][y] = rightSide_EndCoords[y];
	if (!upperSide_EndCoords.empty()) (*points)[x][0] = upperSide_EndCoords[x];

	int count = 0;
	float avg = 0.0;

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

void callDiamondSquareAlgorithm(std::vector <std::vector <GLfloat>> *points,
								GLint size, BiomeParams biomeParams,
								std::vector <GLfloat> rightSide_EndCoords,
								std::vector <GLfloat> upperSide_EndCoords) {
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
	glTranslatef(airPlaneTranslation[0], airPlaneTranslation[1], airPlaneTranslation[2]);
	float multiplier_X = manualControl ? 45.0 : 180.0;
	float multiplier_Y = manualControl ? 15.0 : 60.0;
	if (!manualControl) {
		multiplier_X *= (0.005 / animationAirPlaneSpeed);
		multiplier_Y *= (0.005 / animationAirPlaneSpeed);
	}

	glRotatef((airPlaneTranslation[1] - prevAirPlaneTranslation[1]) * multiplier_Y, 1.0, 0.0, 0.0);
	glRotatef((airPlaneTranslation[0] - prevAirPlaneTranslation[0]) * multiplier_X, 0.0, 1.0, 0.0);
	glScalef(0.5, 0.5, 0.5);

	GLfloat ambient[4] = { 0.19225, 0.19225, 0.19225, 1.0 };
	GLfloat diffuse[4] = { 0.50754, 0.50754, 0.50754, 1.0 };
	GLfloat specular[4] = { 0.508273, 0.508273, 0.508273, 1.0 };
	GLfloat shininess = 0.4 * 128.0;

	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);

	airPlaneTexture.bindTexture();
	airPlaneModel.DrawObj();
	glPopMatrix();
}


std::mutex cout_guard;

void setParamsforPolygon(Chunk *chunk, GLint firstId, GLint paramId) {
	chunk->heightIds.push_back(firstId);
	chunk->colors.push_back(colors[paramId]);
	chunk->textureIds.push_back(textures[paramId]);
	chunk->materialIds.push_back(materials[paramId]);
}

struct between {
	between(GLfloat upperBorder, GLfloat lowerBorder = -100.0) :
		_upperBorder(upperBorder), _lowerBorder(lowerBorder) { }

	bool operator()(TriangularSquare_withNormals val) {
		return val.getCenterCoord_Z() > _lowerBorder && val.getCenterCoord_Z() <= _upperBorder;
	}

	GLfloat _lowerBorder, _upperBorder;
};

bool comp(TriangularSquare_withNormals triangularSquare_withNormals_1, TriangularSquare_withNormals triangularSquare_withNormals_2) {
	return triangularSquare_withNormals_1.center[2] < triangularSquare_withNormals_2.center[2];
}

void calculateBiome(Biome *biome) {
	std::lock_guard<std::mutex> lock(cout_guard);
	srand(time(NULL));
	biome->chunks.clear();

	GLfloat chunkSize = polygonNumberPerSide * polygonSize;
	GLfloat chunkOffset_Y = -chunkSize / 2;

	for (int index = 0; index < visibleBiomeHeight; index++) {
		GLfloat chunkOffset_X = -(visibleBiomeWidth / 2.0) * (chunkSize - polygonSize);

		for (int jndex = 0; jndex < visibleBiomeWidth; jndex++) {
			std::vector <std::vector <float>> points;
			for (int i = 0; i < polygonNumberPerSide; i++) {
				std::vector <float> points_X(polygonNumberPerSide);
				points.push_back(points_X);
			}

			// Связывание текущего чанка и биома с предыдущими
			//============================================================================================
			std::vector <float> prevBiome_upperSide_EndCoords =
				biome->previousBiome_EndCoords.empty() ?
				std::vector <GLfloat>() : biome->previousBiome_EndCoords[jndex];
			std::vector <float> prevChunk_rightSide_EndCoords;
			std::vector <float> prevChunk_upperSide_EndCoords;

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
			std::vector <float> rightSide_EndCoords;
			std::vector <float> upperSide_EndCoords;

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
				setParamsforPolygon(&chunk, std::distance(chunk.chunkPolygons.begin(), groundIterator), textureId);
			}
			if (stoneIterator != chunk.chunkPolygons.end()) {
				setParamsforPolygon(&chunk, std::distance(chunk.chunkPolygons.begin(), stoneIterator), 4);
			}
			if (snowIterator != chunk.chunkPolygons.end()) {
				setParamsforPolygon(&chunk, std::distance(chunk.chunkPolygons.begin(), snowIterator), 2);
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
					glColor3fv(&currentChunk.colors[k][0]);
					glBindTexture(GL_TEXTURE_2D, currentChunk.textureIds[k]);

					GLfloat ambient[4];
					GLfloat diffuse[4];
					GLfloat specular[4];
					for (GLint i = 0; i < 4; i++) {
						if (i != 3) {
							ambient[i] = currentChunk.materialIds[k].ambient;
							diffuse[i] = currentChunk.materialIds[k].diffuse;
							specular[i] = currentChunk.materialIds[k].specular;
						} else {
							ambient[i] = 1.0;
							diffuse[i] = 1.0;
							specular[i] = 1.0;
						}
					}

					glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
					glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
					glMaterialf(GL_FRONT, GL_SHININESS, currentChunk.materialIds[k].shininess * 128.0);
					glBegin(GL_TRIANGLES);
					k++;
				}
				TriangularSquare_withNormals currentPolygon = currentChunk.chunkPolygons[i];

				glNormal3fv(currentPolygon.lowerTriangleNormal);
				glTexCoord2d(0.0, 0.0);
				glVertex3fv(currentPolygon.lowerLeft);
				glTexCoord2d(0.0, 1.0);
				glVertex3fv(currentPolygon.lowerRight);
				glTexCoord2d(0.5, 0.5);
				glVertex3fv(currentPolygon.center);

				glNormal3fv(currentPolygon.rightTriangleNormal);
				glTexCoord2d(0.0, 1.0);
				glVertex3fv(currentPolygon.lowerRight);
				glTexCoord2d(1.0, 1.0);
				glVertex3fv(currentPolygon.upperRight);
				glTexCoord2d(0.5, 0.5);
				glVertex3fv(currentPolygon.center);

				glNormal3fv(currentPolygon.upperTriangleNormal);
				glTexCoord2d(1.0, 1.0);
				glVertex3fv(currentPolygon.upperRight);
				glTexCoord2d(1.0, 0.0);
				glVertex3fv(currentPolygon.upperLeft);
				glTexCoord2d(0.5, 0.5);
				glVertex3fv(currentPolygon.center);

				glNormal3fv(currentPolygon.leftTriangleNormal);
				glTexCoord2d(1.0, 0.0);
				glVertex3fv(currentPolygon.upperLeft);
				glTexCoord2d(0.0, 0.0);
				glVertex3fv(currentPolygon.lowerLeft);
				glTexCoord2d(0.5, 0.5);
				glVertex3fv(currentPolygon.center);
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
// Листы водяного уровня
GLuint waterLevel_lists[visibleBiomeWidth * visibleBiomeHeight];
// Листы текущего биома
GLuint biome_0_lists[visibleBiomeWidth * visibleBiomeHeight];
// Листы будущего биома
GLuint biome_1_lists[visibleBiomeWidth * visibleBiomeHeight];
// Листы поточного биома
GLuint biome_2_lists[visibleBiomeWidth * visibleBiomeHeight];

void createWaterLists() {
	GLfloat polygonNumberPerSide_water = polygonNumberPerSide / 16.0;
	GLfloat polygonSize_water = polygonSize * 16;

	GLfloat chunkSize = polygonNumberPerSide * polygonSize;
	GLfloat chunkOffset_Y = -chunkSize / 2;

	GLfloat ambient[4] = { materials[0].ambient, materials[0].ambient, materials[0].ambient, 1.0f };
	GLfloat diffuse[4] = { materials[0].diffuse, materials[0].diffuse, materials[0].diffuse, 1.0f };
	GLfloat specular[4] = { materials[0].specular, materials[0].specular, materials[0].specular, 1.0f };

	for (int i = 0; i < visibleBiomeHeight; i++) {
		GLfloat chunkOffset_X = -(visibleBiomeWidth / 2.0) * (chunkSize - polygonSize);

		for (int j = 0; j < visibleBiomeWidth; j++) {
			glNewList(waterLevel_lists[i * visibleBiomeWidth + j], GL_COMPILE);
			glColor3fv(&colors[0][0]);
			glNormal3d(0.0, 0.0, 1.0);
			glBindTexture(GL_TEXTURE_2D, textures[0]);

			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, materials[0].shininess);

			glBegin(GL_QUADS);
			for (int polygonNumber_Y = 0; polygonNumber_Y < polygonNumberPerSide_water; polygonNumber_Y++) {
				for (int polygonNumber_X = 0; polygonNumber_X < polygonNumberPerSide_water; polygonNumber_X++) {
					GLfloat resultOffset_Y = chunkOffset_Y + polygonNumber_Y * polygonSize_water;
					GLfloat resultOffset_X = chunkOffset_X + polygonNumber_X * polygonSize_water;

					glTexCoord2d(0.0, 0.0);
					glVertex3f(resultOffset_X, resultOffset_Y, waterLevel);
					glTexCoord2d(0.0, 1.0);
					glVertex3f(resultOffset_X, resultOffset_Y + polygonSize_water, waterLevel);
					glTexCoord2d(1.0, 1.0);
					glVertex3f(resultOffset_X + polygonSize_water, resultOffset_Y + polygonSize_water, waterLevel);
					glTexCoord2d(1.0, 0.0);
					glVertex3f(resultOffset_X + polygonSize_water, resultOffset_Y, waterLevel);
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
	float translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;

	switch (translateIndexator) {
		case 0:
			glPushMatrix();
			glTranslatef(0.0, -animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_0_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(0.0, translationSize - animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_1_lists);
			glPopMatrix();
			break;
		case 1:
			glPushMatrix();
			glTranslatef(0.0, -animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_1_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(0.0, translationSize - animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_2_lists);
			glPopMatrix();
			break;
		case 2:
			glPushMatrix();
			glTranslatef(0.0, -animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_2_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(0.0, translationSize - animationTranslationMoment, 0.0);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_0_lists);
			glPopMatrix();
			break;
	}
}

void cameraController() {
	GLfloat vectorLength = 0.0;
	GLfloat fogEnd = polygonNumberPerSide * polygonSize * 1.5;

	if (viewMode == 0) {
		GLfloat camMove_X = airPlaneTranslation[0] - (animationAirPlaneMoment * (prevAirPlaneTranslation[0] - airPlaneTranslation[0]) / 4.0);
		GLfloat camMove_Y = airPlaneTranslation[1] - (animationAirPlaneMoment * (prevAirPlaneTranslation[1] - airPlaneTranslation[1]));
		vectorLength = sqrt(pow(airPlaneTranslation[0], 2) + pow(airPlaneTranslation[1] + 1.0, 2) + pow(airPlaneTranslation[2] + 1.0, 2));
		camera.pos.setCoords(airPlaneTranslation[0], airPlaneTranslation[1] + 1.0, airPlaneTranslation[2] + 1.0);
		camera.lookPoint.setCoords(camMove_X, camMove_Y + 2.0, airPlaneTranslation[2] + 0.5);
	} else if (viewMode == 1) {
		GLfloat camMove_X = airPlaneTranslation[0] - (animationAirPlaneMoment * (prevAirPlaneTranslation[0] - airPlaneTranslation[0]) * 2.0);
		GLfloat camMove_Y = airPlaneTranslation[1] - (animationAirPlaneMoment * (prevAirPlaneTranslation[1] - airPlaneTranslation[1]) * 2.0);
		vectorLength = sqrt(pow(airPlaneTranslation[0], 2) + pow(airPlaneTranslation[1] - 7.0, 2) + pow(airPlaneTranslation[2] + 4.0, 2));
		camera.pos.setCoords(airPlaneTranslation[0], airPlaneTranslation[1] - 7.0, airPlaneTranslation[2] + 4.0);
		camera.lookPoint.setCoords(camMove_X, camMove_Y, airPlaneTranslation[2]);
	} else if (viewMode == 2) {
		if (!operatorMode) {
			camera.fi1 = M_PI / 180.0 * -90.0;
			camera.fi2 = M_PI / 180.0 * 45.0;
		}
		camera.pos.setCoords(camera.camDist * cos(camera.fi2) * cos(camera.fi1),
							 camera.camDist * cos(camera.fi2) * sin(camera.fi1),
							 camera.camDist * sin(camera.fi2));
		camera.lookPoint.setCoords(0.0, 0.0, maxHeight);
	}
	fogEnd -= vectorLength;

	glFogf(GL_FOG_START, fogEnd / 5.0);
	glFogf(GL_FOG_END, fogEnd);
}

int getBiomeId() {
	if (currentBiomeLength == 0) {
		currentBiomeLength = rand() % 2 + 1;
		currentBiomeId = rand() % biomeParams.size();
	} else currentBiomeLength--;
	return currentBiomeId;
}


GLuint torusList;
void createTorusList() {
	glNewList(torusList, GL_COMPILE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	int partsNumber = 30;
	GLfloat firstPoint[3], secondPoint[3], generalPoint[3];
	for (int i = 0; i < partsNumber; i++) {
		glColor3d(1.0, 1.0, 0.0);
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= partsNumber; j++) {
			for (int k = 1; k >= 0; k--) {
				GLfloat s = (i + k) % partsNumber + 0.5;
				GLfloat t = j % partsNumber;

				GLfloat x = (1.0 + 0.1 * cos(s * 2.0 * M_PI / partsNumber)) * cos(t * 2.0 * M_PI / partsNumber);
				GLfloat y = (1.0 + 0.1 * cos(s * 2.0 * M_PI / partsNumber)) * sin(t * 2.0 * M_PI / partsNumber);
				GLfloat z = 0.1 * sin(s * 2.0 * M_PI / partsNumber);

				glVertex3f(x, y, z);
			}
		}
		glEnd();
	}

	lightMode ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
	textureMode ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
	glEndList();
}

float distance(float x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2) {
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
	animationLightMoment = polygonSize * polygonNumberPerSide * visibleBiomeWidth * 0.9;

	if (quickMultiplier > 1) {
		polygonNumberPerSide = (int) pow(2, quickMultiplier);
		polygonSize = 128.0 / GLfloat(polygonNumberPerSide);
		polygonNumberInChunk = (int) pow(polygonNumberPerSide, 2);

		for (int i = 0; i < (int) biomeParams.size(); i++) {
			biomeParams[i].scale = pow(2, biomeParams[i].scale + quickMultiplier - 8);
		}
	} else {
		for (int i = 0; i < (int) biomeParams.size(); i++) {
			biomeParams[i].scale = pow(2, biomeParams[i].scale);
		}
	}

	torusList = glGenLists(1);
	waterLevel_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	biome_0_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	biome_1_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	biome_2_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);

	for (int i = 1; i < visibleBiomeWidth * visibleBiomeHeight; i++) {
		waterLevel_lists[i] = waterLevel_lists[i - 1] + 1;
		biome_0_lists[i] = biome_0_lists[i - 1] + 1;
		biome_1_lists[i] = biome_1_lists[i - 1] + 1;
		biome_2_lists[i] = biome_2_lists[i - 1] + 1;
	}

	biome_0.biomeParams = biomeParams[getBiomeId()];
	biome_1.biomeParams = biomeParams[getBiomeId()];
	biome_2.biomeParams = biomeParams[getBiomeId()];

	std::thread myThread_0(startThread, &biome_0, &biome_1);
	myThread_0.join();

	std::thread myThread_1(startThread, &biome_1, &biome_2);
	myThread_1.join();

	std::thread myThread_2(startThread, &biome_2, &biome_0);
	myThread_2.join();

	createWaterLists();
	generateLists(biome_0_lists, biome_0);
	generateLists(biome_1_lists, biome_1);
	generateLists(biome_2_lists, biome_2);

	if (gameStarted) createTorusList();
}

void myRender() {
	std::this_thread::sleep_for(std::chrono::microseconds(16000));

	GLfloat fogColor[4] = { sunColor[0] / 255.0f, sunColor[1] / 255.0f, sunColor[2] / 255.0f, 1.0f };
	glFogfv(GL_FOG_COLOR, fogColor);

	animationController();
	lightController();
	cameraController();
	if (isReadyToControl) {
		if (!manualControl) {
			airPlaneAutoFlyController();
		} else airPlaneAutoManualController();
	}
	if (isControlSwitched) airPlaneControlSwitchController();

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
		int ringNeed = 20;
		if (rings.size() != ringNeed) {
			for (int i = 0; i < (ringNeed - rings.size()); i++) {
				// noob : GLfloat speed = 1;
				// default : GLfloat speed = getRandomNumber(0.9, 1.2);
				// master : GLfloat speed = getRandomNumber(0.6, 4);
				float speed = getRandomNumber(0.6, 4.0);
				float size = getRandomNumber(3.5, 4.5);

				float x = getRandomNumber(-15.0, 15.0);
				float y = getRandomNumber(100.0, 500.0);
				float z = maxHeight;

				// Проверка на дистанцию, если рядом с сгенерируемой координатой в дистанции имеется объект - игнорим добавление 
				bool distancePassed = true;
				for (int i = 0; i < rings.size() && distancePassed; i++) {
					Ring *ring = rings[i];
					if (ring != NULL) {
						if (distance(ring->x, ring->y, ring->z, x, y, z) < (ring->size + size) * 5) {
							distancePassed = false;
						}
					}
				}

				// Также проверим чтобы спавн не происходил вблизи/за самолетом
				float translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;
				float yPosReal = y - animationTranslationMoment * speed;
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
				float translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;
				float yPosReal = ring->y - animationTranslationMoment * ring->speed;
				glPushMatrix();
				glTranslatef(ring->x, yPosReal, ring->z);
				glRotatef(90.0, 1.0, 0.0, 0.0);
				glScalef(ring->size, ring->size, ring->size);
				glCallList(torusList);
				glPopMatrix();
			}
		}

		DWORD64 currentMseconds = GetTickCount64();
		// проверка позиций колец
		for (int i = 0; i < rings.size(); i++) {
			Ring *ring = rings[i];
			if (ring != NULL) {
				float translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;
				float yPosReal = ring->y - animationTranslationMoment * ring->speed;
				if (distance(ring->x, yPosReal, ring->z, airPlaneTranslation[0], airPlaneTranslation[1], airPlaneTranslation[2]) < ring->size + 4) {
					// дистанция очень близкая к самолету
					// назначим кол-во милиссикунд после которого кольцо пропадет если это еще не сделано
					if (ring->deadMSeconds == -1) {
						ring->deadMSeconds = currentMseconds + 1000; // через 1 сек после попадания - кольцо пропадет
						BASS_ChannelPlay(streamHit, TRUE);
						torusScore++;
					}
				}
				if (ring->deadMSeconds <= currentMseconds) rings.erase(rings.begin() + i);
				// Тут нужна проверка - если кольцо улетело далеко - его надо удалять из вектора, чтобы тот пополнялся новыми кольцами
			}
		}
	}

	if (viewMode != 0) drawAirPlane();
}
//===========================================

int prevMouseCoord_X = 0, prevMouseCoord_Y = 0; // Старые координаты мыши

void mouseEvent(OpenGL *ogl, GLint mouseCoord_X, GLint mouseCoord_Y) {
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

	if (viewMode == 2 && manualControl && isReadyToControl && !OpenGL::isKeyPressed(VK_LBUTTON)) {
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);
		GLfloat z = airPlaneTranslation[2];

		GLfloat k = 0;
		if (r.direction.Z() == 0) {
			k = 0;
		} else k = (z - r.origin.Z()) / r.direction.Z();

		GLfloat x = k * r.direction.X() + r.origin.X();
		GLfloat y = k * r.direction.Y() + r.origin.Y();

		if (y < 0.0 && y > -30.0) {
			pointToFly[0] = x;
			pointToFly[1] = y;
		}
	}
}

void mouseWheelEvent(OpenGL *ogl, GLint delta) {
	if (operatorMode) {
		if (delta < 0 && camera.camDist <= 1) return;
		if (delta > 0 && camera.camDist >= 100)	return;

		camera.camDist += 0.01 * delta;
	}
}

void keyDownEvent(OpenGL *ogl, GLint key) {
	// if (key == 'G') gameStarted = !gameStarted;

	if (key == 'O') {
		operatorMode = !operatorMode;
		if (!operatorMode) {
			fogMode = true;
			lightMode = true;
			textureMode = true;
			camera.camDist = maxHeight + 20.0f;
			camera.fi1 = M_PI / 180.0f * -90.0f;
			camera.fi2 = M_PI / 180.0f * 45.0f;
		}
	}

	if (!manualControl) {
		if (key == '1') viewMode = 0;
		if (key == '2') viewMode = 1;
	}
	if (key == '3') viewMode = 2;

	if (key == 'A') {
		manualControl = !manualControl;
		if (manualControl) {
			isReadyToControl = false;
			isControlSwitched = true;
			viewMode = 2;
		} else {
			savedAnimStatus = animationAirPlaneMoment;
			isReadyToControl = false;
			isControlSwitched = true;
		}
	}

	if (key == 'M') {
		isMusicStoped = !isMusicStoped;
		if (isMusicStoped) {
			BASS_ChannelStop(streamMusic);
		} else BASS_ChannelPlay(streamMusic, TRUE);
	}

	if (operatorMode) {
		if (key == 'F') fogMode = !fogMode;
		if (key == 'L') lightMode = !lightMode;
		if (key == 'T') textureMode = !textureMode;
		if (key == 'J') {
			for (int i = 0; i < 3; i++) sunColor[i] = 255.0f;
			lightLockMode = !lightLockMode;
		}
		if (key == 'R') {
			viewMode = 2;
			fogMode = true;
			lightMode = true;
			textureMode = true;
			camera.camDist = maxHeight + 20.0f;
			camera.fi1 = M_PI / 180.0f * -90.0f;
			camera.fi2 = M_PI / 180.0f * 45.0f;
			animationLightMoment = 0.0f;
		}
	}
}

void keyUpEvent(OpenGL *ogl, GLint key) {

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

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 0.35);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
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
	rec.setSize(500, 1000);
	rec.setPosition(10, ogl->getHeight() - 1000 - 10);

	std::stringstream ss;

	float angle = 360.0 * animationLightMoment / (polygonSize * polygonNumberPerSide * visibleBiomeWidth);
	int hours = (int) round(angle) / 15 + 6;
	int minutes = (angle - (hours - 6.0) * 15.0) * 4 + 2;
	if (hours >= 24) hours = hours - 24;
	ss << "Time: " << hours << ":" << minutes << std::endl;

	if (angle > 333.0f && angle <= 356.0f) {
		backgroundColor = (angle - 333.0f) / (356.0f - 333.0f);
	} else if (angle > 195.0f && angle <= 217.0f) {
		backgroundColor = 1.0f - (angle - 195.0f) / (217.0f - 195.0f);
	}

	if (angle > 333.0f) {
		if (sunColor[0] + (255.0f / 255.0f) * 3.0f < 255.0f) sunColor[0] += (255.0f / 255.0f) * 3.0f;
		if (sunColor[1] + (225.0f / 255.0f) * 3.0f < 255.0f) sunColor[1] += (225.0f / 255.0f) * 3.0f;
		if (sunColor[2] + (195.0f / 255.0f) * 3.0f < 255.0f) sunColor[2] += (195.0f / 255.0f) * 3.0f;
	} else if (angle > 195.0f) {
		if (sunColor[0] - (255.0f / 255.0f) * 3.0f > 0.0f) sunColor[0] -= (255.0f / 255.0f) * 3.0f;
		if (sunColor[1] - (225.0f / 255.0f) * 3.0f > 0.0f) sunColor[1] -= (225.0f / 255.0f) * 3.0f;
		if (sunColor[2] - (195.0f / 255.0f) * 3.0f > 0.0f) sunColor[2] -= (195.0f / 255.0f) * 3.0f;
	}

	glClearColor(sunColor[0] / 255.0f, sunColor[1] / 255.0f, sunColor[2] / 255.0f, 1.0f);
	GLfloat diffuse[] = {
		sunColor[0] / 255.0f,
		sunColor[1] / 255.0f,
		sunColor[2] / 255.0f, 1.0f
	};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

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
	std::string musicStatus = isMusicStoped ? " [выключена]" : " [включена]";
	ss << "Колец собрано: " << torusScore << std::endl;
	ss << "M - вкл/выкл музыку" << musicStatus << std::endl;
	ss << "A - вкл/выкл режима пилота" << manualStatus << std::endl << std::endl;
	ss << "1 - вид от кабины пилота" << std::endl;
	ss << "2 - вид за самолетом" << std::endl;
	ss << "3 - закрепленный вид" << std::endl << std::endl;

	if (operatorMode) {
		std::string textureStatus = textureMode ? " [вкл]" : " [выкл]";
		std::string fogStatus = fogMode ? " [вкл]" : " [выкл]";
		std::string lightStatus = lightMode ? " [вкл]" : " [выкл]";
		std::string lightLockStatus = lightLockMode ? " [вкл]" : " [выкл]";

		ss << "РЕЖИМ ОПЕРАТОРА" << std::endl << std::endl;
		ss << "T - вкл/выкл текстур" << textureStatus << std::endl;
		ss << "F - вкл/выкл туман" << fogStatus << std::endl;
		ss << "L - вкл/выкл освещение" << lightStatus << std::endl;
		ss << "J - блокировка смены суток" << lightLockStatus << std::endl << std::endl;

		ss << "animationTranslationMoment: " << animationTranslationMoment << std::endl;
		ss << "animationLightMoment: " << animationLightMoment << std::endl;
		ss << "animationAirPlaneMoment: " << animationAirPlaneMoment << std::endl;
		ss << "angle: " << angle << std::endl;
		ss << "sunColor: (" << sunColor[0] << ", " << sunColor[1] << ", " << sunColor[2] << ")" << std::endl;
		ss << "backgroundColor: " << backgroundColor << std::endl;
		ss << "start_new: " << start_new << std::endl;
		ss << "thread_done: " << thread_done << std::endl << std::endl;

		ss << "AirPlane Pos.: (" << airPlaneTranslation[0] << ", " << airPlaneTranslation[1] << ", " << airPlaneTranslation[2] << ")" << std::endl;
		ss << "PointToFly: (" << pointToFly[0] << ", " << pointToFly[1] << ")" << std::endl;
		ss << "Light Pos.: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
		ss << "Camera Pos.: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
		ss << "Cam Look: (" << camera.lookPoint.X() << ", " << camera.lookPoint.Y() << ", " << camera.lookPoint.Z() << ")" << std::endl;
		ss << "Cam Params: R=" << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;
	}

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
	myRender();
	//===================================
	drawMessage(ogl);
}