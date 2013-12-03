#include "m_pd.h"
//#include "lpp.h"
#include "MSPd.h"
static t_class *sarec_class;

// #define MAXBEATS (256)
#define OBJECT_NAME "sarec~"
#define SAREC_RECORD 1
#define SAREC_OVERDUB 2
#define SAREC_PUNCH 3
#define SAREC_LOCK 4
// update time in samples
#define WAVEFORM_UPDATE (2205)

typedef struct _sarec
{

	t_object x_obj;
	float x_f;
	long display_counter;
	int valid; // status of recording buffer (not yet implemented)
	int status; // idle? 0, recording? 1
	int mode; // record, overdub or punch
	int overdub; // 0 ? write over track, 1: overdub into track
	int *armed_chans; // 1, armed, 0, protected
	long counter; // sample counter
	float sync; // position in recording
	long start_frame; // start time in samples
	long end_frame; // end time in samples
	long fadesamps; // number of samples for fades on PUNCH mode
	long regionsamps; // use for fade
	int channel_count; // number of channels (hopefully!) in buffer
	float sr;
	t_symbol *bufname; // name of recording buffer
	t_garray *recbuf; // name of recording buffer
	// local copy
	float *b_samples; // pointer to array data
	long b_valid; // state of array
	long b_frames; // number of frames (in Pd frames are mono)
	long b_nchans;
	float syncphase; // maintain phase for outlet
} t_sarec;

void *sarec_new(t_symbol *msg, short argc, t_atom *argv);
void sarec_region(t_sarec *x, t_symbol *msg, short argc, t_atom *argv);
void sarec_regionsamps(t_sarec *x, t_symbol *msg, short argc, t_atom *argv);
t_int *sarec_perform(t_int *w);
void sarec_dsp(t_sarec *x, t_signal **sp, short *count);
void sarec_assist(t_sarec *x, void *b, long m, long a, char *s);
void sarec_arm(t_sarec *x, t_floatarg chan);
void sarec_disarm(t_sarec *x, t_floatarg chan);
void sarec_overdub(t_sarec *x);
void sarec_attach_buffer(t_sarec *x);

void version(void);


void sarec_overdub(t_sarec *x);
void sarec_record(t_sarec *x);
void sarec_punch(t_sarec *x);
void sarec_lock(t_sarec *x);
void sarec_punchfade(t_sarec *x, t_floatarg fadetime);

void sarec_tilde_setup(void){
	sarec_class = class_new(gensym("sarec~"), (t_newmethod)sarec_new, 
						   NO_FREE_FUNCTION,sizeof(t_sarec), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(sarec_class, t_sarec, x_f);
	class_addmethod(sarec_class,(t_method)sarec_dsp,gensym("dsp"),0);
//	class_addmethod(sarec_class,(t_method)sarec_arm,gensym("arm"),A_FLOAT,0);
//	class_addmethod(sarec_class,(t_method)sarec_disarm,gensym("disarm"),A_FLOAT,0);
	class_addmethod(sarec_class,(t_method)sarec_overdub,gensym("overdub"),0);
	class_addmethod(sarec_class,(t_method)sarec_lock,gensym("lock"),0);
	class_addmethod(sarec_class,(t_method)sarec_punch,gensym("punch"),0);
	class_addmethod(sarec_class,(t_method)sarec_punchfade,gensym("punchfade"),A_FLOAT,0);
	class_addmethod(sarec_class,(t_method)sarec_record,gensym("record"),0);
	class_addmethod(sarec_class,(t_method)sarec_region,gensym("region"),A_GIMME,0);
	class_addmethod(sarec_class,(t_method)sarec_regionsamps,gensym("regionsamps"),A_GIMME,0);
//	class_addmethod(sarec_class,(t_method)version,gensym("version"),0);
	potpourri_announce(OBJECT_NAME);
}

/*
void version(void)
{
	lpp_version(OBJECT_NAME);
}
*/


void *sarec_new(t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	int chans;
	
	if(argc < 2){
		error("%s: must specify buffer and channel count",OBJECT_NAME);
		return (void *)NULL;
	}

    
	chans = 1;

    t_sarec *x = (t_sarec *)pd_new(sarec_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
	x->syncphase = 0.0;
    x->status = 0;
	x->counter = 0;	
	atom_arg_getsym(&x->bufname,0,argc,argv);
	x->channel_count = chans;
	x->armed_chans = (int *) malloc(x->channel_count * sizeof(int));
	x->display_counter = 0;
	x->start_frame = 0;
	x->end_frame = 0;
	x->overdub = 0;// soon will kill
	x->mode = SAREC_RECORD;
	x->sr = sys_getsr();
	x->fadesamps = x->sr * 0.05; // 50 ms fade time 
	// post("fadesamps are %d", x->fadesamps);
	x->start_frame = -1; // initialize to a bad value
	x->end_frame = -1;
	x->regionsamps = 0; 
	for(i = 0; i < x->channel_count; i++){
		x->armed_chans[i] = 1; // all channels armed by default
	}	
	
	
	return (x);
}

/* note - this was built for MaxMSP multi-channel processing
so it is not optimal for Pd mono-only buffers */

t_int *sarec_perform(t_int *w)
{
	int i, j;
	t_sarec *x = (t_sarec *) (w[1]);
	t_float *click_inlet = (t_float *) (w[2]);
	t_float *record_inlet;
	t_float *sync;
	t_int channel_count = x->channel_count;
	t_int n;
	int next_pointer = channel_count + 5;
	int status = x->status;
	int counter = x->counter;
	int *armed_chans = x->armed_chans;
	t_float *samples = x->b_samples;
	long b_frames = x->b_frames;
	long start_frame = x->start_frame;
	long end_frame = x->end_frame;
	//int overdub = x->overdub;
	int mode = x->mode;
	long fadesamps = x->fadesamps;
	long regionsamps = x->regionsamps;
	int clickval;
	float frak;
	float goin_up, goin_down;
	long counter_msf;
	float syncphase = x->syncphase;
	sync = (t_float *) (w[3 + channel_count]);
	n = w[4 + channel_count];
	
	if(mode == SAREC_LOCK){
		while(n--){
			*sync++ = 0;
		}
		return w + next_pointer;
	}
	if(! regionsamps ){
		x->regionsamps = regionsamps = end_frame - start_frame;
	}
	for(i = 0; i < n; i++){
		// could be record (1) or overdub (2)
		if( click_inlet[i] ){
			clickval = (int) click_inlet[i];
			// post("click value is %d", clickval);
			// signal to stop recording
			if(clickval == -2) {
				status = 0;
				counter = 0;
			} 
			else {
				// arm all channels
				if(clickval == -1) {
					// just use all armed channels
				}
				// arm only one channel - protect against fatal bad index
				else if(clickval <= channel_count && clickval > 0) {
					for(j=0; j < channel_count; j++){
						armed_chans[j] = 0;
					};
					armed_chans[clickval - 1] = 1;
					// post("arming channel %d", clickval);
				}
				
				counter = start_frame;
				if(!end_frame){
					end_frame = b_frames;
				}
				status = 1;
			}
			
		} 
		// *click_inlet++;
		


		
		if(counter >= end_frame) {
			counter = 0;
			status = 0;
		}
		// write over track
		if(status && (mode == SAREC_RECORD) ){
			for(j=0; j < channel_count; j++){
				if( armed_chans[j] ){
					record_inlet = (t_float *) (w[3 + j]);
					samples[ (counter * channel_count) + j] = record_inlet[i];
				}
			}
			counter++;
		}
		// overdub
		else if(status && (mode == SAREC_OVERDUB) ){
			for(j=0; j < channel_count; j++){
				if( armed_chans[j] ){
					record_inlet = (t_float *) (w[3 + j]);
					samples[ (counter * channel_count) + j] += record_inlet[i];
				}
			}
			counter++;
		}
		// punch
		
		else if(status && (mode == SAREC_PUNCH)) {
			counter_msf = counter - start_frame;
			for(j=0; j < channel_count; j++){
				if( armed_chans[j] ){
					record_inlet = (t_float *) (w[3 + j]);
					// frak is multiplier for NEW STUFF, (1-frak) is MULTIPLIER for stuff to fade out
					// do power fade though
					
					if( counter_msf < fadesamps ){
						
						// fade in
						frak = (float)counter_msf / (float)fadesamps;
						goin_up = sin(PIOVERTWO * frak);
						goin_down = cos(PIOVERTWO * frak);
						//post("fadein: %d, up: %f, down: %f", counter_msf, goin_up, goin_down);
						samples[ (counter * channel_count) + j] = 
						(samples[ (counter * channel_count) + j] * goin_down)
						+ (record_inlet[i] * goin_up);
					} else if ( counter_msf >= (regionsamps - fadesamps) ){
						frak = (float) (regionsamps - counter_msf) / (float) fadesamps;
						// fade out
					
						// frak = (float)counter_msf / (float)fadesamps;
						goin_up = cos(PIOVERTWO * frak);
						goin_down = sin(PIOVERTWO * frak);
						//post("fadeout: %d, up: %f, down: %f", counter_msf, goin_up, goin_down);
						samples[ (counter * channel_count) + j] = 
						(samples[ (counter * channel_count) + j] * goin_up)
						+ (record_inlet[i] * goin_down);
					} else {
						// straight replace
						samples[ (counter * channel_count) + j] = record_inlet[i];
					}
					
				}
			}
			counter++;
		}
		syncphase = (float) counter / (float) b_frames;
		sync[i] = syncphase;
		
	}
	
	/*for(i = 0; i < n; i++){
		sync[i] = syncphase;
	}*/
	if(status){
		x->display_counter += n;
		if(x->display_counter > WAVEFORM_UPDATE){
			garray_redraw(x->recbuf);
			x->display_counter = 0;
		}
	}
	x->end_frame = end_frame;
	x->status = status;
	x->counter = counter;
	x->syncphase = syncphase;
	return w + next_pointer;
}

void sarec_overdub(t_sarec *x)
{
	x->mode = SAREC_OVERDUB;
}


void sarec_record(t_sarec *x)
{
	x->mode = SAREC_RECORD;
}

void sarec_punchfade(t_sarec *x, t_floatarg fadetime)
{
	x->fadesamps = x->sr * fadetime * 0.001; // read fade in ms.
	//post("punch mode");
}

void sarec_punch(t_sarec *x)
{
	x->mode = SAREC_PUNCH;
}

void sarec_lock(t_sarec *x)
{
	x->mode = SAREC_LOCK;
}

void sarec_disarm(t_sarec *x, t_floatarg chan)
{
//	int i;
	int ichan = (int) chan;
	if(chan <= x->channel_count && chan > 0) {
		x->armed_chans[ichan - 1] = 0;
	}
}

void sarec_arm(t_sarec *x, t_floatarg chan)
{
	int i;
	int ichan = (int) chan;
	if(ichan == -1){
		for(i = 0; i < x->channel_count; i++){
			x->armed_chans[i] = 1;
		}
	} else if(ichan == 0)
	{
		for(i = 0; i < x->channel_count; i++){
			x->armed_chans[i] = 0;
		}
	} else if(chan <= x->channel_count && chan > 0) {
		x->armed_chans[ichan - 1] = 1;
	}
}

void sarec_region(t_sarec *x, t_symbol *msg, short argc, t_atom *argv)
{
	long b_frames = x->b_frames;
	long start_frame, end_frame;
	float sr = x->sr;
	// convert milliseconds to samples:
	start_frame = (long) (sr * 0.001 * atom_getfloatarg(0, argc, argv) );
	end_frame = (long) (sr * 0.001 * atom_getfloatarg(1, argc, argv) );
	start_frame = start_frame < 0 || start_frame > b_frames - 1 ? 0 : start_frame;
	end_frame = end_frame > b_frames ? b_frames : end_frame;
	x->end_frame = end_frame;
	x->start_frame = start_frame;
	x->regionsamps = end_frame - start_frame;
}

void sarec_regionsamps(t_sarec *x, t_symbol *msg, short argc, t_atom *argv)
{
	long b_frames = x->b_frames;
	long start_frame, end_frame;
	start_frame = (long) atom_getfloatarg(0, argc, argv);
	end_frame = (long) atom_getfloatarg(1, argc, argv);
	start_frame = start_frame < 0 || start_frame > b_frames - 1 ? 0 : start_frame;
	end_frame = end_frame > b_frames ? b_frames : end_frame;
	x->end_frame = end_frame;
	x->start_frame = start_frame;
	x->regionsamps = end_frame - start_frame;
}



void player_setbuf(t_sarec *x, t_symbol *wavename)
{
	int frames;
	long b_frames;

	t_garray *a;
	
	x->b_valid = 0;

	x->b_frames = 0;
	x->b_valid = 0;
	if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class)))
    {
		if (*wavename->s_name) pd_error(x, "player~: %s: no such array",
										wavename->s_name);
		x->b_samples = 0;
		x->b_valid = 1;
    }
	else if (!garray_getfloatarray(a, &frames, &x->b_samples))
    {
		pd_error(x, "%s: bad template for player~", wavename->s_name);
		x->b_samples = 0;
		x->b_valid = 1;
    }
	else  {
		x->b_frames = frames;
		x->b_valid = 1;
		garray_usedindsp(a);
	}
	if(! x->b_valid ){
		post("player~ got invalid buffer");
	}	
}


void sarec_attach_buffer(t_sarec *x)
{
	t_garray *b;
	t_symbol *bufname = x->bufname;
	int frames;
	
	if (!(b = (t_garray *)pd_findbyclass(bufname, garray_class)))
    {
		if (*bufname->s_name) pd_error(x, "player~: %s: no such array",
										bufname->s_name);
		x->b_valid = 0;
    }
	else if (!garray_getfloatarray(b, &frames, &x->b_samples))
    {
		pd_error(x, "%s: bad template for player~", bufname->s_name);
		x->b_samples = 0;
		x->b_valid = 1;
    }
	else  {
		x->b_frames = frames;
		x->b_valid = 1;
		x->recbuf = b;
		garray_usedindsp(b);
	}
	if(! x->b_valid ){
		post("player~ got invalid buffer");
	}	
}


void sarec_dsp(t_sarec *x, t_signal **sp, short *count)
{
	long i;
	t_int **sigvec;
	int pointer_count;
	pointer_count = x->channel_count + 4; // all channels, 1 inlet, 1 sync outlet,  the object pointer, vector size N
	
	sarec_attach_buffer(x);
	if( x->start_frame < 0 && x->end_frame < 0){
		x->start_frame = 0;
		x->end_frame = x->b_frames;
		// post("new frame extrema: %ld %ld",x->start_frame,x->end_frame);
	}
	sigvec  = (t_int **) calloc(pointer_count, sizeof(t_int *));	
	for(i = 0; i < pointer_count; i++){
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	}
	sigvec[0] = (t_int *)x; // first pointer is to the object
	
	sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // last pointer is to vector size (N)
	
	for(i = 1; i < pointer_count - 1; i++){ // now attach the inlet and all outlets
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
	}
	x->sr = sp[0]->s_sr;
	dsp_addv(sarec_perform, pointer_count, (t_int *) sigvec); 
	free(sigvec);
	
}

