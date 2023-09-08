#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wav.h"
#include "webrtc/common_audio/vad/include/webrtc_vad.h"

#define FRAME_SIZE 160
#define OUT_CHANNELS 3

/*
 * Arguments: Wav file to use (1..n)
 *            Energy threshold value
 *            Text to append to input file name to use for output file name
 */
int main(int argc, char *argv[])
{
	int fileno = 0;
	int energy_thresh = 24;
	char *out_suffix;
	if (argc > 1)
	{
		fileno = strtol(argv[1], NULL, 10);
		energy_thresh = strtol(argv[2], NULL, 10);
		out_suffix = argv[3];
	}
	VadInst* vad = WebRtcVad_Create();
	WebRtcVad_Init(vad);
	WebRtcVad_set_threshold(vad, energy_thresh);

	/* Set of test recording file names, without .wav extension */
	char *files[4] = {	"C:\\dev\\mobilus\\audio\\0170__Tony__100__Kaneez__Pink_noise__TRUE__annotated",
						"C:\\dev\\mobilus\\audio\\0170__Tony__50__Kaneez__None__FALSE__annotated",
						"C:\\dev\\mobilus\\audio\\0404__MSA-HP__100__Bala__Babble__FALSE__annotated",
						"C:\\dev\\mobilus\\audio\\0404__MSA-HP__100__Bala__White_noise__FALSE__annotated" };

	char infile[100], outfile[100];
	strcpy(infile, files[fileno]);
	strcat(infile, ".wav");
	strcpy(outfile, files[fileno]);
	strcat(outfile, out_suffix);
	strcat(outfile, ".wav");

    WavFile *wfile = wav_open(infile, WAV_OPEN_READ);

    WavU16 n_channels = wav_get_num_channels(wfile);
    short *buf = malloc(sizeof(WavU16) * n_channels * FRAME_SIZE);
    short abuf[n_channels][FRAME_SIZE];
    WavU32 rate = wav_get_sample_rate(wfile);

    WavFile *fp = wav_open(outfile, WAV_OPEN_WRITE);
    wav_set_format(fp, WAV_FORMAT_PCM);
    wav_set_num_channels(fp, OUT_CHANNELS);
    wav_set_sample_rate(fp, rate);

    int vad_result;
    short out_frame[FRAME_SIZE * OUT_CHANNELS];
    int frame_no = 0;
    int16_t info[3];
    while (wav_read(wfile, buf, FRAME_SIZE))
    {
        for (int j=0; j < FRAME_SIZE; j++)
        {
        	for (int i=0; i<n_channels; i++)
        	{
        		abuf[i][j] = buf[j*n_channels + i];
        	}
        }

    	frame_no++;
    	/* Just process left channel at present */
    	vad_result = WebRtcVad_Process(vad, rate, abuf[0], FRAME_SIZE, info);
    	for (int i = 0; i < FRAME_SIZE; i++)
    	{
    		/*
    		 * Output vad and energy flags to multichannel wav file
    		 * for viewing in Audacity
    		 */
    		int start = i * OUT_CHANNELS;
    		out_frame[start] = info[0] * 32767;  // Vad indication
    		out_frame[start+1] = info[1] * 1000; // Log2(energy level). Multiplied to show up in Audacity
    		out_frame[start+3] = abuf[2][i];     // Ground truth from input file
    	}
		wav_write(fp, out_frame, FRAME_SIZE);
    }

    wav_close(fp);
    wav_close(wfile);

    return 0;
}
