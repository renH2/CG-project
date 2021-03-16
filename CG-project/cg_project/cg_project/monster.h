#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>
class Monster {
public:
	glm::vec3 position;
	glm::vec3 volume;
	glm::vec3 front;
	bool isDead = false;
	int HP;
	float zoomRate = 1.0f;
	float rotateAngle = 0.0f;
	unsigned int type;
	Monster();
	Monster(unsigned int type, float zoom_rate, glm::vec3 volume, glm::vec3 position, int HP);
	~Monster();
	void UpdateHP(int damage);
	void UpdateAngle(glm::vec3 targetPosition);
private:
	void dead();
};
