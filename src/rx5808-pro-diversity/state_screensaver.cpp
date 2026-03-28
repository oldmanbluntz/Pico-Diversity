#include <Arduino.h>
#include <math.h>

#include "state_screensaver.h"

#include "receiver.h"
#include "channels.h"
#include "buttons.h"
#include "state.h"
#include "ui.h"
#include "settings_eeprom.h"

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
// 2. 3D Drawing Functions
// ==============================================================================

// Helper to draw a filled 4-sided polygon using two triangles
void fillQuad(Point2D p1, Point2D p2, Point2D p3, Point2D p4, uint16_t color) {
    canvas->fillTriangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, color);
    canvas->fillTriangle(p1.x, p1.y, p3.x, p3.y, p4.x, p4.y, color);
}

void drawWireframeBox(float angle, uint16_t color) {
    float w = 45.0, h = 45.0, d = 45.0;

    Point3D corners[8] = {
        { w,  h,  d}, {-w,  h,  d}, {-w, -h,  d}, { w, -h,  d},
        { w,  h, -d}, {-w,  h, -d}, {-w, -h, -d}, { w, -h, -d}
    };

    Point2D p[8];
    for (int i = 0; i < 8; i++) {
        Point3D rotated = rotateY(corners[i], angle);
        rotated = rotateX(rotated, CAMERA_TILT); 
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
                p = rotateX(p, CAMERA_TILT); 
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

        pA = rotateX(rotateY(pA, angle), CAMERA_TILT);
        pB = rotateX(rotateY(pB, angle), CAMERA_TILT);

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

// Draws a solid 3D scrolling box/wall in a uniform color
void drawScrolling3DBox(const uint8_t rxData[], int dataSize, float zCenter, uint16_t colorMain, uint16_t colorDark) {
    float staticAngle = -0.15; 
    float totalWidth = 340.0;
    
    float hd = 10.0;    // Half-depth (thickness of the box on the Z-axis)
    float yBase = 45.0; // Floor level to create the solid block feel

    for (int i = 0; i < dataSize - 1; i++) {
        float cx1 = ((float)i / (float)(dataSize - 1)) * totalWidth - (totalWidth / 2.0);
        float cx2 = ((float)(i + 1) / (float)(dataSize - 1)) * totalWidth - (totalWidth / 2.0);

        float h1 = (rxData[i] / 100.0) * 80.0;
        if (h1 < 1.0) h1 = 1.0;
        float y1 = 45.0 - h1;

        float h2 = (rxData[i+1] / 100.0) * 80.0;
        if (h2 < 1.0) h2 = 1.0;
        float y2 = 45.0 - h2;

        float zFront = zCenter - hd;
        float zBack = zCenter + hd;

        // The 8 corners of the solid block segment
        Point3D c3d[8] = {
            { cx1, y1, zBack },     // 0: Top Back Left
            { cx2, y2, zBack },     // 1: Top Back Right
            { cx2, y2, zFront },    // 2: Top Front Right
            { cx1, y1, zFront },    // 3: Top Front Left
            { cx1, yBase, zFront }, // 4: Bottom Front Left
            { cx2, yBase, zFront }, // 5: Bottom Front Right
            { cx2, yBase, zBack },  // 6: Bottom Back Right
            { cx1, yBase, zBack }   // 7: Bottom Back Left
        };

        Point2D p[8];
        for (int c = 0; c < 8; c++) {
            p[c] = project(rotateX(rotateY(c3d[c], staticAngle), CAMERA_TILT));
        }

        // 1. Right Side Face (Shadow for depth perception)
        fillQuad(p[1], p[2], p[5], p[6], colorDark);

        // 2. Top Face
        fillQuad(p[0], p[1], p[2], p[3], colorMain);

        // 3. Front Face
        fillQuad(p[3], p[2], p[5], p[4], colorMain);
    }
}

// ==============================================================================
// 3. The State Machine Handlers 
// ==============================================================================

void StateMachine::ScreensaverStateHandler::onEnter() {
    currentAngle = 0.0;

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
    if (!canvas) return; 

    canvas->fillScreen(TFT_BLACK);

    // Branch drawing logic based on EEPROM Selection
    if (EepromSettings.screensaverStyle == 0) {
        // --- SCENE 1: SPINNING CUBE ---
        currentAngle += 0.05; 
        if (currentAngle > 6.28) currentAngle = 0; 

        drawWireframeBox(currentAngle, TFT_RED);
        draw3DGrid(currentAngle, TFT_BLUE);
        
        #ifdef USE_DIVERSITY
            drawGraphData(currentAngle, Receiver::rssiALast, Receiver::rssiBLast, RECEIVER_LAST_DATA_SIZE, TFT_GREEN, TFT_ORANGE);
        #else
            drawGraphData(currentAngle, Receiver::rssiALast, Receiver::rssiALast, RECEIVER_LAST_DATA_SIZE, TFT_YELLOW, TFT_YELLOW);
        #endif

    } else {
        // --- SCENE 2: SOLID SCROLLING 3D BOXES ---
        #ifdef USE_DIVERSITY
            // RXB (BACK LAYER): Center Z = 20.0
            // Main = Cyan, Side Shadow = Blue
            drawScrolling3DBox(Receiver::rssiBLast, RECEIVER_LAST_DATA_SIZE, 20.0, TFT_CYAN, TFT_BLUE);
        #endif

        // RXA (FRONT LAYER): Center Z = -10.0
        // Main = Yellow, Side Shadow = Orange
        drawScrolling3DBox(Receiver::rssiALast, RECEIVER_LAST_DATA_SIZE, -10.0, TFT_YELLOW, 0xFDA0);
    }

    // Blast the fully rendered canvas to the physical screen
    Ui::display.drawRGBBitmap(0, 0, canvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    
    Ui::needDisplay();
}