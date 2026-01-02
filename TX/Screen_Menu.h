#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H

#include "DisplayManager.h"
#include "ModelManager.h"
#include "Theme.h"
#include "InputManager.h"

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
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Breadcrumb Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_HEADER);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_HEADER);
        sprite->setTextDatum(ML_DATUM);
        
        String title = "MENU";
        if (state > 0) title += " > " + String(rootItems[rootSel]);
        sprite->drawString(title, 4, 10, FONT_SMALL);

        int y = 30;
        
        if (state == 0) {
            // ROOT LIST
            for(int i=0; i<3; i++) {
                if (i == rootSel) {
                    sprite->fillRect(0, y-2, SCREEN_WIDTH, 18, COLOR_ACCENT_PRI);
                    sprite->setTextColor(COLOR_BG_MAIN, COLOR_ACCENT_PRI);
                } else {
                    sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
                }
                sprite->drawString(rootItems[i], 10, y+8, FONT_MED);
                y += 20;
            }
        } 
        else if (state == 1 || state == 2) {
            // SUB LIST (Setup Example)
            if (rootSel == 1) { // SETUP
                for(int i=0; i<5; i++) {
                    // Highlight Row
                    if (i == subSel) {
                         sprite->fillRect(0, y-2, SCREEN_WIDTH, 14, COLOR_BG_PANEL);
                         // Blinking if editing
                         if (state == 2 && (millis()/200)%2==0) {
                             sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_PANEL); // Blink dim
                         } else {
                             sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_PANEL);
                         }
                    } else {
                        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
                    }
                    
                    // Label
                    sprite->setTextDatum(ML_DATUM);
                    sprite->drawString(setupItems[i], 5, y+5, FONT_SMALL);
                    
                    // Value
                    int val = 0;
                    ModelData* m = modelManager.getModel();
                    if (i==0) val = m->steering.epaMax;
                    if (i==2) val = m->steering.expo;
                    // ... others
                    
                    sprite->setTextDatum(MR_DATUM);
                    sprite->drawString(String(val), SCREEN_WIDTH-5, y+5, FONT_SMALL);
                    
                    y += 16;
                }
            } else {
                sprite->drawString("DOING...", 10, 50, FONT_SMALL);
            }
        }
    }
};

ScreenMenu screenMenu; // Global instance needs to match extern

#endif // SCREEN_MENU_H
