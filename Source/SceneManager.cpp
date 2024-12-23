///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

void SceneManager::LoadSceneTextures() {
	bool bReturn = false;



	bReturn = CreateGLTexture(
		"textures/street.jpg",
		"street");
	bReturn = CreateGLTexture(
		"textures/blackmat.jpg",
		"bmat");
	bReturn = CreateGLTexture(
		"textures/wall.jpg",
		"wall");
	bReturn = CreateGLTexture(
		"textures/lamp.jpg",
		"lamp");
	bReturn = CreateGLTexture(
		"textures/wood.jpg",
		"wood");


	// Bind all textures to texture slots
	BindGLTextures();
}

void SceneManager::DefineObjectMaterials()
{   

    //Lamp street
	OBJECT_MATERIAL LampMaterial;
	LampMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);    
	LampMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	LampMaterial.shininess = 64.0f;                  
	LampMaterial.tag = "Lamp";
	m_objectMaterials.push_back(LampMaterial);


	// Wall (brick)
	OBJECT_MATERIAL BrickMaterial;
	BrickMaterial.diffuseColor = glm::vec3(0.5f, 0.2f, 0.1f);  
	BrickMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);  
	BrickMaterial.shininess = 16.0f;
	BrickMaterial.tag = "Brick";
	m_objectMaterials.push_back(BrickMaterial);


	// Ground (rough asphalt)
	OBJECT_MATERIAL GroundMaterial;
	GroundMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);   
	GroundMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);  // Reduce specular reflections
	GroundMaterial.shininess = 8.0f;                            // Softer highlights
	GroundMaterial.tag = "Ground";
	m_objectMaterials.push_back(GroundMaterial);

	// Wood Material for Bench
	OBJECT_MATERIAL WoodMaterial;
	WoodMaterial.diffuseColor = glm::vec3(0.55f, 0.27f, 0.07f);  // Warm brown for wood
	WoodMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);    // Slight gloss for polished wood
	WoodMaterial.shininess = 32.0f;                              // Moderate shininess for wood finish
	WoodMaterial.tag = "Wood";
	m_objectMaterials.push_back(WoodMaterial);
}

void SceneManager::SetupSceneLights()
{

	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	// directional light to emulate sunlight coming into scene
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.3f, -0.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.3f, 0.3f, 0.3f);  
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.8f, 0.8f, 0.8f); // Slightly stronger diffuse
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 4.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.3f, 0.3f, 0.2f); // Keep ambient moderate
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.2f, 1.2f, 0.9f); // Slightly reduce diffuse
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 1.0f, 0.8f); // Maintain specular highlights
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f); // Base intensity
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.05f);  // Smooth linear attenuation
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.01f); // Gradual quadratic falloff
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	// point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	// point light 3
	m_pShaderManager->setVec3Value("pointLights[2].position", 3.8f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	// point light 4
	m_pShaderManager->setVec3Value("pointLights[3].position", 3.8f, 3.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);
	// point light 4
	m_pShaderManager->setVec3Value("pointLights[4].position", -3.2f, 6.0f, -4.0f);
	m_pShaderManager->setVec3Value("pointLights[4].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[4].diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[4].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", true);

	//point light 5
	m_pShaderManager->setVec3Value("pointLights[5].position", 1.5f, 2.0f, 0.0f);  // Position the light near the bench
	m_pShaderManager->setVec3Value("pointLights[5].ambient", 0.2f, 0.15f, 0.1f);  // Warm ambient light for natural look
	m_pShaderManager->setVec3Value("pointLights[5].diffuse", 0.8f, 0.6f, 0.3f);  // Softer warm diffuse light
	m_pShaderManager->setVec3Value("pointLights[5].specular", 0.9f, 0.8f, 0.7f);  // Highlight to emphasize texture
	m_pShaderManager->setFloatValue("pointLights[5].constant", 1.0f);             // Base intensity
	m_pShaderManager->setFloatValue("pointLights[5].linear", 0.09f);             // Smooth linear attenuation
	m_pShaderManager->setFloatValue("pointLights[5].quadratic", 0.032f);         // Gradual quadratic falloff
	m_pShaderManager->setBoolValue("pointLights[5].bActive", true);              // Activate the light



	m_pShaderManager->setVec3Value("spotLight.position", 0.0f, 2.0f, 0.5f);   // Position near the lamp
	m_pShaderManager->setVec3Value("spotLight.direction", 0.0f, -1.0f, -0.5f); // Angle toward the wall
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("spotLight.specular", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(35.0f)));  // Wider cone
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(50.0f))); // Smoother edges
	m_pShaderManager->setBoolValue("spotLight.bActive", true);

}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{

	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadCylinderMesh();  // For lamp base, post, and decorative arm
	m_basicMeshes->LoadSphereMesh();    // For lamp head
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene() {
	// Declare the variables for the transformations
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// Render Floor
	scaleXYZ = glm::vec3(10.0f, -1.0f, 8.0f);  // Large plane for the floor
	positionXYZ = glm::vec3(0.0f, -0.1f, 4.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("street");// Dark gray
	SetShaderMaterial("Ground");
	// Light gray floor
	m_basicMeshes->DrawPlaneMesh();  // Render the floor
	// Render Wall
	scaleXYZ = glm::vec3(10.0f, 2.0f, 6.0f);  // Large wall scaled appropriately
	positionXYZ = glm::vec3(0.0f, 5.8f, -4.0f);  // Position wall behind the lamp
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wall"); // Light brick-like color
	SetShaderMaterial("Brick");
	m_basicMeshes->DrawPlaneMesh();
	// 3. Render Lamp Head (Sphere with Light Effect)
	scaleXYZ = glm::vec3(-0.5f, 0.5f, 0.5f);  // Slightly smaller sphere
	positionXYZ = glm::vec3(-0.6f, 5.5f, 0.0f);  // Hanging under the arm
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("Lamp");
	SetShaderTexture("lamp");
	m_basicMeshes->DrawSphereMesh();
	// 1. Render Lamp Base (Bottom Cylinder with Decorative Ring)
	scaleXYZ = glm::vec3(0.6f, 0.3f, 0.6f);  // Larger base
	positionXYZ = glm::vec3(-3.0f, 0.15f, 0.0f);  // Aligned with floor
	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");// Dark gray
	m_basicMeshes->DrawCylinderMesh();

	// 2. Render Lamp Post (Multiple Cylinders for Segments)
	scaleXYZ = glm::vec3(0.2f, 6.0f, 0.2f);  // Tall post
	positionXYZ = glm::vec3(-3.0f, 0.15f, 0.0f);  // Post position
	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");
	m_basicMeshes->DrawCylinderMesh();

	// Decorative section (ring at the top of the post)
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);
	positionXYZ = glm::vec3(-3.0f, 5.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("blackmat");
	m_basicMeshes->DrawCylinderMesh();
	// 1. Render Lamp Holder ( Cylinder with Decorative Ring)
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);  // Larger base
	positionXYZ = glm::vec3(-0.6f, 6.0f, 0.0f);  // Aligned with floor
	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("blackmat");  // Dark gray
	m_basicMeshes->DrawCylinderMesh();


	// Render Smooth Semi-Circle Decorative Arm
	int numSegments = 50;  // High segment count for a smooth curve
	float radius = 1.2f;  // Adjusted radius for closer fit
	glm::vec3 center = glm::vec3(-1.7f, 6.1f, 0.0f);  // Lowered center for alignment

	for (int i = 0; i <= numSegments; ++i) {
		float angle = glm::radians(180.0f * (float(i) / numSegments));  // Angle from 0 to 180 degrees

		scaleXYZ = glm::vec3(0.05f, 0.2f, 0.05f);  // Slightly longer segments for overlap
		positionXYZ = glm::vec3(
			center.x + radius * cos(angle),  // X along semi-circle
			center.y + radius * sin(angle),  // Y along semi-circle
			center.z
		);
		// Black decorative arm
		SetShaderTexture("blackmat");
		SetTransformations(scaleXYZ, 0.0f, glm::degrees(angle), 90.0f, positionXYZ);  // Smooth rotation
		
		m_basicMeshes->DrawCylinderMesh();  // Render each segment
	}



	// Render Bench Seat 
	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.2f);  
	positionXYZ = glm::vec3(2.0f, 1.2f, 1.0f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");  // Brown wood texture
	SetShaderMaterial("Wood");
	m_basicMeshes->DrawBoxMesh();  // Part 1 seat
	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.2f);  
	positionXYZ = glm::vec3(2.0f, 1.4f, 0.77f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, 45.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");  // Brown wood texture
	SetShaderMaterial("Wood");
	m_basicMeshes->DrawBoxMesh();  // Part 2 seat
	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.2f);  
	positionXYZ = glm::vec3(2.0f, 1.2f, 1.3f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");  // Brown wood texture
	SetShaderMaterial("Wood");// Part 3 seat
	m_basicMeshes->DrawBoxMesh();  
	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.2f);  
	positionXYZ = glm::vec3(2.0f, 1.2f, 1.6f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");  // Brown wood texture
	SetShaderMaterial("Wood");// Part 4 seat
	m_basicMeshes->DrawBoxMesh(); 
	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.2f);  
	positionXYZ = glm::vec3(2.0f, 1.1f, 1.9f);  
	SetTransformations(scaleXYZ, 45.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");  // Brown wood texture
	SetShaderMaterial("Wood");// Part 5 seat
	m_basicMeshes->DrawBoxMesh(); 
	// Render Bench Legs and Handlers
	scaleXYZ = glm::vec3(0.1f, 1.0f, 0.1f);  
	positionXYZ = glm::vec3(-0.3f, 1.1f, 1.4f);  
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp"); // Legs 2
	m_basicMeshes->DrawBoxMesh();  // Legs support
	scaleXYZ = glm::vec3(0.1f, 1.2f, 0.1f);  // Wider and thicker seat
	positionXYZ = glm::vec3(-0.3f, 0.5f, 1.7f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp");
	m_basicMeshes->DrawBoxMesh();  // Legs support
	scaleXYZ = glm::vec3(0.1f, 1.4f, 0.1f);  // Wider and thicker seat
	positionXYZ = glm::vec3(-0.3f, 0.5f, 0.8f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, 30.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp");
	m_basicMeshes->DrawBoxMesh();  // Legs 1
	scaleXYZ = glm::vec3(0.1f, 1.0f, 0.1f);  // Wider and thicker seat
	positionXYZ = glm::vec3(4.3f, 1.1f, 1.4f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp"); // Legs 3
	m_basicMeshes->DrawBoxMesh();  // Legs support
	scaleXYZ = glm::vec3(0.1f, 1.2f, 0.1f);  // Wider and thicker seat
	positionXYZ = glm::vec3(4.3f, 0.5f, 1.7f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp");
	m_basicMeshes->DrawBoxMesh();  // Legs support

	scaleXYZ = glm::vec3(0.1f, 1.4f, 0.1f);  // Wider and thicker seat
	positionXYZ = glm::vec3(4.3f, 0.5f, 0.8f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, 30.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp");
	m_basicMeshes->DrawBoxMesh();  // Legs 4

	// Render back seat and handlers
	scaleXYZ = glm::vec3(0.1f, 0.7f, 0.1f);  // Wider and thicker seat
	positionXYZ = glm::vec3(-0.3f, 1.2f, 0.8f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, -40.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp");
	m_basicMeshes->DrawBoxMesh();  // Upper Handler 
	
	scaleXYZ = glm::vec3(0.1f, 0.7f, 0.1f);  // Wider and thicker seat
	positionXYZ = glm::vec3(4.3f, 1.2f, 0.8f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, -40.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp");
	m_basicMeshes->DrawBoxMesh();  // Upper Handler 

	scaleXYZ = glm::vec3(0.1f, 0.8f, 0.1f);  // Wider and thicker seat
	positionXYZ = glm::vec3(-0.3f, 1.8f, 0.57f);  // Repositioned above the floor
	SetTransformations(scaleXYZ, 175.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp");
	m_basicMeshes->DrawBoxMesh();  // Back Handler 


	scaleXYZ = glm::vec3(0.1f, 0.8f, 0.1f);  
	positionXYZ = glm::vec3(4.3f, 1.8f, 0.57f); 
	SetTransformations(scaleXYZ, 175.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bmat");  // Black metallic texture
	SetShaderMaterial("Lamp");
	m_basicMeshes->DrawBoxMesh();  // Back Handler 


	scaleXYZ = glm::vec3(5.0f, 0.1f, 0.9f);  // Wider and thicker 
	positionXYZ = glm::vec3(2.0f, 2.2f, 0.64f);  
	SetTransformations(scaleXYZ, 85.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");  // Brown wood texture
	SetShaderMaterial("Wood");
	m_basicMeshes->DrawBoxMesh();// Last upper part of seat
















}
