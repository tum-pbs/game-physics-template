#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "CollisionDetection.h"

class Rigidbody
{
public:
	static CollisionInfo collide(Rigidbody &body1, Rigidbody &body2);

	Rigidbody();
	Rigidbody(glm::vec3 &position = glm::vec3(0), glm::vec3 &scale = glm::vec3(1), float mass = 1);
	Rigidbody(glm::vec3 &position, glm::quat &rotation, glm::vec3 &scale, float mass);

	void addLocalForce(glm::vec3 &force, glm::vec3 &where);
	void addWorldForce(glm::vec3 &force, glm::vec3 &where);
	void update(float deltaTime);
	glm::mat4 getWorldFromObj();
	glm::mat4 getInertiaTensor();

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::vec3 angularVelocity;

	float mass;

	glm::vec3 velocity;
	glm::vec3 angularMomentum;
	glm::mat4 inverseInertiaTensor;

	glm::vec3 frameForce;
	glm::vec3 frameTorque;
};