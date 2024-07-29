#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "CollisionDetection.h"

class Rigidbody
{
public:
	static bool collide(Rigidbody &body1, Rigidbody &body2);

	Rigidbody();
	Rigidbody(const glm::vec3 center, const glm::vec3 size, float mass);
	virtual ~Rigidbody();

	void addForce(const glm::vec3 force, const glm::vec3 where);
	void addForceWorld(const glm::vec3 force, const glm::vec3 where);
	void update(float deltaTime);

	glm::vec3 collisonPoint;
	glm::vec3 collisioNormal;
	glm::vec3 totalVelocity;
	glm::vec3 relVelocity;

	std::vector<glm::vec3> getCorners();

	static glm::mat4 computeInertiaTensorInverse(const glm::vec3 size, float massInverse);

	static const glm::vec3 s_corners[8];

	CollisionInfo RBcheckCollision(const Rigidbody &other);

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::vec3 angularVelocity;

	float mass;

	glm::vec3 velocity;
	glm::vec3 angularMomentum;
	glm::mat4 inverseInertiaTensor;

	glm::mat4 worldFromObj;
	glm::mat4 objFromWorld;
	glm::mat4 m_scaledObjToWorld;
	glm::mat4 m_worldToScaledObj;

	glm::vec3 frameForce = glm::vec3(0);
	glm::vec3 frameTorque = glm::vec3(0);
};