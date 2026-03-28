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

// Global angle for the screensaver animation
static float currentAngle = 0.0;

// ==============================================================================
// 2. 3D Drawing Functions
// ==============================================================================
void drawWireframeBox(float angle) {
    // Equalized dimensions for a perfect 90x90x90 cube
    float w = 45.0; // Width
    float h = 45.0; // Height 
    float d = 45.0; // Depth 

    Point3D corners[8] = {
        // Front Face (0: Top Right, 1: Top Left, 2: Bottom Left, 3: Bottom Right)
        { w,  h,  d}, {-w,  h,  d}, {-w, -h,  d}, { w, -h,  d},
        // Back Face (4: Top Right, 5: Top Left, 6: Bottom Left, 7: Bottom Right)
        { w,  h, -d}, {-w,  h, -d}, {-w, -h, -d}, { w, -h, -d}
    };

    Point2D p[8];
    for (int i = 0; i < 8; i++) {
        p[i] = project(rotateY(corners[i], angle));
    }

    uint16_t boxColor = TFT_DARKGREY; 
    
    // 1. Draw Top Face
    Ui::display.drawLine(p[2].x, p[2].y, p[3].x, p[3].y, boxColor);
    Ui::display.drawLine(p[6].x, p[6].y, p[7].x, p[7].y, boxColor);
    Ui::display.drawLine(p[2].x, p[2].y, p[6].x, p[6].y, boxColor);
    Ui::display.drawLine(p[3].x, p[3].y, p[7].x, p[7].y, boxColor);

    // 2. Draw Bottom Face
    Ui::display.drawLine(p[0].x, p[0].y, p[1].x, p[1].y, boxColor);
    Ui::display.drawLine(p[4].x, p[4].y, p[5].x, p[5].y, boxColor);
    Ui::display.drawLine(p[0].x, p[0].y, p[4].x, p[4].y, boxColor);
    Ui::display.drawLine(p[1].x, p[1].y, p[5].x, p[5].y, boxColor);
    
    int stepSize = 4; 

    Ui::drawDashedVLine(p[0].x, min(p[0].y, p[3].y), abs(p[3].y - p[0].y), stepSize); // Front Right
    Ui::drawDashedVLine(p[1].x, min(p[1].y, p[2].y), abs(p[2].y - p[1].y), stepSize); // Front Left
    Ui::drawDashedVLine(p[4].x, min(p[4].y, p[7].y), abs(p[7].y - p[4].y), stepSize); // Back Right
    Ui::drawDashedVLine(p[5].x, min(p[5].y, p[6].y), abs(p[6].y - p[5].y), stepSize); // Back Left
}

void draw3DGrid(float angle) {
    uint16_t dotColor = TFT_LIGHTGREY; 

    // Scaled grid loops to match the new 90x90x90 cube bounds
    // We step by 45.0 to give us a clean 3x3x3 grid of dots (27 dots total)
    for (float x = -45.0; x <= 45.0; x += 45.0) {
        for (float y = -45.0; y <= 45.0; y += 45.0) {
            for (float z = -45.0; z <= 45.0; z += 45.0) {
                
                Point3D p = { x, y, z };
                p = rotateY(p, angle);
                Point2D proj = project(p);
                
                Ui::display.fillRect(proj.x, proj.y, 2, 2, dotColor);
            }
        }
    }
}

void drawSpinningGraph(const uint8_t rxDataA[], const uint8_t rxDataB[], int dataSize) {
    drawWireframeBox(currentAngle);
    draw3DGrid(currentAngle);
    Point2D lastProjA, lastProjB;

    for (int i = 0; i < dataSize; i++) {
        // Squished the X scaling to fit the new width of 90 (offset -45)
        float xPos = ((float)i / (float)(dataSize - 1)) * 90.0 - 45.0;
        
        // Height scaling stays at 90 to match the cube
        float yPosA = 45.0 - ((float)rxDataA[i] / 100.0) * 90.0;
        float yPosB = 0;
        
        #ifdef USE_DIVERSITY
            yPosB = 45.0 - ((float)rxDataB[i] / 100.0) * 90.0;
        #endif

        // Pushed depth out to +/- 20 to take advantage of the deeper cube space!
        Point3D pA = { xPos, yPosA, 20.0 };
        Point3D pB = { xPos, yPosB, -20.0 };

        pA = rotateY(pA, currentAngle);
        pB = rotateY(pB, currentAngle);

        Point2D projA = project(pA);
        Point2D projB = project(pB);

        if (i > 0) {
            Ui::display.drawLine(lastProjA.x, lastProjA.y, projA.x, projA.y, TFT_YELLOW);
            #ifdef USE_DIVERSITY
                Ui::display.drawLine(lastProjB.x, lastProjB.y, projB.x, projB.y, TFT_CYAN);
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
}

void StateMachine::ScreensaverStateHandler::onUpdate() {
    // Advance the rotation slightly every frame
    currentAngle += 0.05; 
    if (currentAngle > 6.28) currentAngle = 0; 

    // Tell the UI it needs to draw the next frame of the animation
    Ui::needUpdate(); 
}

void StateMachine::ScreensaverStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    // Any button press exits the screensaver
    StateMachine::switchState(StateMachine::lastState);
}

void StateMachine::ScreensaverStateHandler::onInitialDraw() {
    Ui::clear();
    
    #ifdef USE_DIVERSITY
        drawSpinningGraph(Receiver::rssiALast, Receiver::rssiBLast, RECEIVER_LAST_DATA_SIZE);
    #else
        // Fallback if diversity is disabled
        drawSpinningGraph(Receiver::rssiALast, Receiver::rssiALast, RECEIVER_LAST_DATA_SIZE); 
    #endif
    
    Ui::needDisplay();
}

void StateMachine::ScreensaverStateHandler::onUpdateDraw() {
    this->onInitialDraw();
}