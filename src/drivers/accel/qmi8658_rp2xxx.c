#include "qmi8658_rp2xxx.h"
#include "math.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/time.h"

#include "picowalker_core.h"

struct QMI8658_Config qmi8658_config;
struct QMI8658_PedoConfig pedo_config;

// Is Walking for the Splash screen
#define WALKING_TIMEOUT 4000
static uint32_t last_step_seen = 0;
static uint8_t is_walking = 0;

// Hardware Pedometer Engine Step Counting variables
static volatile bool pedometer_data_ready = false;
static uint32_t add_steps = 0;

// Software Pedometer Engine variables (mimics hardware engine)
uint32_t accumulated_steps = 0;
static unsigned int previous_hardware_steps = 0;
static struct repeating_timer step_timer;
static struct repeating_timer hardware_step_timer;
static uint32_t timer_callback_ms = 20; // get as close to QMI8658_AccOdr_62_5Hz sample rating
const uint32_t min_step_interval_ms = 1000; // Minimum time between steps
#define MAX_SAMPLES 50
static float accel_history[MAX_SAMPLES];
static uint8_t history_index = 0;
static bool history_filled = false;
static uint32_t last_step_time = 0;
static uint8_t consecutive_signals = 0;
static bool in_step_motion = false;
static uint32_t step_motion_start = 0;

/********************************************************************************
 * @brief           Timer callback for hardware pedometer engine (PEDOMETER_ENGINE=1)
 *                  Processes IRQ flag, reads hardware step count, updates walking state
 * @param timer     Repeating timer struct
 * @return bool     true to continue timer
********************************************************************************/
#if PEDOMETER_ENGINE
static bool hardware_pedometer_timer_callback(struct repeating_timer *timer)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool steps_added = false;

    // Hardware pedometer: process when IRQ has fired
    if (pedometer_data_ready)
    {
        unsigned int current_hardware_steps = 0;
        QMI8658_Read_Step_Count(&current_hardware_steps);
        if (current_hardware_steps > previous_hardware_steps)
        {
            // Hardware pedometer is working
            uint32_t new_hardware_steps = current_hardware_steps - previous_hardware_steps;
            accumulated_steps += new_hardware_steps;
            previous_hardware_steps = current_hardware_steps;
            steps_added = true;
            printf("[Debug] Hardware: +%u steps (total: %u)\n", new_hardware_steps, accumulated_steps);
        }
        pedometer_data_ready = false;
    }

    // 4-second walking state window
    if (steps_added)
    {
        last_step_seen = current_time;
        is_walking = 1;
    }
    else if ((current_time - last_step_seen) > WALKING_TIMEOUT)
    {
        is_walking = 0;
    }

    return true;
}
#endif

/********************************************************************************
 * @brief           Timer callback to mimic hardware pedometer engine (PEDOMETER_ENGINE=0)
 * @param timer     Repeating timer struct
 * @return bool     true to continue timer
********************************************************************************/
#if !PEDOMETER_ENGINE
static bool step_processing_timer_callback(struct repeating_timer *timer)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool steps_added = false;

    // Software pedometer using pedo_config parameters
    float accel[3];
    QMI8658_Read_Acc_XYZ(accel);

    // Calculate magnitude (total acceleration)
    float magnitude = sqrtf(accel[0] * accel[0] + accel[1] * accel[1] + accel[2] * accel[2]);

    // Sliding continuous buffer history
    accel_history[history_index] = magnitude;
    history_index = (history_index + 1) % pedo_config.sample_count;
    if (history_index == 0) history_filled = true;
    if (!history_filled) return true;

    // Find crest and trough in the sample window
    float crest = accel_history[0];
    float trough = accel_history[0];
    for (uint8_t i = 1; i < pedo_config.sample_count; i++)
    {
        if (accel_history[i] > crest) crest = accel_history[i];
        if (accel_history[i] < trough) trough = accel_history[i];
    }

    // Check if current motion qualifies as a step signal
    float peak2peak = crest - trough;
    float peak_deviation = fabsf(crest - 9.8f); // against a baseline gravity
    bool is_step_signal = (peak2peak > (pedo_config.fix_peak2peak / 100.0f)) &&
                          (peak_deviation > (pedo_config.fix_peak / 100.0f));
    uint32_t time_since_last_step = current_time - last_step_time;

    // Don't start new motion until minimum interval has passed since last step
    if (is_step_signal && !in_step_motion && time_since_last_step >= min_step_interval_ms)
    {
        in_step_motion = true;
        step_motion_start = current_time;
        consecutive_signals = 1;
    }
    else if (is_step_signal && in_step_motion)
    {
        consecutive_signals++;
        uint32_t motion_duration = current_time - step_motion_start;

        // Check if we have enough signals to confirm a step
        if (consecutive_signals >= pedo_config.signal_count &&
            motion_duration >= pedo_config.time_low &&
            motion_duration <= pedo_config.time_up)
        {
            accumulated_steps++;
            last_step_time = current_time;
            in_step_motion = false;
            consecutive_signals = 0;
            steps_added = true;
            printf("[Debug] Software: Step detected! Total: %u, P2P: %.2f, Peak: %.2f, Signals: %u, Duration: %ums\n",
                   accumulated_steps, peak2peak, peak_deviation, pedo_config.signal_count, motion_duration);
        }
        else if (motion_duration > pedo_config.time_up)
        {
            // Motion too long - reset
            in_step_motion = false;
            consecutive_signals = 0;
        }
    }
    else if (!is_step_signal && in_step_motion)
    {
        // Motion ended - reset
        in_step_motion = false;
        consecutive_signals = 0;
    }

    // 4-second walking state window
    if (steps_added)
    {
        last_step_seen = current_time;
        is_walking = 1;
    }
    else if ((current_time - last_step_seen) > WALKING_TIMEOUT)
    {
        is_walking = 0;
    }

    return true;
}
#endif

/********************************************************************************
 * @brief           Accel IRQ Callback
 * @param gpio      Signal PIN
 * @param events    Events from LVGL
********************************************************************************/
static void accel_irq_callback(uint gpio, uint32_t events)
{
    if (gpio == DOF_INT1)
    {
        pedometer_data_ready = true;
    }
}

/*
 * ============================================================================
 * Picowalker Driver Functions
 * ============================================================================
 */

/********************************************************************************
 * @brief           Accel Initialize with Hardware Pedometer + Software Fallback
 * @param N/A
********************************************************************************/
void pw_accel_init()
{
    // Initialize hardware pedometer configuration (original settings)
    qmi8658_config.inputSelection = QMI8658_CONFIG_ACC_ENABLE;
    qmi8658_config.accRange = QMI8658_AccRange_2g;
    qmi8658_config.accOdr = QMI8658_AccOdr_62_5Hz;
    qmi8658_config.gyrRange = QMI8658_GyrRange_512dps;
    qmi8658_config.gyrOdr = QMI8658_GyrOdr_1000Hz;
    qmi8658_config.magDev = QMI8658_MagDev_AKM09918;
    qmi8658_config.magOdr = QMI8658_MagOdr_125Hz;
    qmi8658_config.aeOdr = QMI8658_AeOdr_128Hz;
    
    qmi8658_config.enablePedometer = 1;
    // Original RP2040 settings (kept for reference)
    pedo_config.sample_count = 50;
    pedo_config.fix_peak2peak = 200;
    pedo_config.fix_peak = 100;
    pedo_config.time_up = 200;
    pedo_config.time_low = 20;
    pedo_config.time_count_entry = 10;
    pedo_config.fix_precision = 0;
    pedo_config.signal_count = 4;

    // More sensitive settings for RP2350
    // pedo_config.sample_count = 80;          // Increased from 50
    // pedo_config.fix_peak2peak = 300;        // Increased from 200
    // pedo_config.fix_peak = 150;             // Increased from 100
    // pedo_config.time_up = 250;              // Increased from 200
    // pedo_config.time_low = 15;              // Decreased from 20
    // pedo_config.time_count_entry = 8;       // Decreased from 10
    // pedo_config.fix_precision = 0;          // Keep same
    // pedo_config.signal_count = 3;  
    
    qmi8658_config.pedoConfig = pedo_config;

    QMI8658_init(qmi8658_config);
    QMI8658_Config_Pedometer_Interrupt();

    // IRQ Config
    gpio_init(DOF_INT1);
    gpio_set_dir(DOF_INT1, GPIO_IN);
    gpio_pull_down(DOF_INT1);

    // Enable interrupt on rising edge (when QMI8658 sets INT1 high)
    gpio_set_irq_enabled_with_callback(DOF_INT1, GPIO_IRQ_EDGE_RISE, true, &accel_irq_callback);

#if PEDOMETER_ENGINE
    // Get initial hardware step count and start hardware pedometer timer
    QMI8658_Read_Step_Count(&previous_hardware_steps);
    add_repeating_timer_ms(200, hardware_pedometer_timer_callback, NULL, &hardware_step_timer);
#else
    // Get initial hardware step count and start software pedometer timer
    QMI8658_Read_Step_Count(&previous_hardware_steps);
    add_repeating_timer_ms(timer_callback_ms, step_processing_timer_callback, NULL, &step_timer);
#endif
}

/********************************************************************************
 * @brief           Accel Sleep - Reduce power consumption
 * @param N/A
********************************************************************************/
void pw_accel_sleep()
{
#if PEDOMETER_ENGINE
    cancel_repeating_timer(&hardware_step_timer);
#else
    // Cancel step processing timer to save power
    cancel_repeating_timer(&step_timer);
#endif
    // Keep accelerometer enabled for hardware pedometer
    QMI8658_Enable_Sensors(QMI8658_CTRL7_ACC_ENABLE);
    printf("[Debug] Accelerometer sleeping - timer stopped, hardware pedometer active\n");
}

/********************************************************************************
 * @brief           Accel Wake up - Resume normal operation
 * @param N/A
********************************************************************************/
void pw_accel_wake()
{
    // Re-enable accelerometer and restart step processing timer
    QMI8658_Enable_Sensors(QMI8658_CTRL7_ACC_ENABLE);
#if PEDOMETER_ENGINE
    add_repeating_timer_ms(200, hardware_pedometer_timer_callback, NULL, &hardware_step_timer);
#else
    history_filled = false;
    history_index = 0;
    add_repeating_timer_ms(timer_callback_ms, step_processing_timer_callback, NULL, &step_timer);
#endif
    printf("[Debug] Accelerometer wake up - timer restarted\n");
}

/********************************************************************************
 * @brief           Accel Get New Steps - Returns accumulated steps from timer
 * @param N/A
 * @return uint32_t Number of new steps since last call
********************************************************************************/
uint32_t pw_accel_get_new_steps()
{
    // Both PEDOMETER_ENGINE and software paths accumulate into accumulated_steps via their timers.
    // This function is called every ~30s to record steps to EEPROM.
    static uint32_t last_recorded_steps = 0;
    uint32_t new_steps = accumulated_steps - last_recorded_steps;
    last_recorded_steps = accumulated_steps;

    // This is for adding steps manually (Cheating...mainly for debugging)
    if (add_steps > 0)
    {
        new_steps += add_steps;
        accumulated_steps += add_steps;
        last_recorded_steps = accumulated_steps;
        add_steps = 0;
    }

    if (new_steps > 0) printf("[Debug] EEPROM: %u new steps (total: %u)\n", new_steps, accumulated_steps);
    return new_steps;
}

/********************************************************************************
 * @brief           Reset Step Counter
 * @param N/A
********************************************************************************/
void pw_accel_reset_steps()
{    
    accumulated_steps = 0;
    previous_hardware_steps = 0;

    // Reset Software Pedometer Engine
    history_index = 0;
    history_filled = false;
    last_step_time = 0;
    consecutive_signals = 0;
    in_step_motion = false;
    step_motion_start = 0;
    for (uint8_t i = 0; i < MAX_SAMPLES; i++) 
    {
        accel_history[i] = 0.0f;
    }

    // Try to reset hardware counter
    QMI8658_Reset_Step_Count();
    QMI8658_Read_Step_Count(&previous_hardware_steps);

    printf("[Debug] Step counter reset - Hardware + Software\n");
}

/********************************************************************************
 * @brief           Add manual steps (for canvas press simulation)
 * @param steps     Number of steps to add
********************************************************************************/
void pw_accel_add_steps(uint32_t steps)
{
    is_walking = 1;
    add_steps += steps;
    printf("[Debug] Added %u manual steps (total: %u)\n", steps, add_steps);
}

/********************************************************************************
 * @brief           Accel Get New Steps - Returns accumulated steps from timer
 * @param N/A
 * @return uint32_t Number of new steps since last call
********************************************************************************/
uint8_t pw_accel_get_activity()
{
    return is_walking;
}