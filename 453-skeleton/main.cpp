//CPSC 453 Assignment 1
// By Thomas Vy 30000789

#include <iostream>
#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Window.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <math.h>

static const std::vector<glm::vec3> colorPalettes = {
	//https://www.color-hex.com/color-palette/98386
	{0.87, 0.08,0.27},
	{0, 0.33, 0.57},
	{0.22, 0, 0.38},
	{0.7, 0.08, 0.7},
	{0.62, 0.57, 0.44},
	//https://www.color-hex.com/color-palette/98383
	{0.87, 0.50, 0.08},
	{0.33, 0.56, 0.44},
	{0.94, 0.87, 0.08},
	{0.91, 1.f, 0.98},
	{0.68, 0.36, 0.36}
}; // 10 colors available so if iteration limit is increased, you'll need to increase this color palette array too.

class MyCallbacks : public CallbackInterface {

public:
	MyCallbacks(ShaderProgram& shader) : shader(shader) {}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_R && action == GLFW_REPEAT) {
			shader.recompile();
		}
		if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
		{
			increaseIteration();
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
		{
			decreaseIteration();
		}
		if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
		{
			increaseScene();
		}
		if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
		{
			decreaseScene();
		}
	}
	int& getIterationCount()
	{
		return iteration;
	}

	unsigned getSceneNum() const
	{
		return scene;
	}
	void setSceneNum(unsigned newScene)
	{
		if (newScene <= 4 && newScene >= 1)
		{
			scene = newScene;
		}
	}

	void increaseScene()
	{
		if (scene < 4) //Only allow 4 scenes 
		{
			scene++;
		}
	}
	void decreaseScene()
	{
		if (scene > 1) //Don't allow to go to below scene 1
		{
			scene--;
		}
	}
	void increaseIteration()
	{
		if (iteration < 7)//Only allow up to 7 iterations. If increasing this limit past 10, increase the color palettes that is above.
		{
			iteration++;
		}
	}
	void decreaseIteration()
	{
		if (iteration > 1) //Don't allow to go <= 0 iterations.
		{
			iteration--;
		}
	}
private:
	ShaderProgram& shader;
	int iteration = 1;
	unsigned scene = 1;
};

struct TrianglePositionVertices {
	glm::vec3 vertexA;
	glm::vec3 vertexB;
	glm::vec3 vertexC;
};

struct QuadPositionVertices {
	glm::vec3 vertexA;
	glm::vec3 vertexB;
	glm::vec3 vertexC;
	glm::vec3 vertexD;
};

//Get the middle vertices between two vertices. numberOfVertices -s pecify how many middle vertices you want.
std::vector<glm::vec3> GetMiddleVertices(const glm::vec3& vertexA, const glm::vec3& vertexB, const unsigned numberOfVertices)
{
	std::vector<glm::vec3> middleVertices;

	glm::vec3 offset = {

		(vertexA.x - vertexB.x) / (numberOfVertices + 1),
		(vertexA.y - vertexB.y) / (numberOfVertices + 1),
		(vertexA.z - vertexB.z) / (numberOfVertices + 1)
	};

	for (unsigned currentVertex = 1; currentVertex <= numberOfVertices; currentVertex++)
	{
		middleVertices.emplace_back(
			vertexB.x + currentVertex * offset.x,
			vertexB.y + currentVertex * offset.y,
			vertexB.z + currentVertex * offset.z
		);
	}
	return middleVertices;
}

void GenerateSierpinski(CPU_Geometry& outCPUGeom, const TrianglePositionVertices& triangleToCheck, const int iteration, const glm::vec3& colors)
{
	if (iteration == 1)
	{
		outCPUGeom.verts.push_back(triangleToCheck.vertexA);
		outCPUGeom.verts.push_back(triangleToCheck.vertexB);
		outCPUGeom.verts.push_back(triangleToCheck.vertexC);
		outCPUGeom.cols.insert(outCPUGeom.cols.end(), 3, colors);
		return;
	}
	else
	{
		glm::vec3 vertexBetweenBC = GetMiddleVertices(triangleToCheck.vertexB, triangleToCheck.vertexC, 1).front();
		glm::vec3 vertexBetweenAB = GetMiddleVertices(triangleToCheck.vertexA, triangleToCheck.vertexB, 1).front();
		glm::vec3 vertexBetweenCA = GetMiddleVertices(triangleToCheck.vertexC, triangleToCheck.vertexA, 1).front();

		GenerateSierpinski(outCPUGeom, { triangleToCheck.vertexA, vertexBetweenAB, vertexBetweenCA }, iteration - 1, { colors.x + iteration* 0.06, colors.y, colors.z});
		GenerateSierpinski(outCPUGeom, { triangleToCheck.vertexB, vertexBetweenAB, vertexBetweenBC }, iteration - 1, { colors.x, colors.y + iteration * 0.06, colors.z });
		GenerateSierpinski(outCPUGeom, { triangleToCheck.vertexC, vertexBetweenBC, vertexBetweenCA }, iteration - 1, { colors.x, colors.y, colors.z + iteration * 0.06 });
	}
}


QuadPositionVertices CalculateInnerQuad(const QuadPositionVertices& square)
{
	QuadPositionVertices innerQuad;
	innerQuad.vertexA = GetMiddleVertices(square.vertexA, square.vertexB, 1).front();
	innerQuad.vertexB = GetMiddleVertices(square.vertexB, square.vertexC, 1).front();
	innerQuad.vertexC = GetMiddleVertices(square.vertexC, square.vertexD, 1).front();
	innerQuad.vertexD = GetMiddleVertices(square.vertexD, square.vertexA, 1).front();
	return innerQuad;
}
void GenerateSquareDiamonds(CPU_Geometry& outCPUGeom, const QuadPositionVertices& square, const int iteration, const float colorShading)
{
	QuadPositionVertices diamond = CalculateInnerQuad(square);

	outCPUGeom.verts.push_back(diamond.vertexA);
	outCPUGeom.verts.push_back(diamond.vertexB);
	outCPUGeom.verts.push_back(diamond.vertexB);
	outCPUGeom.verts.push_back(diamond.vertexC);
	outCPUGeom.verts.push_back(diamond.vertexC);
	outCPUGeom.verts.push_back(diamond.vertexD);
	outCPUGeom.verts.push_back(diamond.vertexD);
	outCPUGeom.verts.push_back(diamond.vertexA);
	outCPUGeom.cols.insert(outCPUGeom.cols.end(), 8, {0, 0, colorShading });

	outCPUGeom.verts.push_back(square.vertexA);
	outCPUGeom.verts.push_back(square.vertexB);
	outCPUGeom.verts.push_back(square.vertexB);
	outCPUGeom.verts.push_back(square.vertexC);
	outCPUGeom.verts.push_back(square.vertexC);
	outCPUGeom.verts.push_back(square.vertexD);
	outCPUGeom.verts.push_back(square.vertexD);
	outCPUGeom.verts.push_back(square.vertexA);
	outCPUGeom.cols.insert(outCPUGeom.cols.end(), 8, {0, colorShading, 0});

	if( iteration > 1)
	{
		QuadPositionVertices innerSquare = CalculateInnerQuad(diamond);
		GenerateSquareDiamonds(outCPUGeom, innerSquare, iteration - 1, (float)(colorShading + 0.1));
	}

}

void GenerateKochSnowflake(CPU_Geometry& outCPUGeom, const TrianglePositionVertices& triangle,  const int iterations)
{
	outCPUGeom.verts.push_back(triangle.vertexA);
	outCPUGeom.verts.push_back(triangle.vertexB);

	outCPUGeom.verts.push_back(triangle.vertexB);
	outCPUGeom.verts.push_back(triangle.vertexC);

	outCPUGeom.verts.push_back(triangle.vertexC);
	outCPUGeom.verts.push_back(triangle.vertexA);
	outCPUGeom.cols.assign(6, colorPalettes[0]);

	const float sinValue = (float)sin(60 * M_PI / 180.0);
	const float cosValue = (float)cos(60 * M_PI / 180.0);

	for (int currentIteration = 1; currentIteration < iterations ; currentIteration++)
	{
		//Because we want the lines to be a different color depending on which subdivision it belongs to, we need to duplicate alot of points.
		for (auto vertsIterator = outCPUGeom.verts.end(), colsIterator = outCPUGeom.cols.end(); vertsIterator != outCPUGeom.verts.begin(); vertsIterator--, colsIterator--)
		{
			//Subtract again to skip over the duplicate point.
			vertsIterator--;
			colsIterator--;

			std::vector<glm::vec3> subdivisionTriangle = GetMiddleVertices(*vertsIterator, *(vertsIterator - 1), 2);
			subdivisionTriangle.insert(
				subdivisionTriangle.begin() + 1,
				2, //We add two of the same point because we need the redundant point to draw lines of different colors. 
				{
					//https://stackoverflow.com/a/2862412
					(cosValue * (subdivisionTriangle[0].x - subdivisionTriangle[1].x) - sinValue * (subdivisionTriangle[0].y - subdivisionTriangle[1].y)) + subdivisionTriangle[1].x,
					(sinValue * (subdivisionTriangle[0].x - subdivisionTriangle[1].x) + cosValue * (subdivisionTriangle[0].y - subdivisionTriangle[1].y)) + subdivisionTriangle[1].y,
					0
				}
			);
			//This duplicates the base points for the sub triangle; for drawing lines of different colors. 
			subdivisionTriangle.insert(subdivisionTriangle.begin(), *subdivisionTriangle.begin());
			subdivisionTriangle.insert(subdivisionTriangle.end(), *(subdivisionTriangle.end() - 1));

			vertsIterator = outCPUGeom.verts.insert(vertsIterator, subdivisionTriangle.begin(), subdivisionTriangle.end());

			std::vector<glm::vec3> colorPaletteSubdivisionTriangle = {
				*(colsIterator - 1), //This keeps the line the same color from it's original subdivision.
				colorPalettes[currentIteration],//This sets the new lines to the new color.
				colorPalettes[currentIteration],
				colorPalettes[currentIteration],
				colorPalettes[currentIteration],
				*colsIterator//This keeps the line the same color from it's original subdivision.
			};
			colsIterator = outCPUGeom.cols.insert(colsIterator, colorPaletteSubdivisionTriangle.begin(), colorPaletteSubdivisionTriangle.end());
		}
	}
}

void GenerateDragonCurve(CPU_Geometry& outCPUGeom, const TrianglePositionVertices& triangle, const int iterations)
{
	outCPUGeom.verts.push_back(triangle.vertexA);
	outCPUGeom.verts.push_back(triangle.vertexB);
	outCPUGeom.verts.push_back(triangle.vertexC);

	for (int currentIteration = 1; currentIteration < iterations; currentIteration++)
	{
		int turn = -1;
		for (auto vertsIterator = outCPUGeom.verts.end() - 1; vertsIterator != outCPUGeom.verts.begin(); vertsIterator--)
		{
			vertsIterator = outCPUGeom.verts.emplace(
				vertsIterator,
				(((vertsIterator - 1)->x - vertsIterator->x) - turn * ((vertsIterator - 1)->y - vertsIterator->y)) / 2 + vertsIterator->x,
				(turn * ((vertsIterator - 1)->x - vertsIterator->x) + ((vertsIterator - 1)->y - vertsIterator->y)) / 2 + vertsIterator->y,
				0
			);
			turn *= -1; //Changes direction.
		}
	}
	outCPUGeom.cols.assign(outCPUGeom.verts.size(), colorPalettes[iterations-1]);
}

int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	Window window(800, 800, "CPSC 453"); // can set callbacks at construction if desired

	GLDebug::enable();

	// SHADERS
	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

	// CALLBACKS
	auto callbacks = std::make_shared<MyCallbacks>(shader);
	window.setCallbacks(callbacks); // can also update callbacks to new ones

	// GEOMETRY
	CPU_Geometry cpuGeom;
	GPU_Geometry gpuGeom;
	GLenum drawMode = GL_TRIANGLES;
	int iteration = 1;
	unsigned sceneNum = 1;
	const TrianglePositionVertices startingTriangle = {
		{0.f, 0.5f, 0.f},
		{-0.5f, -0.5f, 0.f},
		{0.5f, -0.5f, 0.f}
	};
	const QuadPositionVertices startingSquare = {
		{0.5, 0.5, 0},
		{-0.5, 0.5, 0},
		{-0.5, -0.5, 0},
		{0.5, -0.5, 0}
	};
	GenerateSierpinski(cpuGeom, startingTriangle, iteration, { 0.01, 0.01, 0.01 });
	gpuGeom.setVerts(cpuGeom.verts);
	gpuGeom.setCols(cpuGeom.cols);

	// RENDER LOOP
	while (!window.shouldClose()) {
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		unsigned newSceneNum = callbacks->getSceneNum();
		int newIteration = callbacks->getIterationCount();
		if (sceneNum != newSceneNum || iteration != newIteration)
		{
			iteration = newIteration;
			sceneNum = newSceneNum;
			cpuGeom.cols.clear();
			cpuGeom.verts.clear();
			switch (sceneNum)
			{
			case 1:
				GenerateSierpinski(cpuGeom, startingTriangle, iteration, { 0.01, 0.01, 0.01 });
				drawMode = GL_TRIANGLES;
				break;
			case 2:
				GenerateSquareDiamonds(cpuGeom, startingSquare, iteration, 0.1f);
				drawMode = GL_LINES;
				break;
			case 3:
			{
				GenerateKochSnowflake(cpuGeom, startingTriangle, iteration);
				drawMode = GL_LINES;
				break;
			}
			case 4:
				GenerateDragonCurve(cpuGeom, { {-0.5,0,0}, {0,-0.5,0}, {0.5,0,0} }, iteration);
				drawMode = GL_LINE_STRIP;
				break;
			}
			gpuGeom.setVerts(cpuGeom.verts);
			gpuGeom.setCols(cpuGeom.cols);
		}

		shader.use();
		gpuGeom.bind();

		glEnable(GL_FRAMEBUFFER_SRGB);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(drawMode, 0, GLsizei(cpuGeom.verts.size()));
		glDisable(GL_FRAMEBUFFER_SRGB); // disable sRGB for things like imgui

		{
			ImGui::Begin("CPSC 453 Assignment 1 - GUI Integration");      
			ImGui::Text("Scene %d", sceneNum);
			if (ImGui::Button("Scene 1 - Sierpinski Triangle"))
			{
				callbacks->setSceneNum(1);
			}
			if (ImGui::Button("Scene 2 - Squares and Diamonds"))
			{
				callbacks->setSceneNum(2);
			}
			if (ImGui::Button("Scene 3 - Koch Snowflake"))
			{
				callbacks->setSceneNum(3);
			}
			if (ImGui::Button("Scene 4 - Dragon Curve"))
			{
				callbacks->setSceneNum(4);
			}
			ImGui::NewLine();
			ImGui::SliderInt("Iteration", &(callbacks->getIterationCount()), 1, 7);
			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window.swapBuffers();
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}
