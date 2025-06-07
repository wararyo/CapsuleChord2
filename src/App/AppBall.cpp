#include "AppBall.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void AppBall::onCreate()
{
    isActive = false;
    srand(time(NULL)); // Initialize random seed for ball positions
    
    // Initialize the cage
    cage.x = 20;
    cage.y = 20;
    cage.width = CANVAS_WIDTH - 40;
    cage.height = CANVAS_HEIGHT - 40;
    cage.borderSize = 3;
    
    // Allocate memory for canvas buffer
    canvasBuffer = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_HEIGHT));
    
    // Initialize balls with random positions and velocities
    initBalls();
}

void AppBall::onActivate()
{
}

void AppBall::onDeactivate()
{
}

void AppBall::updateImuData()
{
    // Check if IMU is available and updated
    if (M5.Imu.update())
    {
        // Get IMU data
        auto data = M5.Imu.getImuData();
        
        // Store the accelerometer and gyro data
        accelX = data.accel.x;
        accelY = data.accel.y;
        accelZ = data.accel.z;
        
        gyroX = data.gyro.x;
        gyroY = data.gyro.y;
        gyroZ = data.gyro.z;
        
        // Set flag to update GUI on next loop
        needsGuiUpdate = true;
    }
}

void AppBall::initBalls()
{
    balls.clear();
    
    // Create balls with random positions and velocities
    for (int i = 0; i < NUM_BALLS; i++) {
        Ball newBall;
        
        // Random position within the cage
        newBall.radius = 8.0f + (rand() % 8); // Random radius between 8 and 15
        newBall.x = cage.x + newBall.radius + rand() % (cage.width - 2 * (int)newBall.radius);
        newBall.y = cage.y + newBall.radius + rand() % (cage.height - 2 * (int)newBall.radius);
        
        // Random initial velocity
        newBall.velocityX = -2.0f + (rand() % 400) / 100.0f;
        newBall.velocityY = -2.0f + (rand() % 400) / 100.0f;
        
        // Random color (avoid black which is the background)
        uint8_t r = 100 + (rand() % 156);
        uint8_t g = 100 + (rand() % 156);
        uint8_t b = 100 + (rand() % 156);
        newBall.color = lv_color_make(r, g, b);
        
        balls.push_back(newBall);
    }
}

void AppBall::updatePhysics()
{
    // Apply IMU data as gravity/acceleration to all balls
    float gravityX = accelY * 0.3f;
    float gravityY = accelX * 0.3f;
    
    // Update each ball's position and velocity
    for (auto& ball : balls) {
        // Apply gravity from accelerometer
        ball.velocityX += gravityX;
        ball.velocityY += gravityY;
        
        // Apply drag to slow down the ball gradually
        ball.velocityX *= DRAG;
        ball.velocityY *= DRAG;
        
        // Update position
        ball.x += ball.velocityX;
        ball.y += ball.velocityY;
        
        // Check collisions with the cage walls and bounce
        checkCollision(ball);
    }
    
    // Redraw the canvas with updated positions
    drawCageAndBalls();
}

bool AppBall::checkCollision(Ball& ball)
{
    bool collision = false;
    
    // Check collision with left wall
    if (ball.x - ball.radius < cage.x) {
        ball.x = cage.x + ball.radius;
        ball.velocityX = -ball.velocityX * BOUNCE_DAMPING;
        collision = true;
    }
    // Check collision with right wall
    else if (ball.x + ball.radius > cage.x + cage.width) {
        ball.x = cage.x + cage.width - ball.radius;
        ball.velocityX = -ball.velocityX * BOUNCE_DAMPING;
        collision = true;
    }
    
    // Check collision with top wall
    if (ball.y - ball.radius < cage.y) {
        ball.y = cage.y + ball.radius;
        ball.velocityY = -ball.velocityY * BOUNCE_DAMPING;
        collision = true;
    }
    // Check collision with bottom wall
    else if (ball.y + ball.radius > cage.y + cage.height) {
        ball.y = cage.y + cage.height - ball.radius;
        ball.velocityY = -ball.velocityY * BOUNCE_DAMPING;
        collision = true;
    }
    
    // When collision happens, play notes and notify the context
    if (collision && context) {
        // Play active notes
        playActiveNotes();
        // Trigger knock event
        context->knock(this);
    }
    
    return collision;
}

void AppBall::drawCageAndBalls()
{
    if (!canvas || !canvasBuffer) return;
    
    // Clear the canvas
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    
    // Draw the cage (a rectangle with borders)
    lv_draw_rect_dsc_t rectDsc;
    lv_draw_rect_dsc_init(&rectDsc);
    rectDsc.bg_opa = LV_OPA_TRANSP;  // Transparent background
    rectDsc.border_color = lv_color_white();
    rectDsc.border_width = cage.borderSize;
    
    // Draw the cage rectangle
    lv_canvas_draw_rect(canvas, cage.x, cage.y, cage.width, cage.height, &rectDsc);
    
    // Draw each ball as a filled circle
    for (const auto& ball : balls) {
        // Draw a filled circle for each ball
        for (int y = -ball.radius; y <= ball.radius; y++) {
            for (int x = -ball.radius; x <= ball.radius; x++) {
                // Check if point is inside the circle
                if (x*x + y*y <= ball.radius*ball.radius) {
                    int drawX = ball.x + x;
                    int drawY = ball.y + y;
                    
                    // Only draw if within canvas boundaries
                    if (drawX >= 0 && drawX < CANVAS_WIDTH && 
                        drawY >= 0 && drawY < CANVAS_HEIGHT) {
                        lv_canvas_set_px_color(canvas, drawX, drawY, ball.color);
                    }
                }
            }
        }
    }
}

void AppBall::onImuUpdateTimer(lv_timer_t *timer)
{
    AppBall *self = static_cast<AppBall *>(timer->user_data);
    self->updateImuData();
}

void AppBall::onPhysicsTimer(lv_timer_t *timer)
{
    AppBall *self = static_cast<AppBall *>(timer->user_data);
    self->updatePhysics();
}

void AppBall::onShowGui(lv_obj_t *container)
{
    isActive = true;
    
    // Register as a note filter to monitor MIDI input
    if (context && context->pipeline) {
        context->pipeline->addNoteFilter(this);
    }

    // Create the app title
    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, getAppName());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 4);
    
    // Create canvas for drawing balls
    canvas = lv_canvas_create(container);
    lv_canvas_set_buffer(canvas, canvasBuffer, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    
    // First drawing of the cage and balls
    drawCageAndBalls();
    
    // Create small container for IMU data at the bottom
    imuContainer = lv_obj_create(container);
    lv_obj_set_size(imuContainer, 240, 70);
    lv_obj_align(imuContainer, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(imuContainer, lv_color_make(32, 32, 32), 0);
    lv_obj_set_style_bg_opa(imuContainer, LV_OPA_50, 0); // Semi-transparent

    // Create labels for accelerometer and gyro data
    accelLabel = lv_label_create(imuContainer);
    lv_obj_align(accelLabel, LV_ALIGN_TOP_MID, 0, 5);
    lv_label_set_text(accelLabel, "Accel: X=0.00 Y=0.00 Z=0.00");

    gyroLabel = lv_label_create(imuContainer);
    lv_obj_align(gyroLabel, LV_ALIGN_TOP_MID, 0, 30);
    lv_label_set_text(gyroLabel, "Gyro: X=0.00 Y=0.00 Z=0.00");
    
    // Create a label to display currently active notes
    notesLabel = lv_label_create(imuContainer);
    lv_obj_align(notesLabel, LV_ALIGN_TOP_MID, 0, 55);
    lv_label_set_text(notesLabel, "Notes: None");

    // Create timers for updating IMU data and physics
    imuUpdateTimer = lv_timer_create(onImuUpdateTimer, 50, this);   // Update IMU data every 50ms
    physicsTimer = lv_timer_create(onPhysicsTimer, 16, this);       // Update physics at ~60fps
}

void AppBall::onHideGui()
{
    isActive = false;

    // Stop the update timers
    if (imuUpdateTimer) {
        lv_timer_del(imuUpdateTimer);
        imuUpdateTimer = nullptr;
    }
    
    if (physicsTimer) {
        lv_timer_del(physicsTimer);
        physicsTimer = nullptr;
    }
    
    // Delete GUI elements
    lv_obj_del(titleLabel);
    lv_obj_del(imuContainer); // This also deletes child elements like notesLabel
    lv_obj_del(canvas);
    canvas = nullptr;
    
    // Clear pointers
    notesLabel = nullptr;
}

void AppBall::onDestroy()
{
    isActive = false;
    
    // Unregister as a note filter
    if (context && context->pipeline) {
        context->pipeline->removeNoteFilter(this);
    }
    
    // Free canvas buffer
    if (canvasBuffer) {
        free(canvasBuffer);
        canvasBuffer = nullptr;
    }
}

void AppBall::onUpdateGui()
{
    // Only update if there's new data and we're active
    if (needsGuiUpdate && isActive)
    {
        // Update accelerometer label
        char accelStr[64];
        snprintf(accelStr, sizeof(accelStr), "Accel: X=%.2f Y=%.2f Z=%.2f", accelX, accelY, accelZ);
        lv_label_set_text(accelLabel, accelStr);

        // Update gyro label
        char gyroStr[64];
        snprintf(gyroStr, sizeof(gyroStr), "Gyro: X=%.2f Y=%.2f Z=%.2f", gyroX, gyroY, gyroZ);
        lv_label_set_text(gyroLabel, gyroStr);

        needsGuiUpdate = false;
    }
}

// Play all currently active notes when a collision occurs
void AppBall::playActiveNotes()
{
    // Skip if there are no active notes or no context
    if (activeNotes.empty() || !context || !context->pipeline) {
        return;
    }
    
    // Get the first channel, or default to channel 0
    uint8_t channel = activeChannels.empty() ? 0 : *activeChannels.begin();
    
    // Play each active note
    for (uint8_t note : activeNotes) {
        // Send note on with the stored velocity
        context->pipeline->sendNote(true, note, lastVelocity, channel);
    }
}

// Update the notes display label
void AppBall::updateNotesDisplay()
{
    if (!isActive || !notesLabel) {
        return;
    }
    
    char notesStr[64] = "Notes: ";
    char *ptr = notesStr + 7;  // Start after "Notes: "
    
    int count = 0;
    for (uint8_t note : activeNotes) {
        if (count < 5) {  // Limit to showing 5 notes to avoid overflow
            ptr += sprintf(ptr, "%d ", note);
            count++;
        } else {
            ptr += sprintf(ptr, "...");
            break;
        }
    }
    
    if (activeNotes.empty()) {
        strcpy(ptr, "None");
    }
    
    lv_label_set_text(notesLabel, notesStr);
}

// Implementation of NoteFilter::onNoteOn
void AppBall::onNoteOn(uint8_t note, uint8_t vel, uint8_t channel)
{
    // Add the note to our active notes set
    activeNotes.insert(note);
    activeChannels.insert(channel);
    lastVelocity = vel;
    
    // Update the display if visible
    updateNotesDisplay();
}

// Implementation of NoteFilter::onNoteOff
void AppBall::onNoteOff(uint8_t note, uint8_t vel, uint8_t channel)
{
    // Remove the note from our active notes set
    activeNotes.erase(note);
    
    // Only remove channel if no notes are active on it anymore
    bool hasNotesOnChannel = false;
    for (auto it = activeNotes.begin(); it != activeNotes.end(); ++it) {
        // For simplicity, assume all notes use the same channel
        hasNotesOnChannel = true;
        break;
    }
    
    if (!hasNotesOnChannel) {
        activeChannels.erase(channel);
    }
    
    // Update the display if visible
    updateNotesDisplay();
}
