///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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
	glm::mat4 modelView;
	glm::mat4 scale = glm::scale(scaleXYZ);
	glm::mat4 rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);

		// FIXED: Calculate and set the normal matrix for the shader
		// This is the strict requirement for Milestone lighting accuracy
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelView)));
		m_pShaderManager->setMat3Value("normalMatrix", normalMatrix);
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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
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
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();

	// --- DEFINE MATERIALS (Fixes the black screen) ---
	OBJECT_MATERIAL polished;
	polished.tag = "polished";
	polished.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	polished.ambientStrength = 0.3f;
	polished.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	polished.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	polished.shininess = 32.0f;
	m_objectMaterials.push_back(polished);

	OBJECT_MATERIAL wood;
	wood.tag = "wood";
	wood.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	wood.ambientStrength = 0.2f;
	wood.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	wood.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	wood.shininess = 2.0f;
	m_objectMaterials.push_back(wood);

	OBJECT_MATERIAL porcelain;
	porcelain.tag = "porcelain";
	porcelain.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	porcelain.ambientStrength = 0.4f;
	porcelain.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	porcelain.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	porcelain.shininess = 256.0f; // High value = tight, bright highlights
	m_objectMaterials.push_back(porcelain);

	OBJECT_MATERIAL liquid;
	liquid.tag = "liquid";
	liquid.ambientColor = glm::vec3(0.1f, 0.05f, 0.02f); // Very dark brown
	liquid.ambientStrength = 0.2f;
	liquid.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	liquid.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	liquid.shininess = 90.0f; // High shininess for a reflective liquid surface
	m_objectMaterials.push_back(liquid);

	// Load textures
	CreateGLTexture("../../Utilities/textures/pavers.jpg", "brick_diff");
	CreateGLTexture("../../Utilities/textures/stainless.jpg", "metal_diff");
	CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "wood_diff");
	CreateGLTexture("../../Utilities/textures/porcelain.jpg", "porcelain_diff");

	BindGLTextures();
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	if (NULL != m_pShaderManager)
	{
		// LIGHT 0: Point Light (Natural White)
		m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(15.0f, 15.0f, 20.0f));
		// Neutralized Ambient (R=G=B) 
		m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.1f, 0.1f, 0.1f));
		// Balanced White/Yellow (R=1.0, G=0.95, B=0.9)
		m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 0.95f, 0.9f));
		m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 1.0f, 1.0f));

		// LIGHT 1: Fill Light
		m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-20.0f, 10.0f, 15.0f));
		m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.05f, 0.05f, 0.05f));
		// Balanced with blue (R=0.5, G=0.5, B=0.6) 
		m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.5f, 0.5f, 0.6f));
		m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.3f, 0.3f, 0.3f));

		// LIGHT 2: The Under-Table "Wall Wash"
		m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(0.0f, -5.0f, -18.0f));
		m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(2.5f, 2.5f, 2.5f));
		m_pShaderManager->setIntValue("bUseLighting", true);
	}

	glm::vec3 scaleXYZ;
	float XrotationDegrees, YrotationDegrees, ZrotationDegrees;
	glm::vec3 positionXYZ;

	/****************************************************************/
	/*** Draw Backdrop Plane (Wall)                               ***/
	/****************************************************************/
	
	positionXYZ = glm::vec3(0.0f, 0.0f, -35.0f); // Positioning wall behind
	scaleXYZ = glm::vec3(60.0f, 1.0f, 60.0f); // Scaling wall

	// Rotating wall, try negative value if normals face wrong way
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);

	// Applying shader settings
	m_pShaderManager->setIntValue(g_UseTextureName, false);
	SetShaderMaterial("wood"); // Lighting treats it like wood
	SetShaderColor(0.6f, 0.55f, 0.5f, 1.0f); // Grey color
	
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	/*** Draw Side Wall (The Left Wall)                           ***/
	/****************************************************************/
	
	positionXYZ = glm::vec3(-40.0f, 0.0f, 0.0f); // Positioning wall
	scaleXYZ = glm::vec3(60.0f, 1.0f, 60.0f); // Scaling Wall

	// Rotating wall, try negative value if normals face wrong way
	SetTransformations(scaleXYZ, 0.0f, 0.0f, -90.0f, positionXYZ);

	//Applying shader settings
	m_pShaderManager->setIntValue(g_UseTextureName, false);
	SetShaderMaterial("wood"); // Lighting treats it like wood
	SetShaderColor(0.6f, 0.55f, 0.5f, 1.0f); // Grey color

	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/
	/*** Draw Floor Plane                                           ***/
	/******************************************************************/

	positionXYZ = glm::vec3(0.0f, -25.0f, 0.0f); // Positioning Table
	scaleXYZ = glm::vec3(50.0f, 3.0f, 30.0f); // Scaling Table

	// 0 0 0 is flat
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	SetShaderMaterial("wood");
	SetShaderTexture("wood_diff");
	SetTextureUVScale(2.0f, 2.0f); // Makes the grain look natural

	m_basicMeshes->DrawPlaneMesh();

	/***********************************************************************/
	/***********************************************************************/
	/*** TABLE COMPONENTS                                           ********/
	/***********************************************************************/
	/***********************************************************************/

	/******************************************************************/
	/*** Draw Table Top (Box Mesh)                                  ***/
	/******************************************************************/
	
	positionXYZ = glm::vec3(0.0f, -1.5f, 0.0f); // Positioning Table
	scaleXYZ = glm::vec3(50.0f, 3.0f, 30.0f); // Scaling Table
	
	// 0 0 0 is flat
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	SetShaderMaterial("wood");
	SetShaderTexture("wood_diff");
	SetTextureUVScale(2.0f, 2.0f); // Makes the grain look natural

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/*** Draw Table Legs (Translated Down)                        ***/
	/****************************************************************/
	// Scale the legs, X Y Z
	scaleXYZ = glm::vec3(1.5f, 25.0f, 1.5f);

	// Position legs lower
	float legY = -25.0f;

	// Applying shader settings
	SetShaderMaterial("wood");
	m_pShaderManager->setIntValue(g_UseTextureName, false);
	SetShaderColor(0.2f, 0.15f, 0.1f, 1.0f);

	// Front Left
	positionXYZ = glm::vec3(-24.0f, legY, 14.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// Front Right
	positionXYZ = glm::vec3(24.0f, legY, 14.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// Back Left
	positionXYZ = glm::vec3(-24.0f, legY, -14.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// Back Right
	positionXYZ = glm::vec3(24.0f, legY, -14.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	/***********************************************************************/
	/***********************************************************************/
	/*** CUP COMPONENTS                                             ********/
	/***********************************************************************/
	/***********************************************************************/

	/****************************************************************/
	/*** Draw Outer Cylinder (Cup Body - Stainless)               ***/
	/****************************************************************/
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	scaleXYZ = glm::vec3(3.0f, 6.0f, 3.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	// Set the metallic material for the shiny highlights
	SetShaderMaterial("polished");

	// RESET TINT
	// This also temporarily sets bUseTexture to false.
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);

	// APPLY TEXTURE: This flips bUseTexture back to true and binds your JPG.
	SetShaderTexture("metal_diff");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Draw Torus (Cup Handle)                                  ***/
	/****************************************************************/

	scaleXYZ = glm::vec3(1.5f, 2.25f, 1.5f);
	positionXYZ = glm::vec3(3.50f, 3.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	SetShaderMaterial("polished");
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // Ensure white tint
	SetShaderTexture("metal_diff");         // Bind stainless texture

	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Draw Cylinder (Cup contents - Shiny Coffee/Tea)          ***/
	/****************************************************************/

	// Scale: Slightly smaller than the cup interior
	scaleXYZ = glm::vec3(2.9f, 5.8f, 2.9f);
	// Position: Moved up slightly to ensure it's visible inside the cup
	positionXYZ = glm::vec3(0.0f, 0.25f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	// Set the shiny material
	SetShaderMaterial("polished");

	// Apply a deep brown color (R=0.25, G=0.12, B=0.05)
	// This function automatically turns off textures
	SetShaderColor(0.25f, 0.12f, 0.05f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();


	/*********************************************************************/
	/*********************************************************************/
	/*** PLATE PARTS                                              ********/
	/*********************************************************************/
	/*********************************************************************/

// Define the four corner positions
	glm::vec3 platePositions[] = {
		glm::vec3(-13.0f, 0.0f, 9.0f),  // Front Left
		glm::vec3(13.0f, 0.0f, 9.0f),   // Front Right
		glm::vec3(-13.0f, 0.0f, -9.0f), // Back Left
		glm::vec3(13.0f, 0.0f, -9.0f)   // Back Right
	};

	for (int i = 0; i < 4; i++) {
		glm::vec3 currentPos = platePositions[i];

		// --- SAFETY RESET ---
		SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);

		SetShaderMaterial("porcelain");
		SetShaderTexture("porcelain_diff"); // Now re-apply the actual texture

		/****************************************************************/
		/*** Draw Plate Body                                          ***/
		/****************************************************************/
		scaleXYZ = glm::vec3(6.0f, 0.15f, 6.0f); // Slightly thinner body
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(currentPos.x, 0.005f, currentPos.z));

		SetShaderMaterial("porcelain");
		SetShaderTexture("porcelain_diff");
		m_basicMeshes->DrawCylinderMesh();

		/****************************************************************/
		/*** Draw Plate Rim                                           ***/
		/****************************************************************/
		scaleXYZ = glm::vec3(5.5f, 5.5f, 0.9f); // Reduced Z-scale of Torus (thickness)

		SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, glm::vec3(currentPos.x, 0.1f, currentPos.z));
		m_basicMeshes->DrawTorusMesh();

		
	}

	/****************************************************************/
	/*** SPOON GENERATION LOOP                                    ***/
	/****************************************************************/
	for (int i = 0; i < 4; i++) {
		glm::vec3 plateCenter = platePositions[i];

		// Position the spoon slightly more toward the center-right of the plate
		// X + 1.5 and Z + 0.5 moves it away from the rim edge
		glm::vec3 spoonBasePos = plateCenter + glm::vec3(1.5f, 0.3f, 0.5f);

		/****************************************************************/
		/*** Draw Spoon Bowl (Cylinder)                               ***/
		/****************************************************************/
		// X: Width, Y: Thickness, Z: Length of the oval
		scaleXYZ = glm::vec3(0.8f, 0.08f, 1.2f);

		// Tilt it slightly so the "tip" of the spoon touches the plate
		SetTransformations(scaleXYZ, 8.0f, 0.0f, 0.0f, spoonBasePos);

		SetShaderMaterial("polished");
		SetShaderTexture("metal_diff");
		m_basicMeshes->DrawCylinderMesh();

		/****************************************************************/
		/*** Draw Spoon Handle (Box)                                  ***/
		/****************************************************************/
		// Width: 0.25, Height: 0.05, Length: 4.5 (long handle)
		scaleXYZ = glm::vec3(0.25f, 0.05f, 4.5f);

		glm::vec3 handlePos = spoonBasePos + glm::vec3(0.0f, 0.0f, 2.8f);

		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, handlePos);

		SetShaderMaterial("polished");
		SetShaderTexture("metal_diff");
		m_basicMeshes->DrawBoxMesh();
	}
}