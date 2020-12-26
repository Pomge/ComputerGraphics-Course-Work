#ifndef CAMERA_H
#define CAMERA_H

#include "Object.h"
#include "MyVector3d.h"
#include "Ray.h"

// интерфейс для доступа к камере
class Camera : public Object {
public:
	GLfloat cameraDistance;	// Camera distance
	GLfloat cameraAngle_XY;	// Camera rotation angle XY
	GLfloat cameraAngle_Z;	// Camera rotation angle Z
	Vector3 lookPoint;		// Camera look point
	Vector3 normal;

	Camera() {
		cameraDistance = 0.0f;	// Set default
		cameraAngle_XY = 0.0f;	// Set default
		cameraAngle_Z = 0.0f;	// Set default
	}

	void LookAt() {
		gluLookAt(position.X(), position.Y(), position.Z(),
				  lookPoint.X(), lookPoint.Y(), lookPoint.Z(),
				  normal.X(), normal.Y(), normal.Z());
	}

	virtual void SetUpCamera() = 0;

	// узнать вектор из точки ко координатам в окне в глубь сцены.
	static Ray getLookRay(int wndX, int wndY) {
		GLint viewport[4];			// параметры viewport-a.
		GLdouble projection[16];	// матрица проекции.
		GLdouble modelview[16];		// видовая матрица.
		// GLdouble vx, vy, vz;		// координаты курсора мыши в системе координат viewport-a.
		GLdouble wx, wy, wz;		// возвращаемые мировые координаты.

		glGetIntegerv(GL_VIEWPORT, viewport);			// узнаём параметры viewport-a.
		glGetDoublev(GL_PROJECTION_MATRIX, projection);	// узнаём матрицу проекции.
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);	// узнаём видовую матрицу.
		// переводим оконные координаты курсора в систему координат viewport-a.

		gluUnProject(wndX, wndY, 0.0, modelview, projection, viewport, &wx, &wy, &wz);
		Vector3 o(wx, wy, wz);
		gluUnProject(wndX, wndY, 1.0, modelview, projection, viewport, &wx, &wy, &wz);
		Vector3 p(wx, wy, wz);
		Ray ray;
		ray.origin = o;
		ray.direction = (p - o).normolize();
		return ray;
	}
};

#endif