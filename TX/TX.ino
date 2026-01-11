#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

// Midnight Blue definition (roughly #191970 converted to RGB565)
#define TFT_MIDNIGHT_BLUE 0x18CD

void setup() {
  // Initialize the display
  tft.init();
  
  // Set rotation (0-3). 0 is usually Portrait, 1 is Landscape. 
  // Adjust as needed for your specific case design.
  tft.setRotation(0); 

  // Clear the screen to Midnight Blue
  tft.fillScreen(TFT_MIDNIGHT_BLUE);
}

void loop() {
  // Nothing to do here yet
}
