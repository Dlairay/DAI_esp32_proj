#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRRemoteESP32.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// WiFi & Firebase Credentials
#define WIFI_SSID "x" //replace with your info
#define WIFI_PASSWORD "x" //replace with your info
#define API_KEY "x" //replace with your info
#define DATABASE_URL "x" //replace with your info
#define USER_EMAIL "x" //replace with your info
#define USER_PASSWORD "x" //replace with your info

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Timer for Firebase updates
unsigned long lastFirebaseCheck = 0;
const int FIREBASE_CHECK_INTERVAL = 2000;

// LCD and IR Receiver
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2
#define IR_RECEIVER_PIN 4  // Change to GPIO2, 3, or 5 if needed

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
IRRemoteESP32 irRemote(IR_RECEIVER_PIN);

// Variables
String lastDisplayedValue = "";
String lastFirebaseValue = "";
unsigned long lastIRPressTime = 0;
const int IR_DEBOUNCE_DELAY = 200; // 200ms debounce time

void setup()
{
    Serial.begin(115200);

    // ✅ Connect to WiFi (non-blocking)
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) // Timeout at 10s
    {
        Serial.print(".");
        delay(100);
    }
    Serial.println("\nWiFi Connected!");

    // ✅ Set up Firebase
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    Firebase.reconnectNetwork(true);
    Firebase.begin(&config, &auth);

    // ✅ Initialize LCD and IR Remote
    Wire.begin(6, 7); 
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Waiting for IR...");

    // ✅ Ensure Firebase path exists
    if (!Firebase.RTDB.getString(&fbdo, "/ir_remote/button_value"))
    {
        Serial.println("Creating default Firebase value...");
        Firebase.RTDB.setString(&fbdo, "/ir_remote/button_value", "None");
    }
}

void loop()
{
    // ✅ Poll for IR Remote Input (Optimized)
    int result = irRemote.checkRemote();
    if (result != -1 && millis() - lastIRPressTime > IR_DEBOUNCE_DELAY) // Prevent spamming
    {
        lastIRPressTime = millis(); // Update last press time

        Serial.print("You pressed: ");
        Serial.println(result);
        String buttonValue = String(result);

        // ✅ Update LCD only if needed
        if (buttonValue != lastDisplayedValue)
        {
            updateLCD(buttonValue);
        }

        // ✅ Send to Firebase only if changed
        if (buttonValue != lastFirebaseValue)
        {
            if (Firebase.RTDB.setString(&fbdo, "/ir_remote/button_value", buttonValue))
            {
                Serial.println("IR Data Sent to Firebase!");
                lastFirebaseValue = buttonValue;
            }
            else
            {
                Serial.println("Firebase Write Failed: " + String(fbdo.errorReason().c_str()));
            }
        }
    }

    // ✅ Check Firebase for Updates Every 2s
    if (Firebase.ready() && millis() - lastFirebaseCheck > FIREBASE_CHECK_INTERVAL)
    {
        lastFirebaseCheck = millis();

        if (Firebase.RTDB.getString(&fbdo, "/ir_remote/button_value"))
        {
            String firebaseValue = fbdo.stringData();

            // ✅ Update LCD only if Firebase value changed
            if (firebaseValue != lastDisplayedValue)
            {
                Serial.println("Updating LCD from Firebase: " + firebaseValue);
                updateLCD(firebaseValue);
            }
        }
        else
        {
            Serial.println("Firebase Read Failed: " + String(fbdo.errorReason().c_str()));
        }
    }
}

// ✅ Optimized LCD Update Function
void updateLCD(String value)
{
    if (value != lastDisplayedValue)
    {
        lcd.setCursor(0, 1);
        lcd.print("                "); // Clear line without `lcd.clear()`
        lcd.setCursor(0, 1);
        lcd.print(value);
        lastDisplayedValue = value; // Prevent redundant updates
    }
}
