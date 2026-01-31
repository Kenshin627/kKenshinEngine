#pragma once
#include "platform.h"
#include <glm/glm.hpp>

KENSHIN_BEGIN

class Camera
{
public:
	Camera() = default;
	~Camera() = default;
	void setPosition(const glm::vec4& pos);
	void setAspectRatio(float aspectRatio);	
	const glm::vec4& getPosition() const;
	const glm::mat4& getViewMatrix();
	const glm::mat4& getProjectionMatrix();
	const glm::mat4& getViewProjectionMatrix();
	void setCenter(const glm::vec3& center);
	void setUp(const glm::vec3& up);
	void setFovY(float fovY);
	void setNear(float nearPlane);
	void setFar(float farPlane);
private:
	void setViewMatrix();
	void setProjectionMatrix();

private:
	glm::vec4 mPosition				    { 0, 0, 1, 0		  };
	glm::vec3 mCenter				    { 0, 0, 0			  };
	glm::vec3 mUp					    { 0, 1, 0			  };
	float	  mFovY					    { glm::radians(45.0f) };
	float	  mAspectRatio			    { 1.000f			  };
	float	  mNear					    { 0.010f			  };
	float	  mFar					    { 100.0f			  };
	glm::mat4 mViewMatrix				{ glm::mat4(1.0)	  };
	glm::mat4 mProjectionMatrix			{ glm::mat4(1.0)	  };
	glm::mat4 mViewProjectionMatrix		{ glm::mat4(1.0)	  };
	bool mViewMatrixDirty				{ true				  };
	bool mProjectionMatrixDirty			{ true				  };
	bool mViewProjectionwMatrixDirty	{ true				  };
};

KENSHIN_END