
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include "Wire.h" // For I2C
#include "LCD.h" // For LCD
#include "LiquidCrystal_I2C.h" // Added library*

//Define the global variables
#define LED_PIN 8 //Defines the output to the LED lights
#define LED_TOTAL 50  //Change this to the number of LEDs in your strand.
#define LED_HALF  LED_TOTAL/2
#define AUDIO_PIN A0  //Pin IN for the envelope of the sound detector
#define KNOB_PIN  A2  //Pin IN for the trimpot 10K
#define BUTTON_PIN 3  //Pin IN for button push

int  val;//define digital variable val
int style=8; //defines routine to run
int oldStyle;  
int shuffleTiming=15000;//time each routine runs during shuffle in milli seconds
bool change=false;
float stripBrightness;
int color[13][3] = {  //Store the primary colors to an array

    {255,0,0},            //Red
    {255,127,0},          //Orange
    {255,255,0},          //Yellow
    {127,255,0},          //Green Yellow
    {0,255,0},            //Green
    {0,255,127},          //Green Cyan
    {0,255,255},          //Cyan
    {0,127,255},          //Blue Cyan
    {0,0,255},            //Blue
    {127,0,255},          //Blue Magenta
    {255,0,255},          //Magenta
    {255,0,127}           //Red Magenta
};


unsigned long timeLast;         //Used to record time between button push so you only record one per push
unsigned long timeBetween;      //Same
unsigned long timeStart;        //Used for case that cycles the effects
volatile int buttonState = 0;   // variable for reading the pushbutton status

//Sound visualizer unique global variables
uint16_t gradient = 0; //Used to iterate and loop through each color palette gradually
double volume = 0;    //Holds the volume level read from the sound detector.
uint16_t last = 10;      //Holds the value of volume from the previous loop() pass.
float maxVol = 15;     //Holds the largest volume recorded thus far to proportionally adjust the visual's responsiveness.
float knob = 1023.0;   //Holds the percentage of how twisted the trimpot is. Used for adjusting the max brightness.
float avgVol = 0;      //Holds the "average" volume-level to proportionally adjust the visual experience.
float avgBump = 0;     //Holds the "average" volume-change to trigger a "bump."

bool bump = false;     //Used to pass if there was a "bump" in volume

const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;


Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_TOTAL, LED_PIN, NEO_BRG + NEO_KHZ800);//Initializes the string of lights
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
// NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
// NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
// NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
// NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
// NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the default I2C bus address of the backpack-see article
//Set the pins on the I2C chip used for LCD connections
//ADDR,EN,R/W,RS,D4,D5,D6,D7

void setup() {
  pinMode(LED_PIN,OUTPUT);//define LED as a output port
  pinMode(BUTTON_PIN,INPUT);//define switch as a output port
  pinMode(AUDIO_PIN, INPUT);
  pinMode(KNOB_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), pin_ISR, RISING); // Attach an interrupt to the ISR vector
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  lcd.begin(16, 2);
  lcd.setBacklightPin(3,POSITIVE); // BL, BL_POL
  lcd.setBacklight(HIGH);
  
}

void loop() {
  // Some example procedures showing how to display to the pixels:
  Serial.println("Top of Loop");
  Serial.print("Case -");
  Serial.print("\t");
  Serial.println(style);
  String topLine;
  String bottomLine;
  if(oldStyle!=style){  //Checks to see if button was pushed to change program. If yes then it allows for britness adjustment
    oldStyle=style;
    
    lcd.clear();//Clear display and sets cursor t
    topLine="Brightness    "; //start to construct top line
    topLine.concat("%"); //add percentage
    lcd.print(topLine); 
    lcd.setCursor(0,1); //set the cursor to the beginning of the second line
    bottomLine=("NextPg- ");//start to construct bottom line
    bottomLine.concat("  TTG-");
    lcd.print(bottomLine);
    for(int i=0; i<=25;i++){//to give the display more stablity in how it reads only rewrite the values that change
      stripBrightness=analogRead(KNOB_PIN);//Uses rheostat to set brightness
      stripBrightness=map(stripBrightness,0,1021,0,255);  //Changes input 0-1021 to brightness allowable range 0-255
      int percentBright=(stripBrightness/255*100);
      lcd.setCursor(12,0);
      lcd.print(percentBright); //add value of rheostat
      lcd.setCursor(7,1);
      lcd.print(style); //print the next program  
      int timeRemain=(5-i/5);  //calculate the time remaining
      lcd.setCursor(14,1);
      lcd.print(timeRemain);//print the time remaining
      delay(200);
    }
  strip.setBrightness(stripBrightness); //Set strip brightness
  Serial.println("Outside of loop");
  }
 

      switch (style){ 
                             
        case 1://colorWipe color changes down the string one pixel at a time
              Serial.println("1. ColorWipe");
              lcd.clear ();
              lcd.print("1. ColorWipe");
              
              while(style==1){  //random jump around the color circle
                int j=random(12);
                int wait=random(100);
                colorWipe(strip.Color(color[j][0],color[j][1],color[j][2]),wait); 
              }

              break;
              
        case 2://Color Wash.  Gradual transition entire string around the color circle
              Serial.println("2. ColorWash");
              lcd.clear();
              lcd.print("2.ColorWash");
              colorWash(0);
              break;
              
            
        case 3: //Theater Chase.  Light chase around the string with equal spacing of on and off
              Serial.println("3. TheaterChase");
              lcd.clear();
              lcd.print("3.TheaterChase");
              while(style==3){  //random jump around the color circle
                int j=random(12);
                theaterChase(strip.Color(color[j][0],color[j][1],color[j][2]),500);
              }
              break;

        case 4: //Ranbow Cycle
              Serial.println("4. RainbowCycle");
              lcd.clear();
              lcd.print("4.RainbowCycle");
              rainbowCycle(0);
              break;
             
        case 5://Rainbow Chase.  Same as Theather chase but lights move through color circle as around the string.
              Serial.println("5.RaibowChase");
              lcd.clear();
              lcd.print("5.RainbowChase");
              theaterChaseRainbow(0);
              break;        
            
        case 6: // Candy Chase.  Red and white cycling through
              Serial.println("6. CandyChase");
              lcd.clear();
              lcd.print("6.CandyChase");
              candyChase(1000);   
              break;

        case 7: // Snowflake.  White flickering to simulate snow falling
              Serial.println("7. SnowFlake");
              lcd.clear();
              lcd.print("7.SnowFlake");
              snowFlakes(50);
              break;

        case 8://Randomly run all programs 10 times each
              Serial.println("8.Shuffling");
              lcd.clear();
              lcd.print("8.Shuffling");
          
              timeStart=millis();//timing to determine how long to run each routine
              while(((millis()-timeStart))<=shuffleTiming){
                if(change==true){
                  change=false;
                  return;
                }
                unsigned long delta = millis()-timeStart;
                int j=random(12);
                int wait=random(100);
                colorWipe(strip.Color(color[j][0],color[j][1],color[j][2]),wait); 
              }
                  
              timeStart=millis();
              while(((millis()-timeStart))<=shuffleTiming){
                if(change==true){
                  change=false;
                  return;
                }                
                colorWash(0);
              }
                  
              timeStart=millis();
              while(((millis()-timeStart))<=shuffleTiming){
                if(change==true){
                  change=false;
                  return;
                }               
                int j=random(12);
                theaterChase(strip.Color(color[j][0],color[j][1],color[j][2]),500);
               }
                  
              timeStart=millis();
              while(((millis()-timeStart))<=15000){
                if(change==true){
                  change=false;
                  return;
                }                
                rainbowCycle(0);
              }
             
              timeStart=millis();
              while(((millis()-timeStart))<=shuffleTiming){
                if(change==true){
                  change=false;
                  return;
                }
                theaterChaseRainbow(0);
              }
                  
              timeStart=millis();
              while(((millis()-timeStart))<=shuffleTiming){
                if(change==true){
                  change=false;
                  return;
                }                                    
                candyChase(1000);
              }
            
              timeStart=millis();
              while(((millis()-timeStart))<=shuffleTiming){
                if(change==true){
                  change=false;
                  return;
                }
                snowFlakes(50);
              }
              break;

        case 9:  //Sound Visualizer
              lcd.clear();
              lcd.print("9.SoundVisual");
              unsigned long startMillis= millis();  // Start of sample window
              unsigned int peakToPeak = 0;   // peak-to-peak level
              unsigned int signalMax = 0;
              unsigned int signalMin = 1024;

              while (millis() - startMillis < sampleWindow){
                sample = analogRead(A0);
                if (sample < 1024){  // toss out spurious readings
                  if (sample > signalMax){
                    signalMax = sample;  // save just the max levels
                  }
                  else if (sample < signalMin){
                    signalMin = sample;  // save just the min levels
                  }
                }
              }
   
            peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
            volume = (peakToPeak * 5.0) / 102.4;  // convert to volts
  
  
            knob = analogRead(KNOB_PIN) / 1023.0; //Record how far the trimpot is twisted
            avgVol = (avgVol + volume) / 2.0;     //Take our "average" of volumes.
  
            //  Sets a threshold for volume.  In practice I've found noise can get up to 15, so if it's lower, the visual thinks it's silent.
            if (volume < avgVol / 2 || volume < 5) volume = 0; //  Also if the volume is less than average volume / 2 (essentially an average with 0), it's considered silent.
            if (volume > maxVol) maxVol = volume;             //If the current volume is larger than the loudest value recorded, overwrite
 
            if (gradient > 1529) {  //This is where "gradient" is reset to prevent overflow.
              gradient %= 1530;       //When graidient hits 1530 it resets it to 1.  
  

            // Everytime a palette gets completed is a good time to readjust "maxVol," just in case
            //  the song gets quieter; we also don't want to lose brightness intensity permanently.  because of one stray loud sound.
            maxVol = (maxVol + volume) / 2.0;
          }

          //If there is a decent change in volume since the last pass, average it into "avgBump"
          if (volume - last > avgVol - last && avgVol - last > 0) avgBump = (avgBump + (volume - last)) / 2.0;
  
          bump = (volume - last) > avgBump;   //if volume delta is greater than the average set bump to TRUE
          Pulse();   //Calls the visual to be displayed with the globals as they are
          gradient++;    //Increments gradient
          last = volume; //Records current volume for next pass
          delay(30);   //Paces visuals so they aren't too fast to be enjoyable   
          break;
              
        }  //End of Cases
} //End of loop


// Case 1:  ColorWipe.  Fill the dots one after the other with a color

void colorWipe(uint32_t c, uint16_t wait) {

    Serial.println("Case 1: ColorWipeRoutine");
      for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
        } 
}

// Case 2: ColorWash.  The entire string transitions through the color spectrum
void colorWash(uint8_t wait){
    Serial.println("Case 2: ColorWashRoutine");
      for(int b=0; b<=255; b+=5){
        colorWipe(strip.Color(0,255,b),wait);  //Red to Yellow
        if(change==true){
            change=false;
            return;
         }
      }
      
      for(int g=255; g>=0; g-=5){
        colorWipe(strip.Color(0,g,255),wait); //Yellow to Green
        if(change==true){
           return;
        } 
      }
      
      for(int r=0; r<=255;r+=5){
        colorWipe(strip.Color(r,0,255),wait); //Green to Cyan
        if(change==true){
           return;
        }       
      }
      
      for(int b=255; b>=0; b-=5){
        colorWipe(strip.Color(255,0,b),wait); //Cyan to Blue
        if(change==true){
           return;
        }  
      }
      
      for(int g=0; g<=255; g+=5){
        colorWipe(strip.Color(255,g,0),wait); //Blue to Magenta
        if(change==true){
           return;
        }  
      }
      
      for(int r=255; r>=0; r-=5){
        colorWipe(strip.Color(r,255,0),wait); //Magenta to Red
        if(change==true){
          return;
        }    
     }      
}

//Case 3: Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  int temp;
  Serial.println("Case3: TheaterChaseRoutine");
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
      for (int q=0; q < 3; q++) {
        for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
          if(change==true || ((millis()-timeStart)>shuffleTiming && style==8)){//Checks to see if button was cycled or if shuffle time exceeded.
            change=false;
            return;
          }

          strip.setPixelColor(i+q, c);    //turn every third pixel on
       }
       strip.show();
       delay(wait);
       for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
       }
    }
  }
}



// Case 4: Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  Serial.println("RainbowCycleRoutine");
    for(j=0; j<256*5; j=j+5) { // 5 cycles of all colors on wheel
      for(i=0; i< strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        if(change==true || ((millis()-timeStart)>shuffleTiming && style==8)){//Checks to see if button was cycled or if shuffle time exceeded.
           change=false;
           return;
       } 
      }
      strip.show();
      delay(wait);
    }
}


//Case 5: Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  Serial.println("TheaterChaseRoutine");
    for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
      for (int q=0; q < 3; q++) {
        for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        if(change==true || ((millis()-timeStart)>shuffleTiming && style==8)){//Checks to see if button was cycled or if shuffle time exceeded.Need this because wheel is used 
           change=false;
           return;
        } 
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
        delay(wait);
        for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
      }
    }
}

//Case 6:  Red and white chasing lights
void candyChase(uint8_t wait) {
  Serial.println("CandyChaseRoutine");
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
      for (int q=0; q < 3; q++) {
        for (uint16_t i=0; i < strip.numPixels(); i++) {
        if(change==true){
           change=false;
           return;
        } 
          strip.setPixelColor(i+q, 255,255,255);    //turn every pixel white
        }
        for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 255,0,0);    //turn every third pixel red
        }
        strip.show();
        delay(wait);
        for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
      }
  }  
}

// Case 7: Flickering white lights to simulate snow
void snowFlakes(uint8_t wait) {
 // while(style==7){
  int pixel[60];
  Serial.println("SnowFlakeRoutine");
    for(int p=0; p<60; p++){
      pixel[p] = random(0,255); 
    }
    for (int j=0; j<200; j++) {   // Run some snowflake cycles
      if((j%5)==0){   // Every five cycles, light a new pixel
        strip.setPixelColor(random(0,60), 255,255,255);
      }
    }
    
    for(int p=0; p<60; p++){  // Dim all pixels by 10
      strip.setPixelColor(p, pixel[p],pixel[p],pixel[p] );
      pixel[p] =  pixel[p] - 10;
    }
   strip.show();
   delay(wait);
}


// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  Serial.println("DigitalWheel");
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
      return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
      if(change==true){
        return;
      }
    }
    if(WheelPos < 170) {
      WheelPos -= 85;
      return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
      if(change==true){
        return;
      }
    }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  if(change==true){
    return;
  }
  return;
  }


//This is the interupt routine that runs when the button is pushed
void pin_ISR() {
  Serial.println("Interupt Start");
  delay(500);
  timeBetween=millis()-timeLast;
  if(timeBetween>500){
    buttonState = digitalRead(BUTTON_PIN);
    if(style==9){
      style=0;
    }
      style=style+1;
  Serial.print("Style");
  Serial.print("\t");
  Serial.println(style);
  }
timeLast=millis();
Serial.println("Interupt End");
change=true;
Serial.print("Change -");
Serial.print("\t");
Serial.println(change);
return;
}


/////////////////////////////These are the sound visulizer routines////////////////////////////////////

//Pulse from center of the strand
void Pulse() {

  fade(0.75);   //Listed below, this function simply dims the colors a little bit each pass of loop()

  
  if (bump) gradient += 64; //Advances the gradient to the next noticeable color if there is a "bump"
  if (volume > 0) {   //If it's silent, we want the fade effect to take over, hence this if-statement
      uint32_t col = Rainbow(gradient); //Our retrieved 32-bit color
      int start = LED_HALF - (LED_HALF * (volume / maxVol));                              //These variables determine where to start and end the pulse since it starts from the middle of the strand.
      int finish = LED_HALF + (LED_HALF * (volume / maxVol)) + strip.numPixels() % 2;    //The quantities are stored in variables so they only have to be computed once
      
      for (int i = start; i < finish; i++) {
        float damp = float(((finish - start) / 2.0) -               //"damp" creates the fade effect of being dimmer the farther the pixel is from the center of the strand.                                                     
                     abs((i - start) - ((finish - start) / 2.0))    //  It returns a value between 0 and 1 that peaks at 1 at the center of the strand and 0 at the ends.
                   )/ float((finish - start) / 2.0);

      //Sets the each pixel on the strand to the appropriate color and intensity
      //  strand.Color() takes 3 values between 0 & 255, and returns a 32-bit integer.
      //  Notice "knob" affecting the brightness, as in the rest of the visuals.
      //  Also notice split() being used to get the red, green, and blue values.
        strip.setPixelColor(i, strip.Color(
                             split(col, 0) * pow(damp, 2.0) * knob,
                             split(col, 1) * pow(damp, 2.0) * knob,
                             split(col, 2) * pow(damp, 2.0) * knob
                           ));
      }  //End of for

    strip.setBrightness(255.0 * pow(volume / maxVol, 2));    //Sets the max brightness of all LEDs. If it's loud, it's brighter.

  } //End of if
    strip.show();  //This command actually shows the lights.
} // End Pulse



//Fades lights by multiplying them by a value between 0 and 1 each pass of loop().
void fade(float damper) {
  if (damper >= 1) damper = 0.99;  //"damper" must be between 0 and 1, or else you'll end up brightening the lights or doing nothing.
  
  for (int i = 0; i < strip.numPixels(); i++) {
    uint32_t col = (strip.getPixelColor(i)) ? strip.getPixelColor(i) : strip.Color(0, 0, 0);    //Retrieve the color at the current position.

    if (col == 0) continue; //If it's black, you can't fade that any further.
    float colors[3]; //Array of the three RGB values
    for (int j = 0; j < 3; j++) colors[j] = split(col, j) * damper;    //Multiply each value by "damper"
    strip.setPixelColor(i, strip.Color(colors[0] , colors[1], colors[2]));   //Set the dampened colors back to their spot.
  }
}


uint8_t split(uint32_t color, uint8_t i ) {    //0 = Red, 1 = Green, 2 = Blue
  if (i == 0) return color >> 16;
  if (i == 1) return color >> 8;
  if (i == 2) return color >> 0;
  return -1;
}


//This function simply take a value and returns a gradient color
//  in the form of an unsigned 32-bit integer

//The gradient returns a different, changing color for each multiple of 255
//  This is because the max value of any of the 3 LEDs is 255, so it's
//  an intuitive cutoff for the next color to start appearing.
//  Gradients should also loop back to their starting color so there's no jumps in color.

uint32_t Rainbow(unsigned int i) {
  if (i > 1529) return Rainbow(i % 1530);
  if (i > 1274) return strip.Color(255, 0, 255 - (i % 255));   //violet -> red
  if (i > 1019) return strip.Color((i % 255), 0, 255);         //blue -> violet
  if (i > 764) return strip.Color(0, 255 - (i % 255), 255);    //aqua -> blue
  if (i > 509) return strip.Color(0, 255, (i % 255));          //green -> aqua
  if (i > 255) return strip.Color(255 - (i % 255), 255, 0);    //yellow -> green
  return strip.Color(255, i, 0);                               //red -> yellow
}






//Case#:  Another way to do colorWash routine
//void colorWash(uint8_t wait) {
//  uint16_t i, j;
//    Serial.println("Case3: RainbowRoutine");
//    for(j=0; j<256; j+=3) {
//      for(i=0; i<strip.numPixels(); i++) {
//          if(style!=4){
//          return;
//          }
//        strip.setPixelColor(i, Wheel(j));
//      }
//    strip.show();
//    delay(wait);
//    }
//}

