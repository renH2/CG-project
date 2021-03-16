#include "monster.h"
#define PI 3.1415926
Monster::Monster()
{
	this->type = 1;
	this->zoomRate = 1.0f;
	this->volume = { 1.0f, 1.0f, 1.0f };
	this->position = { -1.0f, -1.0f, -1.0f };
	this->HP = 100;
}

Monster::Monster(unsigned int type, float zoom_rate, glm::vec3 volume, glm::vec3 position, int HP = 100)
{
	this->type = type;
	this->zoomRate = zoom_rate;
	this->volume = volume;
	this->position = position;
	this->HP = HP;
}

Monster::~Monster(){}

void Monster::UpdateHP(int damage)
{
	this->HP -= damage;
	if (this->HP <= 0)
		dead();
}

void Monster::UpdateAngle(glm::vec3 targetPosition)
{
	glm::vec3 frontin3D = (targetPosition - this->position);
	this->front = glm::normalize(frontin3D);
	glm::vec2 frontin2D = glm::normalize(glm::vec2(frontin3D.x, frontin3D.z));
	if(frontin2D.x >= 0)
		this->rotateAngle = acos(frontin2D.y);
	else
		this->rotateAngle =  -acos(frontin2D.y);
}

void Monster::dead()
{
	this->zoomRate = 0.0f;
}