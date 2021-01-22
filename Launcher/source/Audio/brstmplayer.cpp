#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <asndlib.h>
#include "BrstmPlayer.h"
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include <gctypes.h>
#include <malloc.h>
#include <ogc/audio.h>

VGMSTREAM *mVgmstream = NULL;
static lwp_t playthread = LWP_THREAD_NULL;
void * playThreadFunc(void*);


static const u8* sound;
static VGMSTREAM * init_vgmstream_brstm(const u8 * streamFile);
static void render_vgmstream_interleave(int16_t * buffer, int32_t sample_count, VGMSTREAM * vgmstream);
static int vgmstream_samples_to_do(int samples_this_block, int samples_per_frame, VGMSTREAM * vgmstream);
//static int32_t dsp_nibbles_to_samples(int32_t nibbles);
static void decode_ngc_dsp(VGMSTREAMCHANNEL * stream, int16_t * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
static VGMSTREAM * allocate_vgmstream(int channel_count, int looped);
static int vgmstream_do_loop(VGMSTREAM * vgmstream);


BrstmPlayer::BrstmPlayer(const u8 * s, s32 l)
{
	sound = s;
	length = l;
}

BrstmPlayer::~BrstmPlayer()
{
	free(sound);
}

bool BrstmPlayer::Init()
{
	return true;
}

static VGMSTREAM * init_vgmstream_brstm(const u8 * streamFile)
{
	VGMSTREAM * vgmstream = NULL;
	//char filename[260];

	coding_t coding_type;

	off_t head_offset;
	int codec_number;
	int channel_count;
	int loop_flag;
	/* Certain Super Paper Mario tracks have a 44.1KHz sample rate in the
	* header, but they should be played at 22.05KHz. We will make this
	* correction if we see a file with a .brstmspm extension. */
	int spm_flag = 0;
	/* Trauma Center Second Opinion has an odd, semi-corrupt header */
	int atlus_shrunken_head = 0;

	off_t start_offset;

	/* check extension, case insensitive */
	/*streamFile->get_name(streamFile, filename, sizeof(filename));
	if (strcasecmp("brstm", filename_extension(filename))) {
	if (strcasecmp("brstmspm", filename_extension(filename))) goto fail;
	else spm_flag = 1;
	}*/

	/* check header */
	if ((uint32_t)read_32bitBE(0, streamFile) != 0x5253544D) /* "RSTM" */
		goto fail;
	if ((uint32_t)read_32bitBE(4, streamFile) != 0xFEFF0100)
	{
		if ((uint32_t)read_32bitBE(4, streamFile) != 0xFEFF0001)
			goto fail;
		else
			atlus_shrunken_head = 1;
	}



	/* get head offset, check */
	head_offset = read_32bitBE(0x10, streamFile);
	if (atlus_shrunken_head)
	{
		/* the HEAD chunk is where we would expect to find the offset of that
		* chunk... */

		if ((uint32_t)head_offset != 0x48454144 || read_32bitBE(0x14, streamFile) != 8)
			goto fail;

		head_offset = -8;   /* most of the normal Nintendo RSTM offsets work
							with this assumption */
	}
	else
	{
		if ((uint32_t)read_32bitBE(head_offset, streamFile) != 0x48454144) /* "HEAD" */
			goto fail;
	}

	/* check type details */
	codec_number = read_8bit(head_offset + 0x20, streamFile);
	loop_flag = read_8bit(head_offset + 0x21, streamFile);
	channel_count = read_8bit(head_offset + 0x22, streamFile);

	switch (codec_number) {
	case 0:
		coding_type = coding_PCM8;
		break;
	case 1:
		coding_type = coding_PCM16BE;
		break;
	case 2:
		coding_type = coding_NGC_DSP;
		break;
	default:
		goto fail;
	}

	if (channel_count < 1) goto fail;

	/* build the VGMSTREAM */

	vgmstream = allocate_vgmstream(channel_count, loop_flag);
	if (!vgmstream) goto fail;


	/* fill in the vital statistics */
	vgmstream->num_samples = read_32bitBE(head_offset + 0x2c, streamFile);
	vgmstream->sample_rate = (uint16_t)read_16bitBE(head_offset + 0x24, streamFile);
	/* channels and loop flag are set by allocate_vgmstream */
	vgmstream->loop_start_sample = read_32bitBE(head_offset + 0x28, streamFile);
	vgmstream->loop_end_sample = vgmstream->num_samples;

	vgmstream->coding_type = coding_type;
	if (channel_count == 1)
		vgmstream->layout_type = layout_none;
	else
		vgmstream->layout_type = layout_interleave_shortblock;
	vgmstream->meta_type = meta_RSTM;
	if (atlus_shrunken_head)
		vgmstream->meta_type = meta_RSTM_shrunken;

	if (spm_flag&& vgmstream->sample_rate == 44100) {
		vgmstream->meta_type = meta_RSTM_SPM;
		vgmstream->sample_rate = 22050;
	}

	vgmstream->interleave_block_size = read_32bitBE(head_offset + 0x38, streamFile);
	vgmstream->interleave_smallblock_size = read_32bitBE(head_offset + 0x48, streamFile);

	if (vgmstream->coding_type == coding_NGC_DSP) {
		off_t coef_offset;
		off_t coef_offset1;
		off_t coef_offset2;
		int i, j;
		int coef_spacing = 0x38;

		if (atlus_shrunken_head)
		{
			coef_offset = 0x50;
			coef_spacing = 0x30;
		}
		else
		{
			coef_offset1 = read_32bitBE(head_offset + 0x1c, streamFile);
			coef_offset2 = read_32bitBE(head_offset + 0x10 + coef_offset1, streamFile);
			coef_offset = coef_offset2 + 0x10;
		}

		for (j = 0; j < vgmstream->channels; j++) {
			for (i = 0; i < 16; i++) {
				vgmstream->ch[j].adpcm_coef[i] = read_16bitBE(head_offset + coef_offset + j*coef_spacing + i * 2, streamFile);
			}
		}
	}

	start_offset = read_32bitBE(head_offset + 0x30, streamFile);

	/* open the file for reading by each channel */
	{
		int i;
		for (i = 0; i < channel_count; i++) {
			if (vgmstream->layout_type == layout_interleave_shortblock)
				;// vgmstream->ch[i].streamfile = streamFile->open(streamFile, filename, vgmstream->interleave_block_size);
			else
				;// vgmstream->ch[i].streamfile = streamFile->open(streamFile, filename, 0x1000);

			//if (!vgmstream->ch[i].streamfile) goto fail;

			vgmstream->ch[i].channel_start_offset =
				vgmstream->ch[i].offset =
				start_offset + i*vgmstream->interleave_block_size;
		}
	}

	return vgmstream;

	/* clean up anything we may have opened */
fail:
	//if (vgmstream) close_vgmstream(vgmstream);
	return NULL;
}

static VGMSTREAM * allocate_vgmstream(int channel_count, int looped)
{
	VGMSTREAM * vgmstream;
	VGMSTREAM * start_vgmstream;
	VGMSTREAMCHANNEL * channels;
	VGMSTREAMCHANNEL * start_channels;
	VGMSTREAMCHANNEL * loop_channels;

	if (channel_count <= 0) return NULL;

	vgmstream = (VGMSTREAM*)calloc(1, sizeof(VGMSTREAM));
	if (!vgmstream) return NULL;

	vgmstream->ch = NULL;
	vgmstream->start_ch = NULL;
	vgmstream->loop_ch = NULL;
	vgmstream->start_vgmstream = NULL;
	vgmstream->codec_data = NULL;

	start_vgmstream = (VGMSTREAM*)calloc(1, sizeof(VGMSTREAM));
	if (!start_vgmstream) {
		free(vgmstream);
		return NULL;
	}
	vgmstream->start_vgmstream = start_vgmstream;
	start_vgmstream->start_vgmstream = start_vgmstream;

	channels = (VGMSTREAMCHANNEL*)calloc(channel_count, sizeof(VGMSTREAMCHANNEL));
	if (!channels) {
		free(vgmstream);
		free(start_vgmstream);
		return NULL;
	}
	vgmstream->ch = channels;
	vgmstream->channels = channel_count;


	start_channels = (VGMSTREAMCHANNEL*)calloc(channel_count, sizeof(VGMSTREAMCHANNEL));
	if (!start_channels) {
		free(vgmstream);
		free(start_vgmstream);
		free(channels);
		return NULL;
	}
	vgmstream->start_ch = start_channels;

	if (looped) {
		loop_channels = (VGMSTREAMCHANNEL*)calloc(channel_count, sizeof(VGMSTREAMCHANNEL));
		if (!loop_channels) {
			free(vgmstream);
			free(start_vgmstream);
			free(channels);
			free(start_channels);
			return NULL;
		}
		vgmstream->loop_ch = loop_channels;
	}

	vgmstream->loop_flag = looped;

	return vgmstream;
}

static int vgmstream_do_loop(VGMSTREAM * vgmstream)
{
	/*    if (vgmstream->loop_flag) {*/
	/* is this the loop end? */
	if (vgmstream->current_sample == vgmstream->loop_end_sample) {
		/* against everything I hold sacred, preserve adpcm
		* history through loop for certain types */



		/* restore! */
		memcpy(vgmstream->ch, vgmstream->loop_ch, sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);
		vgmstream->current_sample = vgmstream->loop_sample;
		vgmstream->samples_into_block = vgmstream->loop_samples_into_block;
		vgmstream->current_block_size = vgmstream->loop_block_size;
		vgmstream->current_block_offset = vgmstream->loop_block_offset;
		vgmstream->next_block_offset = vgmstream->loop_next_block_offset;

		return 1;
	}


	/* is this the loop start? */
	if (!vgmstream->hit_loop && vgmstream->current_sample == vgmstream->loop_start_sample) {
		/* save! */
		memcpy(vgmstream->loop_ch, vgmstream->ch, sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);

		vgmstream->loop_sample = vgmstream->current_sample;
		vgmstream->loop_samples_into_block = vgmstream->samples_into_block;
		vgmstream->loop_block_size = vgmstream->current_block_size;
		vgmstream->loop_block_offset = vgmstream->current_block_offset;
		vgmstream->loop_next_block_offset = vgmstream->next_block_offset;
		vgmstream->hit_loop = 1;
	}
	/*}*/
	return 0;
}

static void render_vgmstream_interleave(int16_t * buffer, int32_t sample_count, VGMSTREAM * vgmstream)
{
	int samples_written = 0;

	int frame_size = 0;
	switch (vgmstream->coding_type) {
	case coding_NGC_DSP:
		frame_size = 8;
		break;
	case coding_PCM16BE:
		frame_size = 2;
		break;
	case coding_PCM8:
		frame_size = 1;
	}
	int samples_per_frame = (vgmstream->coding_type == coding_NGC_DSP ? 14 : 1);
	int samples_this_block;

	samples_this_block = vgmstream->interleave_block_size / frame_size * samples_per_frame;

	if (vgmstream->layout_type == layout_interleave_shortblock &&
		vgmstream->current_sample - vgmstream->samples_into_block + samples_this_block > vgmstream->num_samples) {
		//frame_size = get_vgmstream_shortframe_size(vgmstream);
		//samples_per_frame = get_vgmstream_samples_per_shortframe(vgmstream);

		samples_this_block = vgmstream->interleave_smallblock_size / frame_size * samples_per_frame;
	}

	while (samples_written < sample_count) {
		int samples_to_do;

		if (vgmstream->loop_flag && vgmstream_do_loop(vgmstream)) {
			/* we assume that the loop is not back into a short block */
			if (vgmstream->layout_type == layout_interleave_shortblock) {
				//frame_size = get_vgmstream_frame_size(vgmstream);
				//samples_per_frame = get_vgmstream_samples_per_frame(vgmstream);
				samples_this_block = vgmstream->interleave_block_size / frame_size * samples_per_frame;
			}
			continue;
		}

		samples_to_do = vgmstream_samples_to_do(samples_this_block, samples_per_frame, vgmstream);
		/*printf("vgmstream_samples_to_do(samples_this_block=%d,samples_per_frame=%d,vgmstream) returns %d\n",samples_this_block,samples_per_frame,samples_to_do);*/

		if (samples_written + samples_to_do > sample_count)
			samples_to_do = sample_count - samples_written;

		for (int chan = 0; chan < vgmstream->channels; chan++) {
			decode_ngc_dsp(&vgmstream->ch[chan], buffer + samples_written*vgmstream->channels + chan,
				vgmstream->channels, vgmstream->samples_into_block,
				samples_to_do);
		}

		samples_written += samples_to_do;
		vgmstream->current_sample += samples_to_do;
		vgmstream->samples_into_block += samples_to_do;

		if (vgmstream->samples_into_block == samples_this_block) {
			int chan;
			if (vgmstream->layout_type == layout_interleave_shortblock &&
				vgmstream->current_sample + samples_this_block > vgmstream->num_samples) {
				//frame_size = get_vgmstream_shortframe_size(vgmstream);
				//samples_per_frame = get_vgmstream_samples_per_shortframe(vgmstream);

				samples_this_block = vgmstream->interleave_smallblock_size / frame_size * samples_per_frame;
				for (chan = 0; chan < vgmstream->channels; chan++)
					vgmstream->ch[chan].offset += vgmstream->interleave_block_size*(vgmstream->channels - chan) + vgmstream->interleave_smallblock_size*chan;
			}
			else {

				for (chan = 0; chan < vgmstream->channels; chan++)
					vgmstream->ch[chan].offset += vgmstream->interleave_block_size*vgmstream->channels;
			}
			vgmstream->samples_into_block = 0;
		}

	}
}

static void decode_ngc_dsp(VGMSTREAMCHANNEL * stream, int16_t * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do)
{
	int i = first_sample;
	int32_t sample_count;

	int framesin = first_sample / 14;

	int8_t header = read_8bit(framesin * 8 + stream->offset, sound);
	int32_t scale = 1 << (header & 0xf);
	int coef_index = (header >> 4) & 0xf;
	int32_t hist1 = stream->adpcm_history1_16;
	int32_t hist2 = stream->adpcm_history2_16;
	int coef1 = stream->adpcm_coef[coef_index * 2];
	int coef2 = stream->adpcm_coef[coef_index * 2 + 1];

	first_sample = first_sample % 14;

	for (i = first_sample, sample_count = 0; i < first_sample + samples_to_do; i++, sample_count += channelspacing) {
		int sample_byte = read_8bit(framesin * 8 + stream->offset + 1 + i / 2, sound);

#ifdef DEBUG
		if (hist1 == stream->loop_history1 && hist2 == stream->loop_history2) fprintf(stderr, "yo! %#x (start %#x) %d\n", stream->offset + framesin * 8 + i / 2, stream->channel_start_offset, stream->samples_done);
		stream->samples_done++;
#endif

		outbuf[sample_count] = clamp16((
			(((i & 1 ?
			get_low_nibble_signed(sample_byte) :
			get_high_nibble_signed(sample_byte)
			) * scale) << 11) + 1024 +
			(coef1 * hist1 + coef2 * hist2)) >> 11
			);

		hist2 = hist1;
		hist1 = outbuf[sample_count];
	}

	stream->adpcm_history1_16 = hist1;
	stream->adpcm_history2_16 = hist2;
}

///*
//* The original DSP spec uses nibble counts for loop points, and some
//* variants don't have a proper sample count, so we (who are interested
//* in sample counts) need to do this conversion occasionally.
//*/
//static int32_t dsp_nibbles_to_samples(int32_t nibbles)
//{
//	int32_t whole_frames = nibbles / 16;
//	int32_t remainder = nibbles % 16;
//
//	/*
//	fprintf(stderr,"%d (%#x) nibbles => %x bytes and %d samples\n",nibbles,nibbles,whole_frames*8,remainder);
//	*/
//
//#if 0
//	if (remainder > 0 && remainder < 14)
//		return whole_frames * 14 + remainder;
//	else if (remainder >= 14)
//		fprintf(stderr, "***** last frame %d leftover nibbles makes no sense\n", remainder);
//#endif
//	if (remainder>0) return whole_frames * 14 + remainder - 2;
//	else return whole_frames * 14;
//}

static int vgmstream_samples_to_do(int samples_this_block, int samples_per_frame, VGMSTREAM * vgmstream)
{
	int samples_to_do;
	int samples_left_this_block;

	samples_left_this_block = samples_this_block - vgmstream->samples_into_block;
	samples_to_do = samples_left_this_block;

	/* fun loopy crap */
	/* Why did I think this would be any simpler? */
	if (vgmstream->loop_flag) {
		/* are we going to hit the loop end during this block? */
		if (vgmstream->current_sample + samples_left_this_block > vgmstream->loop_end_sample) {
			/* only do to just before it */
			samples_to_do = vgmstream->loop_end_sample - vgmstream->current_sample;
		}

		/* are we going to hit the loop start during this block? */
		if (!vgmstream->hit_loop && vgmstream->current_sample + samples_left_this_block > vgmstream->loop_start_sample) {
			/* only do to just before it */
			samples_to_do = vgmstream->loop_start_sample - vgmstream->current_sample;
		}

	}

	/* if it's a framed encoding don't do more than one frame */
	if (samples_per_frame > 1 && (vgmstream->samples_into_block%samples_per_frame) + samples_to_do > samples_per_frame) samples_to_do = samples_per_frame - (vgmstream->samples_into_block%samples_per_frame);

	return samples_to_do;
}

static bool killPlayer = false;
void* playThreadFunc(void*)
{
	mVgmstream = init_vgmstream_brstm(sound);
	if (!mVgmstream)
		return 0;
	//don't do this for final
	memcpy(mVgmstream->start_ch, mVgmstream->ch, sizeof(VGMSTREAMCHANNEL)*mVgmstream->channels);
	memcpy(mVgmstream->start_vgmstream, mVgmstream, sizeof(VGMSTREAM));

	int vol = 255 * (100 / 100.0);
	s32 voice;
	voice = 0;// ASND_GetFirstUnusedVoice();
	int16_t * buf = NULL;
	int BUFSIZE = 1024 * 512;
	buf = (int16_t*)malloc(BUFSIZE*sizeof(int16_t)* mVgmstream->channels);
	//int bufferIndex = 0;
	//int len = mVgmstream->num_samples;
	int ready = false;
	
	while (!killPlayer)
	{
		if (ready == false && ASND_TestPointer(voice, buf) == 0)
		{

			switch (mVgmstream->layout_type)
			{
			case layout_interleave:
			case layout_interleave_shortblock:
				render_vgmstream_interleave(buf, BUFSIZE, mVgmstream);
				break;
			case layout_none:
			default:
				;// render_vgmstream_nolayout(buf, toget, vgmstream);
				break;
			};
			ready = true;
		}

		if (ready && ASND_StatusVoice(voice) == SND_UNUSED)
		{

			ASND_SetVoice(voice, VOICE_STEREO_16BIT_BE, mVgmstream->sample_rate, 0,
				buf, sizeof(int16_t)* mVgmstream->channels * BUFSIZE, vol, vol, NULL);
			ready = false;
		}
		//fwrite(buf + j*s->channels + (only_stereo * 2), sizeof(sample), 2, outfile);

		usleep(100);
	}

	return nullptr;
}

void BrstmPlayer::Play()
{
	LWP_CreateThread(&playthread, playThreadFunc, NULL, NULL, 0, 96);
	LWP_ResumeThread(playthread);
	//playStream(this->trackLength, offset, voice, pitch, sound);
}

void BrstmPlayer::Stop()
{
	killPlayer = true;
}

void BrstmPlayer::Pause()
{

}

void BrstmPlayer::Resume()
{

}

bool BrstmPlayer::IsPlaying()
{
	return false;
}

void BrstmPlayer::SetVolume(int vol)
{
}

void BrstmPlayer::SetLoop(bool l)
{

}

