#include "relay_test.h"
#include "main.h"

#define DEBOUNCE_MS 20   // contact counts as stable after 20 ms without changes
#define TIMEOUT_MS  500  // give up if the relay does not react within 500 ms
#define PAUSE_MS    300  // pause between switching the relay

// Level on PA1 that switches the relay on.
// This module is active LOW: the relay clicks when IN goes to GND.
#define RELAY_ON    GPIO_PIN_RESET
#define RELAY_OFF   GPIO_PIN_SET

// Contact levels on PA4 (NO to PA4, COM to GND, pull-up on PA4).
#define CONTACT_CLOSED  GPIO_PIN_RESET
#define CONTACT_OPEN    GPIO_PIN_SET

// Filled by the interrupt while a measurement is running.
static volatile bool     listening = false;
static volatile uint32_t changeCount = 0;
static volatile uint32_t firstChangeMs = 0;
static volatile uint32_t lastChangeMs = 0;

void onRelayContactInterrupt(void)
{
    if (!listening) {
        return;
    }

    const uint32_t now = HAL_GetTick();
    if (changeCount == 0) {
        firstChangeMs = now;
    }
    lastChangeMs = now;
    changeCount++;
}

static uint32_t msSinceLastChange(void)
{
    const uint32_t last = lastChangeMs;  // read first: the interrupt may update it
    return HAL_GetTick() - last;
}

// Switches the relay and measures how its contact reacts.
// Returns false (and leaves both times at 0) if the relay did not respond.
static bool measureSwitch(GPIO_PinState coil, GPIO_PinState expectedContact,
                          uint32_t *switchMs, uint32_t *bounceMs)
{
    *switchMs = 0;
    *bounceMs = 0;

    changeCount = 0;
    listening = true;

    const uint32_t startMs = HAL_GetTick();
    HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, coil);

    // 1. Wait for the first contact change.
    while (changeCount == 0) {
        if (HAL_GetTick() - startMs > TIMEOUT_MS) {
            listening = false;
            return false;
        }
    }

    // 2. Wait until the contact stops bouncing.
    while (msSinceLastChange() < DEBOUNCE_MS) {
    }
    listening = false;

    // 3. Check that the contact ended up where it should.
    if (HAL_GPIO_ReadPin(RELAY_CONTACT_GPIO_Port, RELAY_CONTACT_Pin) != expectedContact) {
        return false;
    }

    *switchMs = firstChangeMs - startMs;
    *bounceMs = lastChangeMs - firstChangeMs;
    return true;
}

static uint32_t average(uint32_t total, uint32_t count)
{
    return count ? total / count : 0;
}

void runMeasurements(RelayResults *results)
{
    uint32_t totalOnMs = 0, totalOffMs = 0;
    uint32_t onCount = 0, offCount = 0;

    results->done = false;

    HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, RELAY_OFF);
    HAL_Delay(PAUSE_MS);

    for (int i = 0; i < MEASUREMENT_COUNT; i++) {
        // Relay on: the contact closes, the pin reads LOW.
        if (measureSwitch(RELAY_ON, CONTACT_CLOSED,
                          &results->switchOnMs[i], &results->switchOnBounceMs[i])) {
            totalOnMs += results->switchOnMs[i];
            onCount++;
        }
        HAL_Delay(PAUSE_MS);

        // Relay off: the contact opens, the pull-up returns the pin to HIGH.
        if (measureSwitch(RELAY_OFF, CONTACT_OPEN,
                          &results->switchOffMs[i], &results->switchOffBounceMs[i])) {
            totalOffMs += results->switchOffMs[i];
            offCount++;
        }
        HAL_Delay(PAUSE_MS);
    }

    results->averageOnMs = average(totalOnMs, onCount);
    results->averageOffMs = average(totalOffMs, offCount);
    results->done = true;
}
