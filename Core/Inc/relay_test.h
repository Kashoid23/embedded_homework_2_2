#ifndef RELAY_TEST_H
#define RELAY_TEST_H

#include <stdbool.h>
#include <stdint.h>

#define MEASUREMENT_COUNT  10  // how many on/off cycles to measure

// All results in milliseconds. A switch time of 0 means "no response".
typedef struct {
    uint32_t switchOnMs[MEASUREMENT_COUNT];
    uint32_t switchOffMs[MEASUREMENT_COUNT];
    uint32_t switchOnBounceMs[MEASUREMENT_COUNT];
    uint32_t switchOffBounceMs[MEASUREMENT_COUNT];
    uint32_t averageOnMs;
    uint32_t averageOffMs;
    bool     done;
} RelayResults;

// Switches the relay MEASUREMENT_COUNT times on and off and fills the results.
void runMeasurements(RelayResults *results);

// Call from the EXTI interrupt: records the time of a relay contact change.
void onRelayContactInterrupt(void);

#endif /* RELAY_TEST_H */
