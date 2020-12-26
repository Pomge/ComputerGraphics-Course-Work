#ifndef LIGHT_H
#define LIGHT_H

#include "Object.h"

class Light : public Object {
public:
	virtual void DrawLightSource() = 0;
	virtual void SetUpLight() = 0;
};

#endif