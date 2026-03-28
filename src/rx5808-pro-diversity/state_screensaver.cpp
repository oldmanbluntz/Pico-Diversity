#include <Arduino.h>
#include <math.h>

#include "state_screensaver.h"

#include "receiver.h"
#include "channels.h"
#include "buttons.h"
#include "state.h"
#include "ui.h"

// ==============================================================================
// 1. 3D Engine Math & Structures
// ==============================================================================
struct Point3D { float x, y, z; };
struct Point2D { int x, y; };

const float FOV = 64.0;
const int CENTER_X = SCREEN_WIDTH / 2;
const int CENTER_Y = SCREEN_HEIGHT / 2;

Point2D project(Point3D p) {
    Point2D p2d;
    float z = p.z + 100.0; // Push object into the distance
    p2d.x = (p.x * FOV) / z + CENTER_X;
    p2d.y = (p.y * FOV) / z + CENTER_Y;
    return p2d;
}

Point3D rotateY(Point3D p, float angle) {
    Point3D rotated;
    rotated.x = p.x * cos(angle) - p.z * sin(angle);
    rotated.y = p.y;
    rotated.z = p.x * sin(angle) + p.z * cos(angle);
    return rotated;
}

const float CAMERA_TILT = 0.4; // Radians. 0.4 is roughly a 23-degree downward tilt.

Point3D rotateX(Point3D p, float angle) {
    Point3D rotated;
    rotated.x = p.x;
    rotated.y = p.y * cos(angle) - p.z * sin(angle);
    rotated.z = p.y * sin(angle) + p.z * cos(angle);
    return rotated;
}

static float currentAngle = 0.0;

// Allocate a pointer for our 64KB Double Buffer Canvas
static GFXcanvas16* canvas = nullptr;

// ==============================================================================
// 2. 3D Drawing Functions (Now drawing to the Canvas!)
// ==============================================================================
void drawWireframeBox(float angle, uint16_t color) {
    float w = 45.0, h = 45.0, d = 45.0;

    Point3D corners[8] = {
        { w,  h,  d}, {-w,  h,  d}, {-w, -h,  d}, { w, -h,  d},
        { w,  h, -d}, {-w,  h, -d}, {-w, -h, -d}, { w, -h, -d}
    };

    Point2D p[8];
    for (int i = 0; i < 8; i++) {
        Point3D rotated = rotateY(corners[i], angle);
        rotated = rotateX(rotated, CAMERA_TILT); // Apply downward pitch
        p[i] = project(rotated);
    }

    // Front Face
    canvas->drawLine(p[0].x, p[0].y, p[1].x, p[1].y, color);
    canvas->drawLine(p[1].x, p[1].y, p[2].x, p[2].y, color);
    canvas->drawLine(p[2].x, p[2].y, p[3].x, p[3].y, color);
    canvas->drawLine(p[3].x, p[3].y, p[0].x, p[0].y, color);
    
    // Back Face
    canvas->drawLine(p[4].x, p[4].y, p[5].x, p[5].y, color);
    canvas->drawLine(p[5].x, p[5].y, p[6].x, p[6].y, color);
    canvas->drawLine(p[6].x, p[6].y, p[7].x, p[7].y, color);
    canvas->drawLine(p[7].x, p[7].y, p[4].x, p[4].y, color);
    
    // Connecting Lines
    canvas->drawLine(p[0].x, p[0].y, p[4].x, p[4].y, color);
    canvas->drawLine(p[1].x, p[1].y, p[5].x, p[5].y, color);
    canvas->drawLine(p[2].x, p[2].y, p[6].x, p[6].y, color);
    canvas->drawLine(p[3].x, p[3].y, p[7].x, p[7].y, color);
}

void draw3DGrid(float angle, uint16_t color) {
    for (float x = -45.0; x <= 45.0; x += 45.0) {
        for (float y = -45.0; y <= 45.0; y += 45.0) {
            for (float z = -45.0; z <= 45.0; z += 45.0) {
                Point3D p = { x, y, z };
                p = rotateY(p, angle);
                p = rotateX(p, CAMERA_TILT); // Apply downward pitch
                Point2D proj = project(p);
                canvas->fillRect(proj.x, proj.y, 2, 2, color);
            }
        }
    }
}

void drawGraphData(float angle, const uint8_t rxDataA[], const uint8_t rxDataB[], int dataSize, uint16_t colorA, uint16_t colorB) {
    Point2D lastProjA, lastProjB;

    for (int i = 0; i < dataSize; i++) {
        float xPos = ((float)i / (float)(dataSize - 1)) * 90.0 - 45.0;
        float yPosA = 45.0 - ((float)rxDataA[i] / 100.0) * 90.0;
        float yPosB = 0;
        
        #ifdef USE_DIVERSITY
            yPosB = 45.0 - ((float)rxDataB[i] / 100.0) * 90.0;
        #endif

        Point3D pA = { xPos, yPosA, 20.0 };
        Point3D pB = { xPos, yPosB, -20.0 };

        pA = rotateY(pA, angle);
        pB = rotateY(pB, angle);

        pA = rotateX(pA, CAMERA_TILT); // Apply downward pitch
        pB = rotateX(pB, CAMERA_TILT); // Apply downward pitch

        Point2D projA = project(pA);
        Point2D projB = project(pB);

        if (i > 0) {
            canvas->drawLine(lastProjA.x, lastProjA.y, projA.x, projA.y, colorA);
            #ifdef USE_DIVERSITY
                canvas->drawLine(lastProjB.x, lastProjB.y, projB.x, projB.y, colorB);
            #endif
        }

        lastProjA = projA;
        lastProjB = projB;
    }
}

// ==============================================================================
// 3. The State Machine Handlers 
// ==============================================================================

void StateMachine::ScreensaverStateHandler::onEnter() {
    currentAngle = 0.0;

    // Dynamically allocate the 64KB Canvas in memory the first time we enter the screensaver
    if (!canvas) {
        canvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void StateMachine::ScreensaverStateHandler::onUpdate() {
    Ui::needUpdate(); 
}

void StateMachine::ScreensaverStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    StateMachine::switchState(StateMachine::lastState);
}

void StateMachine::ScreensaverStateHandler::onInitialDraw() {
    Ui::needDisplay();
}

void StateMachine::ScreensaverStateHandler::onUpdateDraw() {
    if (!canvas) return; // Safety check

    // 1. Wipe the invisible canvas clean
    canvas->fillScreen(TFT_BLACK);

    // 2. Advance rotation math
    currentAngle += 0.05; 
    if (currentAngle > 6.28) currentAngle = 0; 

    // 3. Draw the new frame completely onto the invisible canvas
    drawWireframeBox(currentAngle, TFT_RED);
    draw3DGrid(currentAngle, TFT_BLUE);
    
    #ifdef USE_DIVERSITY
        drawGraphData(currentAngle, Receiver::rssiALast, Receiver::rssiBLast, RECEIVER_LAST_DATA_SIZE, TFT_GREEN, TFT_ORANGE);
    #else
        drawGraphData(currentAngle, Receiver::rssiALast, Receiver::rssiALast, RECEIVER_LAST_DATA_SIZE, TFT_YELLOW, TFT_YELLOW);
    #endif

    // 4. Blast the fully rendered canvas to the physical screen in one shot!
    Ui::display.drawRGBBitmap(0, 0, canvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    
    Ui::needDisplay();
}