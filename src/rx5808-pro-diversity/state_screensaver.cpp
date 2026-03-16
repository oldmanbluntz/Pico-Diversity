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
    float w = 60.0; // Width 
    float h = 30.0; // Height 
    float d = 20.0; // Depth 

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
    // Choose a subtle color for the background dots
    uint16_t dotColor = TFT_LIGHTGREY; 

    // Loop through 3D space to create a 5x3x3 matrix of dots (45 dots total)
    // X goes from Left to Right (-60 to +60 in steps of 30)
    for (float x = -60.0; x <= 60.0; x += 30.0) {
        
        // Y goes from Bottom to Top (-30 to +30 in steps of 30)
        for (float y = -30.0; y <= 30.0; y += 30.0) {
            
            // Z goes from Front to Back (-20 to +20 in steps of 20)
            for (float z = -20.0; z <= 20.0; z += 20.0) {
                
                Point3D p = { x, y, z };
                
                // Rotate and project each dot
                p = rotateY(p, angle);
                Point2D proj = project(p);
                
                // Draw a small 2x2 pixel square so the dots are visible
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
        // Space the data points evenly across the width of our 3D box (-60 to +60)
        float xPos = ((float)i / (float)(dataSize - 1)) * 120.0 - 60.0;
        
        // Map RSSI amplitude to the height of our 3D box (-30 to +30)
        float yPosA = 30.0 - ((float)rxDataA[i] / 100.0) * 60.0;
        float yPosB = 0;
        
        #ifdef USE_DIVERSITY
            yPosB = 30.0 - ((float)rxDataB[i] / 100.0) * 60.0;
        #endif

        Point3D pA = { xPos, yPosA, 10.0 };
        Point3D pB = { xPos, yPosB, -10.0 };

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