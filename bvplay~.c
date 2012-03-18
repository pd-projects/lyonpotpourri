#include "MSPd.h"

#if __MSP__
void *bvplay_class;
#endif
#if __PD__
static t_class *bvplay_class;
#endif

#define OBJECT_NAME "bvplay~"
typedef struct {
	float *b_samples;
	long b_valid;
	long b_nchans;
	long b_frames;
} t_guffer; // stuff we care about from garrays and buffers


typedef struct _bvplay
{
#if __MSP__
    t_pxobject x_obj;
#endif
#if __PD__
    t_object x_obj;
	float x_f;
#endif
    t_symbol *sfname; // name of soundfile
	t_guffer *wavebuf; // store needed buffer or garray data

	long object_chans; // number of channels for a given instantiation
    float taper_dur;
    int R;
    int framesize;
    float *notedata;
    int active;
    float buffer_duration;
    int taper_frames;
    float amp;
    int start_frame;
    int note_frames;
    int end_frame;
    float increment;
    float findex;
    int index ;
	short verbose;
	short mute;
} t_bvplay;

t_int *bvplay_perform_mono(t_int *w);
t_int *bvplay_perform_stereo(t_int *w);
void bvplay_dsp(t_bvplay *x, t_signal **sp);
void bvplay_set(t_bvplay *x, t_symbol *s);
void *bvplay_new(t_symbol *s, t_floatarg chan, t_floatarg taperdur);
void bvplay_notelist(t_bvplay *x, t_symbol *msg, short argc, t_atom *argv );
void bvplay_verbose(t_bvplay *x, t_floatarg t);
void bvplay_mute(t_bvplay *x, t_floatarg t);
void bvplay_assist(t_bvplay *x, void *b, long m, long a, char *s);
void bvplay_dblclick(t_bvplay *x);
void bvplay_taper(t_bvplay *x, t_floatarg t);
void bvplay_dsp_free(t_bvplay *x);

// t_symbol *ps_buffer;

#if __MSP__
void main(void)
{
	setup((t_messlist **)&bvplay_class, (method)bvplay_new, (method)dsp_free, 
	(short)sizeof(t_bvplay), 0, A_SYM, A_FLOAT, A_FLOAT, 0);
	addmess((method)bvplay_dsp, "dsp", A_CANT, 0);
	addmess((method)bvplay_notelist, "list", A_GIMME, 0);
	addmess((method)bvplay_assist, "assist", A_CANT, 0);
	addmess((method)bvplay_dblclick, "dblclick", A_CANT, 0);
	addmess((method)bvplay_verbose, "verbose", A_FLOAT, 0);
	addmess((method)bvplay_mute, "mute", A_FLOAT, 0);
	addmess((method)bvplay_taper, "taper", A_FLOAT, 0);
	dsp_initclass();
//	ps_buffer = gensym("buffer~");
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif
#if __PD__
void bvplay_tilde_setup(void)
{
	bvplay_class = class_new(gensym("bvplay~"),(t_newmethod)bvplay_new,
		(t_method)bvplay_dsp_free, sizeof(t_bvplay), 0, A_SYMBOL, A_FLOAT, A_FLOAT,0);
	CLASS_MAINSIGNALIN(bvplay_class,t_bvplay, x_f);
	class_addmethod(bvplay_class,(t_method)bvplay_dsp,gensym("dsp"),A_CANT,0);
	class_addmethod(bvplay_class,(t_method)bvplay_notelist,gensym("list"),A_GIMME,0);
	class_addmethod(bvplay_class,(t_method)bvplay_assist,gensym("assist"),A_CANT,0);
	class_addmethod(bvplay_class,(t_method)bvplay_dblclick,gensym("dblclick"),A_CANT,0);
	class_addmethod(bvplay_class,(t_method)bvplay_verbose,gensym("verbose"),A_FLOAT,0);
	class_addmethod(bvplay_class,(t_method)bvplay_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(bvplay_class,(t_method)bvplay_taper,gensym("taper"),A_FLOAT,0);

	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif
void bvplay_taper(t_bvplay *x, t_floatarg t)
{
	if(t>0){
		x->taper_dur = (float)t/1000.0;
		x->taper_frames = x->R * x->taper_dur;
	}
}


void bvplay_mute(t_bvplay *x, t_floatarg f)
{
    x->mute = (short)f;
}

void bvplay_verbose(t_bvplay *x, t_floatarg f)
{
    x->verbose = (short)f;
}


void bvplay_notelist(t_bvplay *x, t_symbol *msg, short argc, t_atom *argv) 
{

//	t_buffer *b;
/*
NB - THIS CRASHES IF THE DACS ARE NOT TURNED ON !!!
*/
#if __MSP__
	if( ! sys_getdspstate() ){
	    if( x->verbose )
			error("cannot receive notes when the DSP is off!");
		return;
	}
#endif
	if( x->active ){
		if( x->verbose )
			error("object still playing - cannot add note!");
		return;
	}
	bvplay_set(x, x->sfname);
	if(! x->wavebuf->b_valid){
		post("%s: no valid buffer yet",OBJECT_NAME);
		return;
	}
/*
	b = x->l_buf;
	if( x->framesize != b->b_frames ) {
	    if( x->verbose ) {
			post("framesize reset");
		}
		x->framesize = b->b_frames ;
		x->buffer_duration = (float)  b->b_frames / (float) x->R ;
		if( x->verbose ) {
			post("there are %d frames in this buffer. Duration = %.2f, Channels: %d ",
				b->b_frames, x->buffer_duration, b->b_nchans);
		}
	}	
	else if( x->buffer_duration <= 0.0 ) {
		x->framesize = b->b_frames ;
		x->buffer_duration = (float)  b->b_frames / (float) x->R ;
		if( x->verbose ){
			post("there are %d frames in this buffer. Duration = %.2f ",
			b->b_frames, x->buffer_duration);
		}
	}
	if( x->buffer_duration <= 0.0 ) {
		post("buffer contains no data. Please fill it and try again");
		return;
	}
*/
	// read note data
	if( argc != 4 ){
		if( x->verbose ){
			post("improper note data");
			post("notelist parameters: skiptime, duration, increment, amplitude");
		}
	}

	x->notedata[0] = atom_getfloatarg(0,argc,argv) / 1000.0;
	x->notedata[1] = atom_getfloatarg(1,argc,argv) / 1000.0;
	x->notedata[2] = atom_getfloatarg(2,argc,argv);
	x->notedata[3] = atom_getfloatarg(3,argc,argv);

	x->start_frame = x->notedata[0] * x->R;
	x->increment = x->notedata[2];
	x->index = x->findex = x->start_frame;

	if( x->increment == 0.0 ){
		if( x->verbose )
			post("zero increment!");
		return;
	}
	x->note_frames =  x->notedata[1] * x->increment  * x->R;
	x->end_frame = x->start_frame + x->note_frames;

    x->amp = x->notedata[3];
	if( x->start_frame < 0 || x->start_frame >= x->wavebuf->b_frames){
		if( x->verbose )
			post("%s: bad start time",OBJECT_NAME);
		return;
	}
	if( x->end_frame < 0 || x->end_frame >= x->wavebuf->b_frames){
		if( x->verbose )
			post("%s: bad end time",OBJECT_NAME);
		return;
	}

	x->active = 1;	
}

t_int *bvplay_perform_mono(t_int *w)
{
    t_bvplay *x = (t_bvplay *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    t_int n = w[3];
//	t_buffer *b = x->l_buf;
	float *tab;
//	long chan = x->l_chan;
//	long frames = b->b_frames;
//	long nc = b->b_nchans;
	long iindex = x->index;
	float findex = x->findex;
	int end_frame = x->end_frame;
	float increment = x->increment;
	int start_frame = x->start_frame;
	int taper_frames = x->taper_frames;
	float noteamp = x->amp;
	float frac, amp;
/**********************/
	if(!x->wavebuf->b_valid) {
		post("invalid buffer");
		memset(out, 0, sizeof(float) * n);
		return (w+4);
	}
	tab = x->wavebuf->b_samples;
	
	if(x->active){
		while(n--){			
			if((increment > 0 && iindex < end_frame) || (increment < 0 && iindex > end_frame)) {
				// envelope
				if( increment > 0 ){
					if( findex < start_frame + taper_frames ){
						amp = noteamp * ((findex - (float) start_frame) / (float) taper_frames );
					} else if ( findex > end_frame - taper_frames) {
						amp = noteamp * (((float)end_frame - findex) / (float) taper_frames);
					} else {
						amp = noteamp;
					}
				} else { // negative increment case
					if( findex > start_frame - taper_frames ){
						amp =  noteamp * ( (start_frame - findex) / taper_frames );
					} else if ( findex < end_frame + taper_frames) {
						amp = noteamp * (( findex - end_frame ) / taper_frames) ;
					} else {
						amp = noteamp;
					}

				}
				frac = findex - iindex ;
				*out++ = amp * (tab[iindex] + frac * (tab[iindex + 1] - tab[iindex]));
				findex += increment;
				iindex = findex ;
			} else {
				*out++ = 0;
				x->active = 0;
			}
		}

	}	
	else{
		while(n--){
			*out++ = 0;
		}
	}

	x->index = iindex;
	x->findex = findex;

	return (w+4);
}

t_int *bvplay_perform_stereo(t_int *w)
{
    t_bvplay *x = (t_bvplay *)(w[1]);
    t_float *out1 = (t_float *)(w[2]);
    t_float *out2 = (t_float *)(w[3]);
    t_int n = w[4];
//	t_buffer *b = x->l_buf;
	float *tab;
	long iindex = x->index;
	float findex = x->findex;
	int end_frame = x->end_frame;
	float increment = x->increment;
	int start_frame = x->start_frame;
	int taper_frames = x->taper_frames;
	float noteamp = x->amp;
	float frac, amp;
	long index2,index2a;
/**********************/

	if(!x->wavebuf->b_valid) {
		post("invalid buffer");
		memset(out1, 0, sizeof(float) * n);
		memset(out2, 0, sizeof(float) * n);
		return (w+5);
	}
	tab = x->wavebuf->b_samples;	
	if( x->active ){
		while(n--){			
			if(  (increment > 0 && iindex < end_frame) || (increment < 0 && iindex > end_frame) ) {
				// envelope
				if( increment > 0 ){
					if( findex < start_frame + taper_frames ){
						amp = noteamp * ((findex - (float) start_frame) / (float) taper_frames);
					} else if ( findex > end_frame - taper_frames) {
						amp = noteamp * (((float)end_frame - findex) / (float) taper_frames);
					} else {
						amp = noteamp;
					}
				} else { // negative increment case
					if( findex > start_frame - taper_frames ){
						amp =  noteamp * ( (start_frame - findex) / taper_frames );
					} else if ( findex < end_frame + taper_frames) {
						amp = noteamp * (( findex - end_frame ) / taper_frames) ;
					} else {
						amp = noteamp;
					}
				}
				frac = findex - iindex ;
				index2 = iindex * 2 ;
				index2a = index2 + 1;
				*out1++ = amp * (tab[index2] + frac * (tab[index2 + 2] - tab[index2]));
				*out2++ = amp * (tab[index2a] + frac * (tab[index2a + 2] - tab[index2a]));

				findex += increment;
				iindex = findex ;
			} else {
				*out1++ = *out2++ = 0;
				x->active = 0;
			}
		}

	}	
	else{
		while(n--){
			*out1++ = *out2++ = 0;
		}
	}

	x->index = iindex;
	x->findex = findex;

	return (w+5);
}

void bvplay_set(t_bvplay *x, t_symbol *wavename)
{
#if __MSP__
	t_buffer *b;		
	if ((b = (t_buffer *)(wavename->s_thing)) && ob_sym(b) == gensym("buffer~")) {
    	if( b->b_nchans > 2 ){
      		error("%s: wavetable must be a mono or stereo buffer", OBJECT_NAME);
      		x->wavebuf->b_valid = 0;
    	} else {
    	    x->wavebuf->b_samples = b->b_samples;
			x->wavebuf->b_valid = b->b_valid;
			x->wavebuf->b_nchans = b->b_nchans;
			x->wavebuf->b_frames = b->b_frames;
    	}		
	} else {
    	error("%s: no such buffer~ %s", OBJECT_NAME, wavename->s_name);
		x->wavebuf->b_valid = 0;
	}	
	x->sfname = wavename;
#endif
	
#if __PD__
	t_garray *a;
	int b_frames;
	float *b_samples;
	if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
		if (*wavename->s_name) pd_error(x, "%s: %s: no such array",OBJECT_NAME,wavename->s_name);
		x->wavebuf->b_valid = 0;
    }
	else if (!garray_getfloatarray(a, &b_frames, &b_samples)) {
		pd_error(x, "%s: bad array for %s", wavename->s_name,OBJECT_NAME);
		x->wavebuf->b_valid = 0; 
    }
	else  {
		x->wavebuf->b_valid = 1;
		x->wavebuf->b_frames = (long)b_frames;
		x->wavebuf->b_nchans = 1;
		x->wavebuf->b_samples = b_samples;		
		garray_usedindsp(a);
	}
	#endif
}



void bvplay_dblclick(t_bvplay *x)
{
#if __MSP__
	t_buffer *b;
  t_symbol *wavename = x->sfname;
	
  if ((b = (t_buffer *)(wavename->s_thing)) && ob_sym(b) == gensym("buffer~"))
    mess0((t_object *)b,gensym("dblclick"));
#endif
}

void bvplay_assist(t_bvplay *x, void *b, long msg, long arg, char *dst)
{
#if __MSP__
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(list) Note Data [st,dur,incr,amp]"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
#endif
}

void *bvplay_new(t_symbol *s, t_floatarg chan, t_floatarg taperdur)
{
int ichan = (int)chan;
//	float erand();
#if __MSP__
	t_bvplay *x = (t_bvplay *)newobject(bvplay_class);
	dsp_setup((t_pxobject *)x,0);
	if( ichan == 1 ) {
		outlet_new((t_object *)x, "signal");
	} else if( ichan == 2 ){
		outlet_new((t_object *)x, "signal");
		outlet_new((t_object *)x, "signal");
	} else {
		post("%s: illegal channels %d", OBJECT_NAME, ichan);
		return 0;
	}	
#endif
#if __PD__
	t_bvplay *x = (t_bvplay *)pd_new(bvplay_class);
	if( ichan == 1 ) {
		outlet_new(&x->x_obj, gensym("signal"));
	} else if( ichan == 2 ){
		outlet_new(&x->x_obj, gensym("signal"));
		outlet_new(&x->x_obj, gensym("signal"));
	} else {
		post("%s: illegal channels %d", OBJECT_NAME, ichan);
		return 0;
	}	
#endif
	x->object_chans = (long)ichan;
    taperdur /= 1000.0; // convert to seconds	
	if(taperdur <= 0)
		taperdur = .005;
	x->sfname = s;
	x->R = sys_getsr();
	if(! x->R){
		error("zero sampling rate - set to 44100");
		x->R = 44100;
	}
	x->notedata = (float *) calloc(4, sizeof(float));
	x->wavebuf = (t_guffer *) calloc(1, sizeof(t_guffer));
	x->taper_dur = taperdur;
	x->taper_frames = x->R * x->taper_dur;
	x->buffer_duration = 0.0 ;
	x->framesize = 0;
	x->active = 0;
	x->verbose = 0;
	x->mute = 0;
	// post("channels %d, taper duration %.4f, taperframes %d", chan, taperdur, x->taper_frames );

	// post("arguments: channels, taper_duration(secs.)");
	return (x);
}

void bvplay_dsp_free(t_bvplay *x)
{
#if __MSP__
	dsp_free((t_pxobject *)x);
#endif
	free(x->notedata);
	free(x->wavebuf);
}

void bvplay_dsp(t_bvplay *x, t_signal **sp)
{
    bvplay_set(x,x->sfname);
    
    if(x->R != sp[0]->s_sr){
    	x->R = sp[0]->s_sr;
    	x->taper_frames = x->R * x->taper_dur;
    }
    if( x->wavebuf->b_nchans == 1 ) {
    	// post("initiating mono processor");
   	 	dsp_add(bvplay_perform_mono, 3, x, sp[0]->s_vec, sp[0]->s_n);
    } else if( x->wavebuf->b_nchans == 2) {
    	// post("initiating stereo processor");
   	 	dsp_add(bvplay_perform_stereo,4,x,sp[0]->s_vec,sp[1]->s_vec,sp[0]->s_n);
    } else {
     	post("%s: bad channel spec: %d, cannot initiate dsp code",OBJECT_NAME,  x->wavebuf->b_nchans);
    }
}
