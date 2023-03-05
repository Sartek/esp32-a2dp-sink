#include "AudioFilter.h"

static const char* TAG = "AudioFilter";

int32_t sampledataCHUNK[SAMPLEDATALENGTH];

AudioFilter::AudioFilter(uint8_t stages, double* FIRCOEFFS, double* IIRCOEFFS) {
	stages_ = stages;
	//sampledata = new int32_t[SAMPLEDATALENGTH];
	sampledata = sampledataCHUNK;
	ACCOUTPUT_ = new double[stages_*2];		
	buffer1_ = new double[stages_*2];		
	buffer2_ = new double[stages_*2];
	for (int i = 0; i < (stages_ * 2); i++) {
		ACCINPUT_[i] = 0.0f;
		ACCOUTPUT_[i] = 0.0f;
		buffer1_[i] = 0.0f;
		buffer2_[i] = 0.0f;
	}			
	
	FIRCOEFFS_ = new double[stages_ * 3];
	for (int i = 0; i < (stages_ * 3); i++) {
		FIRCOEFFS_[i] = FIRCOEFFS[i];
	}			
	
	IIRCOEFFS_ = new double[stages_ * 2];
	for (int i = 0; i < (stages_ * 2); i++) {
		IIRCOEFFS_[i] = IIRCOEFFS[i];
	}	
}

void AudioFilter::filter(const uint8_t *data, uint32_t len, uint8_t I2SBYTEWIDTH) {
	double input_x;
	double input_y;
	int n = len/I2SBYTEWIDTH/2;
	
	int32_t* data32=(int32_t*)data;
	
	int32_t fx;
	int32_t fy;
	
	for (int i = 0; i < n; i++)
	{
		fx=*data32; //get value and assign to fx
		fy=*(data32+1);
		input_x = (double)fx; // cast to double
		input_y = (double)fy;
		for (int ii = 0; ii < stages_; ii++)
		{
			ACCINPUT_[ii] = (input_x + buffer1_[ii] * -IIRCOEFFS_[ii*2+0] + buffer2_[ii] * -IIRCOEFFS_[ii*2+1]);
			
			ACCOUTPUT_[ii] = (ACCINPUT_[ii] * FIRCOEFFS_[ii*3+0] + buffer1_[ii] * FIRCOEFFS_[ii*3+1] + buffer2_[ii] * FIRCOEFFS_[ii*3+2]);
			
			buffer2_[ii] = buffer1_[ii];
			buffer1_[ii] = ACCINPUT_[ii];
			
			input_x = ACCOUTPUT_[ii];
		}
		
		for (int ii = 0; ii < stages_; ii++)
		{
			int j = ii + stages_;
			ACCINPUT_[j] = (input_y + buffer1_[j] * -IIRCOEFFS_[ii*2+0] + buffer2_[j] * -IIRCOEFFS_[ii*2+1]);
			
			ACCOUTPUT_[j] = (ACCINPUT_[j] * FIRCOEFFS_[ii*3+0] + buffer1_[j] * FIRCOEFFS_[ii*3+1] + buffer2_[j] * FIRCOEFFS_[ii*3+2]);
			
			buffer2_[j] = buffer1_[j];
			buffer1_[j] = ACCINPUT_[j];
			
			input_y = ACCOUTPUT_[j];
		}

		fx = (int32_t)(input_x + 0.5f); // cast from double
		fy = (int32_t)(input_y + 0.5f); // cast from double
		//std::cout <<"X: " << input_x << " rounded: " << (int32_t)(input_x + 0.5f) << '\n';
		//std::cout <<"Y: " << input_y << " rounded: " << (int32_t)(input_y + 0.5f) << '\n';
		*data32 = fx; //assign updated sample value
		*(data32+1) = fy;
		data32++; //move to next sample
		data32++; //move to next sample
	}
}

void AudioFilter::monoFilter(const uint8_t *data, uint32_t len, uint8_t I2SBYTEWIDTH) {
	double input_c; //= 0;
	int n = len/I2SBYTEWIDTH;
	
	int32_t* data32=(int32_t*)data;
	
	int32_t fy;
	
	for (int i = 0; i < n; i++)
	{
		fy=*data32; //get value and assign to fy
		input_c = (double)fy; // cast to double

		for (int ii = 0; ii < stages_; ii++)
		{
			ACCINPUT_[ii] = (input_c + buffer1_[ii] * -IIRCOEFFS_[ii*2+0] + buffer2_[ii] * -IIRCOEFFS_[ii*2+1]);
			
			ACCOUTPUT_[ii] = (ACCINPUT_[ii] * FIRCOEFFS_[ii*3+0] + buffer1_[ii] * FIRCOEFFS_[ii*3+1] + buffer2_[ii] * FIRCOEFFS_[ii*3+2]);
			
			buffer2_[ii] = buffer1_[ii];
			buffer1_[ii] = ACCINPUT_[ii];
			
			input_c = ACCOUTPUT_[ii];
		}

		fy = (int32_t)(input_c + 0.5f); // cast from double
		//std::cout << input_c << " rounded: " << (int32_t)(input_c + 0.5f) << '\n';
		*data32 = fy; //assign updated sample value
		data32++; //move to next sample
	}
}

const uint8_t* AudioFilter::monoSumFilter(const uint8_t *data, uint32_t len, uint8_t I2SBYTEWIDTH) {
	double input_c; //= 0;
	int n = len/I2SBYTEWIDTH/2;

	uint16_t counter = 0;

	int32_t* sampleptr = sampledataCHUNK;
	if( I2SBYTEWIDTH == 2) {
		if (n >= SAMPLEDATALENGTH) {
			ESP_LOGE(TAG, "Filter data block out of bounds  len=%d,n=%d",len/I2SBYTEWIDTH,n);
		}
		//int32_t* sampleptr = sampledataCHUNK;
		int16_t* data16=(int16_t*)data;
		//int16_t fl;
		//int16_t fr;

		for (int i = 0; i < n; i++)
		{
			/*
			fl=*data16;
			fr=*(data16+1);
			input_c = (double)fl+fr; // cast to double
			*/
			//boost volume 32768 for 32 bit
			input_c = ((double)*(data16)+*(data16+1))* 32768;// * 0.5f;
			for (int ii = 0; ii < stages_; ii++)
			{
				ACCINPUT_[ii] = (input_c + buffer1_[ii] * -IIRCOEFFS_[ii*2+0] + buffer2_[ii] * -IIRCOEFFS_[ii*2+1]);

				ACCOUTPUT_[ii] = (ACCINPUT_[ii] * FIRCOEFFS_[ii*3+0] + buffer1_[ii] * FIRCOEFFS_[ii*3+1] + buffer2_[ii] * FIRCOEFFS_[ii*3+2]);

				buffer2_[ii] = buffer1_[ii];
				buffer1_[ii] = ACCINPUT_[ii];

				input_c = ACCOUTPUT_[ii];
			}

			//fl = fr = (int16_t)(input_c + 0.5f); // cast from double
			//*data16 = (int16_t)(input_c + 0.5f);
			//*(data16+1) = *data16;
			data16 = data16 + 2;

			//32 alternative
			/*if (counter > SAMPLEDATALENGTH) {
				ESP_LOGE(TAG, "Filter data block out of bounds");
			}*/

			sampleptr[counter] = (int32_t)(input_c + 0.5f);
			sampleptr[counter+1] = sampleptr[counter];
			counter = counter + 2;

			//*sampleptr = (int32_t)(input_c + 0.5f);
			//*(sampleptr+1) = *sampleptr;          
			//sampleptr = sampleptr + 2;	
		}
		//data = (uint8_t*)sampledataCHUNK;//not working?

	} else if (I2SBYTEWIDTH == 4) {
		int32_t* data32=(int32_t*)data;
		int32_t fy;
		for (int i = 0; i < n; i++)
		{
			fy=*data32; //get value and assign to fy
			input_c = (double)fy; // cast to double

			data32++;

			fy=*data32;
			input_c += (double)fy;
			for (int ii = 0; ii < stages_; ii++)
			{
				ACCINPUT_[ii] = (input_c + buffer1_[ii] * -IIRCOEFFS_[ii*2+0] + buffer2_[ii] * -IIRCOEFFS_[ii*2+1]);

				ACCOUTPUT_[ii] = (ACCINPUT_[ii] * FIRCOEFFS_[ii*3+0] + buffer1_[ii] * FIRCOEFFS_[ii*3+1] + buffer2_[ii] * FIRCOEFFS_[ii*3+2]);

				buffer2_[ii] = buffer1_[ii];
				buffer1_[ii] = ACCINPUT_[ii];

				input_c = ACCOUTPUT_[ii];
			}

			fy = (int32_t)(input_c + 0.5f); // cast from double
			*data32 = fy; //assign updated sample value
			*(data32-1) = fy; // update other value?
			data32++; //move to next sample
		}
	}

	return (const uint8_t*)sampleptr;
}