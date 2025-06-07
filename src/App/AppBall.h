#pragma once

#include "AppBase.h"
#include "Assets/Icons.h"
#include "ChordPipeline.h"
#include <M5Unified.h>
#include <vector>
#include <set>

class AppBall : public AppBase, public ChordPipeline::NoteFilter
{
public:
    char *getAppName() { return "ボール"; }
    // lv_img_dsc_t *getIcon() override { return (lv_img_dsc_t *)&app_ball; }
    bool runsInBackground() { return true; } // Need to run in background to track notes

    bool getActive() { return isActive; }
    void onCreate() override;
    void onActivate() override;
    void onDeactivate() override;
    void onShowGui(lv_obj_t *container) override;
    void onHideGui() override;
    void onDestroy() override;
    void onUpdateGui() override;
    
    // NoteFilter implementation
    bool modifiesNote() override { return true; }
    void onNoteOn(uint8_t note, uint8_t vel, uint8_t channel) override;
    void onNoteOff(uint8_t note, uint8_t vel, uint8_t channel) override;

private:
    // Physics ball structure
    struct Ball {
        float x;          // Position X
        float y;          // Position Y
        float velocityX;  // Velocity X
        float velocityY;  // Velocity Y
        float radius;     // Radius of the ball
        lv_color_t color; // Color of the ball
    };

    // Cage structure
    struct Cage {
        int x;          // Top-left X position
        int y;          // Top-left Y position
        int width;      // Width of the cage
        int height;     // Height of the cage
        int borderSize; // Border size of the cage
    };

    // Constants for physics
    static constexpr float GRAVITY = 1.0f;
    static constexpr float BOUNCE_DAMPING = 0.8f;
    static constexpr float DRAG = 0.99f;
    static constexpr int NUM_BALLS = 6;
    static constexpr int CANVAS_WIDTH = 200;
    static constexpr int CANVAS_HEIGHT = 200;
    
    // Currently active notes
    std::set<uint8_t> activeNotes;
    std::set<uint8_t> activeChannels;
    uint8_t lastVelocity = 64; // Default velocity

    bool isActive = false;
    lv_obj_t *titleLabel;
    lv_obj_t *imuContainer;
    lv_obj_t *accelLabel;
    lv_obj_t *gyroLabel;
    lv_obj_t *notesLabel;  // Display for active notes

    // Canvas for drawing balls and cage
    lv_obj_t *canvas;
    lv_color_t *canvasBuffer = nullptr; // Buffer for canvas
    
    // Collection of balls
    std::vector<Ball> balls;
    Cage cage;

    // Flag to indicate the IMU data has been updated and UI needs refresh
    bool needsGuiUpdate = false;
    float accelX = 0.0f, accelY = 1.0f, accelZ = 0.0f;
    float gyroX = 0.0f, gyroY = 0.0f, gyroZ = 0.0f;

    // Timer for IMU update and physics
    lv_timer_t *imuUpdateTimer = nullptr;
    lv_timer_t *physicsTimer = nullptr;
    
    // Update IMU data from M5Unified
    void updateImuData();
    
    // Update physics and ball positions
    void updatePhysics();
    
    // Draw everything on canvas
    void drawCageAndBalls();
    
    // Check collision with cage walls and trigger notes if collision happens
    bool checkCollision(Ball &ball);
    
    // Play currently active notes when a collision occurs
    void playActiveNotes();
    
    // Initialize the balls with random positions and velocities
    void initBalls();
    
    // Timer callbacks
    static void onImuUpdateTimer(lv_timer_t *timer);
    static void onPhysicsTimer(lv_timer_t *timer);
    
    // Update the notes display
    void updateNotesDisplay();
};
