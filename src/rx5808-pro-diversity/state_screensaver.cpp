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
// Shared 3D Helpers
// ==============================================================================
struct Point3D { float x, y, z; };
struct Point2D { int x, y; };

void drawCanvasDashedLine(int x0, int y0, int x1, int y1, uint16_t color) {
    if (!canvas) return;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist < 1.0f) return;
    
    int dashLength = 4;
    int steps = dist / dashLength;
    if (steps == 0) {
        canvas->drawLine(x0, y0, x1, y1, color);
        return;
    }
    
    for (int i = 0; i < steps; i += 2) {
        int px0 = x0 + (dx * i) / steps;
        int py0 = y0 + (dy * i) / steps;
        int px1 = x0 + (dx * (i + 1)) / steps;
        int py1 = y0 + (dy * (i + 1)) / steps;
        canvas->drawLine(px0, py0, px1, py1, color);
    }
}

void fillQuad(Point2D p1, Point2D p2, Point2D p3, Point2D p4, uint16_t color) {
    if (!canvas) return;
    canvas->fillTriangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, color);
    canvas->fillTriangle(p1.x, p1.y, p3.x, p3.y, p4.x, p4.y, color);
}

// ==============================================================================
// 1. SPINNING 3D RECTANGLE (Buffered, Pitched, 360-Solid Blocks)
// ==============================================================================
const float FOV_CUBE = 64.0;

const float CUBE_W = 75.0; 
const float CUBE_H = 35.0; 
const float CUBE_D = 35.0; 

Point2D projectCube(Point3D p) {
    Point2D p2d;
    float z = p.z + 100.0; 
    if (z < 1.0) z = 1.0;
    p2d.x = (p.x * FOV_CUBE) / z + CENTER_X;
    p2d.y = (p.y * FOV_CUBE) / z + CENTER_Y;
    return p2d;
}

Point3D transformCube(Point3D p, float angle) {
    float x1 = p.x * cos(angle) - p.z * sin(angle);
    float z1 = p.x * sin(angle) + p.z * cos(angle);
    float y1 = p.y;

    float pitch = 0.75; 
    float y2 = y1 * cos(pitch) - z1 * sin(pitch);
    float z2 = y1 * sin(pitch) + z1 * cos(pitch);

    return { x1, y2, z2 };
}

void drawWireframeBoxCube(float angle) {
    if (!canvas) return;

    Point3D corners[8] = {
        { CUBE_W,  CUBE_H,  CUBE_D}, {-CUBE_W,  CUBE_H,  CUBE_D}, {-CUBE_W, -CUBE_H,  CUBE_D}, { CUBE_W, -CUBE_H,  CUBE_D},
        { CUBE_W,  CUBE_H, -CUBE_D}, {-CUBE_W,  CUBE_H, -CUBE_D}, {-CUBE_W, -CUBE_H, -CUBE_D}, { CUBE_W, -CUBE_H, -CUBE_D}
    };

    Point2D p[8];
    for (int i = 0; i < 8; i++) {
        p[i] = projectCube(transformCube(corners[i], angle));
    }

    uint16_t boxColor = TFT_DARKGREY; 
    
    canvas->drawLine(p[2].x, p[2].y, p[3].x, p[3].y, boxColor);
    canvas->drawLine(p[6].x, p[6].y, p[7].x, p[7].y, boxColor);
    canvas->drawLine(p[2].x, p[2].y, p[6].x, p[6].y, boxColor);
    canvas->drawLine(p[3].x, p[3].y, p[7].x, p[7].y, boxColor);

    canvas->drawLine(p[0].x, p[0].y, p[1].x, p[1].y, boxColor);
    canvas->drawLine(p[4].x, p[4].y, p[5].x, p[5].y, boxColor);
    canvas->drawLine(p[0].x, p[0].y, p[4].x, p[4].y, boxColor);
    canvas->drawLine(p[1].x, p[1].y, p[5].x, p[5].y, boxColor);
    
    drawCanvasDashedLine(p[0].x, p[0].y, p[3].x, p[3].y, boxColor); 
    drawCanvasDashedLine(p[1].x, p[1].y, p[2].x, p[2].y, boxColor); 
    drawCanvasDashedLine(p[4].x, p[4].y, p[7].x, p[7].y, boxColor); 
    drawCanvasDashedLine(p[5].x, p[5].y, p[6].x, p[6].y, boxColor); 
}

void draw3DGridCube(float angle) {
    if (!canvas) return;
    uint16_t dotColor = TFT_LIGHTGREY; 

    for (float x = -CUBE_W; x <= CUBE_W; x += CUBE_W) {
        for (float y = -CUBE_H; y <= CUBE_H; y += CUBE_H) {
            for (float z = -CUBE_D; z <= CUBE_D; z += CUBE_D) {
                Point3D p = transformCube({ x, y, z }, angle);
                Point2D proj = projectCube(p);
                canvas->fillRect(proj.x, proj.y, 2, 2, dotColor);
            }
        }
    }
}

void drawCubeSolidTrail(const uint8_t rxData[], int dataSize, float zCenter, uint16_t colorFront, uint16_t colorTop, uint16_t colorSide) {
    if (!canvas) return;

    float s_h = 12.0; 
    float s_d = 12.0; 

    float max_y = CUBE_H - s_h - 1.0; 
    float min_y = -CUBE_H + s_h + 1.0; 

    bool drawLeftToRight = (sin(currentAngle) <= 0);
    bool drawFaceA = (cos(currentAngle) < 0);
    bool rightCapVisible = (sin(currentAngle) < 0);
    bool leftCapVisible = (sin(currentAngle) > 0);

    int start = drawLeftToRight ? 0 : dataSize - 2;
    int end = drawLeftToRight ? dataSize - 1 : -1;
    int step = drawLeftToRight ? 1 : -1;

    for (int i = start; i != end; i += step) {
        float cx1 = ((float)i / (float)(dataSize - 1)) * (CUBE_W * 2) - CUBE_W;
        float cx2 = ((float)(i + 1) / (float)(dataSize - 1)) * (CUBE_W * 2) - CUBE_W;

        float y1 = max_y - (rxData[i] / 100.0) * (max_y - min_y);
        float y2 = max_y - (rxData[i+1] / 100.0) * (max_y - min_y);

        Point3D c3d[8] = {
            { cx1, y1 - s_h, zCenter - s_d }, 
            { cx2, y2 - s_h, zCenter - s_d }, 
            { cx2, y2 - s_h, zCenter + s_d }, 
            { cx1, y1 - s_h, zCenter + s_d }, 
            { cx1, y1 + s_h, zCenter - s_d }, 
            { cx2, y2 + s_h, zCenter - s_d }, 
            { cx2, y2 + s_h, zCenter + s_d }, 
            { cx1, y1 + s_h, zCenter + s_d }  
        };

        Point2D p[8];
        for (int c = 0; c < 8; c++) {
            p[c] = projectCube(transformCube(c3d[c], currentAngle));
        }

        // 1. Draw end-caps FIRST so they get cleanly overlapped by the main faces
        if (i == dataSize - 2 && rightCapVisible) {
            fillQuad(p[1], p[2], p[6], p[5], colorSide);
            canvas->drawLine(p[1].x, p[1].y, p[2].x, p[2].y, TFT_BLACK);
            canvas->drawLine(p[2].x, p[2].y, p[6].x, p[6].y, TFT_BLACK);
            canvas->drawLine(p[6].x, p[6].y, p[5].x, p[5].y, TFT_BLACK);
            canvas->drawLine(p[5].x, p[5].y, p[1].x, p[1].y, TFT_BLACK);
        }
        if (i == 0 && leftCapVisible) {
            fillQuad(p[0], p[3], p[7], p[4], colorSide);
            canvas->drawLine(p[0].x, p[0].y, p[3].x, p[3].y, TFT_BLACK);
            canvas->drawLine(p[3].x, p[3].y, p[7].x, p[7].y, TFT_BLACK);
            canvas->drawLine(p[7].x, p[7].y, p[4].x, p[4].y, TFT_BLACK);
            canvas->drawLine(p[4].x, p[4].y, p[0].x, p[0].y, TFT_BLACK);
        }

        // 2. Draw Top face
        fillQuad(p[0], p[1], p[2], p[3], colorTop);

        // 3. Draw Front or Back face with matching longitudinal lines
        if (drawFaceA) {
            fillQuad(p[3], p[2], p[6], p[7], colorFront);
            canvas->drawLine(p[0].x, p[0].y, p[1].x, p[1].y, TFT_DARKGREY); // Back edge
            canvas->drawLine(p[3].x, p[3].y, p[2].x, p[2].y, TFT_BLACK);    // Front edge
            canvas->drawLine(p[7].x, p[7].y, p[6].x, p[6].y, TFT_BLACK);    // Bottom edge
        } else {
            fillQuad(p[0], p[1], p[5], p[4], colorFront);
            canvas->drawLine(p[3].x, p[3].y, p[2].x, p[2].y, TFT_DARKGREY); // Front edge
            canvas->drawLine(p[0].x, p[0].y, p[1].x, p[1].y, TFT_BLACK);    // Back edge
            canvas->drawLine(p[4].x, p[4].y, p[5].x, p[5].y, TFT_BLACK);    // Bottom edge
        }
    }
}

void drawSpinningGraphCube(const uint8_t rxDataA[], const uint8_t rxDataB[], int dataSize) {
    if (!canvas) return;
    drawWireframeBoxCube(currentAngle);
    draw3DGridCube(currentAngle);

    #ifdef USE_DIVERSITY
        if (cos(currentAngle) > 0) {
            drawCubeSolidTrail(rxDataA, dataSize, 15.0, TFT_YELLOW, TFT_YELLOW, TFT_YELLOW); 
            drawCubeSolidTrail(rxDataB, dataSize, -15.0, TFT_CYAN, TFT_CYAN, TFT_CYAN);  
        } else {
            drawCubeSolidTrail(rxDataB, dataSize, -15.0, TFT_CYAN, TFT_CYAN, TFT_CYAN);  
            drawCubeSolidTrail(rxDataA, dataSize, 15.0, TFT_YELLOW, TFT_YELLOW, TFT_YELLOW); 
        }
    #else
        drawCubeSolidTrail(rxDataA, dataSize, 15.0, TFT_YELLOW, TFT_YELLOW, TFT_YELLOW);
    #endif
}

// ==============================================================================
// 2. CARTESIAN 3D ENGINE (Reverted to the proven working version)
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

void drawCartesianTrail(const uint8_t rxData[], int dataSize, float yCenter, uint16_t colorFront, uint16_t colorTop, uint16_t colorRight) {
    if (!canvas) return;

    float s_y = 12.0; 
    float s_z = 12.0; 
    
    float yaw = -0.40;   
    float pitch = 0.50; 

    for (int i = 0; i < dataSize - 1; i++) {
        float cx1 = -400.0 + ((float)i / (float)(dataSize - 1)) * 800.0;
        float cx2 = -400.0 + ((float)(i + 1) / (float)(dataSize - 1)) * 800.0;

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

        fillQuad(p[0], p[1], p[2], p[3], colorTop);
        fillQuad(p[3], p[2], p[6], p[7], colorFront);

        canvas->drawLine(p[0].x, p[0].y, p[1].x, p[1].y, TFT_DARKGREY); 
        canvas->drawLine(p[3].x, p[3].y, p[2].x, p[2].y, TFT_BLACK);    
        canvas->drawLine(p[7].x, p[7].y, p[6].x, p[6].y, TFT_BLACK);    

        if (i == dataSize - 2) {
            fillQuad(p[1], p[2], p[6], p[5], colorRight);
            
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
    uint32_t now = millis();
    float dt = (now - lastDrawTime) / 1000.0f; 
    lastDrawTime = now;

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

    canvas->fillScreen(TFT_BLACK);

    if (EepromSettings.screensaverStyle == 0) {
        #ifdef USE_DIVERSITY
            drawSpinningGraphCube(Receiver::rssiALast, Receiver::rssiBLast, RECEIVER_LAST_DATA_SIZE);
        #else
            drawSpinningGraphCube(Receiver::rssiALast, Receiver::rssiALast, RECEIVER_LAST_DATA_SIZE); 
        #endif
        
    } else {
        #ifdef USE_DIVERSITY
            drawCartesianTrail(Receiver::rssiBLast, RECEIVER_LAST_DATA_SIZE, 40.0, 
                TFT_CYAN, TFT_CYAN, TFT_CYAN);
        #endif

        drawCartesianTrail(Receiver::rssiALast, RECEIVER_LAST_DATA_SIZE, -40.0, 
            TFT_YELLOW, TFT_YELLOW, TFT_YELLOW);
    }
    
    Ui::display.drawRGBBitmap(0, 0, canvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    Ui::needDisplay();
}