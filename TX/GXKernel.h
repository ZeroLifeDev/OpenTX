#ifndef GX_KERNEL_H
#define GX_KERNEL_H

#include <TFT_eSPI.h>
#include "Theme.h"

// ==========================================
//          GRAPHICS KERNEL (GX)
// ==========================================
// Advanced rendering engine for "Holographic" effects
// particle systems, and procedural noise.

struct Particle {
    float x, y;
    float vx, vy;
    int life;
    uint16_t color;
    bool active;
};

#define MAX_PARTICLES 40

class GXKernel {
private:
    Particle particles[MAX_PARTICLES];
    
public:
    void init() {
        for(int i=0; i<MAX_PARTICLES; i++) particles[i].active = false;
    }

    // --- PARTICLE SYSTEM ---
    
    void spawnParticle(int x, int y, uint16_t col) {
        for(int i=0; i<MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                particles[i].active = true;
                particles[i].x = x;
                particles[i].y = y;
                // Random vector
                particles[i].vx = random(-100, 100) / 50.0f;
                particles[i].vy = random(-100, 100) / 50.0f;
                particles[i].life = random(10, 40);
                particles[i].color = col;
                return;
            }
        }
    }
    
    void updateParticles() {
        for(int i=0; i<MAX_PARTICLES; i++) {
            if (particles[i].active) {
                particles[i].x += particles[i].vx;
                particles[i].y += particles[i].vy;
                particles[i].life--;
                if (particles[i].life <= 0) particles[i].active = false;
            }
        }
    }
    
    void drawParticles(TFT_eSprite* sprite) {
        for(int i=0; i<MAX_PARTICLES; i++) {
            if (particles[i].active) {
                sprite->drawPixel((int)particles[i].x, (int)particles[i].y, particles[i].color);
            }
        }
    }
    
    // --- HOLOGRAPHIC PRIMITIVES ---
    
    // Draws a box with corner accents and scanline fill
    void drawHoloRect(TFT_eSprite* sprite, int x, int y, int w, int h, uint16_t color) {
        // Frame
        sprite->drawRect(x, y, w, h, COLOR_BG_PANEL);
        sprite->drawRect(x+1, y+1, w-2, h-2, COLOR_BG_PANEL);
        
        // Corners
        int cw = w/4;
        int ch = h/4;
        sprite->drawLine(x, y, x+cw, y, color);
        sprite->drawLine(x, y, x, y+ch, color);
        
        sprite->drawLine(x+w-1, y+h-1, x+w-1-cw, y+h-1, color);
        sprite->drawLine(x+w-1, y+h-1, x+w-1, y+h-1-ch, color);
        
        // Digital Noise Fill (occasional)
        if (random(0, 10) > 8) {
            int ny = y + random(0, h);
            sprite->drawFastHLine(x, ny, w, color);
        }
    }
    
    // Draws text with a "Glow" shadow
    void drawGlowText(TFT_eSprite* sprite, String text, int x, int y, int font) {
        // Shadow
        sprite->setTextColor(COLOR_BG_PANEL, COLOR_BG_MAIN);
        sprite->drawString(text, x+1, y+1, font);
        sprite->drawString(text, x-1, y-1, font);
        
        // Main
        sprite->setTextColor(COLOR_TEXT_GLOW, COLOR_BG_MAIN);
        sprite->drawString(text, x, y, font);
    }
};

// Global Instance
GXKernel gxKernel;

#endif // GX_KERNEL_H
