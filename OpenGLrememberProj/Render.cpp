#pragma comment (lib, "bass.lib")

#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "bass.h"
#include "MyOGL.h"
#include "Light.h"
#include "Camera.h"
#include "Render.h"
#include "Texture.h"
#include "ObjLoader.h"
#include "MyShaders.h"
#include "Primitives.h"
#include "GUItextRectangle.h"

#include <GL\GL.h>
#include <GL\GLU.h>
#include <windows.h>

// *** *** *** *** *** *** *** *** *** Do not touch parameters *** *** *** *** *** *** *** *** ***
// Struct for biome parameters [watch string 66: Biome generation parameters]
struct BiomeParams {
	GLushort biomeId;		// Biome id
	GLfloat minAltitude;		// Min height
	GLfloat maxAltitude;		// Max height
	GLfloat scale;			// Scale
};

// Struct for surface materials [watch string 106: Materials parameters]
struct Material {
	GLfloat ambient;		// Ambient color
	GLfloat diffuse;		// Diffuse color
	GLfloat specular;		// Specular color
	GLfloat shininess;		// Shininess color
};
//************************************************************************************************

// *** *** *** *** *** *** *** ***  Parameters that can be changed  *** *** *** *** *** *** *** ***
// FPS parameters
GLushort FPS_MIN = 60;			// Must be defined			| Default: 30
const GLshort FPS_MAX = 60;		// Work if FPS_MAX (0; 200)		| Default: 60
const GLfloat maxHeight = 30.0f;	// Max height				| Deafult: 30.0f

// Parameters of the rendered area (better not touch it)
// Fast switching, works if quickMultiplier > 1
// Automatically determines the values for:
// polygonSize, polygonNumberPerSide, polygonNumberInChunk, BiomeParams.scale
const GLushort quickMultiplier = 7;	// The lower, the easier the render (work if quickMultiplier > 1) 	| Default: 7
GLfloat polygonSize = pow(2, -1);						// Change only the exponent	| Default: -1
GLuint polygonNumberPerSide = (int) pow(2, 8);					// Change only the exponent 	| Default: 8
const GLushort visibleBiomeHeight = 2;						// Visible biome height		| Default: 2
const GLushort visibleBiomeWidth = 2 * visibleBiomeHeight + 1;			// Visible biome width		| Default: 2 * visibleBiomeHeight + 1

// Ground cover levels parameters
const GLfloat waterLevel = 0.0f;	// Water level height							| Default: 0.0f
const GLfloat groundLevel = 5.0f;	// Ground level height (grass, sand, snow)				| Default: 5.0f
const GLfloat stoneLevel = 20.0f;	// Stone level height							| Default: 20.0f
const GLfloat snowLevel = 25.0f;	// Snow level height (snow caps of mountains)	| Default: 25.0f

// Biome generation parameters
// Quick adjustment works if quickMultiplier > 3
// [id, min height, max height, scale]
// * Don't change the id!
// * The larger the scale, the greater the relief
std::vector<BiomeParams> biomeParams = {
	{ 0, 	-5.0f,		waterLevel,	2 },	// Ocean
	{ 1, 	waterLevel,	groundLevel,	2 },	// Plain (earthy)
	{ 2, 	waterLevel,	groundLevel,	2 },	// Plain (snowy)
	{ 3, 	waterLevel,	groundLevel,	1 },	// Plain (sandy)
	{ 4, 	-groundLevel,	groundLevel,	1 },	// Swamp
	{ 5, 	waterLevel,	snowLevel,	0 },	// Mountains
	{ 6, 	-groundLevel,	snowLevel,	0 },	// Mountains (with water)
	{ 7, 	-groundLevel,	groundLevel,	3 },	// Shores (earthy)
	{ 8, 	-groundLevel,	groundLevel,	3 },	// Shores (snowy)
	{ 9, 	-groundLevel,	groundLevel,	3 },	// Shores (sandy)
	{ 10, 	-waterLevel,	groundLevel,	0 },	// Hills (earthy)
	{ 11, 	-waterLevel,	groundLevel,	0 },	// Hills (snowy)
	{ 12, 	-waterLevel,	groundLevel,	0 }	// Hills (sandy)
};

// Texture names (better not touch it)
// * But you can change the texture files (bmp, 24 bit - required)
const std::string textureNames[] = {
	"water.bmp",			// 0 - water texture
	"grass.bmp",			// 1 - grass texture
	"snow.bmp",			// 2 - snow texture
	"sand.bmp",			// 3 - sand texture
	"stone.bmp"			// 4 - stone texture
};

// Colors parameters
const std::vector <std::vector <GLfloat>> colors = {
	{ 0.42, 0.48, 0.97 },		// 0 - water color	| Default { 0.42, 0.48, 0.97 }
	{ 0.47, 0.68, 0.33 },		// 1 - grass color	| Default { 0.47, 0.68, 0.33 }
	{ 0.94, 0.98, 0.98 },		// 2 - snow color	| Default { 0.94, 0.98, 0.98 }
	{ 0.86, 0.83, 0.63 },		// 3 - sand color	| Default { 0.86, 0.83, 0.63 }
	{ 0.49, 0.49, 0.49 }		// 4 - stone color	| Default { 0.49, 0.49, 0.49 }
};

// Materials parameters
const std::vector <Material> materials = {
	{ 0.06, 0.95, 0.90, 0.50 },	// 0 - water material	| Default { 0.06, 0.95, 0.90, 0.50 }
	{ 0.32, 0.90, 0.10, 0.05 },	// 1 - grass material	| Default { 0.32, 0.90, 0.10, 0.05 }
	{ 0.87, 0.82, 0.35, 0.35 },	// 2 - snow material	| Default { 0.87, 0.82, 0.35, 0.35 }
	{ 0.18, 0.90, 0.15, 0.10 },	// 3 - sand material	| Default { 0.18, 0.90, 0.15, 0.10 }
	{ 0.40, 0.92, 0.05, 0.02 }	// 4 - stone material	| Default { 0.40, 0.92, 0.05, 0.02 }
};

// Animation parameters
const GLfloat animationSpeed = 0.25f;		// Default: 0.25f
const GLfloat animationAirCraftSpeed = 0.005f;	// Default: 0.005f
//************************************************************************************************

// *** *** *** *** *** *** *** *** *** Do not touch parameters *** *** *** *** *** *** *** *** ***
// Class for setting up the camera
class MyCamera : public Camera {
public:
	// Set default params
	MyCamera() {
		cameraDistance = maxHeight + 20.0f;
		cameraAngle_XY = M_PI / 180.0f * -90.0f;
		cameraAngle_Z = M_PI / 180.0f * 45.0f;
	}

	// Set up the camera, called by the engine
	void SetUpCamera() {
		if (myCamera.cameraAngle_XY < 0.0f) myCamera.cameraAngle_XY += 2.0f * M_PI;
		if (myCamera.cameraAngle_XY > 2.0f * M_PI) myCamera.cameraAngle_XY -= 2.0f * M_PI;

		myCamera.cameraAngle_XY += 2.0f * M_PI;

		if (cosf(cameraAngle_Z) <= 0.0f) {
			normal.setCoords(0.0f, 0.0f, -1.0f);
		} else normal.setCoords(0.0f, 0.0f, 1.0f);

		LookAt();
	}
}  myCamera;

// Class for setting up the light
class MyLight : public Light {
public:
	// Set default params
	MyLight() { }

	// Draw the light source
	void DrawLightSource() { }

	// Set up the light, called by the engine
	void SetUpLight() {
		GLfloat ambient[] = { 0.05f, 0.05f, 0.05f, 1.0f };	// Ambient color
		GLfloat specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };	// Specular color

		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);		// Setting ambient color
		glLightfv(GL_LIGHT0, GL_SPECULAR, specular);		// Setting specular color

		glEnable(GL_LIGHT0);					// Sets the light source
	}
} myLight;

// Class for polygon generating and drawing
class TriangularSquare_withNormals {
private:
	// Calculates the normal
	void calculateNormal(GLfloat *firstPoint, GLfloat *secondPoint, GLfloat *resultNormal) {
		GLfloat x_1 = firstPoint[0];
		GLfloat y_1 = firstPoint[1];
		GLfloat z_1 = firstPoint[2];

		GLfloat x_2 = secondPoint[0];
		GLfloat y_2 = secondPoint[1];
		GLfloat z_2 = secondPoint[2];

		GLfloat x_3 = center[0];
		GLfloat y_3 = center[1];
		GLfloat z_3 = center[2];

		GLfloat line_1[] = { x_2 - x_1, y_2 - y_1, z_2 - z_1 };
		GLfloat line_2[] = { x_2 - x_3, y_2 - y_3, z_2 - z_3 };

		GLfloat n_x = line_1[1] * line_2[2] - line_2[1] * line_1[2];
		GLfloat n_y = line_1[0] * line_2[2] - line_2[0] * line_1[2];
		GLfloat n_z = line_1[0] * line_2[1] - line_2[0] * line_1[1];

		GLfloat length = sqrtf(powf(n_x, 2.0f) + powf(n_y, 2.0f) + powf(n_z, 2.0f));
		resultNormal[0] = n_x / length;
		resultNormal[1] = -n_y / length;
		resultNormal[2] = n_z / length;
	}

public:
	GLfloat lowerLeft[3];			// Lower left point for polygon
	GLfloat lowerRight[3];			// Lower right point for polygon
	GLfloat upperRight[3];			// Upper left point for polygon
	GLfloat upperLeft[3];			// Upper right point for polygon
	GLfloat center[3];			// Center point for polygon

	GLfloat lowerTriangleNormal[3];	// Normal for lower triangle
	GLfloat rightTriangleNormal[3];	// Normal for right triangle
	GLfloat upperTriangleNormal[3];	// Normal for upper triangle
	GLfloat leftTriangleNormal[3];	// Normal for left triangle

	// Sets the lower left point
	void setLowerLeft(GLfloat x, GLfloat y, GLfloat z) {
		lowerLeft[0] = x;
		lowerLeft[1] = y;
		lowerLeft[2] = z;
	}

	// Sets the lower right point
	void setLowerRight(GLfloat x, GLfloat y, GLfloat z) {
		lowerRight[0] = x;
		lowerRight[1] = y;
		lowerRight[2] = z;
	}

	// Sets the upper right point
	void setUpperRight(GLfloat x, GLfloat y, GLfloat z) {
		upperRight[0] = x;
		upperRight[1] = y;
		upperRight[2] = z;
	}

	// Sets the upper left point
	void setUpperLeft(GLfloat x, GLfloat y, GLfloat z) {
		upperLeft[0] = x;
		upperLeft[1] = y;
		upperLeft[2] = z;
	}

	// Calculates the center point
	void calculateCenter() {
		center[0] = (lowerLeft[0] + upperRight[0]) / 2.0f;
		center[1] = (lowerLeft[1] + upperRight[1]) / 2.0f;
		center[2] = (lowerLeft[2] + lowerRight[2] + upperRight[2] + upperLeft[2]) / 4.0f;
	}

	// Returns "Z" coordinate of the center point
	GLfloat getCenterCoord_Z() {
		return center[2];
	}

	// Calculates all normals
	void calculateNormals() {
		calculateNormal(lowerLeft, lowerRight, lowerTriangleNormal);
		calculateNormal(lowerRight, upperRight, rightTriangleNormal);
		calculateNormal(upperRight, upperLeft, upperTriangleNormal);
		calculateNormal(upperLeft, lowerLeft, leftTriangleNormal);
	}
};

// Class for chunk generating and drawing
class Chunk {
public:
	std::vector <TriangularSquare_withNormals> chunkPolygons;	// All polygons in chunk
	std::vector <GLfloat> rightSide_EndCoords;			// Right side end coordinates
	std::vector <GLfloat> upperSide_EndCoords;			// Upper side end coordinates

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

// Struct for torus parameters
struct Ring {
	GLfloat speed, size, x, y, z;
	DWORD64 deadMSeconds;
};

// Class for biome generating and drawing
class Biome {
public:
	std::vector <Chunk> chunks;				// All chunks in biome
	std::vector <std::vector <GLfloat>> biome_EndCoords;	// Biome upper end coordinates
	BiomeParams biomeParams;				// Biome generating parameters

	Biome() {
		this->biomeParams = ::biomeParams[0];
	}
};

// Additional parameters and rendering options
GLboolean operatorMode = false;		// Is "Operator mode" enabled	| Default: false
GLboolean gameStarted = true;		// Is game started		| Default: true
GLboolean fogMode = true;		// Is fog enabled		| Default: true
GLboolean lightMode = true;		// Is light enabled		| Default: true
GLboolean textureMode = true;		// Is textures enabled		| Default: true
GLboolean lightLockMode = false;	// Is light locked		| Default: false
GLboolean isMusicStoped = false;	// Indicates whether to turn off the music
GLuint polygonNumberInChunk = (int) pow(polygonNumberPerSide, 2);	// Number of polygons in a chunk
ObjFile airCraftModel, moonModel;	// Objects (.obj)
GLushort currentBiomeId;		// ID of current biome
GLushort currentBiomeLength;		// Specifies how many biomes with the same id will be generated
GLushort translateIndexator;		// Specifies which biomes to interact with
GLfloat sunColor[3];			// Sun, fog, background color

// Display lists for biome, game and other
std::vector<Ring *> rings;						// Rings array
GLuint torusList;							// Torus list
Biome biome_0, biome_1, biome_2;					// Biomes
GLuint waterLevel_lists[visibleBiomeWidth * visibleBiomeHeight];	// Water level lists
GLuint biome_0_lists[visibleBiomeWidth * visibleBiomeHeight];		// Display lists for biome_0
GLuint biome_1_lists[visibleBiomeWidth * visibleBiomeHeight];		// Display lists for biome_1 
GLuint biome_2_lists[visibleBiomeWidth * visibleBiomeHeight];		// Display lists for biome_2

// Thread params
std::mutex cout_guard;				// To synchronize threads
std::atomic<bool> start_new { true };		// Indicates when to start generating a new biome
std::atomic<bool> thread_done { false };	// Indicates when a new biome is generated

// Aircraft control parameters
GLushort torusScore = 0;			// Counter of collected rings
GLushort viewMode = 2;				// âèä íà ñàìîëåò: îò êàáèíû, ñ õâîñòà, ïî óìîë÷àíèþ
GLfloat pointToFly[2];				// òî÷êà, ê êîòîðîé äîëæåí ëåòåòü ñàìîëåò
GLfloat savedAnimationStatus;			// ñîõðàíÿåò ñòàòóñ àíèìàöèè
GLboolean isControlSwitched = false;		// îïðåäåëåíèå ïåðåêëþ÷åíèÿ ðåæèìà óïðàâëåíèÿ
GLboolean isReadyToControl = true;		// óâåäîìëÿåò î ãîòîâíîñòè ïåðåäàòü êîíòðîëü
GLboolean manualControl = false;		// ðåæèì óïðàâëåíèÿ ñàìîëåòîì
GLfloat airCraftTranslation[3];			// òåêóùèå êîîðäèíàòû ñàìîëåòà
GLfloat prevAirCraftTranslation[3];		// ïðåäûäóùèå êîîðäèíàòû ñàìîëåòà

// Texture parameters
GLushort textureId = 0;						// Texture id
Texture airCraftTexture, moonTexture;				// Object textures
GLuint textures[sizeof(textureNames) / sizeof(std::string)];	// Array of surface textures

// Animation parameters
GLfloat animationLightMoment;			// The time animation of the light
GLfloat animationAirCraftMoment;		// The time animation of the aircraft
GLfloat animationTranslationMoment;		// The time animation of the aircraft translation

// Additional animation params
DWORD currentTick;	// Current tick
DWORD previousTick;	// Previous tick
GLfloat multiplier;	// Multiplier for the same animation speed at different FPS

// FPS counter params
GLuint FPS_counter;	// Counts frames
GLuint FPS_timerStart;	// Calculates 1 second
GLuint FPS_string;	// Write result

// Sound params
HSTREAM streamHit;	// Hit sound stream
HSTREAM streamMusic;	// Music stream

// Store the old mouse coordinates
GLint previousMouseCoord_X = 0;	// Old mouse coordinates - X
GLint previousMouseCoord_Y = 0; // Old mouse coordinates - Y
//************************************************************************************************

void loadTexture(std::string textureName) {
	RGBTRIPLE *textureArray;
	GLchar *textureCharArray;
	GLint textureWidth, textureHeight;

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
	for (GLushort i = 0; i < sizeof(textureNames) / sizeof(std::string); i++) loadTexture(textureNames[i]);
}

GLboolean initializeSound() {
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		MessageBox(NULL, "BASS version error", NULL, 0);
		return true;
	}

	if (!BASS_Init(-1, 22050, BASS_DEVICE_3D, 0, NULL)) {
		MessageBox(NULL, "Failed to initialize BASS", NULL, 0);
		return true;
	}

	GLchar sampleFilepath[] = "sounds/hit.mp3";
	streamHit = BASS_StreamCreateFile(FALSE, sampleFilepath, 0, 0, BASS_SAMPLE_3D);
	if (!streamHit) {
		MessageBox(NULL, "Failed to load sound", NULL, 0);
		return true;
	}

	GLchar streamFilepath[] = "sounds/music.mp3";
	streamMusic = BASS_StreamCreateFile(FALSE, streamFilepath, 0, 0, BASS_SAMPLE_3D);
	if (!streamMusic) {
		MessageBox(NULL, "Failed to load music", NULL, 0);
		return true;
	}

	if (BASS_SampleGetChannel(streamHit, FALSE) || 
		BASS_SampleGetChannel(streamMusic, FALSE)) {
		MessageBox(NULL, "Failed to detect audio channel", NULL, 0);
		return true;
	}

	BASS_ChannelSetAttribute(streamHit, BASS_ATTRIB_VOL, 0.5);
	BASS_ChannelSetAttribute(streamMusic, BASS_ATTRIB_VOL, 0.1);

	return false;
}


void animationController() {
	previousTick = currentTick;
	currentTick = GetTickCount64();
	if (viewMode == 2) {
		GLushort difference = (currentTick - previousTick) > 0 ? currentTick - previousTick : 1;
		multiplier = difference / 15.0f;
	} else multiplier = (1.0f / GLfloat(FPS_MIN) - 1.0f / 200.0f) * powf(10.0f, 3.0f) / 15.0f;

	animationLightMoment += animationSpeed * multiplier;
	animationTranslationMoment += animationSpeed * multiplier;
	animationAirCraftMoment += animationAirCraftSpeed * multiplier;

	if (animationLightMoment > polygonSize * polygonNumberPerSide * visibleBiomeWidth) animationLightMoment = 0.0f;
	if (animationTranslationMoment > polygonSize * polygonNumberPerSide * visibleBiomeHeight - 2.0f * polygonSize) {
		animationTranslationMoment = 0.0f;
		translateIndexator++;
		if (translateIndexator > 2) translateIndexator = 0;
	}
	if (animationAirCraftMoment > 2.0f * M_PI) animationAirCraftMoment = 0.0f;
}

void cameraController() {
	GLfloat vectorLength = 0.0f;
	GLfloat fogEnd = polygonNumberPerSide * polygonSize * 1.5f;

	if (viewMode == 0) {
		GLfloat camMove_X = (airCraftTranslation[0] - (animationAirCraftMoment / multiplier * (prevAirCraftTranslation[0] - airCraftTranslation[0]) / 4.0f));
		GLfloat camMove_Y = (airCraftTranslation[1] - (animationAirCraftMoment / multiplier * (prevAirCraftTranslation[1] - airCraftTranslation[1])));
		vectorLength = sqrtf(powf(airCraftTranslation[0], 2.0f) + powf(airCraftTranslation[1] + 1.0f, 2.0f) + powf(airCraftTranslation[2] + 1.0f, 2.0f));
		myCamera.position.setCoords(airCraftTranslation[0], airCraftTranslation[1] + 1.0f, airCraftTranslation[2] + 1.0f);
		myCamera.lookPoint.setCoords(camMove_X, camMove_Y + 2.0f, airCraftTranslation[2] + 0.5f);
	} else if (viewMode == 1) {
		GLfloat camMove_X = airCraftTranslation[0] - (animationAirCraftMoment / multiplier * (prevAirCraftTranslation[0] - airCraftTranslation[0]) * 2.0f);
		GLfloat camMove_Y = airCraftTranslation[1] - (animationAirCraftMoment / multiplier * (prevAirCraftTranslation[1] - airCraftTranslation[1]) * 2.0f);
		vectorLength = sqrtf(powf(airCraftTranslation[0], 2.0f) + powf(airCraftTranslation[1] - 7.0f, 2) + powf(airCraftTranslation[2] + 4.0f, 2.0f));
		myCamera.position.setCoords(airCraftTranslation[0], airCraftTranslation[1] - 7.0f, airCraftTranslation[2] + 4.0f);
		myCamera.lookPoint.setCoords(camMove_X, camMove_Y, airCraftTranslation[2]);
	} else if (viewMode == 2) {
		if (!operatorMode) {
			myCamera.cameraAngle_XY = M_PI / 180.0f * -90.0f;
			myCamera.cameraAngle_Z = M_PI / 180.0f * 45.0f;
		}
		myCamera.position.setCoords(myCamera.cameraDistance * cos(myCamera.cameraAngle_Z) * cos(myCamera.cameraAngle_XY),
									myCamera.cameraDistance * cos(myCamera.cameraAngle_Z) * sin(myCamera.cameraAngle_XY),
									myCamera.cameraDistance * sin(myCamera.cameraAngle_Z));
		myCamera.lookPoint.setCoords(0.0f, 0.0f, maxHeight);
	}
	fogEnd -= vectorLength;

	glFogf(GL_FOG_START, fogEnd / 5.0f);
	glFogf(GL_FOG_END, fogEnd);
}

void lightController() {
	if (lightLockMode) animationLightMoment = polygonSize * polygonNumberPerSide * visibleBiomeWidth / 4.0f;
	GLfloat i = animationLightMoment / (polygonSize * polygonNumberPerSide * visibleBiomeWidth);

	GLfloat sunAngle = 2.0f * M_PI * i;
	GLfloat sun_dx = visibleBiomeWidth * polygonNumberPerSide * polygonSize * cosf(sunAngle);
	GLfloat sun_dz = visibleBiomeWidth * polygonNumberPerSide * polygonSize * sinf(sunAngle);
	GLfloat sunPosition[3] = { sun_dx, 0.0f, sun_dz };

	glDisable(GL_FOG);

	glPushMatrix();
	GLfloat moonColor = 1.4f - sunColor[0] / 255.0f;
	GLfloat blueColorReducer = (sunColor[0] + sunColor[1]) / (255.0f + sunColor[2]);
	if (blueColorReducer > 1.0f) blueColorReducer -= 1.0f - blueColorReducer;
	GLfloat ambient[] = { moonColor, moonColor, moonColor, 1.0f };
	GLfloat diffuse[] = { sunColor[1] / 255.0f, sunColor[0] / 255.0f, blueColorReducer, 1.0f };
	GLfloat specular[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLfloat shininess = 0.3f * 128.0f;

	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);

	GLfloat moonSize = polygonNumberPerSide * polygonSize / 4.0f;
	GLfloat moonAngle = sunAngle + 166.0f;
	GLfloat moon_dx = moon_dx = polygonNumberPerSide * polygonSize * cosf(moonAngle);
	GLfloat moon_dz = moon_dz = polygonNumberPerSide * polygonSize * sinf(moonAngle);

	glTranslatef(moon_dx, polygonNumberPerSide * polygonSize * visibleBiomeHeight + moonSize, moon_dz);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(-45.0f, 0.0f, 0.0f, 1.0f);
	glScalef(moonSize / 64.0f, moonSize / 64.0f, moonSize / 64.0f);
	glDepthMask(false);
	moonTexture.bindTexture();
	moonModel.DrawObj();
	glDepthMask(true);
	glPopMatrix();

	fogMode ? glEnable(GL_FOG) : glDisable(GL_FOG);
}

void airCraftAutoManualController() {
	GLfloat speed = animationAirCraftSpeed * 200.0f * (0.005f / animationAirCraftSpeed);
	GLfloat inaccuracy = 0.1f;

	GLfloat acceleration_X = powf(max(pointToFly[0], airCraftTranslation[0]) -
								  min(pointToFly[0], airCraftTranslation[0]), 2) / 100.0f;
	GLfloat acceleration_Y = powf(max(pointToFly[1], airCraftTranslation[1]) -
								  min(pointToFly[1], airCraftTranslation[1]), 2) / 100.0f;

	acceleration_X = acceleration_X > 1.0f ? acceleration_X : 1.0f;
	acceleration_Y = acceleration_Y > 1.0f ? acceleration_Y : 1.0f;
	acceleration_X = acceleration_X < 20.0f ? acceleration_X : 20.0f;
	acceleration_Y = acceleration_Y < 20.0f ? acceleration_Y : 20.0f;

	GLfloat vectorLength = sqrtf(powf(pointToFly[0], 2.0f) + powf(pointToFly[1], 2.0f));
	if (vectorLength == 0.0f) vectorLength = 1.0f;
	GLfloat speed_X = speed * acceleration_X * (max(pointToFly[0], airCraftTranslation[0]) -
												min(pointToFly[0], airCraftTranslation[0])) / vectorLength;
	GLfloat speed_Y = speed * acceleration_Y * (max(pointToFly[1], airCraftTranslation[1]) -
												min(pointToFly[1], airCraftTranslation[1])) / vectorLength;

	prevAirCraftTranslation[0] = airCraftTranslation[0];
	prevAirCraftTranslation[1] = airCraftTranslation[1];
	prevAirCraftTranslation[2] = airCraftTranslation[2];

	if (airCraftTranslation[0] > pointToFly[0] + inaccuracy) {
		airCraftTranslation[0] -= speed_X;
	} else if (airCraftTranslation[0] < pointToFly[0] - inaccuracy) airCraftTranslation[0] += speed_X;

	if (airCraftTranslation[1] > pointToFly[1] + inaccuracy) {
		airCraftTranslation[1] -= speed_Y;
	} else if (airCraftTranslation[1] < pointToFly[1] - inaccuracy) airCraftTranslation[1] += speed_Y;
}

void airCraftAutoFlyController() {
	GLfloat x = 25.0f * (sqrtf(2.0f) * cosf(animationAirCraftMoment)) / (1.0f + powf(sinf(animationAirCraftMoment), 2.0f));
	GLfloat y = 10.0f * exp(sin(animationAirCraftMoment));
	GLfloat z = 10.0f * (sqrtf(2.0f) * cosf(animationAirCraftMoment) * sinf(animationAirCraftMoment)) /
		(1.0f + powf(sinf(animationAirCraftMoment), 2.0f)) + maxHeight + 5.0f;

	prevAirCraftTranslation[0] = airCraftTranslation[0];
	prevAirCraftTranslation[1] = airCraftTranslation[1];
	prevAirCraftTranslation[2] = airCraftTranslation[2];

	airCraftTranslation[0] = x;
	airCraftTranslation[1] = y;
	airCraftTranslation[2] = z;
}

void airCraftControlSwitchController() {
	prevAirCraftTranslation[0] = airCraftTranslation[0];
	prevAirCraftTranslation[1] = airCraftTranslation[1];
	prevAirCraftTranslation[2] = airCraftTranslation[2];

	GLfloat speed = animationAirCraftSpeed * 50.0f;
	GLfloat inaccuracy = 2.0f * speed;

	if (manualControl) {
		GLfloat acceleration_X = pow(airCraftTranslation[0], 2) / 100.0f;
		GLfloat acceleration_Y = pow(-10.0f - airCraftTranslation[1], 2) / 100.0f;
		GLfloat acceleration_Z = pow(maxHeight - airCraftTranslation[2], 2) / 100.0f;

		acceleration_X = acceleration_X > 0.5f * (0.005f / animationAirCraftSpeed) ?
			acceleration_X : 0.5f * (0.005f / animationAirCraftSpeed);
		acceleration_Y = acceleration_Y > 0.5f * (0.005f / animationAirCraftSpeed) ?
			acceleration_Y : 0.5f * (0.005f / animationAirCraftSpeed);
		acceleration_Z = acceleration_Z > 0.5f * (0.005f / animationAirCraftSpeed) ?
			acceleration_Z : 0.5f * (0.005f / animationAirCraftSpeed);

		if (airCraftTranslation[0] > inaccuracy || airCraftTranslation[0] < -inaccuracy ||
			airCraftTranslation[1] > -10.0f + inaccuracy || airCraftTranslation[1] < -10.0f - inaccuracy ||
			airCraftTranslation[2] > maxHeight + inaccuracy ||
			airCraftTranslation[2] < maxHeight - inaccuracy) {
			if (airCraftTranslation[0] > inaccuracy) {
				airCraftTranslation[0] -= speed * acceleration_X;
			} else if (airCraftTranslation[0] < -inaccuracy) {
				airCraftTranslation[0] += speed * acceleration_X;
			}

			if (airCraftTranslation[1] > -10.0 + inaccuracy) {
				airCraftTranslation[1] -= speed * acceleration_Y;
			} else if (airCraftTranslation[1] < -10.0f - inaccuracy) {
				airCraftTranslation[1] += speed * acceleration_Y;
			}

			if (airCraftTranslation[2] > maxHeight + inaccuracy) {
				airCraftTranslation[2] -= speed * acceleration_Z;
			} else if (airCraftTranslation[2] < maxHeight - inaccuracy) {
				airCraftTranslation[2] += speed * acceleration_Z;
			}
		} else {
			isReadyToControl = true;
			isControlSwitched = false;
		}
	} else {
		GLfloat animOffset = 0.75f;
		if (savedAnimationStatus + animOffset > 2.0f * M_PI) animOffset = 2.0f * M_PI - savedAnimationStatus;

		GLfloat x = 25.0f * (sqrtf(2.0f) * cosf(savedAnimationStatus + animOffset)) /
			(1.0f + powf(sinf(savedAnimationStatus + animOffset), 2.0f));
		GLfloat y = 10.0f * expf(sinf(savedAnimationStatus + animOffset));
		GLfloat z = 10.0f * (sqrtf(2.0) * cosf(savedAnimationStatus + animOffset) * sinf(savedAnimationStatus + animOffset)) / 
			(1.0f + powf(sinf(savedAnimationStatus + animOffset), 2.0f)) + maxHeight + 5.0f;

		GLfloat acceleration_X = powf(x - airCraftTranslation[0], 2.0f) / 100.0f;
		GLfloat acceleration_Y = powf(y - airCraftTranslation[1], 2.0f) / 100.0f;
		GLfloat acceleration_Z = powf(z - airCraftTranslation[2], 2.0f) / 100.0f;

		acceleration_X = acceleration_X > 0.5f * (0.005f / animationAirCraftSpeed) ? 
			acceleration_X : 0.5f * (0.005f / animationAirCraftSpeed);
		acceleration_Y = acceleration_Y > 0.5f * (0.005f / animationAirCraftSpeed) ? 
			acceleration_Y : 0.5f * (0.005f / animationAirCraftSpeed);
		acceleration_Z = acceleration_Z > 0.5f * (0.005f / animationAirCraftSpeed) ? 
			acceleration_Z : 0.5f * (0.005f / animationAirCraftSpeed);

		if (airCraftTranslation[0] > x + inaccuracy || airCraftTranslation[0] < x - inaccuracy ||
			airCraftTranslation[1] > y + inaccuracy || airCraftTranslation[1] < y - inaccuracy ||
			airCraftTranslation[2] > z + inaccuracy || airCraftTranslation[2] < z - inaccuracy ||
			animationAirCraftMoment > savedAnimationStatus + animOffset + animationAirCraftSpeed ||
			animationAirCraftMoment < savedAnimationStatus + animOffset - animationAirCraftSpeed) {
			if (airCraftTranslation[0] > x + inaccuracy) {
				airCraftTranslation[0] -= speed * acceleration_X;
			} else if (airCraftTranslation[0] < x - inaccuracy) {
				airCraftTranslation[0] += speed * acceleration_X;
			}

			if (airCraftTranslation[1] > y + inaccuracy) {
				airCraftTranslation[1] -= speed * acceleration_Y;
			} else if (airCraftTranslation[1] < y - inaccuracy) {
				airCraftTranslation[1] += speed * acceleration_Y;
			}

			if (airCraftTranslation[2] > z + inaccuracy) {
				airCraftTranslation[2] -= speed * acceleration_Z;
			} else if (airCraftTranslation[2] < z - inaccuracy) {
				airCraftTranslation[2] += speed * acceleration_Z;
			}
		} else {
			isReadyToControl = true;
			isControlSwitched = false;
		}
	}
}

void translationController() {
	GLfloat translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0f * polygonSize;

	switch (translateIndexator) {
		case 0:
			glPushMatrix();
			glTranslatef(0.0f, -animationTranslationMoment, 0.0f);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_0_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(0.0f, translationSize - animationTranslationMoment, 0.0f);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_1_lists);
			glPopMatrix();
			break;
		case 1:
			glPushMatrix();
			glTranslatef(0.0f, -animationTranslationMoment, 0.0f);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_1_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(0.0f, translationSize - animationTranslationMoment, 0.0f);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_2_lists);
			glPopMatrix();
			break;
		case 2:
			glPushMatrix();
			glTranslatef(0.0f, -animationTranslationMoment, 0.0f);
			glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, biome_2_lists);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(0.0f, translationSize - animationTranslationMoment, 0.0f);
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

GLfloat getRandomNumber(GLfloat lowerBorder, GLfloat upperBorder) {
	return lowerBorder + static_cast <GLfloat> (rand()) / (static_cast <GLfloat> (RAND_MAX / (upperBorder - lowerBorder)));
}

GLfloat getRandomHeight(GLint count, GLint range, GLfloat avg, BiomeParams biomeParams) {
	GLfloat biomeBorder_max = biomeParams.maxAltitude * count - avg;
	GLfloat biomeBorder_min = biomeParams.minAltitude * count - avg;

	GLfloat randomBorder_max = GLfloat(range) / biomeParams.scale;
	GLfloat randomBorder_min = -randomBorder_max;

	GLfloat lowerBorder = max(biomeBorder_min, randomBorder_min);
	GLfloat upperBorder = min(biomeBorder_max, randomBorder_max);

	return (avg + getRandomNumber(lowerBorder, upperBorder)) / count;
}

GLfloat distance(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2) {
	return sqrtf(powf((x1 - x2), 2.0f) + powf((y1 - y2), 2.0f) + powf((z1 - z2), 2.0f));
}


void diamondSquareAlgorithm_SquareStep(std::vector <std::vector <GLfloat>> *points,
									   GLint x, GLint y, GLint reach, BiomeParams biomeParams) {
	GLuint count = 0;
	GLfloat avg = 0.0f;

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

	GLuint count = 0;
	GLfloat avg = 0.0f;

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
	GLuint half = size / 2;
	if (half < 1) return;

	// Square steps
	for (GLuint y = half; y < polygonNumberPerSide; y += size) {
		for (GLuint x = half; x < polygonNumberPerSide; x += size) {
			diamondSquareAlgorithm_SquareStep(points, x % polygonNumberPerSide, y % polygonNumberPerSide, half, biomeParams);
		}
	}

	// Diamond steps
	GLuint column = 0;
	for (GLuint y = 0; y < polygonNumberPerSide; y += half) {
		column++;
		// If this is an odd column.
		if (column % 2 == 1) {
			for (GLuint x = half; x < polygonNumberPerSide; x += size) {
				diamondSquareAlgorithm_DiamondStep(points, y % polygonNumberPerSide, x % polygonNumberPerSide, half,
									biomeParams, rightSide_EndCoords, upperSide_EndCoords);
			}
		} else {
			for (GLuint x = 0; x < polygonNumberPerSide; x += size) {
				diamondSquareAlgorithm_DiamondStep(points, y % polygonNumberPerSide, x % polygonNumberPerSide, half,
									biomeParams, rightSide_EndCoords, upperSide_EndCoords);
			}
		}
	}
	callDiamondSquareAlgorithm(points, size / 2, biomeParams, rightSide_EndCoords, upperSide_EndCoords);
}


struct between {
	between(GLfloat upperBorder, GLfloat lowerBorder = -100.0f) :
		_upperBorder(upperBorder), _lowerBorder(lowerBorder) { }

	GLboolean operator()(TriangularSquare_withNormals val) {
		return val.getCenterCoord_Z() > _lowerBorder && val.getCenterCoord_Z() <= _upperBorder;
	}

	GLfloat _lowerBorder, _upperBorder;
};

bool comp(TriangularSquare_withNormals triangularSquare_withNormals_1, TriangularSquare_withNormals triangularSquare_withNormals_2) {
	return triangularSquare_withNormals_1.center[2] < triangularSquare_withNormals_2.center[2];
}

void setParamsforPolygon(Chunk *chunk, GLint firstId, GLint paramId) {
	chunk->heightIds.push_back(firstId);
	chunk->colors.push_back(colors[paramId]);
	chunk->textureIds.push_back(textures[paramId]);
	chunk->materialIds.push_back(materials[paramId]);
}

void calculateBiome(Biome *biome) {
	std::lock_guard<std::mutex> lock(cout_guard);
	srand(time(NULL));

	GLboolean isFirstBiome = biome->biome_EndCoords.empty();
	GLfloat chunkSize = polygonNumberPerSide * polygonSize;
	GLfloat chunkOffset_Y = -chunkSize / 2;
	std::vector <std::vector<GLfloat>> newBiome_EndCoords;

	for (GLushort index = 0; index < visibleBiomeHeight; index++) {
		GLfloat chunkOffset_X = -(visibleBiomeWidth / 2.0) * (chunkSize - polygonSize);

		for (GLushort jndex = 0; jndex < visibleBiomeWidth; jndex++) {
			std::vector <std::vector <GLfloat>> points;
			for (GLuint i = 0; i < polygonNumberPerSide; i++) {
				std::vector <GLfloat> points_X(polygonNumberPerSide);
				points.push_back(points_X);
			}

			// Ñâÿçûâàíèå òåêóùåãî ÷àíêà è áèîìà ñ ïðåäûäóùèìè
			//============================================================================================
			std::vector <GLfloat> prevBiome_upperSide_EndCoords =
				isFirstBiome ? std::vector <GLfloat>() : biome->biome_EndCoords[jndex];
			std::vector <GLfloat> prevChunk_rightSide_EndCoords;
			std::vector <GLfloat> prevChunk_upperSide_EndCoords;

			if (jndex - 1 >= 0) prevChunk_rightSide_EndCoords =
				biome->chunks[index * visibleBiomeWidth + jndex - 1].rightSide_EndCoords;
			if (index - 1 >= 0) prevChunk_upperSide_EndCoords =
				biome->chunks[(index - 1) * visibleBiomeWidth + jndex].upperSide_EndCoords;

			if (!isFirstBiome && index == 0) {
				callDiamondSquareAlgorithm(&points, polygonNumberPerSide, biome->biomeParams,
								prevChunk_rightSide_EndCoords, prevBiome_upperSide_EndCoords);
			} else {
				callDiamondSquareAlgorithm(&points, polygonNumberPerSide, biome->biomeParams,
								prevChunk_rightSide_EndCoords, prevChunk_upperSide_EndCoords);
			}
			//============================================================================================

			// Ñîõðàíåíèå âûñîò ïðàâîé è âåðõíåé ãðàíèöû ÷àíêà
			//============================================================================================
			std::vector <GLfloat> rightSide_EndCoords;
			std::vector <GLfloat> upperSide_EndCoords;

			for (GLuint i = 0; i < polygonNumberPerSide; i++) {
				rightSide_EndCoords.push_back(points[polygonNumberPerSide - 1][i]);
				upperSide_EndCoords.push_back(points[i][polygonNumberPerSide - 1]);
			}
			//============================================================================================

			// Ñîçäàíèå ïîëèãîíîâ ÷àíêà
			//============================================================================================
			std::vector <TriangularSquare_withNormals> chunkPolygons;
			for (GLuint i = 0; i < polygonNumberPerSide - 1; i++) {
				for (GLuint j = 0; j < polygonNumberPerSide - 1; j++) {
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
			for (GLuint i = 0; i < (GLuint) chunk.chunkPolygons.size(); i++) {
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
				GLushort textureId = 1;
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
			if (index == visibleBiomeHeight - 1) newBiome_EndCoords.push_back(chunk.upperSide_EndCoords);
		}
		chunkOffset_Y += chunkSize - polygonSize;
	}
	biome->biome_EndCoords = newBiome_EndCoords;
}

void generateLists(GLuint *biomeLists, Biome *biome) {
	for (int index = 0; index < visibleBiomeHeight; index++) {
		for (int jndex = 0; jndex < visibleBiomeWidth; jndex++) {
			glNewList(biomeLists[index * visibleBiomeWidth + jndex], GL_COMPILE);
			Chunk currentChunk = biome->chunks[index * visibleBiomeWidth + jndex];

			int k = 0;
			for (int i = 0; i < (int) currentChunk.chunkPolygons.size(); i++) {
				if (k < currentChunk.heightIds.size() && currentChunk.heightIds[k] == i) {
					if (k > 0) glEnd();
					glColor3fv(&currentChunk.colors[k][0]);
					glBindTexture(GL_TEXTURE_2D, currentChunk.textureIds[k]);

					Material currentMaterial = currentChunk.materialIds[k];
					GLfloat ambient[4] = { currentMaterial.ambient, currentMaterial.ambient, currentMaterial.ambient, 1.0f };
					GLfloat diffuse[4] = { currentMaterial.diffuse,currentMaterial.diffuse, currentMaterial.diffuse, 1.0f };
					GLfloat specular[4] = { currentMaterial.specular, currentMaterial.specular, currentMaterial.specular, 1.0f };

					glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
					glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
					glMaterialf(GL_FRONT, GL_SHININESS, currentChunk.materialIds[k].shininess * 128.0f);
					glBegin(GL_TRIANGLES);
					k++;
				}
				TriangularSquare_withNormals currentPolygon = currentChunk.chunkPolygons[i];

				glNormal3fv(currentPolygon.lowerTriangleNormal);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3fv(currentPolygon.lowerLeft);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3fv(currentPolygon.lowerRight);
				glTexCoord2f(0.5f, 0.5f);
				glVertex3fv(currentPolygon.center);

				glNormal3fv(currentPolygon.rightTriangleNormal);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3fv(currentPolygon.lowerRight);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3fv(currentPolygon.upperRight);
				glTexCoord2f(0.5f, 0.5f);
				glVertex3fv(currentPolygon.center);

				glNormal3fv(currentPolygon.upperTriangleNormal);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3fv(currentPolygon.upperRight);
				glTexCoord2f(1.0f, 0.0f);
				glVertex3fv(currentPolygon.upperLeft);
				glTexCoord2f(0.5f, 0.5f);
				glVertex3fv(currentPolygon.center);

				glNormal3fv(currentPolygon.leftTriangleNormal);
				glTexCoord2f(1.0f, 0.0f);
				glVertex3fv(currentPolygon.upperLeft);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3fv(currentPolygon.lowerLeft);
				glTexCoord2f(0.5f, 0.5f);
				glVertex3fv(currentPolygon.center);
			}
			glEnd();
			glEndList();
		}
	}
	biome->chunks.clear();
}

void startThread(Biome *biome, Biome *nextBiome) {
	calculateBiome(biome);
	nextBiome->biome_EndCoords = biome->biome_EndCoords;
	biome->biome_EndCoords.clear();
	thread_done = true;
}

void createWaterLists() {
	GLfloat polygonNumberPerSide_water = polygonNumberPerSide / 16.0f;
	GLfloat polygonSize_water = polygonSize * 16.0f;

	GLfloat chunkSize = polygonNumberPerSide * polygonSize;
	GLfloat chunkOffset_Y = -chunkSize / 2.0f;

	GLfloat ambient[4] = { materials[0].ambient, materials[0].ambient, materials[0].ambient, 1.0f };
	GLfloat diffuse[4] = { materials[0].diffuse, materials[0].diffuse, materials[0].diffuse, 1.0f };
	GLfloat specular[4] = { materials[0].specular, materials[0].specular, materials[0].specular, 1.0f };

	for (GLushort i = 0; i < visibleBiomeHeight; i++) {
		GLfloat chunkOffset_X = -(visibleBiomeWidth / 2.0f) * (chunkSize - polygonSize);

		for (GLushort j = 0; j < visibleBiomeWidth; j++) {
			glNewList(waterLevel_lists[i * visibleBiomeWidth + j], GL_COMPILE);
			glColor3fv(&colors[0][0]);
			glNormal3f(0.0f, 0.0f, 1.0f);
			glBindTexture(GL_TEXTURE_2D, textures[0]);

			glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT, GL_SHININESS, materials[0].shininess * 128.0f);

			glBegin(GL_QUADS);
			for (GLushort polygonNumber_Y = 0; polygonNumber_Y < polygonNumberPerSide_water; polygonNumber_Y++) {
				for (GLushort polygonNumber_X = 0; polygonNumber_X < polygonNumberPerSide_water; polygonNumber_X++) {
					GLfloat resultOffset_Y = chunkOffset_Y + polygonNumber_Y * polygonSize_water;
					GLfloat resultOffset_X = chunkOffset_X + polygonNumber_X * polygonSize_water;

					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(resultOffset_X, resultOffset_Y, waterLevel);
					glTexCoord2f(0.0f, 1.0f);
					glVertex3f(resultOffset_X, resultOffset_Y + polygonSize_water, waterLevel);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f(resultOffset_X + polygonSize_water, resultOffset_Y + polygonSize_water, waterLevel);
					glTexCoord2f(1.0f, 0.0f);
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

void createTorusList() {
	glNewList(torusList, GL_COMPILE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	GLushort partsNumber = 30;
	GLfloat firstPoint[3], secondPoint[3], generalPoint[3];
	for (int i = 0; i < partsNumber; i++) {
		glColor3d(1.0, 1.0, 0.0);
		glBegin(GL_QUAD_STRIP);
		for (GLushort j = 0; j <= partsNumber; j++) {
			for (GLshort k = 1; k >= 0; k--) {
				GLfloat s = (i + k) % partsNumber + 0.5f;
				GLfloat t = j % partsNumber;

				GLfloat x = (1.0f + 0.1f * cosf(s * 2.0f * M_PI / partsNumber)) * cosf(t * 2.0f * M_PI / partsNumber);
				GLfloat y = (1.0f + 0.1f * cosf(s * 2.0f * M_PI / partsNumber)) * sinf(t * 2.0f * M_PI / partsNumber);
				GLfloat z = 0.1f * sinf(s * 2.0f * M_PI / partsNumber);

				glVertex3f(x, y, z);
			}
		}
		glEnd();
	}

	lightMode ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
	textureMode ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
	glEndList();
}

void drawAirCraft() {
	glCullFace(GL_BACK);

	glPushMatrix();
	glTranslatef(airCraftTranslation[0], airCraftTranslation[1], airCraftTranslation[2]);
	GLfloat multiplier_X = manualControl ? 45.0f : 180.0f;
	GLfloat multiplier_Y = manualControl ? 15.0f : 60.0f;
	if (!manualControl) {
		multiplier_X *= (0.005f / animationAirCraftSpeed) / multiplier;
		multiplier_Y *= (0.005f / animationAirCraftSpeed) / multiplier;
	}

	glRotatef((airCraftTranslation[1] - prevAirCraftTranslation[1]) * multiplier_Y, 1.0f, 0.0f, 0.0f);
	glRotatef((airCraftTranslation[0] - prevAirCraftTranslation[0]) * multiplier_X, 0.0f, 1.0f, 0.0f);
	glScalef(0.5f, 0.5f, 0.5f);

	GLfloat ambient[4] = { 0.19225, 0.19225, 0.19225, 1.0 };
	GLfloat diffuse[4] = { 0.50754, 0.50754, 0.50754, 1.0 };
	GLfloat specular[4] = { 0.508273, 0.508273, 0.508273, 1.0 };
	GLfloat shininess = 0.4 * 128.0;

	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);

	airCraftTexture.bindTexture();
	airCraftModel.DrawObj();
	glPopMatrix();
}


void myInitRender() {
	srand(time(NULL));
	glEnable(GL_CULL_FACE);
	FPS_timerStart = clock();
	if (initializeSound()) return;
	initializeTextures();
	loadModel("models\\airplane.obj", &airCraftModel);
	airCraftTexture.loadTextureFromFile("textures\\airplane.bmp");
	loadModel("models\\moon.obj", &moonModel);
	moonTexture.loadTextureFromFile("textures\\moon.bmp");

	if (FPS_MAX >= 0 && FPS_MAX <= 200) {
		if (FPS_MIN > FPS_MAX && FPS_MAX > 1) {
			FPS_MIN = (GLushort) ceil(GLfloat(FPS_MAX) / 2.0f);
		} else FPS_MIN = 60;
	}

	if (quickMultiplier > 1) {
		polygonNumberPerSide = (GLint) powf(2.0f, quickMultiplier);
		polygonSize = 128.0f / GLfloat(polygonNumberPerSide);
		polygonNumberInChunk = (GLint) powf(polygonNumberPerSide, 2.0f);

		for (GLuint i = 0; i < (GLuint) biomeParams.size(); i++) {
			biomeParams[i].scale = powf(2.0f, biomeParams[i].scale + quickMultiplier - 8);
		}
	} else {
		for (GLuint i = 0; i < (GLuint) biomeParams.size(); i++) {
			biomeParams[i].scale = powf(2.0f, biomeParams[i].scale);
		}
	}

	torusList = glGenLists(1);
	waterLevel_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	biome_0_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	biome_1_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);
	biome_2_lists[0] = glGenLists(visibleBiomeWidth * visibleBiomeHeight);

	for (GLushort i = 1; i < visibleBiomeWidth * visibleBiomeHeight; i++) {
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
	generateLists(biome_0_lists, &biome_0);
	generateLists(biome_1_lists, &biome_1);
	generateLists(biome_2_lists, &biome_2);

	if (gameStarted) createTorusList();
}

void myRender() {
	FPS_counter++;
	glCullFace(GL_FRONT);

	if (FPS_MAX > 0 && FPS_MAX < 200) {
		if (viewMode == 2) {
			GLuint threadSleepTime = GLuint((1.0f / GLfloat(FPS_MAX) - 1.0f / 200.0f) * powf(10.0f, 6.0f));
			std::this_thread::sleep_for(std::chrono::microseconds(threadSleepTime));
		}
	}
	if (viewMode != 2) {
		GLuint threadSleepTime = GLuint((1.0f / GLfloat(FPS_MIN) - 1.0f / 200.0f) * powf(10.0f, 6.0f));
		std::this_thread::sleep_for(std::chrono::microseconds(threadSleepTime));
	}

	if (!BASS_ChannelIsActive(streamMusic) && !isMusicStoped) BASS_ChannelPlay(streamMusic, TRUE);

	GLfloat fogColor[4] = { sunColor[0] / 255.0f, sunColor[1] / 255.0f, sunColor[2] / 255.0f, 1.0f };
	glFogfv(GL_FOG_COLOR, fogColor);

	animationController();
	lightController();
	cameraController();
	if (isReadyToControl) {
		if (!manualControl) {
			airCraftAutoFlyController();
		} else airCraftAutoManualController();
	}
	if (isControlSwitched) airCraftControlSwitchController();

	glCallLists(visibleBiomeWidth * visibleBiomeHeight, GL_UNSIGNED_INT, waterLevel_lists);

	if (animationTranslationMoment == 0.0f && start_new) {
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
				generateLists(biome_2_lists, &biome_2);
				break;
			case 1:
				generateLists(biome_0_lists, &biome_0);
				break;
			case 2:
				generateLists(biome_1_lists, &biome_1);
				break;
		}
		start_new = true;
	}

	translationController();

	if (gameStarted) {
	// Ïðè ïîïàäàíèè â êîëüöî - îíî óäàëÿåòñÿ èç âåêòîðà, ñîîòâåò. íåîáõîäèìî ãåíåðèðîâàòü íîâîå
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

				// Ïðîâåðêà íà äèñòàíöèþ, åñëè ðÿäîì ñ ñãåíåðèðóåìîé êîîðäèíàòîé â äèñòàíöèè èìååòñÿ îáúåêò - èãíîðèì äîáàâëåíèå 
				bool distancePassed = true;
				for (int i = 0; i < rings.size() && distancePassed; i++) {
					Ring *ring = rings[i];
					if (ring != NULL) {
						if (distance(ring->x, ring->y, ring->z, x, y, z) < (ring->size + size) * 5) {
							distancePassed = false;
						}
					}
				}

				// Òàêæå ïðîâåðèì ÷òîáû ñïàâí íå ïðîèñõîäèë âáëèçè/çà ñàìîëåòîì
				float translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;
				float yPosReal = y - animationTranslationMoment * speed;
				if (abs(airCraftTranslation[1] - yPosReal) < 100) distancePassed = false;


				if (distancePassed) {
					Ring *r = new Ring({ speed, size, x, y, z, (DWORD64) -1 });
					rings.push_back(r);
				}
			}
		}

		// îòðèñîâêà êîëåö
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
		// ïðîâåðêà ïîçèöèé êîëåö
		for (int i = 0; i < rings.size(); i++) {
			Ring *ring = rings[i];
			if (ring != NULL) {
				float translationSize = polygonNumberPerSide * polygonSize * visibleBiomeHeight - 2.0 * polygonSize;
				float yPosReal = ring->y - animationTranslationMoment * ring->speed;
				if (distance(ring->x, yPosReal, ring->z, airCraftTranslation[0], airCraftTranslation[1], airCraftTranslation[2]) < ring->size + 4) {
					// äèñòàíöèÿ î÷åíü áëèçêàÿ ê ñàìîëåòó
					// íàçíà÷èì êîë-âî ìèëèññèêóíä ïîñëå êîòîðîãî êîëüöî ïðîïàäåò åñëè ýòî åùå íå ñäåëàíî
					if (ring->deadMSeconds == -1) {
						ring->deadMSeconds = currentMseconds + 1000; // ÷åðåç 1 ñåê ïîñëå ïîïàäàíèÿ - êîëüöî ïðîïàäåò
						BASS_ChannelPlay(streamHit, TRUE);
						torusScore++;
					}
				}
				if (ring->deadMSeconds <= currentMseconds) rings.erase(rings.begin() + i);
				// Òóò íóæíà ïðîâåðêà - åñëè êîëüöî óëåòåëî äàëåêî - åãî íàäî óäàëÿòü èç âåêòîðà, ÷òîáû òîò ïîïîëíÿëñÿ íîâûìè êîëüöàìè
			}
		}
	}

	if (viewMode != 0) drawAirCraft();
}


void mouseEvent(OpenGL *ogl, GLint mouseCoord_X, GLint mouseCoord_Y) {
	if (operatorMode) {
		GLint dx = previousMouseCoord_X - mouseCoord_X;
		GLint dy = previousMouseCoord_Y - mouseCoord_Y;
		previousMouseCoord_X = mouseCoord_X;
		previousMouseCoord_Y = mouseCoord_Y;

		// Ìåíÿåì óãëû êàìåðû ïðè íàæàòîé ïðàâîé êíîïêå ìûøè
		if (OpenGL::isKeyPressed(VK_RBUTTON)) {
			myCamera.cameraAngle_XY += 0.01f * dx;
			myCamera.cameraAngle_Z += -0.01f * dy;
		}
	}

	if (viewMode == 2 && manualControl && isReadyToControl && !OpenGL::isKeyPressed(VK_LBUTTON)) {
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = myCamera.getLookRay(POINT->x, POINT->y);
		GLfloat z = airCraftTranslation[2];

		GLfloat k = 0;
		if (r.direction.Z() == 0) {
			k = 0;
		} else k = (z - r.origin.Z()) / r.direction.Z();

		GLfloat x = k * r.direction.X() + r.origin.X();
		GLfloat y = k * r.direction.Y() + r.origin.Y();

		if (y < 0.0f && y > -30.0f) {
			pointToFly[0] = x;
			pointToFly[1] = y;
		}
	}
}

void mouseWheelEvent(OpenGL *ogl, GLint delta) {
	if (operatorMode) {
		if (delta < 0 && myCamera.cameraDistance <= 1.0f) return;
		if (delta > 0 && myCamera.cameraDistance >= 100.0f) return;

		myCamera.cameraDistance += 0.01f * delta;
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
			myCamera.cameraDistance = maxHeight + 20.0f;
			myCamera.cameraAngle_XY = M_PI / 180.0f * -90.0f;
			myCamera.cameraAngle_Z = M_PI / 180.0f * 45.0f;
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
			savedAnimationStatus = animationAirCraftMoment;
			isReadyToControl = false;
			isControlSwitched = true;
		}
	}

	if (key == 'M') {
		isMusicStoped = !isMusicStoped;
		if (isMusicStoped) BASS_ChannelStop(streamMusic);
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
			myCamera.cameraDistance = maxHeight + 20.0f;
			myCamera.cameraAngle_XY = M_PI / 180.0f * -90.0f;
			myCamera.cameraAngle_Z = M_PI / 180.0f * 45.0f;
			animationLightMoment = 0.0f;
		}
	}
}

void keyUpEvent(OpenGL *ogl, GLint key) {
}


void initRender(OpenGL *ogl) {
	myInitRender();

	// êàìåðó è ñâåò ïðèâÿçûâàåì ê "äâèæêó"
	ogl->mainCamera = &myCamera;
	ogl->mainLight = &myLight;

	glEnable(GL_NORMALIZE);						// íîðìàëèçàöèÿ íîðìàëåé
	glEnable(GL_LINE_SMOOTH);					// óñòðàíåíèå ñòóïåí÷àòîñòè äëÿ ëèíèé
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 0.35f);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
}

void drawMessage(OpenGL *ogl) {
	glCullFace(GL_FRONT);

	// Ñîîáùåíèå ââåðõó ýêðàíà
	glMatrixMode(GL_PROJECTION);	// Äåëàåì àêòèâíîé ìàòðèöó ïðîåêöèé. 
					// (âñåê ìàòðè÷íûå îïåðàöèè, áóäóò åå âèäîèçìåíÿòü.)
	glPushMatrix();			// ñîõðàíÿåì òåêóùóþ ìàòðèöó ïðîåöèðîâàíèÿ (êîòîðàÿ îïèñûâàåò ïåðñïåêòèâíóþ ïðîåêöèþ) â ñòåê 				    
	glLoadIdentity();		// Çàãðóæàåì åäèíè÷íóþ ìàòðèöó
	glOrtho(0.0f, ogl->getWidth(), 0.0f, ogl->getHeight(), 0.0f, 1.0f);	// âðóáàåì ðåæèì îðòîãîíàëüíîé ïðîåêöèè

	glMatrixMode(GL_MODELVIEW);	// ïåðåêëþ÷àåìñÿ íà ìîäåë-âüþ ìàòðèöó
	glPushMatrix();			// ñîõðàíÿåì òåêóùóþ ìàòðèöó â ñòåê (ïîëîæåíèå êàìåðû, ôàêòè÷åñêè)
	glLoadIdentity();		// ñáðàñûâàåì åå â äåôîëò

	glDisable(GL_LIGHTING);

	GuiTextRectangle rec;
	rec.setSize(500, 1000);
	rec.setPosition(10, ogl->getHeight() - 1000 - 10);

	std::stringstream ss;

	GLfloat angle = 360.0f * animationLightMoment / (polygonSize * polygonNumberPerSide * visibleBiomeWidth);
	GLushort hours = (int) round(angle) / 15 + 6;
	GLushort minutes = (angle - (hours - 6.0f) * 15.0f) * 4 + 2;
	if (hours >= 24) hours = hours - 24;
	ss << "Time: " << hours << ":" << minutes << std::endl;

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
	GLushort biomeId = 0;
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
			ss << "Plain (Grass)" << std::endl;
			break;
		case 2:
			ss << "Plain (Snow)" << std::endl;
			break;
		case 3:
			ss << "Plain (Sand)" << std::endl;
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

	std::string manualStatus = manualControl ? " [âêë]" : " [âûêë]";
	std::string musicStatus = isMusicStoped ? " [âûêëþ÷åíà]" : " [âêëþ÷åíà]";
	ss << "Êîëåö ñîáðàíî: " << torusScore << std::endl;
	ss << "M - âêë/âûêë ìóçûêó" << musicStatus << std::endl;
	ss << "A - âêë/âûêë ðåæèìà ïèëîòà" << manualStatus << std::endl << std::endl;
	ss << "1 - âèä îò êàáèíû ïèëîòà" << std::endl;
	ss << "2 - âèä çà ñàìîëåòîì" << std::endl;
	ss << "3 - çàêðåïëåííûé âèä" << std::endl << std::endl;

	if (operatorMode) {
		std::string textureStatus = textureMode ? " [âêë]" : " [âûêë]";
		std::string fogStatus = fogMode ? " [âêë]" : " [âûêë]";
		std::string lightStatus = lightMode ? " [âêë]" : " [âûêë]";
		std::string lightLockStatus = lightLockMode ? " [âêë]" : " [âûêë]";

		ss << "ÐÅÆÈÌ ÎÏÅÐÀÒÎÐÀ" << std::endl << std::endl;
		ss << "T - âêë/âûêë òåêñòóð" << textureStatus << std::endl;
		ss << "F - âêë/âûêë òóìàí" << fogStatus << std::endl;
		ss << "L - âêë/âûêë îñâåùåíèå" << lightStatus << std::endl;
		ss << "J - áëîêèðîâêà ñìåíû ñóòîê" << lightLockStatus << std::endl << std::endl;
		
		ss << "animationTranslationMoment: " << animationTranslationMoment << std::endl;
		ss << "animationLightMoment: " << animationLightMoment << std::endl;
		ss << "animationAirCraftMoment: " << animationAirCraftMoment << std::endl;
		ss << "angle: " << angle << std::endl;
		ss << "sunColor: (" << sunColor[0] << ", " << sunColor[1] << ", " << sunColor[2] << ")" << std::endl;

		if ((clock() - FPS_timerStart) / CLOCKS_PER_SEC >= 1) {
			FPS_string = FPS_counter;
			FPS_counter = 0;
			FPS_timerStart = clock();
		}
		
		ss << "FPS_MIN: " << FPS_MIN << std::endl;
		ss << "FPS_MAX: " << FPS_MAX << std::endl;
		ss << "FPS: " << FPS_string << std::endl;
		ss << "Tick difference: " << currentTick - previousTick << std::endl;
		ss << "Multiplier: " << multiplier << std::endl;
		ss << "start_new: " << start_new << std::endl;
		ss << "thread_done: " << thread_done << std::endl << std::endl;

		ss << "AirCraft Pos.: (" << airCraftTranslation[0] << ", " << airCraftTranslation[1] << ", " << airCraftTranslation[2] << ")" << std::endl;
		ss << "PointToFly: (" << pointToFly[0] << ", " << pointToFly[1] << ")" << std::endl;
		ss << "Light Pos.: (" << myLight.position.X() << ", " << myLight.position.Y() << ", " << myLight.position.Z() << ")" << std::endl;
		ss << "Camera Pos.: (" << myCamera.position.X() << ", " << myCamera.position.Y() << ", " << myCamera.position.Z() << ")" << std::endl;
		ss << "Cam Look: (" << myCamera.lookPoint.X() << ", " << myCamera.lookPoint.Y() << ", " << myCamera.lookPoint.Z() << ")" << std::endl;
		ss << "Cam Params: R=" << myCamera.cameraDistance << ", fi1=" << myCamera.cameraAngle_XY << ", fi2=" << myCamera.cameraAngle_Z << std::endl;
	}

	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION); // âîññòàíàâëèâàåì ìàòðèöû ïðîåêöèè è ìîäåë-âüþ îáðàòüíî èç ñòåêà.
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void Render(OpenGL *ogl) {
	glEnable(GL_DEPTH_TEST);

	fogMode ? glEnable(GL_FOG) : glDisable(GL_FOG);
	lightMode ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
	textureMode ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);

	// ×òîá áûëî êðàñèâî, áåç êâàäðàòèêîâ (ñãëàæèâàíèå îñâåùåíèÿ)
	glShadeModel(GL_SMOOTH);
	//===================================
	myRender();
	//===================================
	drawMessage(ogl);
}
