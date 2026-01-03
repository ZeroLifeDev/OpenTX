#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H

#include "DisplayManager.h"
#include "ModelManager.h"
#include "Theme.h"
#include "InputManager.h"
#include "AnimationUtils.h"
#include "GraphicsUtils.h"

// ==========================================
//          NESTED MENU SYSTEM
// ==========================================
// M1: MODEL (Select, Name)
// M2: SETUP (EPA, Expo, Subtrim)
// M3: SYSTEM (Calib, Sound)

class ScreenMenu {
private:
    int rootSel = 0;
    int subSel = 0;
    int state = 0; // 0=Root, 1=SubMenu, 2=Edit
    
    AnimFloat animY; // Visual offset for scrolling lists
    
public:
    ScreenMenu() : animY(0, 0.2, 0.7) {}
    
    // Root Items
    const char* rootItems[3] = {"MODEL", "SETUP", "SYSTEM"};
    
    // Sub Items (Setup)
    const char* setupItems[5] = {"EPA ST", "EPA TH", "EXPO ST", "EXPO TH", "SUBTRIM"};
    
public:
    void init() {
        rootSel = 0;
        subSel = 0;
        state = 0;
    }

    bool isEditing() { return state == 2; }
    
    
    void next() {
        if (state == 0) rootSel = (rootSel + 1) % 3;
        else if (state == 1) subSel = (subSel + 1) % 5;
        // else if state == 2 (Edit), handled by value change
    }
    
    void prev() {
        if (state == 0) { rootSel--; if(rootSel < 0) rootSel = 2; }
        else if (state == 1) { subSel--; if(subSel < 0) subSel = 4; }
    }
    
    void select() {
        if (state == 0) state = 1;
        else if (state == 1) state = 2; // Enter Edit
        else if (state == 2) state = 1; // Exit Edit (Save logic needed)
    }
    
    void back() {
        if (state == 2) state = 1;
        else if (state == 1) state = 0;
    }

    // Value Editor Logic (Simplified for brevity, expands to full functionality)
    void adjustValue(int delta) {
        if (state != 2) return;
        
        ModelData* m = modelManager.getModel();
        
        if (rootSel == 1) { // SETUP
            switch(subSel) {
                case 0: // EPA ST
                    m->steering.epaMax += delta;
                    m->steering.epaMin += delta; 
                    // Simplified: adjusting both. Pro output would separate L/R
                    break;
                case 2: // EXPO ST
                    m->steering.expo += delta;
                    break;
                 // map others...
            }
        }
    }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // Cyber Global BG
        GraphicsUtils::fillGradientRect(sprite, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BG_MAIN, COLOR_BG_DIM);
        GraphicsUtils::drawTechGrid(sprite);
        
        // Breadcrumb Header with Gradient
        GraphicsUtils::fillGradientRect(sprite, 0, 0, SCREEN_WIDTH, 20, COLOR_BG_PANEL, COLOR_BG_DIM);
        sprite->drawFastHLine(0, 20, SCREEN_WIDTH, COLOR_ACCENT_PRI); // Cyan separator
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL); 
        sprite->setTextDatum(ML_DATUM);
        
        String title = "MENU";
        if (state > 0) title += " > " + String(rootItems[rootSel]);
        sprite->drawString(title, 4, 10, FONT_SMALL);

        // Update Animation
        int targetY = 0;
        if (state == 0) targetY = rootSel * 20;
        else targetY = subSel * 16;
        
        animY.set(targetY);
        animY.update();
        
        int scrollOffset = 30; // disable scroll for root for now if fits
        
        if (state == 0) {
            // ROOT LIST
            for(int i=0; i<3; i++) {
                int drawY = 30 + (i * 26);
                
                // Card Style with borders
                uint16_t cardBg = (i == rootSel) ? COLOR_BG_PANEL : COLOR_BG_DIM;
                uint16_t txtCol = (i == rootSel) ? COLOR_ACCENT_PRI : COLOR_TEXT_DIM;
                uint16_t borderCol = (i == rootSel) ? COLOR_ACCENT_PRI : COLOR_BORDER;
                
                // Draw Box
                sprite->fillRoundRect(5, drawY, SCREEN_WIDTH-10, 22, 2, cardBg);
                sprite->drawRoundRect(5, drawY, SCREEN_WIDTH-10, 22, 2, borderCol);
                
                // Active Glow indicator
                if (i == rootSel) {
                     sprite->fillCircle(12, drawY+11, 3, COLOR_ACCENT_TER); // Green dot
                }
                
                // Label
                sprite->setTextColor(txtCol, cardBg);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(rootItems[i], SCREEN_WIDTH/2 + 5, drawY + 11, FONT_MED);
            }
        }  
        else if (state == 1 || state == 2) {
            // SUB LIST (Setup Example)
            if (rootSel == 1) { // SETUP
                for(int i=0; i<5; i++) {
                    int drawY = scrollOffset + (i * 20); // Spaced up
                    
                    // Clip offscreen
                    if (drawY < 20 || drawY > SCREEN_HEIGHT) continue;

                    // Highlight Row
                    if (i == subSel) {
                         // Cyber Selection
                         GraphicsUtils::fillGradientRect(sprite, 0, drawY, SCREEN_WIDTH, 18, COLOR_BG_PANEL, COLOR_BG_DIM);
                         sprite->drawRect(0, drawY, SCREEN_WIDTH, 18, COLOR_ACCENT_PRI);
                         
                         // Blinking if editing
                         if (state == 2 && (millis()/200)%2==0) {
                             sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_PANEL); 
                         } else {
                             sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_PANEL);
                         }
                    } else {
                        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN); // On top of grid? might need bg fill
                    }
                    
                    // Label
                    sprite->setTextDatum(ML_DATUM);
                    sprite->drawString(setupItems[i], 5, drawY+5, FONT_SMALL);
                    
                    // Value
                    int val = 0;
                    ModelData* m = modelManager.getModel();
                    if (i==0) val = m->steering.epaMax;
                    else if (i==1) val = m->steering.epaMin; 
                    else if (i==2) val = m->steering.expo; 
                    else if (i==3) val = m->throttle.expo;
                    else if (i==4) val = m->steering.subTrim;
                    
                    sprite->setTextDatum(MR_DATUM);
                    sprite->drawString(String(val), SCREEN_WIDTH-5, drawY+5, FONT_SMALL);
                }
            } else {
                sprite->drawString("COMING SOON...", 10, 50, FONT_SMALL);
            }
        }
    }
};

// ScreenMenu instance is managed by UIManager

#endif // SCREEN_MENU_H
