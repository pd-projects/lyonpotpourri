#include "MSPd.h"

#if __MSP__
void *player_class;
#endif

#if __PD__
static t_class *player_class;
#endif

#include "time.h"
#include "stdlib.h"

#define MAX_CHANNELS (4)
#define DEFAULT_MAX_OVERLAP (8) // number of overlapping instances allowed
#define FORWARD 1
#define BACKWARD 2
#define ACTIVE 0
#define INACTIVE 1
#define MAX_VEC 2048

#define MAXIMUM_VECTOR (8192)

#define OBJECT_NAME "player~"
#define COMPILE_DATE "7.3.06"

typedef struct
{
	float phase; // current phase in frames
	float gain; // gain for this note
	short status;// status of this event slot
		float increment;// first increment noted (only if using static increments)
			
} t_event;

typedef struct _player
{
#if __MSP__
	t_pxobject x_obj;
	t_buffer *wavebuf; // holds waveform samples
#endif
#if __PD__
	t_object x_obj;
	float x_f;
#endif 
	t_symbol *wavename; // name of waveform buffer
	float sr; // sampling rate
	short hosed; // buffers are bad
	float fadeout; // fadeout time in sample frames (if truncation)
	float sync; // input from groove sync signal
	float increment; // read increment
	short direction; // forwards or backwards
	int most_recent_event; // position in array where last note was initiated
	long b_nchans; // channels of buffer
	int overlap_max; // max number of simultaneous plays 
	t_event *events; //note attacks
	int active_events; // how many currently activated notes?
	short connections[4]; // state of signal connections
	short interpolation_tog; // select for interpolation or not
	short mute;
	short static_increment; // flag to use static increment (off by default)
							// variables only for Pd
	int vs; // signal vector size
	float *trigger_vec; // copy of input vector (Pd only)
	float *increment_vec; // copy of input vector (Pd only)
	float *b_samples; // pointer to array data
	long b_valid; // state of array
	long b_frames; // number of frames (in Pd frames are mono)
} t_player;

void player_setbuf(t_player *x, t_symbol *wavename);
void *player_new(t_symbol *msg, short argc, t_atom *argv);
t_int *player_perform_mono(t_int *w);
t_int *player_perform_mono_interpol(t_int *w);
t_int *player_perform_stereo(t_int *w);
t_int *player_perform_stereo_interpol(t_int *w);
t_int *player_perform_stereo_interpol_nocopy(t_int *w);
t_int *player_perform_hosed1(t_int *w);
t_int *player_perform_hosed2(t_int *w);
t_int *pd_player(t_int *w);

void player_dsp(t_player *x, t_signal **sp, short *count);
void player_dblclick(t_player *x);
float player_boundrand(float min, float max);
void player_assist (t_player *x, void *b, long msg, long arg, char *dst);
void player_dsp_free(t_player *x);
void player_float(t_player *x, double f);
void player_interpolation(t_player *x, t_float f);
void player_mute(t_player *x, t_floatarg f);
void player_static_increment(t_player *x, t_floatarg f);
void player_stop(t_player *x);
void player_info(t_player *x);
void player_init(t_player *x,short initialized);

#if __MSP__
void main(void)
{
	setup((t_messlist **)&player_class, (method)player_new, (method)player_dsp_free, 
		  (short)sizeof(t_player), 0, A_GIMME, 0);
	addmess((method)player_dsp,"dsp", A_CANT, 0);
	addmess((method)player_assist,"assist", A_CANT, 0);
	addmess((method)player_dblclick,"dblclick", A_CANT, 0);
	addmess((method)player_interpolation,"interpolation", A_FLOAT, 0);
	addmess((method)player_setbuf,"setbuf", A_SYM, 0);
	addmess((method)player_stop,"stop", 0);
	addmess((method)player_mute,"mute", A_FLOAT, 0);
	addmess((method)player_static_increment,"static_increment", A_FLOAT, 0);
	addfloat((method)player_float); 
	dsp_initclass();
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
	
}
#endif

#if __PD__
void player_tilde_setup(void)
{
	player_class = class_new(gensym("player~"), (t_newmethod)player_new, 
							 (t_method)player_dsp_free ,sizeof(t_player), 0, A_GIMME,0);
	CLASS_MAINSIGNALIN(player_class, t_player, x_f );
	class_addmethod(player_class, (t_method)player_dsp, gensym("dsp"), 0);
	class_addmethod(player_class, (t_method)player_mute, gensym("mute"), A_DEFFLOAT,0);
	class_addmethod(player_class, (t_method)player_setbuf, gensym("setbuf"),A_DEFSYM, 0);
	class_addmethod(player_class, (t_method)player_mute, gensym("mute"), A_FLOAT, 0);
	class_addmethod(player_class, (t_method)player_static_increment, gensym("static_increment"), A_FLOAT, 0);
	class_addmethod(player_class, (t_method)player_stop, gensym("stop"), 0);
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
	
}
#endif

void player_static_increment(t_player *x, t_floatarg f)
{
	x->static_increment = f;
}

void player_stop(t_player *x)
{
	int i;
	
	for(i = 0; i < x->overlap_max; i++){
		x->events[i].status = INACTIVE;
		x->events[i].phase = 0.0;
		x->events[i].phase = 0.0;
		x->events[i].gain = 0.0;
	}
}

#if __MSP__
void player_interpolation(t_player *x, t_float f)
{
	x->interpolation_tog = f;
	if(sys_getdspstate()){
		post("You must turn the DACs off and on before this change goes into effect");
	}	
}
#endif

void player_mute(t_player *x, t_floatarg f)
{
	x->mute = f;
}



void *player_new(t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	
	
#if __MSP__
	t_player *x = (t_player *)newobject(player_class);
	atom_arg_getsym(&x->wavename,0,argc,argv);
	atom_arg_getlong(&x->b_nchans,1,argc,argv);    
	if(x->b_nchans < 1 || x->b_nchans > 2) {
		post("player~: auto setting channels to 1");
		x->b_nchans = 1;
	}
	dsp_setup((t_pxobject *)x,2);
	x->x_obj.z_misc |= Z_NO_INPLACE; 
	for(i = 0; i < x->b_nchans; i++){
		outlet_new((t_pxobject *)x, "signal");
	}
	
#endif
	
#if __PD__
	t_player *x = (t_player *)pd_new(player_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal") );
	x->wavename = atom_getsymbolarg(0,argc,argv);
	x->b_nchans = 1;	
#endif
	if(argc < 1){
		error("%s: must specify buffer name",OBJECT_NAME);
		return 0;
	}
	x->overlap_max = atom_getfloatarg(2,argc,argv);
	if(x->overlap_max <= 0 || x->overlap_max > 128){
		x->overlap_max = DEFAULT_MAX_OVERLAP;
	}
	// post("%d overlaps for %s",x->overlap_max,x->wavename->s_name);
	x->sr = sys_getsr();
	x->vs = sys_getblksize();
	if(!x->sr)
		x->sr = 44100;
	if(!x->vs)
		x->vs = 256;  
	player_init(x,0);
	//   player_setbuf(x, x->wavename);
	return (x);
}

void player_init(t_player *x,short initialized)
{
	int i;
	
	if(!initialized){
		x->most_recent_event = 0;
		x->active_events = 0;
		x->increment = 1.0;
		x->direction = FORWARD;
		
		x->events = (t_event *) calloc(x->overlap_max, sizeof(t_event));
		x->mute = 0;
		x->interpolation_tog = 1; // interpolation by default
		x->static_increment = 0; // by default increment is adjustable through note
		for(i = 0; i < x->overlap_max; i++){
			x->events[i].status = INACTIVE;
			x->events[i].increment = 0.0;
			x->events[i].phase = 0.0;
			x->events[i].gain = 0.0;
		}
#if __PD__
		//		post("using local vectors");
		x->increment_vec = malloc(MAXIMUM_VECTOR * sizeof(float));
		x->trigger_vec = malloc(MAXIMUM_VECTOR * sizeof(float));
#endif
	} else {
#if __PD__
		for(i = 0; i < x->overlap_max; i++){
			x->events[i].status = INACTIVE;
		}
		x->increment_vec = realloc(x->increment_vec, x->vs * sizeof(float));
		x->trigger_vec = realloc(x->trigger_vec, x->vs * sizeof(float));
#endif
	}
	
}

#if __MSP__
void player_dblclick(t_player *x)
{
	t_buffer *b;
	t_symbol *wavename = x->wavename;
	
	if ((b = (t_buffer *)(wavename->s_thing)) && ob_sym(b) == gensym("buffer~"))
		mess0((t_object *)b,gensym("dblclick"));
	/*	post("bufchans %d wanted %d",b->b_nchans, x->b_nchans);
	if(b->b_nchans != x->b_nchans){
		post("hosing");
		x->hosed = 1;
	} */
}
#endif

void player_setbuf(t_player *x, t_symbol *wavename)
{
	int frames;
	long b_frames;
	
#if __MSP__
	t_buffer *b;
#endif
#if __PD__
	t_garray *a;
#endif
	
	x->hosed = 0;
	
#if __MSP__
	if ((b = (t_buffer *)(wavename->s_thing)) && ob_sym(b) == gensym("buffer~")) {
		if( b->b_nchans != x->b_nchans ){
			//      error("channel mismatch between buffer %s and player", wavename->s_name);
			post("%s wanted %d chans but got %d chans in buffer %s",OBJECT_NAME, x->b_nchans,b->b_nchans,wavename->s_name  );
			x->hosed = 1;
		} 
		else if(! b->b_valid){
			post("%s: no such buffer: %s",OBJECT_NAME, wavename->s_name);
			x->hosed = 1;
		} else if( b->b_frames <= 0){
			post("%s: empty buffer: %s",OBJECT_NAME, wavename->s_name);
			x->hosed = 1;
		}
		else {
			x->wavename = wavename;
			//      x->wavebuf = b;
			x->b_frames = b->b_frames; // trying this out
			x->b_samples = b->b_samples;//ditto
				x->b_valid = b->b_valid;
		}		
	}
	else {
		post("player~: no such buffer~ %s", wavename->s_name);
		x->hosed = 1;
	}
	
#endif
	
#if __PD__
	x->b_frames = 0;
	x->b_valid = 0;
	if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class)))
    {
		if (*wavename->s_name) pd_error(x, "player~: %s: no such array",
										wavename->s_name);
		x->b_samples = 0;
		x->hosed = 1;
    }
	else if (!garray_getfloatarray(a, &frames, &x->b_samples))
    {
		pd_error(x, "%s: bad template for player~", wavename->s_name);
		x->b_samples = 0;
		x->hosed = 1;
    }
	else  {
		x->b_frames = frames;
		x->b_valid = 1;
		garray_usedindsp(a);
	}
	if(! x->b_valid ){
		post("player~ got invalid buffer");
	}
#endif
	
}

t_int *player_perform_hosed1(t_int *w)
{
	
	//  t_player *x = (t_player *) (w[1]);
	float *outchan = (t_float *)(w[4]);
	t_int n = w[5];
	
	memset((void *)outchan,0,sizeof(float) * n);
	return(w+6);
}

t_int *player_perform_hosed2(t_int *w)
{
	
	//  t_player *x = (t_player *) (w[1]);
	float *out1 = (t_float *)(w[4]);
	float *out2 = (t_float *)(w[5]);
	t_int n = w[6];
	
	//  while(n--) *outchan++ = 0.0;
	memset((void *)out1,0,sizeof(float) * n);
	memset((void *)out2,0,sizeof(float) * n);
	return(w+7);
}


t_int *player_perform_mono(t_int *w)
{
	
	t_player *x = (t_player *) (w[1]);
	float *trigger_vec = (t_float *)(w[2]);
	float *increment_vec = (t_float *)(w[3]);
	float *outchan = (t_float *)(w[4]);
	int n = w[5];
	
	//  float *b_samples = x->wavebuf->b_samples;
	float *b_samples = x->b_samples;
	//  long b_nchans = x->b_nchans;
	long b_frames = x->b_frames; // experimental
	t_event *events = x->events;
	//  int active_events = x->active_events;
	float increment = x->increment;
	int overlap_max = x->overlap_max;
	int iphase;
	float gain;
	short insert_success;
	int new_insert = 0;
	int i,j,k;
	short *connections = x->connections;
	short bail;
	float maxphase;
	int theft_candidate;
	//case of only one note active (usual case)
	
	/*if(x->mute || x->hosed){
		while(n--) *outchan++ = 0.0;
    return(w+6);
	}*/
	
	player_setbuf(x, x->wavename);
	b_samples = x->b_samples;
	//  b_nchans = x->b_nchans;
	b_frames = x->b_frames;  
	
	if(x->mute || x->hosed || ! x->b_valid ){
		memset((void *)outchan,0,sizeof(float) * n);
		return(w+6);
	}
	
	
	
	
	
	// first of all if nothing is active, we bail
	bail = 1;	
	for(i = 0; i < overlap_max; i++){
		if(events[i].status == ACTIVE){
			bail = 0;
			break;
		}
	} 
	if(bail){
		for(i = 0; i < n; i++){
			if(trigger_vec[i]){
				bail = 0;
			}
		}
	}
	if(bail){
		for(i=0; i<n; i++){
			outchan[i] = 0.0;
		}
		return(w+6);
	}
	
	/* ok we do have business here */
	
	
	for(i=0; i<n; i++){
		outchan[i] = 0.0;
	}
	/* not interpolating? */	
	for(i = 0; i < overlap_max; i++){
		if( events[i].status == ACTIVE){
			gain = events[i].gain;
			for(j = 0; j < n; j++){
				iphase = (int)events[i].phase;
				outchan[j] += b_samples[iphase] * gain;
				if(connections[1]){
					events[i].phase += increment_vec[j];//changed to j, was a bug as i
				} else {
					events[i].phase += increment;
				}
				if(events[i].phase < 0.0 || events[i].phase >= b_frames){
					events[i].status = INACTIVE;
					break;
				}
			}
		}
	}
	
	for(i=0; i<n; i++){
		if(trigger_vec[i]){
			gain = trigger_vec[i];
			if(connections[1]){
				increment = increment_vec[i];// grab instantaneous increment
			}
			insert_success = 0;
			for(j=0; j<overlap_max; j++){
				if(events[j].status == INACTIVE){
					events[j].status = ACTIVE;
					events[j].gain = gain;
					if(increment > 0){
						events[j].phase = 0.0;
					} else {
						events[j].phase = b_frames - 1;
					}
					insert_success = 1;
					new_insert = j;
					break;
				}
			}	
			
			if(!insert_success){ // steal a note
				
				maxphase = 0;
				theft_candidate = 0;
				for(k = 0; k < overlap_max; k++){
					if(events[k].phase > maxphase){
						maxphase = events[k].phase;
						theft_candidate = k;
					}
				}
				// post("stealing note at slot %d", theft_candidate);
				new_insert = theft_candidate;
				events[new_insert].gain = gain;
				if(increment > 0){
					events[new_insert].phase = 0.0;
				} else {
					events[new_insert].phase = b_frames - 1;
				}
				insert_success = 1;
			}
			
			for(k=i; k<n; k++){
				//roll out for remaining portion of vector
				iphase = (int)events[new_insert].phase;
				outchan[k] += b_samples[iphase] * gain;
				if(connections[1]){
					events[new_insert].phase += increment_vec[k];
				} else {
					events[new_insert].phase += increment;
				}
				if( events[new_insert].phase < 0.0 || events[new_insert].phase >= b_frames){
					events[new_insert].status = INACTIVE;
					break;
				}
			}					
		}
	}
	return (w+6);
	
}



#if __MSP__
t_int *player_perform_stereo(t_int *w)
{
	t_player *x = (t_player *) (w[1]);
	float *trigger_vec = (t_float *)(w[2]);
	float *increment_vec = (t_float *)(w[3]);
	float *outchanL = (t_float *)(w[4]);
	float *outchanR = (t_float *)(w[5]);
	int n = w[6];
	
	
	float *b_samples = x->wavebuf->b_samples;
	//  long b_nchans = x->wavebuf->b_nchans;
	//  long b_frames = x->wavebuf->b_frames;
	t_event *events = x->events;
	//  int active_events = x->active_events;
	float increment = x->increment;
	int overlap_max = x->overlap_max;
	int iphase;
	float fphase;
	float gain;
	short insert_success;
	int new_insert;
	int i,j,k;
	short *connections = x->connections;
	short bail;
	float maxphase;
	int theft_candidate;
	//case of only one note active (usual case)
	
	player_setbuf(x, x->wavename);
	b_samples = x->b_samples;
	//  b_frames = x->b_frames;  
	
	if(x->mute || x->hosed || ! x->wavebuf->b_valid ){
		memset((void *)outchanL,0,sizeof(float) * n);
		memset((void *)outchanR,0,sizeof(float) * n);
		return(w+7);
	}
	
	
	
	if(! x->wavebuf->b_valid) {
		player_stop(x);
		while(n--) {
			*outchanL++ = *outchanR++ = 0.0;
		}
		return(w+7);		
	}
	
	// first of all if nothing is active, we bail
	bail = 1;	
	for(i = 0; i < overlap_max; i++){
		if(events[i].status == ACTIVE){
			bail = 0;
			break;
		}
	} 
	if(bail){
		for(i = 0; i < n; i++){
			if(trigger_vec[i]){
				bail = 0;
			}
		}
	}
	if(bail){
		for(i=0; i<n; i++){
			outchanL[i] = outchanR[i] = 0.0;
		}
		return(w+7);
	}
	
	/* ok we do have business here */
	
	for(i=0; i<n; i++){
		outchanL[i] = outchanR[i] = 0.0;
	}
	
	for(i = 0; i < overlap_max; i++){
		if( events[i].status == ACTIVE){
			gain = events[i].gain;
			for(j = 0; j < n; j++){
				iphase = (int)(events[i].phase * 2.0);
				outchanL[j] += b_samples[iphase] * gain;
				outchanR[j] += b_samples[iphase+1] * gain;
				if(connections[1]){
					events[i].phase += increment_vec[i];
				} else {
					events[i].phase += increment;
				}
				if( events[i].phase < 0.0 || events[i].phase >= x->wavebuf->b_frames){
					events[i].status = INACTIVE;
					// post("ending note");
					break;
				}
			}
		}
	}
	/* first check for initiation click. If found,
		add to list. If necessary, steal a note 
		*/
	for(i=0; i<n; i++){
		if(trigger_vec[i]){
			gain = trigger_vec[i];
			if(connections[1]){
				increment = increment_vec[i];// grab instantaneous increment
			}
			// post("attempting insert at sample %d, gain is %f",i, gain);
			insert_success = 0;
			for(j=0; j<overlap_max; j++){
				if(events[j].status == INACTIVE){
					events[j].status = ACTIVE;
					events[j].gain = gain;
					if(increment > 0){
						events[j].phase = 0.0;
					} else {
						events[j].phase = x->wavebuf->b_frames - 1;
					}
					insert_success = 1;
					new_insert = j;
					break;
				}
			}	
			
			if(!insert_success){ // steal a note
				
				maxphase = 0;
				theft_candidate = 0;
				for(k = 0; k < overlap_max; k++){
					if(events[k].phase > maxphase){
						maxphase = events[k].phase;
						theft_candidate = k;
					}
				}
				// post("stealing note at slot %d", theft_candidate);
				new_insert = theft_candidate;
				events[new_insert].gain = gain;
				if(increment > 0){
					events[new_insert].phase = 0.0;
				} else {
					events[new_insert].phase = x->wavebuf->b_frames - 1;
				}
				insert_success = 1;
			}
			//			gain = events[new_insert].gain;
			for(k=i; k<n; k++){
				//roll out for remaining portion of vector
				fphase = events[new_insert].phase * 2.0;
				iphase = (int)floorf(fphase);
				
				
				
				outchanL[k] += b_samples[iphase] * gain;
				outchanR[k] += b_samples[iphase+1] * gain;
				
				
				
				events[new_insert].phase += increment;
				/* termination conditions */
				if( events[new_insert].phase < 0.0 || events[new_insert].phase >= x->wavebuf->b_frames){
					events[new_insert].status = INACTIVE;
					// post("ending note in new run");
					break;
				}
			}					
		}
	}
	//	x->increment = increment;
	return (w+7);
}
#endif
/* New mono version for Pd */

t_int *player_perform_mono_interpol(t_int *w)
{
	t_player *x = (t_player *) (w[1]);
	float *t_vec = (t_float *)(w[2]);
	float *i_vec = (t_float *)(w[3]);
	float *outchan = (t_float *)(w[4]);
	int n = w[5];
	float *b_samples;
	long b_nchans;
	t_event *events = x->events;

	float increment = x->increment;
	int overlap_max = x->overlap_max;
	int iphase;
	float fphase;
	float gain;
	short insert_success;
	int new_insert;
	int i,j,k;
	float *trigger_vec = x->trigger_vec;
	float *increment_vec = x->increment_vec;
	short *connections = x->connections;
	short bail;
	short static_increment = x->static_increment;
	float maxphase;
	float frac;
	int theft_candidate;
	int flimit;
	float samp1, samp2;
	long b_frames;
	float vincrement;
	
	if(x->mute || x->hosed){
		memset((void *)outchan,0,sizeof(float) * n);
		return(w+6);
	}
	player_setbuf(x, x->wavename);
	b_samples = x->b_samples;
	b_nchans = x->b_nchans;
	b_frames = x->b_frames;  
    
	if(! x->b_valid) {
		player_stop(x);
		memset((void *)outchan,0,sizeof(float) * n);
		return(w+6);		
	}
	// DO THIS BETTER
#if __PD__
	for(i = 0; i < n; i++){
		trigger_vec[i] = t_vec[i];
		increment_vec[i] = i_vec[i];
	}
#endif	
#if __MSP__
	trigger_vec = t_vec;
	increment_vec = i_vec;
#endif

	/* test if we even need to do anything */
	bail = 1;	
	for(i = 0; i < overlap_max; i++){
		if(events[i].status == ACTIVE){
			bail = 0;
			break;
		}
	} 
	if(bail){
		for(i = 0; i < n; i++){
			if(trigger_vec[i]){
				bail = 0;
			}
		}
	}
	if(bail){
		memset((void *)outchan,0,sizeof(float) * n);
		return(w+6);
	}
	
	/* main sample playback code */
#if __MSP__
	if(connections[1]){// increment connected
		vincrement = increment_vec[0]; // initial increment test
	} else {
		vincrement = 1.0; // default increment
	}
#endif
#if __PD__
	vincrement = increment_vec[0];
#endif

	memset((void *)outchan,0,sizeof(float) * n);
	flimit = (b_frames - 1) * 2;
	for(i = 0; i < overlap_max; i++){
		if(events[i].status == ACTIVE){
			gain = events[i].gain;
			for(j = 0; j < n; j++){ //vector loop
				iphase = events[i].phase;
				frac = events[i].phase - iphase;
				if(static_increment){
					increment = events[i].increment;
				} else {
#if __MSP__
					increment = vincrement;
#endif
#if __PD__
					increment = increment_vec[j];
#endif

				}
			//	iphase *= 2;
				if(increment > 0){ // moving forward into sample
					if(iphase == flimit){
						outchan[j] += b_samples[iphase] * gain;
					} else {
						samp1 = b_samples[iphase];
						samp2 = b_samples[iphase + 1];
						outchan[j] += gain * (samp1 + frac * (samp2-samp1));
					}
				} 
				// moving backwards into sample				
				else {
					if(iphase == 0.0){
						outchan[j] += b_samples[iphase] * gain;
					} else {
						samp2 = b_samples[iphase];
						samp1 = b_samples[iphase - 1];
						outchan[j] += gain * (samp1 + frac * (samp2-samp1));
					}
				}
				
				if(static_increment){
					events[i].phase += events[i].increment;
				}
				else {
#if __MSP__
					if(connections[1]){
						events[i].phase += increment_vec[j]; // was BUG!!!!!! [i] was index
					} else {
						events[i].phase += 1.0; // default 1.0 increment
					}
#endif
#if __PD__
					events[i].phase += increment_vec[j];
#endif
				}
				if( events[i].phase < 0.0 || events[i].phase >= b_frames){
					events[i].status = INACTIVE;
					break;
				}
			}
		}
	}
	/* trigger responder and initial playback code */
	for(i=0; i<n; i++){
		if(trigger_vec[i]){
			gain = trigger_vec[i];
#if __MSP__
			if(connections[1]){
				increment = increment_vec[i];// grab instantaneous increment
			} else {
				increment = 1.0; // no input, we assume 1.0 increment (which may be a problem)
			}
#endif
#if __PD__
			increment = increment_vec[i];
#endif
			insert_success = 0;
			
			/* put new event into event list */
			for(j=0; j<overlap_max; j++){
				if(events[j].status == INACTIVE){
					events[j].status = ACTIVE;
					events[j].gain = gain;
					events[j].increment = increment;
					if(increment > 0){
						events[j].phase = 0.0;
					} else {
						events[j].phase = b_frames - 1;
					}
					insert_success = 1;
					new_insert = j;
					break;
				}
			}	
			
			if(!insert_success){ // steal a note if necessary
				
				maxphase = 0;
				theft_candidate = 0;
				for(k = 0; k < overlap_max; k++){
					if(events[k].phase > maxphase){
						maxphase = events[k].phase;
						theft_candidate = k;
					}
				}
				// post("stealing note at slot %d", theft_candidate);
				new_insert = theft_candidate;
				events[new_insert].gain = gain;
				events[new_insert].increment = increment;
				if(increment > 0){
					events[new_insert].phase = 0.0;
				} else {
					events[new_insert].phase = b_frames - 1;
				}
				insert_success = 1;
			}
			
			for(k=i; k<n; k++){
				
				//roll out for remaining portion of vector
				fphase = events[new_insert].phase;
				iphase = (int)floorf(fphase);
				frac = fphase - iphase;
		//		iphase *= 2; // double for stereo
				/* do interpolation */
				if(increment > 0){ // moving forward into sample
					if(iphase == flimit){
						outchan[k] += b_samples[iphase] * gain;
					} else {
						samp1 = b_samples[iphase];
						samp2 = b_samples[iphase + 1];
						outchan[k] += gain * (samp1 + frac * (samp2-samp1));
					}
				} 
				// moving backwards into sample				
				else {
					if(iphase == 0.0){
						outchan[k] += b_samples[iphase] * gain;
					} else {
						samp2 = b_samples[iphase];
						samp1 = b_samples[iphase - 1];
						outchan[k] += gain * (samp1 + frac * (samp2-samp1));
					}
				}
				/* advance phase */
				if(static_increment){
					increment = events[new_insert].increment;
				} else {
#if __MSP__
					increment = vincrement;
#endif
#if __PD__
					increment = increment_vec[k];
#endif
				}
				
				events[new_insert].phase += increment; 
				
				
				/* note termination conditions */
				if( events[new_insert].phase < 0.0 || events[new_insert].phase >= b_frames){
					events[new_insert].status = INACTIVE;
					break;
				}
			}					
		}
	}
	return (w+6);
}



t_int *pd_player(t_int *w)
{
	t_player *x = (t_player *) (w[1]);
	float *t_vec = (t_float *)(w[2]);
	float *i_vec = (t_float *)(w[3]);
	float *outchan = (t_float *)(w[4]);
	
	int n = w[5];
	float *b_samples = x->b_samples;
	//  long b_nchans = x->b_nchans;
	long b_frames = x->b_frames;
	t_event *events = x->events;
	//  int active_events = x->active_events;
	float increment = x->increment;
	int overlap_max = x->overlap_max;
	int iphase;
	float gain;
	short insert_success;
	int new_insert = 0;
	int i,j,k;
	float *trigger_vec = x->trigger_vec;
	float *increment_vec = x->increment_vec;
	//  short *connections = x->connections;
	short bail;
	float maxphase;
	float frac;
	int theft_candidate;
	int flimit;
	float samp1, samp2;
	
	
	if(x->mute || x->hosed){
		while(n--) *outchan++ = 0.0;
		return(w+6);
	}
	
	// Pd lacks Z_NO_INPLACE mechanism so we must protect input buffers
	for(i = 0; i < n; i++){
		trigger_vec[i] = t_vec[i];
		increment_vec[i] = i_vec[i];
	}
	bail = 1;	
	for(i = 0; i < overlap_max; i++){
		if(events[i].status == ACTIVE){
			bail = 0;
			break;
		}
	} 
	if(bail){
		for(i = 0; i < n; i++){
			if(trigger_vec[i]){
				bail = 0;
			}
		}
	}
	if(bail){
		for(i=0; i<n; i++){
			outchan[i] = 0.0;
		}
		return(w+6);
	}
	
	/* ok we do have business here */
	
	increment = increment_vec[0]; // startup
	
	for(i=0; i<n; i++){
		outchan[i] = 0.0;
	}
	flimit = (b_frames - 1) ;
	
	for(i = 0; i < overlap_max; i++){
		if( events[i].status == ACTIVE){
			gain = events[i].gain;
			
			for(j = 0; j < n; j++){
				
				if( events[i].phase >= b_frames){ // safety
					events[i].status = INACTIVE;
					break;
				}
				iphase = events[i].phase;
				frac = events[i].phase - iphase;
				if(increment > 0){
					if(iphase == flimit ){
						outchan[j] += b_samples[iphase] * gain;
					} else {
						samp1 = b_samples[iphase];
						samp2 = b_samples[iphase+1];
						outchan[j] += gain * (samp1 + frac * (samp2-samp1));
					}
				} else {
					if(iphase == 0.0 ){
						outchan[j] += b_samples[iphase] * gain;
					} else {
						samp2 = b_samples[iphase];
						samp1 = b_samples[iphase-1];
						outchan[j] += gain * (samp1 + frac * (samp2-samp1));
					}
				}
				
				// bug ??
				//	events[i].phase += increment_vec[i];
				events[i].phase += increment_vec[j];
				if( events[i].phase < 0.0 || events[i].phase >= b_frames){
					//	  post("ending note");
					events[i].status = INACTIVE;
					break;
				}
			}
		}
	}
	
	for(i=0; i<n; i++){
		if(trigger_vec[i]){
			//	  post("activation request at sample %d", i);
			gain = trigger_vec[i];
			increment = increment_vec[i];// grab instantaneous increment
				insert_success = 0;
				for(j=0; j<overlap_max; j++){
					if(events[j].status == INACTIVE){
						events[j].status = ACTIVE;
						events[j].gain = gain;
						if(increment > 0){
							events[j].phase = 0.0;
						} else {
							events[j].phase = b_frames - 1;
						}
						insert_success = 1;
						new_insert = j;
						break;
					}
				}	
				
				if(!insert_success){ // steal a note
					
					maxphase = 0;
					theft_candidate = 0;
					for(k = 0; k < overlap_max; k++){
						if(events[k].phase > maxphase){
							maxphase = events[k].phase;
							theft_candidate = k;
						}
					}
					// post("stealing note at slot %d", theft_candidate);
					new_insert = theft_candidate;
					events[new_insert].gain = gain;
					if(increment > 0){
						events[new_insert].phase = 0.0;
					} else {
						events[new_insert].phase = b_frames - 1;
					}
					insert_success = 1;
				}
				for(k=i; k<n; k++){
					//roll out for remaining portion of vector
					iphase = (int)events[new_insert].phase;
					outchan[k] += b_samples[iphase] * gain;
					events[new_insert].phase += increment_vec[k];
					if( events[new_insert].phase < 0.0 || events[new_insert].phase >= b_frames){
						//		  post("ending note in rollout");
						events[new_insert].status = INACTIVE;
						break;
					}
				}					
		}
	}
	
	return (w+6);
}

float player_boundrand(float min, float max)
{
	return min + (max-min) * ((float) (rand() % RAND_MAX)/ (float) RAND_MAX);
}


void player_dsp_free(t_player *x)
{
#if __MSP__
	dsp_free((t_pxobject *)x);
#endif
	free(x->events);
#if __PD__
	free(x->increment_vec);
	free(x->trigger_vec);
#endif
}

void player_dsp(t_player *x, t_signal **sp, short *count)
{
	
	player_setbuf(x, x->wavename);
	
	if(x->sr != sp[0]->s_sr){
		x->sr = sp[0]->s_sr;
		if(!x->sr){
			post("warning: zero sampling rate!");
			x->sr = 44100;
		}
	} 
	
	if(x->vs != sp[0]->s_n){
		x->vs = sp[0]->s_n;
		player_init(x,1);
	}
	
	
#if __MSP__
	if(! x->hosed) {
		x->connections[1] = count[1];
		x->b_frames = x->wavebuf->b_frames;
	}
#endif
	
	if(x->b_frames <= 0 && ! x->hosed){
		post("empty buffer, external disabled until it a sound is loaded");
		x->hosed = 1;
	}
	
	
	player_stop(x); // turn off all players to start
#if __MSP__
	if(x->b_nchans == 1) {
		if(x->hosed){
			dsp_add(player_perform_hosed1, 5, x, 
					sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
			//	  post("%s: mono hosed",OBJECT_NAME);
		}			
		else if(x->interpolation_tog){
			dsp_add(player_perform_mono_interpol, 5, x, 
					sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
			//	  post("%s: mono interpolated",OBJECT_NAME);	
		}
		else {
			dsp_add(player_perform_mono, 5, x, 
					sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
			//	  post("%s: mono non-interpolated",OBJECT_NAME);
		}
	} else if (x->b_nchans == 2){
 		if(x->hosed){
			dsp_add(player_perform_hosed2, 6, x, 
					sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
			//    post("%s: stereo hosed",OBJECT_NAME);
 		}
		else if(x->interpolation_tog){
			dsp_add(player_perform_stereo_interpol, 6, x, 
					sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
			//	  post("%s: stereo interpolated",OBJECT_NAME);
		}
		else {
			dsp_add(player_perform_stereo, 6, x, 
					sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);	
			//	  post("%s: stereo non-interpolated",OBJECT_NAME);
		}	
	} else {
		error("%s: can only play mono or stereo",OBJECT_NAME);
	}
#endif
	
#if __PD__
	if(x->hosed)
		dsp_add(player_perform_hosed1, 5, x, 
				sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
	else{
		//   was pd_player
		dsp_add(player_perform_mono_interpol, 5, x, 
				sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);	
	}
#endif
	
}

#if __MSP__
void player_float(t_player *x, double f)
{
	int inlet = ((t_pxobject*)x)->z_in;
	float fval = (float) f;
	
	// post("float value is %f",fval);
	if(inlet == 1){
		if(fval == 0.0){
			error("zero increment rejected");
			return;
		}
		x->increment = fval;
	}
}

void player_assist (t_player *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Click Trigger"); break;
			case 1: sprintf(dst,"(signal) Increment"); break;
		}
	} else if (msg==2) {
		if(x->b_nchans == 1){
			switch (arg){
				case 0: sprintf(dst,"(signal) Channel 1 Output"); break;
			}  
		} else if(x->b_nchans == 2) {
			switch (arg){
				case 0: sprintf(dst,"(signal) Channel 1 Output"); break;
				case 1: sprintf(dst,"(signal) Channel 2 Output"); break;
			} 
		}
	}
}
#endif
