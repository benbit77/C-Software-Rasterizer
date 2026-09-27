#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

struct Vec2 {
    float x,y;
};

struct Vec3 {
    float x,y,z;
};

struct Triangle {
    int a, b, c;
    uint32_t colour;
};

void updatePixel(std::vector<uint32_t>& pixels, int x, int y, uint32_t colour);
void drawLine(std::vector<uint32_t>& pixels, int x0, int y0, int x1, int y1, uint32_t colour);
Vec3 project(Vec3 point);
Vec3 rotateY(Vec3 point, float angle);
Vec3 rotateX(Vec3 point, float angle);
float edgeFunction(Vec2 a, Vec2 b, Vec2 c);
void FillTriangle(std::vector<uint32_t>& pixels, std::vector<float>& zBuffer, Vec3 v0, Vec3 v1, Vec3 v2, uint32_t colour);

//.obj reader
bool loadOBJ(const std::string& filename, std::vector<Vec3>& outVertices, std::vector<Triangle>& outTriangles) {
    std::ifstream file(filename);
    if(!file.is_open()) {
        std::cerr << "Failed to open model: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        // If the line starts with 'v', it's a vertex
        if (prefix == "v") {
            Vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            outVertices.push_back(vertex);
        }

        // If the line starts with 'f', it's a triangle
        else if (prefix == "f") {
            int a, b, c;
            ss >> a >> b >> c;

            //Reset .OBJ offset due to starting at index 1
            a--; b--; c--;

            // Assing a random color so we can see the individual 3D faces
            uint32_t randomColor = 0xFF000000 | (100 + (rand() % 156) << 16) | (100 + (rand() % 156) << 8) | (100 + (rand() % 156) << 0);

            outTriangles.push_back({a, b, c, randomColor});
            }
    }
    return true;
}

int main(int argc, char *argv[]) {
  
// Create empty buffers
std::vector<Vec3> modelVertices;
std::vector<Triangle> modelTriangles;

// Load the file
if (!loadOBJ("model.obj", modelVertices, modelTriangles)) {
    return 1; // Exit if the file wasn't found
}

int numPoints = modelVertices.size();

//Create the window
SDL_Window* window = SDL_CreateWindow(
        "Software Rasterizer", 
        SCREEN_WIDTH, 
        SCREEN_HEIGHT, 
        SDL_WINDOW_RESIZABLE
    );
    
// Create a renderer
SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

// Create a texture
SDL_Texture* texture = SDL_CreateTexture(
    renderer, 
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    SCREEN_WIDTH, 
    SCREEN_HEIGHT
);

// Create CPU pixel buffer
std::vector<uint32_t> pixels(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFF000000);

//Create a Z buffer
std::vector<float> zBuffer(SCREEN_WIDTH * SCREEN_HEIGHT);

    //Initialize SDL with video subsystem
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return 1;
}

//Check if window creation was successful
if (!window) {
        std::cerr << "Window could not be created! Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
}

//Main loop
    bool isRunning = true;
    SDL_Event event;

float rotationAngle = 0.0f;

    while (isRunning) {
        // Poll for events (keyboard inputs, mouse movements, closing the window)
        while (SDL_PollEvent(&event)) {
            // Triggered when the user clicks the 'X' to close the window
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
        }

//Reset the screen to black
std::fill(pixels.begin(), pixels.end(), 0xFF000000);

//Reset the Z buffer to be super far 
std::fill(zBuffer.begin(), zBuffer.end(), 99999.0f);

//Rotate the model
rotationAngle += 0.0005f;

//Temporary vector to hold rotated points
std::vector<Vec3> rotated3DPoints;

for(int i = 0; i < numPoints; i++) {
    Vec3 points = modelVertices[i];

//Rotate in the y
    Vec3 rotatedPoints = rotateY(points, rotationAngle);

    //Move the rotated point 2.0 units away from the camera
    rotatedPoints.z += 2.0f;

    rotated3DPoints.push_back(rotatedPoints);
}

// Project all points from 3D to 2D
std::vector<Vec3> projected2DPoints;
for(int i = 0; i < numPoints; i++) {
    projected2DPoints.push_back(project(rotated3DPoints[i]));
    
}

// Loop through all 12 triangles in our Index Buffer
for (int i = 0; i < modelTriangles.size(); i++) {
    Triangle tri = modelTriangles[i];

    // Get the projected 2D coordinates for the 3 corners of this specific triangle
    Vec3 pA = projected2DPoints[tri.a];
    Vec3 pB = projected2DPoints[tri.b];
    Vec3 pC = projected2DPoints[tri.c];

    // Draw the solid triangle
    FillTriangle(pixels, zBuffer, pA, pB, pC, tri.colour);
}

//Update the texture by sending it to the CPU
SDL_UpdateTexture(
    texture,
    NULL,
    pixels.data(),
    SCREEN_WIDTH * 4
);

//Update the screen
SDL_RenderClear(renderer);
SDL_RenderTexture(renderer, texture, NULL, NULL); 
SDL_RenderPresent(renderer);
        
    }
//Destroy the window and exit
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

/*
 * Function to update a pixel in the framebuffer with a specific colour.
 * @param x The x-coordinate of the pixel to update.
 * @param y The y-coordinate of the pixel to update.
 * @param colour The colour value to set the pixel to (in ARGB format).
*/
void updatePixel(std::vector<uint32_t>& pixels, int x, int y, uint32_t colour) {
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    //Convert the pixel data into a 1D format
    int index = (y * SCREEN_WIDTH) + x;

    pixels[index] = colour;
}

/*
 * Function to draw a line.
 * @param x0 The x-coordinate of the beginning of the line.
 * @param y0 The y-coordinate of the beginning of the line.
 * @param x1 The x-coordinate of the end of the line.
 * @param y1 The y-coordinate of the end of the line.
 * @param colour The colour value to set the pixel to (in ARGB format).
*/
void drawLine(std::vector<uint32_t>& pixels, int x0, int y0, int x1, int y1, uint32_t colour) {

    //Calculate the absolute distance of each coordinate
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);

    //Determine which direction to step in (+1 or -1)
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    //Initialize the error
    int error = dx + dy;

    while (true) {
    updatePixel(pixels, x0, y0, colour);

    // Break when the exact destination is reached
    if (x0 == x1 && y0 == y1) {
        break;
    }

    int e2 = 2 * error;

    if (e2 >= dy) {
        if (x0 == x1) break; // Extra safety check
        error += dy;
        x0 += sx;
    }
    if (e2 <= dx) {
        if (y0 == y1) break; // Extra safety check
        error += dx;
        y0 += sy;
    }
}
}

Vec3 project(Vec3 point) {

    Vec3 screenCoord;
    float fov = 128.0f;

    //Ensure not to divide by 0
    if(point.z == 0.0f) {
        point.z = 0.00001f;
    }

    //Project the 3D points onto the 2D plane
    //Round to try to make sure that there's no gaps in the mesh
    screenCoord.x = (point.x * fov / point.z) + SCREEN_WIDTH / 2;
    screenCoord.y = (point.y * fov / point.z) + SCREEN_HEIGHT / 2;
    screenCoord.z = point.z;

    return screenCoord;
}
/*
* Function to rotate around the y
* @param point The point you are moving
* @param angle The angle you are shifting
*/
Vec3 rotateY(Vec3 point, float angle) {
    //Calculate the new x and z values
    float newX = (point.x * std::cos(angle)) + (point.z * std::sin(angle));
    float newZ = (-1 * point.x * std::sin(angle)) + (point.z * std::cos(angle));

    point.x = newX;
    point.z = newZ;

    return point;
}
/*
* Function To rotate around the x
* @param point The point you are moving
* @param angle The angle you are shifting
*/
Vec3 rotateX(Vec3 point, float angle) {
    //Calculate the new y and z values
    float newY = (point.y * std::cos(angle)) - (point.z * std::sin(angle));
    float newZ = (point.y * std::sin(angle)) + (point.z * std::cos(angle));

    point.y = newY;
    point.z = newZ;

    return point;
}

/*
 * Function to calculate the 2D cross-product of a line and a point.
 * Used to determine if a pixel is to the left, right, or on the line.
 * @param a The starting vertex of the edge.
 * @param b The ending vertex of the edge.
 * @param c The pixel coordinate being tested.
 * @return Positive if left of the line, negative if right, 0 if exactly on the line.
 */
float edgeFunction(Vec2 a, Vec2 b, Vec2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/*
 * Function to rasterize a solid triangle using Barycentric coordinates.
 * @param pixels The framebuffer array to draw to.
 * @param v0 The first vertex of the triangle (must be counter-clockwise).
 * @param v1 The second vertex of the triangle.
 * @param v2 The third vertex of the triangle.
 * @param colour The colour value to fill the triangle with (in ARGB format).
 */
void FillTriangle(std::vector<uint32_t>& pixels, std::vector<float>& zBuffer, Vec3 v0, Vec3 v1, Vec3 v2, uint32_t colour) {

    //2D vectors for the math that doesn't require 3D
    Vec2 p0 = {v0.x, v0.y};
    Vec2 p1 = {v1.x, v1.y};
    Vec2 p2 = {v2.x, v2.y};

    //Find the bounding box
    int minX = std::min({v0.x, v1.x, v2.x});
    int minY = std::min({v0.y, v1.y, v2.y});
    int maxX = std::max({v0.x, v1.x, v2.x});
    int maxY = std::max({v0.y, v1.y, v2.y});

    //Ensure the bounding box is within the screen boundaries
    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(SCREEN_WIDTH - 1, maxX);
    maxY = std::min(SCREEN_HEIGHT - 1, maxY);

    //Find the area of the bounding box
    float area = edgeFunction(p0,p1,p2);
    
    // Run through all values in the bounding box
    for(int x = minX; x <= maxX; x++) {
        for(int y = minY; y <= maxY; y++) {

            Vec2 point = {(float)x, (float)y};

            // Check each value with the upgraded float edge function
            float test0 = edgeFunction(p1, p2, point);
            float test1 = edgeFunction(p2, p0, point);
            float test2 = edgeFunction(p0, p1, point);

            // A pixel is inside if it passes the CCW test OR the CW test
            bool isCCW = (test0 >= 0 && test1 >= 0 && test2 >= 0);
            bool isCW  = (test0 <= 0 && test1 <= 0 && test2 <= 0);

            if(isCCW || isCW) {
                
                // Safety check: Prevent division by zero if a triangle is perfectly flat
                if (area == 0.0f) continue;

                // Find the depth of each pixel
                float alpha = test0 / area;
                float beta  = test1 / area;
                float gamma = test2 / area;

                //Find the z reciprocals to fix depth
                float z0 = 1.0f / v0.z;
                float z1 = 1.0f / v1.z;
                float z2 = 1.0f / v2.z;

                float pixelDepth = 1.0f / ((z0 * alpha) + (z1 * beta) + (z2 * gamma));
                int index = (y * SCREEN_WIDTH) + x;

                // Overwrite pixels that are behind
                if(pixelDepth < zBuffer[index]) {
                    zBuffer[index] = pixelDepth;
                    updatePixel(pixels, x, y, colour);
                }
            }
        }
    }
}