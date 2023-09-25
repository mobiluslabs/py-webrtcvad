#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wav.h"
#include "webrtc/common_audio/vad/include/webrtc_vad.h"

#define FRAME_SIZE 160

/*
 * Arguments: Wav file to use (1..n)
 *            Energy threshold value
 *            Text to append to input file name to use for output file name
 *            Convert text to wav
 */
int main(int argc, char *argv[])
{
	int fileno = 0;
	int energy_thresh = 24;
	char *out_suffix;
	int txt_to_wav = 0;
	if (argc > 1)
	{
		fileno = strtol(argv[1], NULL, 10);
		energy_thresh = strtol(argv[2], NULL, 10);
		out_suffix = argv[3];
		if (argc > 4)
		{
			txt_to_wav = 1;
		}
	}
	VadInst* vad = WebRtcVad_Create();
	WebRtcVad_Init(vad);
	WebRtcVad_set_threshold(vad, energy_thresh);

	/* Set of test recording file names, without .wav extension */
	char *files[6] = {	"C:\\dev\\mobilus\\audio\\0170__Tony__100__Kaneez__Pink_noise__TRUE__annotated",
						"C:\\dev\\mobilus\\audio\\0170__Tony__50__Kaneez__None__FALSE__annotated",
						"C:\\dev\\mobilus\\audio\\0404__MSA-HP__100__Bala__Babble__FALSE__annotated",
						"C:\\dev\\mobilus\\audio\\0404__MSA-HP__100__Bala__White_noise__FALSE__annotated",
						"C:\\dev\\mobilus\\audio\\2023-03-07 mobiWAN-V2-4-16-APX-Digital, No Noise, List 69, 8k",
						"C:\\dev\\mobilus\\audio\\i2s2_3" };
	char infile[100], outfile[100];
	strcpy(infile, files[fileno]);
	strcpy(outfile, files[fileno]);

	/*
	 * For converting text files from saleae Logic 2 to wav files
	 */
	if (txt_to_wav)
	{
		strcat(infile, ".txt");
		strcat(outfile, ".wav");
		FILE *txtfile;
		txtfile = fopen(infile, "r");
	    WavFile *fp = wav_open(outfile, WAV_OPEN_WRITE);
	    wav_set_format(fp, WAV_FORMAT_PCM);
	    wav_set_num_channels(fp, 2);
	    wav_set_sample_rate(fp, 8000);

	    char line[100];
	    float time;
		int chan, value;
		short out_frame[2];
	    fgets(line, 100, txtfile); // Get rid of header line
	    while (fgets(line, 100, txtfile))
	    {
	    	sscanf(line, "%f,%d,%d", &time, &chan, &value);
	    	out_frame[chan-1] = value;
	    	if (chan == 2)
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
    short *buf = malloc(sizeof(WavU16) * in_channels * FRAME_SIZE);
    short abuf[in_channels][FRAME_SIZE];
    WavU32 rate = wav_get_sample_rate(wfile);

    WavFile *fp = wav_open(outfile, WAV_OPEN_WRITE);
    wav_set_format(fp, WAV_FORMAT_PCM);
    /* Make output channel count the same as input
     * Assume third input channel is 'ground truth'
     * and copy this to third output channel
     */
    wav_set_num_channels(fp, in_channels);
    wav_set_sample_rate(fp, rate);

    int vad_result;
    short out_frame[FRAME_SIZE * in_channels];
    int frame_no = 0;
    int16_t info[3];
    while (wav_read(wfile, buf, FRAME_SIZE))
    {
        for (int j=0; j < FRAME_SIZE; j++)
        {
        	for (int i=0; i<in_channels; i++)
        	{
        		abuf[i][j] = buf[j*in_channels + i];
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
    		int start = i * in_channels;
    		out_frame[start] = info[0] * 32767;  // Vad indication
    		out_frame[start+1] = info[1] * 1000; // Log2(energy level). Multiplied to show up in Audacity
    		if (in_channels == 3) out_frame[start+3] = abuf[2][i];     // Ground truth from input file
    	}
		wav_write(fp, out_frame, FRAME_SIZE);
    }

    wav_close(fp);
    wav_close(wfile);

    return 0;
}
