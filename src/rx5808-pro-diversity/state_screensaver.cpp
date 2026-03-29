#include <Arduino.h>
#include <math.h>

#include "state_screensaver.h"

#include "receiver.h"
#include "channels.h"
#include "buttons.h"
#include "state.h"
#include "ui.h"
#include "settings_eeprom.h"

const int CENTER_X = SCREEN_WIDTH / 2;
const int CENTER_Y = SCREEN_HEIGHT / 2;
static float currentAngle = 0.0;
static uint32_t lastDrawTime = 0;
static GFXcanvas16* canvas = nullptr;

// ==============================================================================
// Helper Function for Canvas Dashed Lines
// ==============================================================================
void drawCanvasDashedVLine(int x, int y, int h, int stepSize) {
    if (!canvas) return;
    for (int i = 0; i < h; i += stepSize * 2) {
        int lineH = min(stepSize, h - i);
        canvas->drawFastVLine(x, y + i, lineH, TFT_DARKGREY);
    }
}

// ==============================================================================
// 1. ORIGINAL SPINNING CUBE MATH & DRAWING (Buffered & Depth Sorted)
// ==============================================================================
struct Point3D { float x, y, z; };
struct Point2D { int x, y; };

const float FOV_CUBE = 64.0;

Point2D projectCube(Point3D p) {
    Point2D p2d;
    float z = p.z + 100.0; // Push object into the distance
    p2d.x = (p.x * FOV_CUBE) / z + CENTER_X;
    p2d.y = (p.y * FOV_CUBE) / z + CENTER_Y;
    return p2d;
}

Point3D rotateYCube(Point3D p, float angle) {
    Point3D rotated;
    rotated.x = p.x * cos(angle) - p.z * sin(angle);
    rotated.y = p.y;
    rotated.z = p.x * sin(angle) + p.z * cos(angle);
    return rotated;
}

void drawWireframeBoxCube(float angle) {
    if (!canvas) return;
    
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
        p[i] = projectCube(rotateYCube(corners[i], angle));
    }

    uint16_t boxColor = TFT_DARKGREY; 
    
    // 1. Draw Top Face
    canvas->drawLine(p[2].x, p[2].y, p[3].x, p[3].y, boxColor);
    canvas->drawLine(p[6].x, p[6].y, p[7].x, p[7].y, boxColor);
    canvas->drawLine(p[2].x, p[2].y, p[6].x, p[6].y, boxColor);
    canvas->drawLine(p[3].x, p[3].y, p[7].x, p[7].y, boxColor);

    // 2. Draw Bottom Face
    canvas->drawLine(p[0].x, p[0].y, p[1].x, p[1].y, boxColor);
    canvas->drawLine(p[4].x, p[4].y, p[5].x, p[5].y, boxColor);
    canvas->drawLine(p[0].x, p[0].y, p[4].x, p[4].y, boxColor);
    canvas->drawLine(p[1].x, p[1].y, p[5].x, p[5].y, boxColor);
    
    int stepSize = 4; 

    drawCanvasDashedVLine(p[0].x, min(p[0].y, p[3].y), abs(p[3].y - p[0].y), stepSize); // Front Right
    drawCanvasDashedVLine(p[1].x, min(p[1].y, p[2].y), abs(p[2].y - p[1].y), stepSize); // Front Left
    drawCanvasDashedVLine(p[4].x, min(p[4].y, p[7].y), abs(p[7].y - p[4].y), stepSize); // Back Right
    drawCanvasDashedVLine(p[5].x, min(p[5].y, p[6].y), abs(p[6].y - p[5].y), stepSize); // Back Left
}

void draw3DGridCube(float angle) {
    if (!canvas) return;
    uint16_t dotColor = TFT_LIGHTGREY; 

    // Scaled grid loops to match the new 90x90x90 cube bounds
    // We step by 45.0 to give us a clean 3x3x3 grid of dots (27 dots total)
    for (float x = -45.0; x <= 45.0; x += 45.0) {
        for (float y = -45.0; y <= 45.0; y += 45.0) {
            for (float z = -45.0; z <= 45.0; z += 45.0) {
                
                Point3D p = { x, y, z };
                p = rotateYCube(p, angle);
                Point2D proj = projectCube(p);
                
                canvas->fillRect(proj.x, proj.y, 2, 2, dotColor);
            }
        }
    }
}

void drawSpinningGraphCube(const uint8_t rxDataA[], const uint8_t rxDataB[], int dataSize) {
    if (!canvas) return;
    drawWireframeBoxCube(currentAngle);
    draw3DGridCube(currentAngle);
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

        pA = rotateYCube(pA, currentAngle);
        pB = rotateYCube(pB, currentAngle);

        Point2D projA = projectCube(pA);
        Point2D projB = projectCube(pB);

        if (i > 0) {
            #ifdef USE_DIVERSITY
                // Z-Depth Sorting (Painter's Algorithm)
                // Higher Z value means it is further away from the camera.
                // We draw the further line first so the closer line seamlessly overlaps it.
                if (pA.z > pB.z) {
                    canvas->drawLine(lastProjA.x, lastProjA.y, projA.x, projA.y, TFT_YELLOW);
                    canvas->drawLine(lastProjB.x, lastProjB.y, projB.x, projB.y, TFT_CYAN);
                } else {
                    canvas->drawLine(lastProjB.x, lastProjB.y, projB.x, projB.y, TFT_CYAN);
                    canvas->drawLine(lastProjA.x, lastProjA.y, projA.x, projA.y, TFT_YELLOW);
                }
            #else
                canvas->drawLine(lastProjA.x, lastProjA.y, projA.x, projA.y, TFT_YELLOW);
            #endif
        }

        lastProjA = projA;
        lastProjB = projB;
    }
}

// ==============================================================================
// 2. NEW CARTESIAN 3D ENGINE & DRAWING (Solid Tubes)
// ==============================================================================
const float FOV_CARTESIAN = 100.0;

Point2D projectCartesian(Point3D p) {
    Point2D p2d;
    float distance = p.y + 250.0; 
    if (distance < 1.0) distance = 1.0; 

    p2d.x = CENTER_X + (p.x * FOV_CARTESIAN) / distance;
    p2d.y = CENTER_Y - (p.z * FOV_CARTESIAN) / distance;
    
    return p2d;
}

void fillQuadCartesian(Point2D p1, Point2D p2, Point2D p3, Point2D p4, uint16_t color) {
    if (!canvas) return;
    canvas->fillTriangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, color);
    canvas->fillTriangle(p1.x, p1.y, p3.x, p3.y, p4.x, p4.y, color);
}

void drawCartesianTrail(const uint8_t rxData[], int dataSize, float yCenter, uint16_t colorFront, uint16_t colorTop, uint16_t colorRight) {
    if (!canvas) return;

    float s_y = 12.0; 
    float s_z = 12.0; 
    
    float yaw = -0.40;   
    float pitch = 0.50; 

    for (int i = 0; i < dataSize - 1; i++) {
        float cx1 = -110.0 + ((float)i / (float)(dataSize - 1)) * 220.0;
        float cx2 = -110.0 + ((float)(i + 1) / (float)(dataSize - 1)) * 220.0;

        float z1 = -40.0 + (rxData[i] / 100.0) * 80.0;
        float z2 = -40.0 + (rxData[i+1] / 100.0) * 80.0;

        Point3D c3d[8] = {
            { cx1, yCenter + s_y, z1 + s_z }, 
            { cx2, yCenter + s_y, z2 + s_z }, 
            { cx2, yCenter - s_y, z2 + s_z }, 
            { cx1, yCenter - s_y, z1 + s_z }, 
            { cx1, yCenter + s_y, z1 - s_z }, 
            { cx2, yCenter + s_y, z2 - s_z }, 
            { cx2, yCenter - s_y, z2 - s_z }, 
            { cx1, yCenter - s_y, z1 - s_z }  
        };

        Point2D p[8];
        for (int c = 0; c < 8; c++) {
            Point3D r = c3d[c];
            
            float x1 = r.x * cos(yaw) - r.y * sin(yaw);
            float y1 = r.x * sin(yaw) + r.y * cos(yaw);
            r.x = x1;
            r.y = y1;
            
            float y2 = r.y * cos(pitch) - r.z * sin(pitch);
            float z2 = r.y * sin(pitch) + r.z * cos(pitch);
            r.y = y2;
            r.z = z2;

            p[c] = projectCartesian(r);
        }

        fillQuadCartesian(p[0], p[1], p[2], p[3], colorTop);
        fillQuadCartesian(p[3], p[2], p[6], p[7], colorFront);

        canvas->drawLine(p[0].x, p[0].y, p[1].x, p[1].y, TFT_DARKGREY); 
        canvas->drawLine(p[3].x, p[3].y, p[2].x, p[2].y, TFT_BLACK);    
        canvas->drawLine(p[7].x, p[7].y, p[6].x, p[6].y, TFT_BLACK);    

        if (i == dataSize - 2) {
            fillQuadCartesian(p[1], p[2], p[6], p[5], colorRight);
            
            canvas->drawLine(p[1].x, p[1].y, p[2].x, p[2].y, TFT_BLACK);
            canvas->drawLine(p[2].x, p[2].y, p[6].x, p[6].y, TFT_BLACK);
            canvas->drawLine(p[6].x, p[6].y, p[5].x, p[5].y, TFT_BLACK);
            canvas->drawLine(p[5].x, p[5].y, p[1].x, p[1].y, TFT_BLACK);
        }
    }
}

// ==============================================================================
// 3. The State Machine Handlers 
// ==============================================================================

void StateMachine::ScreensaverStateHandler::onEnter() {
    currentAngle = 0.0;
    lastDrawTime = millis();
    if (!canvas) {
        canvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void StateMachine::ScreensaverStateHandler::onUpdate() {
    // Calculate Delta Time (seconds since last frame)
    uint32_t now = millis();
    float dt = (now - lastDrawTime) / 1000.0f; 
    lastDrawTime = now;

    // 10 RPM = 10 Revolutions per 60 Seconds = 60 degrees/sec = ~1.047 radians/sec
    currentAngle += dt * 1.04719f; 
    if (currentAngle > 6.28318f) currentAngle -= 6.28318f; 

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

    // We clear the double buffer instead of the physical screen
    canvas->fillScreen(TFT_BLACK);

    if (EepromSettings.screensaverStyle == 0) {
        // --- SCENE 1: ORIGINAL SPINNING CUBE ---
        #ifdef USE_DIVERSITY
            drawSpinningGraphCube(Receiver::rssiALast, Receiver::rssiBLast, RECEIVER_LAST_DATA_SIZE);
        #else
            drawSpinningGraphCube(Receiver::rssiALast, Receiver::rssiALast, RECEIVER_LAST_DATA_SIZE); 
        #endif
        
    } else {
        // --- SCENE 2: CARTESIAN EXTRUDED RSSI TUBES ---
        #ifdef USE_DIVERSITY
            drawCartesianTrail(Receiver::rssiBLast, RECEIVER_LAST_DATA_SIZE, 40.0, 
                TFT_CYAN, 0x05F7, 0x03EF);
        #endif

        drawCartesianTrail(Receiver::rssiALast, RECEIVER_LAST_DATA_SIZE, -40.0, 
            TFT_YELLOW, 0xBDE0, 0x7BE0);
    }
    
    // Blast the fully rendered buffer to the physical screen all at once
    Ui::display.drawRGBBitmap(0, 0, canvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    Ui::needDisplay();
}