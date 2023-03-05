/*
 * SPDX-FileCopyrightText: 2022 Chris Johnson <tinfc@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "BluetoothA2DPSink.h"

#include "AudioFilter.h"

//custom
const uint8_t stages = 1;

double FIRCOEFFS[3*stages] = {0x1.9178030151d56p-16,0x1.9178030151d56p-15,0x1.9178030151d56p-16};//[2.3929404965342592e-05, 4.7858809930685184e-05, 2.3929404965342592e-05]
double IIRCOEFFS[2*stages] = {-0x1.fc721cae9fa04p+0,0x1.f8f0c51d574b1p-1};//[-1.9861162115408897, 0.9862119291607511]
	
AudioFilter* bassfilter = new AudioFilter(stages, FIRCOEFFS, IIRCOEFFS);

static const char *TAG = "main";

BluetoothA2DPSink a2dp_sink;

uint8_t bps_bluetooth;

i2s_port_t i2s_port = I2S_NUM_0; 
size_t i2s_bytes_written;


void bits_per_sample_cb(uint8_t bit_depth)
{
    ESP_LOGE(TAG, "%s: bit_depth = %d", __func__, bit_depth);
    bps_bluetooth = bit_depth;
    if (bit_depth == 32) {
	  a2dp_sink.set_swap_lr_channels(true);
    } else {
	  a2dp_sink.set_swap_lr_channels(false);
    }
}

void filter_callback(const uint8_t *data, uint32_t len) {
	const uint8_t* dataptr = bassfilter->monoSumFilter(data, len, bps_bluetooth/8);
	uint32_t ptrlen;
	if (bps_bluetooth == 32) {
		ptrlen = len;
	} else if (bps_bluetooth == 16) {
		ptrlen = len * 2;
	} else if (bps_bluetooth == 8) {
		ptrlen = len * 4;
	} else if (bps_bluetooth == 24) {
		ptrlen = (len/3)*4;
	} else {
		ESP_LOGE(TAG, "bluetoothsamplerate not programmed for: %d", bps_bluetooth);
		ptrlen = len;
	}

	i2s_write(i2s_port,dataptr, ptrlen, &i2s_bytes_written, portMAX_DELAY);
}

void setup() {
	esp_log_level_set("*", ESP_LOG_VERBOSE);
	esp_log_level_set(TAG,ESP_LOG_VERBOSE);

	a2dp_sink.activate_pin_code(false);

	const i2s_config_t i2s_config = i2s_config_t {
	#ifdef CONFIG_A2DP_SINK_OUTPUT_INTERNAL_DAC
		.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
	#else
		.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
	#endif
		.sample_rate = 96000,//44100
		.bits_per_sample = (i2s_bits_per_sample_t)32,//16
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
	#ifdef CONFIG_A2DP_SINK_OUTPUT_INTERNAL_DAC
		.communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_STAND_MSB),
	#else
		.communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_STAND_I2S),
	#endif
		.intr_alloc_flags = 0, // default interrupt priority
		.dma_buf_count = 3,//8
		.dma_buf_len = 1024,//64
	#ifdef CONFIG_A2DP_SINK_OUTPUT_INTERNAL_DAC
		.use_apll = false,
		.tx_desc_auto_clear = false,
	#else
		.use_apll = true,
		.tx_desc_auto_clear = true, // avoiding noise in case of data unavailability
	#endif
		.fixed_mclk = 0,
		.mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,
		.bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT,
 	 };
  	a2dp_sink.set_i2s_config(i2s_config);
	a2dp_sink.set_bits_per_sample(32);

	#ifdef CONFIG_A2DP_SINK_OUTPUT_EXTERNAL_I2S
  		i2s_pin_config_t pin_config = i2s_pin_config_t {
			.bck_io_num = CONFIG_A2DP_SINK_I2S_BCK_PIN,
			.ws_io_num = CONFIG_A2DP_SINK_I2S_LRCK_PIN,
			.data_out_num = CONFIG_A2DP_SINK_I2S_DATA_PIN,
			.data_in_num = I2S_PIN_NO_CHANGE
  	};
  	a2dp_sink.set_pin_config(pin_config);
	#endif

  	a2dp_sink.set_bps_callback(bits_per_sample_cb);
	ESP_LOGE(TAG, "callback value, address %d, %d", (uint32_t)filter_callback,(uint32_t)&filter_callback);
	a2dp_sink.set_stream_reader(filter_callback);

  	a2dp_sink.start("InternalDAC", false);
}

extern "C" void app_main(void)
{
    setup();
}
