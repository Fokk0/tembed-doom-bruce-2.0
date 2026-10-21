// Doom (doomgeneric) fuer LilyGO T-Embed CC1101 (Plus): Display, Drehgeber, Task.
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>

extern "C" {
#include "doomgeneric.h"
#include "doomkeys.h"
extern int menuactive;   // m_menu.c
extern int gamestate;    // 0 = GS_LEVEL
}

// ---- Pins (T-Embed CC1101). Bei Abweichung per build_flags ueberschreibbar ----
#ifndef DOOM_ENC_A
#define DOOM_ENC_A 4
#endif
#ifndef DOOM_ENC_B
#define DOOM_ENC_B 5
#endif
#ifndef DOOM_ENC_KEY
#define DOOM_ENC_KEY 0
#endif
#ifndef DOOM_ENC_DIR
#define DOOM_ENC_DIR 1          // -1 = Drehrichtung umkehren
#endif
#ifndef DOOM_ROTATION
#define DOOM_ROTATION 3         // 1 oder 3 (Display um 180 Grad drehen)
#endif
#define SCR_W 320
#define SCR_H 170

static TFT_eSPI tft;
static uint16_t *fb;

// ---------------- Drehgeber ----------------
static volatile int encAcc = 0;
static volatile uint8_t encSt = 0;
static void IRAM_ATTR encIsr() {
    static const DRAM_ATTR int8_t tbl[16] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};
    encSt = ((encSt << 2) | (digitalRead(DOOM_ENC_A) << 1) | digitalRead(DOOM_ENC_B)) & 0x0F;
    encAcc += tbl[encSt];
}

// ---------------- Tastenwarteschlange ----------------
struct Ev { uint8_t pressed, key; };
static Ev q[64]; static uint8_t qh = 0, qt = 0;
static void push(int pressed, int key) {
    uint8_t n = (qt + 1) & 63;
    if (n == qh) return;
    q[qt] = { (uint8_t)pressed, (uint8_t)key }; qt = n;
}
struct Rel { uint8_t key; uint32_t at; bool used; };
static Rel rel[8];
static void tap(int key, uint32_t holdMs = 70) {
    push(1, key);
    for (auto &r : rel) if (!r.used) { r = { (uint8_t)key, millis() + holdMs, true }; return; }
    push(0, key);
}

// ---------------- Eingabelogik ----------------
static bool btn = false, btnRaw = false, rotated = false, longDone = false;
static uint32_t btnChange = 0, pressT = 0, lastClick = 0;
static int clicks = 0, weapon = 2, turnKey = 0, turnDir = 0;
static uint32_t turnUntil = 0;
static bool walking = false, lastMenuMode = true;

static void releaseHeld() {
    if (walking) { push(0, KEY_UPARROW); walking = false; }
    if (turnKey) { push(0, turnKey); turnKey = 0; }
    turnUntil = 0;
}

static void onClick(uint32_t now, bool menuMode) {
    clicks = (now - lastClick < 350) ? clicks + 1 : 1;
    lastClick = now;
    if (menuMode) {
        if (clicks == 1) tap(KEY_ENTER);
        else if (clicks == 2) { tap('y'); clicks = 0; }       // z.B. Quit bestaetigen
    } else {
        tap(KEY_FIRE);                                         // Klick = Schuss
        if (clicks == 2) tap(KEY_USE);                         // Doppelklick = Benutzen/Tuer
        else if (clicks == 3) { tap(KEY_ESCAPE); clicks = 0; } // Dreifachklick = Menue
    }
}

static void onLong(bool menuMode) {
    if (menuMode) tap(KEY_ESCAPE);                             // Langer Druck im Menue = Zurueck
    else { walking = !walking; push(walking ? 1 : 0, KEY_UPARROW); } // Dauer-Vorwaerts an/aus
}

static void pump() {
    uint32_t now = millis();
    for (auto &r : rel)
        if (r.used && (int32_t)(now - r.at) >= 0) { push(0, r.key); r.used = false; }

    bool menuMode = menuactive || gamestate != 0;
    if (menuMode != lastMenuMode) { lastMenuMode = menuMode; releaseHeld(); }

    int steps;
    noInterrupts(); steps = encAcc / 4; encAcc -= steps * 4; interrupts();
    steps *= DOOM_ENC_DIR;

    bool raw = digitalRead(DOOM_ENC_KEY) == LOW;
    if (raw != btnRaw) { btnRaw = raw; btnChange = now; }
    if (now - btnChange > 20 && raw != btn) {
        btn = raw;
        if (btn) { pressT = now; rotated = false; longDone = false; }
        else if (!longDone && !rotated) onClick(now, menuMode);
    }
    if (btn && !longDone && !rotated && now - pressT >= 700) { longDone = true; onLong(menuMode); }

    if (steps) {
        if (btn) {                                             // gedrueckt drehen = Waffe waehlen
            rotated = true;
            if (!menuMode) { weapon = (((weapon - 1 + steps) % 7) + 7) % 7 + 1; tap('0' + weapon); }
        } else if (menuMode) {
            for (int i = 0; i < abs(steps); i++) tap(steps > 0 ? KEY_DOWNARROW : KEY_UPARROW, 40);
        } else { turnDir = steps > 0 ? 1 : -1; turnUntil = now + 150; }
    }
    int want = (!menuMode && (int32_t)(turnUntil - now) > 0) ? (turnDir > 0 ? KEY_RIGHTARROW : KEY_LEFTARROW) : 0;
    if (want != turnKey) { if (turnKey) push(0, turnKey); if (want) push(1, want); turnKey = want; }
}

// ---------------- doomgeneric-Schnittstelle ----------------
extern "C" {
void DG_Init() {}
void DG_SleepMs(uint32_t ms) { delay(ms); }
uint32_t DG_GetTicksMs() { return millis(); }
void DG_SetWindowTitle(const char *) {}

int DG_GetKey(int *pressed, unsigned char *key) {
    pump();
    if (qh == qt) return 0;
    *pressed = q[qh].pressed; *key = q[qh].key; qh = (qh + 1) & 63;
    return 1;
}

void DG_DrawFrame() {
    const uint32_t *src = DG_ScreenBuffer;
    for (int y = 0; y < SCR_H; y++) {                          // 200 -> 170 Zeilen (Nearest)
        const uint32_t *row = src + ((y * 200) / SCR_H) * SCR_W;
        uint16_t *d = fb + y * SCR_W;
        for (int x = 0; x < SCR_W; x++) {
            uint32_t p = row[x];
            d[x] = ((p >> 8) & 0xF800) | ((p >> 5) & 0x07E0) | ((p >> 3) & 0x001F);
        }
    }
    tft.pushImage(0, 0, SCR_W, SCR_H, fb);
}
}

static void doomTask(void *) {
    static char a0[] = "doom", a1[] = "-iwad", a2[] = "doom1.wad";
    char *argv[] = { a0, a1, a2, NULL };
    doomgeneric_Create(3, argv);
    for (;;) doomgeneric_Tick();
}

void setup() {
    Serial.begin(115200);
    pinMode(15, OUTPUT); digitalWrite(15, HIGH);               // Peripherie-Strom (T-Embed PWR_EN)
    delay(50);
    tft.init(); tft.setRotation(DOOM_ROTATION); tft.setSwapBytes(true); tft.fillScreen(TFT_BLACK);
#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
#endif
    fb = (uint16_t *)heap_caps_malloc(SCR_W * SCR_H * 2, MALLOC_CAP_8BIT);
    pinMode(DOOM_ENC_A, INPUT_PULLUP); pinMode(DOOM_ENC_B, INPUT_PULLUP); pinMode(DOOM_ENC_KEY, INPUT_PULLUP);
    attachInterrupt(DOOM_ENC_A, encIsr, CHANGE); attachInterrupt(DOOM_ENC_B, encIsr, CHANGE);
    xTaskCreatePinnedToCore(doomTask, "doom", 65536, NULL, 1, NULL, 1);
}

void loop() { vTaskDelete(NULL); }
