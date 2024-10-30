#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "acc_config.h"
#include "acc_definitions_a121.h"
#include "acc_definitions_common.h"
#include "acc_hal_definitions_a121.h"
#include "acc_hal_integration_a121.h"
#include "acc_integration.h"
#include "acc_processing.h"
#include "acc_rss_a121.h"
#include "acc_sensor.h"
#include "acc_detector_distance.h"

#include "acc_version.h"

#define SENSOR_ID          (1U)
#define SENSOR_TIMEOUT_MS  (1000U)
#define MAX_DATA_ENTRY_LEN (15U) // "-32000+-32000i" + zero termination

// Global/static variables to retain state between function calls
static acc_config_t *config = NULL;
static acc_processing_t *processing = NULL;
static acc_sensor_t *sensor = NULL;
static void *buffer = NULL;
static uint32_t buffer_size = 0;
static acc_processing_metadata_t proc_meta;
static bool is_initialized = false; // Global flag to track initialization

static void set_config(acc_config_t *config);
static bool do_sensor_calibration_and_prepare(acc_sensor_t *sensor, acc_config_t *config, void *buffer, uint32_t buffer_size);
static void print_data(acc_int16_complex_t *data, uint16_t data_length);
static void cleanup();

bool acc_example_service_init();
int acc_example_service(int argc, uint8_t argv[]);

bool acc_example_service_init()
{
    const acc_hal_a121_t *hal = acc_hal_rss_integration_get_implementation();

    if (!acc_rss_hal_register(hal))
    {
        return false;
    }

    config = acc_config_create();
    if (config == NULL)
    {
        printf("acc_config_create() failed\n");
        cleanup();
        return false;
    }

    set_config(config);

    // Print the configuration
    acc_config_log(config);

    processing = acc_processing_create(config, &proc_meta);
    if (processing == NULL)
    {
        printf("acc_processing_create() failed\n");
        cleanup();
        return false;
    }

    if (!acc_rss_get_buffer_size(config, &buffer_size))
    {
        printf("acc_rss_get_buffer_size() failed\n");
        cleanup();
        return false;
    }

    buffer = acc_integration_mem_alloc(buffer_size);
    if (buffer == NULL)
    {
        printf("Buffer allocation failed\n");
        cleanup();
        return false;
    }

    acc_hal_integration_sensor_supply_on(SENSOR_ID);
    acc_hal_integration_sensor_enable(SENSOR_ID);

    sensor = acc_sensor_create(SENSOR_ID);
    if (sensor == NULL)
    {
        printf("acc_sensor_create() failed\n");
        cleanup();
        return false;
    }

    if (!do_sensor_calibration_and_prepare(sensor, config, buffer, buffer_size))
    {
        printf("do_sensor_calibration_and_prepare() failed\n");
        acc_sensor_status(sensor);
        cleanup();
        return false;
    }

    is_initialized = true; // Mark as initialized
    return true;
}

int acc_example_service(int argc, uint8_t argv[])
{
     //Initialize on first run
    if (!is_initialized)
    {
        if (!acc_example_service_init())
        {
            return EXIT_FAILURE;
        }
    }

    acc_processing_result_t proc_result;

    if (!acc_sensor_measure(sensor))
    {
        printf("acc_sensor_measure failed\n");
        acc_sensor_status(sensor);
        cleanup();
        return EXIT_FAILURE;
    }

    if (!acc_hal_integration_wait_for_sensor_interrupt(SENSOR_ID, SENSOR_TIMEOUT_MS))
    {
        printf("Sensor interrupt timeout\n");
        acc_sensor_status(sensor);
        cleanup();
        return EXIT_FAILURE;
    }

    if (!acc_sensor_read(sensor, buffer, buffer_size))
    {
        printf("acc_sensor_read failed\n");
        acc_sensor_status(sensor);
        cleanup();
        return EXIT_FAILURE;
    }

    acc_processing_execute(processing, buffer, &proc_result);

    if (proc_result.calibration_needed)
    {
        printf("The current calibration is not valid for the current temperature.\n");
        printf("The sensor needs to be re-calibrated.\n");

        if (!do_sensor_calibration_and_prepare(sensor, config, buffer, buffer_size))
        {
            printf("do_sensor_calibration_and_prepare() failed\n");
            acc_sensor_status(sensor);
            cleanup();
            return EXIT_FAILURE;
        }

        printf("The sensor was successfully re-calibrated.\n");
    }
    else
    {

		for (uint16_t j = 0; (j < proc_meta.frame_data_length) && (4*(j+1) <= argc); j++)

		{

            argv[1+4*j] = (uint8_t)(proc_result.frame[j].real & 0xFF);
            argv[0+4*j] = (uint8_t)((proc_result.frame[j].real >> 8) & 0xFF);
            argv[3+4*j] = (uint8_t)(proc_result.frame[j].imag & 0xFF);
            argv[2+4*j] = (uint8_t)((proc_result.frame[j].imag >> 8) & 0xFF);
		}
       print_data(proc_result.frame, proc_meta.frame_data_length);
    }

    return EXIT_SUCCESS;
}

//static void set_config(acc_config_t *config)
//{
//	acc_config_sweep_rate_set(config,110.0f);
//    acc_config_hwaas_set(config,10);
//    acc_config_start_point_set(config, 20);
//    acc_config_num_points_set(config, 40);
//    acc_config_step_length_set(config,1);
//    acc_config_profile_set (config , ACC_CONFIG_PROFILE_1);
//    acc_config_continuous_sweep_mode_set(config,true);
//    acc_config_receiver_gain_set(config,12);
//    acc_config_inter_sweep_idle_state_set(config,ACC_CONFIG_IDLE_STATE_READY);
//    acc_config_inter_frame_idle_state_set(config,ACC_CONFIG_IDLE_STATE_READY);
//    acc_config_prf_set(config,ACC_CONFIG_PRF_19_5_MHZ);
//
////    acc_config_phase_enhancement_set(config,true);
////    acc_config_double_buffering_set(config,true);
////    acc_config_enable_loopback_set(config,true);
////    acc_detector_distance_config_start_set()
//
//}


static void set_config(acc_config_t *config)
{
	acc_config_sweep_rate_set(config,110.0f);
    acc_config_hwaas_set(config,100);
    acc_config_start_point_set(config, 20);
    acc_config_num_points_set(config, 40);
    acc_config_step_length_set(config,1);
    acc_config_profile_set (config , ACC_CONFIG_PROFILE_1);
    acc_config_continuous_sweep_mode_set(config,true);
    acc_config_receiver_gain_set(config,2);
    acc_config_inter_sweep_idle_state_set(config,ACC_CONFIG_IDLE_STATE_READY);
    acc_config_inter_frame_idle_state_set(config,ACC_CONFIG_IDLE_STATE_READY);
    acc_config_prf_set(config,ACC_CONFIG_PRF_19_5_MHZ);

//    acc_config_phase_enhancement_set(config,true);
//    acc_config_double_buffering_set(config,true);
//    acc_config_enable_loopback_set(config,true);
//    acc_detector_distance_config_start_set()

}

static bool do_sensor_calibration_and_prepare(acc_sensor_t *sensor, acc_config_t *config, void *buffer, uint32_t buffer_size)
{
    bool status = false;
    bool cal_complete = false;
    acc_cal_result_t cal_result;
    const uint16_t calibration_retries = 1U;

    // Random disturbances may cause the calibration to fail. At failure, retry at least once.
    for (uint16_t i = 0; !status && (i <= calibration_retries); i++)
    {
        // Reset sensor before calibration by disabling/enabling it
        acc_hal_integration_sensor_disable(SENSOR_ID);
        acc_hal_integration_sensor_enable(SENSOR_ID);

        do
        {
            status = acc_sensor_calibrate(sensor, &cal_complete, &cal_result, buffer, buffer_size);

            if (status && !cal_complete)
            {
                status = acc_hal_integration_wait_for_sensor_interrupt(SENSOR_ID, SENSOR_TIMEOUT_MS);
            }
        } while (status && !cal_complete);
    }

    if (status)
    {
        // Reset sensor after calibration by disabling/enabling it
        acc_hal_integration_sensor_disable(SENSOR_ID);
        acc_hal_integration_sensor_enable(SENSOR_ID);

        status = acc_sensor_prepare(sensor, config, &cal_result, buffer, buffer_size);
    }

    return status;
}

static void print_data(acc_int16_complex_t *data, uint16_t data_length)
{
    printf("Processed data:\n");
    char buffer[MAX_DATA_ENTRY_LEN];

    for (uint16_t i = 0; i < data_length; i++)
    {
        if ((i > 0) && ((i % 8) == 0))
        {
            printf("\n");
        }

        snprintf(buffer, sizeof(buffer), "%" PRIi16 "+%" PRIi16 "i", data[i].real, data[i].imag);

        printf("%14s ", buffer);
    }

    printf("\n");
}

static void cleanup()
{
    acc_hal_integration_sensor_disable(SENSOR_ID);
    acc_hal_integration_sensor_supply_off(SENSOR_ID);

    if (sensor != NULL)
    {
        acc_sensor_destroy(sensor);
        sensor = NULL;
    }

    if (processing != NULL)
    {
        acc_processing_destroy(processing);
        processing = NULL;
    }

    if (config != NULL)
    {
        acc_config_destroy(config);
        config = NULL;
    }

    if (buffer != NULL)
    {
        acc_integration_mem_free(buffer);
        buffer = NULL;
    }

    is_initialized = false; // Reset initialization flag
}
