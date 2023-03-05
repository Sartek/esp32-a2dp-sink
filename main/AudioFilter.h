#ifndef AUDIOFILTER_H
#define AUDIOFILTER_H

//#define I2SBYTEWIDTH 4
#define SAMPLEDATALENGTH 2048

#include <CSTDINT>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
//#include <iostream>

class AudioFilter {
	public:
		AudioFilter(uint8_t stages, double* FIRCOEFFS, double* IIRCOEFFS);
		void filter(const uint8_t *data, uint32_t len, uint8_t I2SBYTEWIDTH);
		void monoFilter(const uint8_t *data, uint32_t len, uint8_t I2SBYTEWIDTH);
		const uint8_t* monoSumFilter(const uint8_t *data, uint32_t len, uint8_t I2SBYTEWIDTH);
	private:
		uint8_t stages_;
		double *ACCINPUT_;
		double *ACCOUTPUT_;
		double *buffer1_;
		double *buffer2_;
		double *FIRCOEFFS_;
		double *IIRCOEFFS_;

		int32_t *sampledata;
};

#endif