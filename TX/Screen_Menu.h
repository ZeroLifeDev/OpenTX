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
        // GraphicsUtils::drawTechGrid(sprite); // Deprecated
        
        // Breadcrumb Header
        sprite->fillRoundRect(2, 2, SCREEN_WIDTH-4, 24, 4, COLOR_BG_PANEL);
        sprite->drawFastHLine(2, 26, SCREEN_WIDTH-4, COLOR_ACCENT_PRI);
        
        display->loadFont(FONT_PATH_BOLD);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL); 
        sprite->setTextDatum(ML_DATUM);
        
        String title = "MENU";
        if (state > 0) title += " > " + String(rootItems[rootSel]);
        sprite->drawString(title, 10, 14);
        display->unloadFont();

        // Update Animation
        int targetY = 0;
        if (state == 0) targetY = rootSel * 20;
        else targetY = subSel * 16;
        
        animY.set(targetY);
        animY.update();
        
        display->loadFont(FONT_PATH_REG);
        int scrollOffset = 30; 
        
        if (state == 0) {
            // ROOT LIST (Cards)
            for(int i=0; i<3; i++) {
                int drawY = 32 + (i * 35); // Bigger cards
                
                // Card Style with borders
                uint16_t cardBg = (i == rootSel) ? COLOR_WIDGET_BG : COLOR_BG_PANEL;
                uint16_t txtCol = (i == rootSel) ? COLOR_ACCENT_PRI : COLOR_TEXT_DIM;
                uint16_t borderCol = (i == rootSel) ? COLOR_ACCENT_PRI : COLOR_BG_PANEL;
                
                // Draw Box
                sprite->fillRoundRect(5, drawY, SCREEN_WIDTH-10, 30, 4, cardBg);
                sprite->drawRoundRect(5, drawY, SCREEN_WIDTH-10, 30, 4, borderCol);
                
                // Active Glow indicator
                if (i == rootSel) {
                     sprite->fillCircle(15, drawY+15, 3, COLOR_ACCENT_TER); // Green dot
                }
                
                // Label
                sprite->setTextColor(txtCol, cardBg);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(rootItems[i], SCREEN_WIDTH/2 + 5, drawY + 11);
            }
        }  
        else if (state == 1 || state == 2) {
            // SUB LIST (Setup Example)
            if (rootSel == 1) { // SETUP
                for(int i=0; i<5; i++) {
                    int drawY = scrollOffset + (i * 24); 
                    
                    if (drawY < 24 || drawY > SCREEN_HEIGHT) continue;

                    // Highlight Row
                    if (i == subSel) {
                         // Active Pill
                         sprite->fillRoundRect(2, drawY, SCREEN_WIDTH-4, 20, 3, COLOR_WIDGET_BG);
                         sprite->drawRoundRect(2, drawY, SCREEN_WIDTH-4, 20, 3, COLOR_ACCENT_PRI);
                         
                         // Blinking if editing
                         if (state == 2 && (millis()/200)%2==0) {
                             sprite->setTextColor(COLOR_TEXT_DIM, COLOR_WIDGET_BG); 
                         } else {
                             sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_WIDGET_BG);
                         }
                    } else {
                        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN); 
                    }
                    
                    // Label
                    sprite->setTextDatum(ML_DATUM);
                    sprite->drawString(setupItems[i], 10, drawY + 9);
                    
                    // Value (Right aligned)
                    ModelData* m = modelManager.getModel();
                    sprite->setTextDatum(MR_DATUM);
                    
                    if (i==0) sprite->drawNumber(m->steering.epaMax, SCREEN_WIDTH-10, drawY+10);
                    else if (i==1) sprite->drawNumber(m->steering.epaMin, SCREEN_WIDTH-10, drawY+10);
                    else if (i==2) sprite->drawNumber(m->steering.expo, SCREEN_WIDTH-10, drawY+10);
                    else if (i==3) sprite->drawNumber(m->throttle.expo, SCREEN_WIDTH-10, drawY+10);
                    else if (i==4) sprite->drawNumber(m->steering.subTrim, SCREEN_WIDTH-10, drawY+10);
                }
            } else {
                sprite->drawString("COMING SOON...", 10, 50); // Removed FONT_SMALL
            }
        }
        display->unloadFont();
    }
};

// ScreenMenu instance is managed by UIManager

#endif // SCREEN_MENU_H
