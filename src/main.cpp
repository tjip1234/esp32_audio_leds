#include <WiFi.h>
#include <Arduino.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <iostream>
#include <string>
#include <sstream>
#include <WiFiUdp.h>
#include <EEPROM.h>
#define BUFFER_LEN 1024
#define EEPROM_SIZE 3
using namespace std;
const uint16_t PixelCount = 200; // make sure to set this to the number of pixels in your strip
const uint16_t PixelPin = 2; 
const uint16_t AnimCount = 1;
const uint16_t TailLength = 199; 
const float MaxLightness = 0.5f; 
NeoGamma<NeoGammaTableMethod> colorGamma;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(AnimCount);
// Replace with your network credentials
const char* ssid     = "Dopke2G4";
const char* password = "FjaereZolder";
unsigned int localPort = 7777;
char packetBuffer[BUFFER_LEN];


WiFiUDP port;
WiFiServer server(80);
int animationsallowed = 0;
int fading = 0;
int audio = 0;
uint8_t N = 0;
String header;
int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;
String redString = "0";
String greenString = "0";
String blueString = "0";

unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
boolean fadeToColor = true;
void LoopAnimUpdate(const AnimationParam& param)
{
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);

        // rotate the complete strip one pixel to the right on every update
        strip.RotateRight(1);
    }
}

void DrawTailPixels()
{
    // using Hsl as it makes it easy to pick from similiar saturated colors
    float hue = random(360) / 360.0f;
    for (uint16_t index = 0; index < strip.PixelCount() && index <= TailLength; index++)
    {
        float lightness = index * MaxLightness / TailLength;
        RgbColor color = HslColor(hue, 1.0f, lightness);

        strip.SetPixelColor(index, colorGamma.Correct(color));
    }
}
struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};
MyAnimationState animationState[AnimCount];
void BlendAnimUpdate(const AnimationParam& param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);

    // apply the color to the strip
    for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
    {
        strip.SetPixelColor(pixel, updatedColor);
    }
}

void FadeInFadeOutRinseRepeat(float luminance)
{
    if (fadeToColor)
    {
        // Fade upto a random color
        // we use HslColor object as it allows us to easily pick a hue
        // with the same saturation and luminance so the colors picked
        // will have similiar overall brightness
        RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
        uint16_t time = random(1280, 1281);

        animationState[0].StartingColor = strip.GetPixelColor(0);
        animationState[0].EndingColor = target;

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }
    else 
    {
        // fade to black
        uint16_t time = random(2000, 4000);

        animationState[0].StartingColor = strip.GetPixelColor(0);
        animationState[0].EndingColor = RgbColor(0); 

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }

    // toggle to the next effect state
    fadeToColor = !fadeToColor;
}
void setup() {
  EEPROM.begin(EEPROM_SIZE);
  RgbColor color = RgbColor(EEPROM.read(0), EEPROM.read(1), EEPROM.read(2));
  strip.Begin();
  strip.Show();
  for(int i=0; i < PixelCount; i++) {
  strip.SetPixelColor(i,color);
  }
  
  
  // Connect to Wi-Fi network with SSID and password
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    
  }
  // Print local IP address and start web server
  port.begin(localPort);
  server.begin();
}
void webserver(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
             // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {            // loop while the client's connected
      currentTime = millis();
      if (client.available()) { 
                   // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
                   
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link rel=\"stylesheet\" href=\"https://tjip1234.github.io/style.css\">");
            client.println("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>");
            client.println("</head><body>");
            client.println("<h1>Esp 32</h1>");
            client.println("<br>");
            client.println("<a class=\"button_default\" href=\"?audio\" id=\"audio\" role=\"button\">Audio</a>");
            client.println("<br>");
            client.println("<a class=\"button_default\" href=\"?looping\" id=\"looping\" role=\"button\">Looping</a> ");
            client.println("<br>");
            client.println("<a class=\"button_default\" href=\"?black\" id=\"black\" role=\"button\">black</a> ");
            client.println("<br>");
            client.println("<a class=\"button_default\" href=\"?fading\" id=\"fading\" role=\"button\">Fading</a> ");
            client.println("<br>");
            client.println("<a class=\"button_default\" href=\"#\" id=\"fill\" role=\"button\">Fill</a> ");
            client.println("<input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\"></div>");
            client.println("<script>function update(picker) {document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);");
            client.println("document.getElementById(\"fill\").href=\"?r\" + Math.round(picker.rgb[0]) + \"g\" +  Math.round(picker.rgb[1]) + \"b\" + Math.round(picker.rgb[2]) + \"&\";}</script></body></html>");
           
            // The HTTP response ends with another blank line
            client.println();
            if (header.indexOf("GET /?looping") >= 0)
            {
              RgbColor color = RgbColor(0,0,0);
              for(int i=0; i < PixelCount; i++) {
              strip.SetPixelColor(i,color);
              }
              DrawTailPixels(); 
              animations.StartAnimation(0, 66, LoopAnimUpdate);
              animationsallowed = 1;
              fading = 0;
              audio = 0;
            }
            else if (header.indexOf("GET /?fading") >= 0){
              RgbColor color = RgbColor(0,0,0);
              for(int i=0; i < PixelCount; i++) {
              strip.SetPixelColor(i,color);
              }
              FadeInFadeOutRinseRepeat(0.3f);
              fading = 1;
              animationsallowed = 1;
              audio = 0;
            }
            else if (header.indexOf("GET /?audio") >= 0)
            {
              RgbColor color = RgbColor(0,0,0);
              for(int i=0; i < PixelCount; i++) {
              strip.SetPixelColor(i,color);
              }
              animationsallowed = 0; 
              fading = 0;
              audio =1;
            }
            else if (header.indexOf("GET /?black") >= 0)
            {
              RgbColor color = RgbColor(0,0,0);
              for(int i=0; i < PixelCount; i++) {
              strip.SetPixelColor(i,color);
              }
              animationsallowed = 0; 
              fading = 0;
              audio = 0;
            }
            
            else if(header.indexOf("GET /?r") >= 0) {
              pos1 = header.indexOf('r');
              pos2 = header.indexOf('g');
              pos3 = header.indexOf('b');
              pos4 = header.indexOf('&');
              redString = header.substring(pos1+1, pos2);
              greenString = header.substring(pos2+1, pos3);
              blueString = header.substring(pos3+1, pos4);
              RgbColor color = RgbColor(redString.toInt(),greenString.toInt(),blueString.toInt());
              EEPROM.write(0,redString.toInt());
              EEPROM.write(1,greenString.toInt());
              EEPROM.write(2,blueString.toInt());
              EEPROM.commit();
              for(int i=0; i < PixelCount; i++) {
              strip.SetPixelColor(i,color);
              }
              strip.Show();
              animationsallowed = 0; 
              fading = 0;
              audio = 0;
            }
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
 
  }
}

void loop(){
  if (animationsallowed == 1)
  {
    if (fading == 1)
    {
      if (animations.IsAnimating())
      {
          webserver();
          animations.UpdateAnimations();
          strip.Show();
      }
      else
      {
          // no animation runnning, start some 
          //
          FadeInFadeOutRinseRepeat(0.3f); // 0.0 = black, 0.25 is normal, 0.5 is bright
      }
    }
    else
    {
      webserver();
      strip.Show();
      animations.UpdateAnimations();
    }
  }
  else if (audio == 1)
  {
    webserver();
    int packetSize = port.parsePacket();
    // If packets have been received, interpret the command
    if (packetSize) {
        int len = port.read(packetBuffer, BUFFER_LEN);
        for(int i = 0; i < len; i+=4) {
            packetBuffer[len] = 0;
            N = packetBuffer[i];
            RgbColor pixel((uint8_t)packetBuffer[i+1], (uint8_t)packetBuffer[i+2], (uint8_t)packetBuffer[i+3]);
            strip.SetPixelColor(N, pixel);
        } 
        strip.Show();

    }
  }
  
  else
  {
  webserver();
  strip.Show();
  }

}