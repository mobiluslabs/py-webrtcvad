#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wav.h"
#include "webrtc/common_audio/vad/include/webrtc_vad.h"

#define OUT_CHANNELS 2
#define NUM_FILES 8

/*
 * Arguments: Wav file to use (1..n). Set to 99 to use all in turn
 *            Energy threshold value
 *            Frame size, 64 or 80
 *            Text to append to input file name to use for output file name
 *            Convert text to wav
 */
int main(int argc, char *argv[])
{
	int fileno = 0;
	int energy_thresh = 24;
	char *out_suffix;
	int frame_size = 80;
	int txt_to_wav = 0;
	if (argc > 1)
	{
		fileno = strtol(argv[1], NULL, 10);
		energy_thresh = strtol(argv[2], NULL, 10);
		frame_size = strtol(argv[3], NULL, 10);
		out_suffix = argv[4];
		if (argc > 5)
		{
			txt_to_wav = 1;
		}
	}
	VadInst* vad = WebRtcVad_Create();
	WebRtcVad_Init(vad);
	WebRtcVad_set_threshold(vad, energy_thresh);

	/* Set of test recording file names, without .wav extension */
	char *files[NUM_FILES] = {	"C:\\dev\\mobilus\\audio\\wav files\\ben_walking_35db",
						"C:\\dev\\mobilus\\audio\\wav files\\ben_walking_70db",
						"C:\\dev\\mobilus\\audio\\wav files\\jason_headshaking_35db",
						"C:\\dev\\mobilus\\audio\\wav files\\jason_headshaking_70db",
						"C:\\dev\\mobilus\\audio\\wav files\\jason_none_35db",
						"C:\\dev\\mobilus\\audio\\wav files\\jason_none_70db",
						"C:\\dev\\mobilus\\audio\\wav files\\jason_walking_35db",
						"C:\\dev\\mobilus\\audio\\wav files\\jason_walking_70db"};

	/*  If fileno set to 99, then iterate across all files */
	int fileno_limit = fileno==99 ? NUM_FILES : fileno+1;
	for (fileno=0; fileno<fileno_limit; fileno++)
	{
		char infile[100], outfile[100];
		strcpy(infile, files[fileno]);
		strcpy(outfile, files[fileno]);

		/*
		 * For converting text files from saleae Logic 2 to wav files
		 */
		if (txt_to_wav)
		{
			strcat(infile, ".csv");
			strcat(outfile, ".wav");
			FILE *txtfile;
			txtfile = fopen(infile, "r");
			WavFile *fp = wav_open(outfile, WAV_OPEN_WRITE);
			wav_set_format(fp, WAV_FORMAT_PCM);
			wav_set_num_channels(fp, 2);
			wav_set_sample_rate(fp, 8000);

			char line[100];
			float time, time2;
			int chan, value;
			short out_frame[2];
			fgets(line, 100, txtfile); // Get rid of header line
			while (fgets(line, 100, txtfile))
			{
				sscanf(line, "\"I2S / PCM [1]\",\"data\",%f,%f,%d,%d", &time, &time2, &chan, &value);
				out_frame[chan] = value;
				if (chan == 1)
				{
					wav_write(fp, out_frame, 1);
				}
			}
			wav_close(fp);
			strcpy(infile, files[fileno]);
			strcpy(outfile, files[fileno]);
		}

		strcat(outfile, out_suffix);
		strcat(outfile, ".wav");
		strcat(infile, ".wav");

		WavFile *wfile = wav_open(infile, WAV_OPEN_READ);

		WavU16 in_channels = wav_get_num_channels(wfile);
		short *buf = malloc(sizeof(WavU16) * in_channels * frame_size);
		short abuf[in_channels][frame_size];
		WavU32 rate = wav_get_sample_rate(wfile);

		WavFile *fp = wav_open(outfile, WAV_OPEN_WRITE);
		wav_set_format(fp, WAV_FORMAT_PCM);
		/* Make output channel count the same as input
		 * Assume third input channel is 'ground truth'
		 * and copy this to third output channel
		 */
		wav_set_num_channels(fp, OUT_CHANNELS);
		wav_set_sample_rate(fp, rate);

		int vad_result;
		short out_frame[frame_size * OUT_CHANNELS];
		int frame_no = 0;
		int16_t info[3];
		while (wav_read(wfile, buf, frame_size))
		{
			for (int j=0; j < frame_size; j++)
			{
				for (int i=0; i<in_channels; i++)
				{
					abuf[i][j] = buf[j*in_channels + i];
				}
			}

			frame_no++;
			/* Just process left channel at present */
			vad_result = WebRtcVad_Process(vad, rate, abuf[0], frame_size, info);
			for (int i = 0; i < frame_size; i++)
			{
				/*
				 * Output vad and energy flags to multichannel wav file
				 * for viewing in Audacity
				 */
				int start = i * OUT_CHANNELS;
				out_frame[start] = info[0] * 32767;  // Vad indication
				out_frame[start+1] = info[1] * 1000; // Log2(energy level). Multiplied to show up in Audacity
			}
			wav_write(fp, out_frame, frame_size);
		}

		wav_close(fp);
		wav_close(wfile);
	}

    return 0;
}
