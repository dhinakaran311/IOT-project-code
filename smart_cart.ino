#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <MFRC522.h>

// =============================================
// PIN DEFINITIONS
// =============================================
#define OLED_SDA 21
#define OLED_SCL 22
#define RFID_SDA 5
#define RFID_RST 4

// =============================================
// OLED DISPLAY SETUP
// =============================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =============================================
// RFID SETUP
// =============================================
MFRC522 mfrc522(RFID_SDA, RFID_RST);

// =============================================
// PRODUCT DATABASE
// =============================================
struct Product {
  String uid;
  String name;
  float price;
  String category;
};

Product products[] = {
  {"8331721E", "Apple", 80.0, "Fruits"},
  {"427B5222", "Milk", 60.0, "Dairy"},
  {"ADD MORE", "Banana", 40.0, "Fruits"},
  {"ADD MORE", "Bread", 45.0, "Bakery"},
  {"ADD MORE", "Eggs", 70.0, "Dairy"},
  {"ADD MORE", "Rice", 120.0, "Grains"},
  {"ADD MORE", "Chocolate", 50.0, "Snacks"}
};

// =============================================
// SHOPPING CART VARIABLES
// =============================================
float totalAmount = 0.0;
int itemCount = 0;
bool cartActive = false;
String lastProduct = "None";
unsigned long lastDisplayUpdate = 0;


// =============================================
// EVENT EMIT FUNCTION (SEND DATA TO LAPTOP)
// =============================================
void emitEvent(const String& type, const String& name, float price) {
  Serial.println("EVENT:" + type + ":" + name + ":" + String(price, 2));
}

// =============================================
// SETUP FUNCTION
// =============================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== SMART CART SYSTEM STARTING ===");
  Serial.println("Using your RFID cards:");
  Serial.println("Card 1: 8331721E -> Apple");
  Serial.println("Card 2: 427B5222 -> Milk");

  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED Display
  Serial.println("Initializing OLED Display...");
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("❌ SSD1306 allocation failed!");
    while(true);
  }

  // Initialize RFID for ESP32
  Serial.println("Initializing RFID Reader...");
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS (for ESP32)
  mfrc522.PCD_Init();
  delay(4);
  Serial.println("✅ RFID Reader Ready!");

  // Display welcome message
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  showWelcomeScreen();

  Serial.println("✅ System Ready! Scan your RFID cards to start shopping.");
  Serial.println("📱 Serial Commands: 'reset', 'checkout', 'status', 'help'");
}

// =============================================
// MAIN LOOP
// =============================================
void loop() {
  checkRFID();
  checkSerialCommands();
  updateDisplayPeriodically();
  delay(100);
}

// =============================================
// RFID FUNCTIONS
// =============================================
void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String cardUID = getCardUID();
  Serial.println("RFID Detected: " + cardUID);

  if (!cartActive) {
    startShopping();
  } else {
    scanProduct(cardUID);
  }

  // ✅ Correct halt commands for ESP32-compatible MFRC522
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

String getCardUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// =============================================
// CART CONTROL FUNCTIONS
// =============================================
void startShopping() {
  cartActive = true;
  totalAmount = 0.0;
  itemCount = 0;
  lastProduct = "None";

  Serial.println("🛒 SHOPPING STARTED!");

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SHOPPING STARTED");
  display.println("Scan products...");
  display.display();

  delay(2000);
  updateMainDisplay();
}

void scanProduct(String cardUID) {
  Product* product = findProduct(cardUID);

  if (product != NULL) {
    totalAmount += product->price;
    itemCount++;
    lastProduct = product->name;

    Serial.println("📦 " + product->name + " - ₹" + String(product->price));
    Serial.println("💰 Total: ₹" + String(totalAmount));
    Serial.println("------------------------");

    emitEvent("ADD", product->name, product->price);

    showProductAddedScreen(product->name, product->price);
  } else {
    Serial.println("❌ Unknown product: " + cardUID);
    showUnknownProductScreen();
  }
}

Product* findProduct(String uid) {
  for (int i = 0; i < sizeof(products)/sizeof(products[0]); i++) {
    if (products[i].uid == uid) return &products[i];
  }
  return NULL;
}

// =============================================
// DISPLAY FUNCTIONS
// =============================================
void showWelcomeScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 0);
  display.println("SMART");
  display.setCursor(25, 20);
  display.println("CART");
  display.setTextSize(1);
  display.setCursor(15, 45);
  display.println("Scan RFID to start");
  display.display();
}

void updateMainDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART CART - ACTIVE");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setCursor(0, 15);
  display.print("Last: "); display.println(lastProduct);
  display.setCursor(0, 25);
  display.print("Items: "); display.println(itemCount);
  display.setCursor(0, 35);
  display.print("Total: ₹"); display.println(totalAmount);
  display.display();
}

void showProductAddedScreen(String productName, float price) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("ADDED!");
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.print("Product: "); display.println(productName);
  display.setCursor(0, 35);
  display.print("Price: ₹"); display.println(price);
  display.setCursor(0, 45);
  display.print("Total: ₹"); display.println(totalAmount);
  display.display();
  delay(2000);
  updateMainDisplay();
}

void showUnknownProductScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("UNKNOWN");
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println("PRODUCT!");
  display.setCursor(0, 40);
  display.println("Try another card");
  display.display();
  delay(2000);
  updateMainDisplay();
}

void showCheckoutScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 0);
  display.println("BILL");
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.print("Items: "); display.println(itemCount);
  display.setCursor(0, 35);
  display.print("Total: ₹"); display.println(totalAmount);
  display.setCursor(10, 50);
  display.println("Thank You!");
  display.display();
}

void updateDisplayPeriodically() {
  unsigned long currentTime = millis();
  if (currentTime - lastDisplayUpdate > 5000) {
    lastDisplayUpdate = currentTime;
    if (cartActive) updateMainDisplay();
  }
}

// =============================================
// SERIAL COMMANDS
// =============================================
void checkSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "reset") resetCart();
    else if (command == "checkout") checkout();
    else if (command == "status") showStatus();
    else if (command == "help") showHelp();
    else if (command.startsWith("add ")) addManualProduct(command);
  }
}

void resetCart() {
  cartActive = false;
  totalAmount = 0.0;
  itemCount = 0;
  lastProduct = "None";
  showWelcomeScreen();
  Serial.println("✅ Cart reset!");
}

void checkout() {
  if (itemCount == 0) {
    Serial.println("❌ Cart is empty!");
    return;
  }
  Serial.println("\n💰 CHECKOUT COMPLETED");
  Serial.println("====================");
  Serial.println("Items: " + String(itemCount));
  Serial.println("Total: ₹" + String(totalAmount));
  Serial.println("Thank you for shopping!");
  showCheckoutScreen();
}

void showStatus() {
  Serial.println("\n📊 CART STATUS");
  Serial.println("==============");
  Serial.println("Cart Active: " + String(cartActive ? "Yes" : "No"));
  Serial.println("Items: " + String(itemCount));
  Serial.println("Total: ₹" + String(totalAmount));
  Serial.println("Last Product: " + lastProduct);
}

void showHelp() {
  Serial.println("\n🆘 SERIAL COMMANDS");
  Serial.println("=================");
  Serial.println("reset        - Reset shopping cart");
  Serial.println("checkout     - Show final bill");
  Serial.println("status       - Show current status");
  Serial.println("add [name] [price] - Manually add item");
  Serial.println("help         - Show this help message");
}

void addManualProduct(String command) {
  if (!cartActive) startShopping();

  int firstSpace = command.indexOf(' ');
  int secondSpace = command.indexOf(' ', firstSpace + 1);

  if (firstSpace != -1 && secondSpace != -1) {
    String productName = command.substring(firstSpace + 1, secondSpace);
    float price = command.substring(secondSpace + 1).toFloat();

    totalAmount += price;
    itemCount++;
    lastProduct = productName;

    emitEvent("ADD", productName, price);
    
    Serial.println("📦 " + productName + " - ₹" + String(price) + " (Manual)");
    Serial.println("💰 Total: ₹" + String(totalAmount));
    showProductAddedScreen(productName, price);
  } else {
    Serial.println("❌ Usage: add [name] [price]");
  }
}

// reset        # Reset cart
// checkout     # Show final bill  
// status       # Check cart status
// add Banana 40 # Manually add Banana for ₹40
// help         # Show all commands