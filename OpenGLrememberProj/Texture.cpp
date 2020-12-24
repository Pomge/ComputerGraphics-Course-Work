#include "Texture.h"
#define GL_GLEXT_PROTOTYPES
#include "GL\glext.h"
#include <GL\GLU.h>

Texture::~Texture() {
	deleteTexture();
}

void Texture::loadTextureFromFile(const char *filename) {
	RGBTRIPLE *texarray;
	char *texCharArray;
	int texW, texH;
	Texture::LoadBMP(filename, &texW, &texH, &texarray);
	Texture::RGBtoChar(texarray, texW, texH, &texCharArray);

	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

	free(texCharArray);
	free(texarray);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
	//glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture::deleteTexture() {
	glDeleteTextures(1, &texId);
}

void Texture::bindTexture() {
	glBindTexture(GL_TEXTURE_2D, texId);
}

int Texture::LoadBMP(__in LPCSTR  filename, __out int *Wigth, __out int *Height, __out RGBTRIPLE **arr) {
	DWORD nBytesRead = 0;
	int read_size = 0;
	int i = 0;
	int width, height, size;
	BITMAPINFOHEADER infoh;
	BITMAPFILEHEADER fileh;

	HANDLE file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	//Считываем заголовки BMP файла
	ReadFile((HANDLE) file, &fileh, sizeof(BITMAPFILEHEADER), &nBytesRead, 0);
	ReadFile((HANDLE) file, &infoh, sizeof(BITMAPINFOHEADER), &nBytesRead, 0);

	/*
	Подробнее о заголовках https://msdn.microsoft.com/ru-ru/library/windows/desktop/dd183374(v=vs.85).aspx
	https://msdn.microsoft.com/ru-ru/library/windows/desktop/dd183376(v=vs.85).aspx

	*/

	width = infoh.biWidth;
	height = infoh.biHeight;
	*Wigth = width;
	*Height = height;

	size = width * 3 + width % 4;
	size = size * height;
	nBytesRead = fileh.bfOffBits;
	*arr = (RGBTRIPLE *) malloc(size);

	while (read_size < size) {
		ReadFile(file, *arr + i, sizeof(RGBTRIPLE), &nBytesRead, 0);
		read_size += nBytesRead;
		i++;
	}
	CloseHandle(file);

	return 1;
}

//Переводим BMP в массив чароов.  Один пиксаль кодируется последовательно 4мя байтами (R G B A)
//Так же тут картинка переворачивается, в BMP она хранится перевернутой.
int Texture::RGBtoChar(__in RGBTRIPLE *arr, __in int width, __in int height, __out char **out) {
	int size = height * width * 4;
	char *mas;
	if (width <= 0 || height <= 0) {
		return 0;
	}

	mas = (char *) malloc(size * sizeof(char));
	for (int i = height - 1; i >= 0; i--)
		for (int j = 0; j < width; j++) {
			*(mas + i * width * 4 + j * 4 + 0) = arr[(i) *width + j].rgbtRed;
			*(mas + i * width * 4 + j * 4 + 1) = arr[(i) *width + j].rgbtGreen;
			*(mas + i * width * 4 + j * 4 + 2) = arr[(i) *width + j].rgbtBlue;
			*(mas + i * width * 4 + j * 4 + 3) = 255;
		}
	*out = mas;
	return 1;
}