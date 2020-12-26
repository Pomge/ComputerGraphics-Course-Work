#ifndef CAMERA_H
#define CAMERA_H

#include "Object.h"
#include "MyVector3d.h"
#include "Ray.h"

// ��������� ��� ������� � ������
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

	// ������ ������ �� ����� �� ����������� � ���� � ����� �����.
	static Ray getLookRay(int wndX, int wndY) {
		GLint viewport[4];			// ��������� viewport-a.
		GLdouble projection[16];	// ������� ��������.
		GLdouble modelview[16];		// ������� �������.
		// GLdouble vx, vy, vz;		// ���������� ������� ���� � ������� ��������� viewport-a.
		GLdouble wx, wy, wz;		// ������������ ������� ����������.

		glGetIntegerv(GL_VIEWPORT, viewport);			// ����� ��������� viewport-a.
		glGetDoublev(GL_PROJECTION_MATRIX, projection);	// ����� ������� ��������.
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);	// ����� ������� �������.
		// ��������� ������� ���������� ������� � ������� ��������� viewport-a.

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