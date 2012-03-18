#include "MSPd.h"
#define MAX_CHANS (64)
#define CS_LINEAR (0)
#define CS_POWER (1)

#if __PD__
static t_class *clean_selector_class;
#endif

#if __MSP__
void *clean_selector_class;
#endif

typedef struct _clean_selector
{
#if __MSP__
	t_pxobject x_obj;
#endif
#if __PD__
	t_object x_obj;
	float x_f;
#endif
	// Variables Here
	short input_chans;
	short active_chan;
	short last_chan;
	int samps_to_fade;
	int fadesamps;
	float fadetime;
	float pi_over_two;
	short fadetype;
	short *connected_list;
	float **bulk ; // array to point to all input audio channels
	float sr;
	float vs;
	int inlet_count;
} t_clean_selector;

#define OBJECT_NAME "clean_selector~"

void *clean_selector_new(t_symbol *s, int argc, t_atom *argv);

t_int *clean_selector_perform(t_int *w);
void clean_selector_dsp(t_clean_selector *x, t_signal **sp, short *count);
void clean_selector_assist(t_clean_selector *x, void *b, long m, long a, char *s);
void clean_selector_float(t_clean_selector *x, t_float f);
void clean_selector_fadetime(t_clean_selector *x, t_floatarg f);
void clean_selector_int(t_clean_selector *x, t_int i);
void clean_selector_channel(t_clean_selector *x, t_floatarg i);
void clean_selector_dsp_free(t_clean_selector *x);

#if __MSP__
void main(void)
{
	setup((t_messlist **)&clean_selector_class, (method)clean_selector_new, (method)dsp_free, 
		  (short)sizeof(t_clean_selector), 0, A_GIMME, 0);
	addmess((method)clean_selector_dsp, "dsp", A_CANT, 0);
	addmess((method)clean_selector_assist,"assist",A_CANT,0);
	addmess((method)clean_selector_fadetime,"fadetime",A_FLOAT,0);
	addmess((method)clean_selector_channel,"channel",A_FLOAT,0);
	addfloat((method)clean_selector_float);
	addint((method)clean_selector_int);
	dsp_initclass();
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

#if __PD__
void clean_selector_tilde_setup(void) {
	clean_selector_class = class_new(gensym("clean_selector~"), (t_newmethod)clean_selector_new, 
									 (t_method)clean_selector_dsp_free,sizeof(t_clean_selector), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(clean_selector_class, t_clean_selector, x_f);
	class_addmethod(clean_selector_class,(t_method)clean_selector_dsp,gensym("dsp"),0);
	class_addmethod(clean_selector_class,(t_method)clean_selector_fadetime,gensym("fadetime"),A_FLOAT,0);
	class_addmethod(clean_selector_class,(t_method)clean_selector_channel,gensym("channel"),A_FLOAT,0);
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

void clean_selector_assist (t_clean_selector *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		if(arg == 0){
			sprintf(dst,"(signal/int) Input 0, Channel Number");
		} else {
			sprintf(dst,"(signal) Input %ld",arg);
		}
		
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void *clean_selector_new(t_symbol *s, int argc, t_atom *argv)
{
	int i;
	t_clean_selector *x;
#if __MSP__
	x = (t_clean_selector *)newobject(clean_selector_class);
#endif
#if __PD__
	x = (t_clean_selector *)pd_new(clean_selector_class);
#endif
		x->fadetime = 0.05;
		x->inlet_count = 8;
		if(argc >= 1){
			x->inlet_count = (int)atom_getfloatarg(0,argc,argv);
			if(x->inlet_count < 2 || x->inlet_count > MAX_CHANS){
				error("%s: %d is illegal number of inlets",OBJECT_NAME,x->inlet_count);
				return (void *) NULL;
			}

		}  
		if(argc >= 2){
			x->fadetime = atom_getfloatarg(1,argc,argv) / 1000.0;
		}

//		post("argc %d inlet count %d fadetime %f",argc, x->inlet_count, x->fadetime);
#if __MSP__
	dsp_setup((t_pxobject *)x, x->inlet_count); 
	outlet_new((t_pxobject *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE;
#endif
#if __PD__
	for(i=0; i< x->inlet_count - 1; i++){// create 16 inlets in total
		inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	}
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	
	
	x->sr = sys_getsr();
	if(!x->sr){
		x->sr = 44100.0;
		error("zero sampling rate - set to 44100");
	}
	x->fadetype = CS_POWER;
	x->pi_over_two = 1.57079632679;
	
	
    
    if(x->fadetime <= 0.0)
    	x->fadetime = .05;
    x->fadesamps = x->fadetime * x->sr;
    
    x->connected_list = (short *) t_getbytes(MAX_CHANS * sizeof(short));
    for(i=0;i<16;i++){
    	x->connected_list[i] = 0;
    }
    x->active_chan = x->last_chan = 0;
    x->bulk = (t_float **) t_getbytes(16 * sizeof(t_float *));
    x->samps_to_fade = 0;
	return (x);
}

void clean_selector_dsp_free(t_clean_selector *x)
{
#if __MSP__
	dsp_free((t_pxobject *)x);
#endif
	t_freebytes(x->bulk, 16 * sizeof(t_float *));
}


void clean_selector_fadetime(t_clean_selector *x, t_floatarg f)
{
	float fades = (float)f / 1000.0;
	
	if( fades < .0001 || fades > 1000.0 ){
		error("fade time is constrained to 0.1 - 1000000, but you wanted %f",f );
		return;
	}
	x->fadetime = fades;
	x->fadesamps = x->sr * x->fadetime;
	x->samps_to_fade = 0;
}

t_int *clean_selector_perform(t_int *w)
{
	
	t_clean_selector *x = (t_clean_selector *) (w[1]);
	int i;
	t_int n;
	t_float *out;
	
	int fadesamps = x->fadesamps;
	short active_chan = x->active_chan;
	short last_chan = x->last_chan;
	int samps_to_fade = x->samps_to_fade;
	float m1, m2;
	float **bulk = x->bulk;
	float pi_over_two = x->pi_over_two;
	short fadetype = x->fadetype;
	float phase;
	int inlet_count = x->inlet_count;
	
	for ( i = 0; i < inlet_count; i++ ) {
		bulk[i] = (t_float *)(w[2 + i]);
	}
	out = (t_float *)(w[inlet_count + 2]);
	n = w[inlet_count + 3]; 
	
	/********************************************/
	if ( active_chan >= 0 ) {
		while( n-- ) {
			if ( samps_to_fade >= 0 ) {
				if( fadetype == CS_POWER ){
					phase = pi_over_two * (1.0 - (samps_to_fade / (float) fadesamps)) ;
					m1 = sin( phase );
					m2 = cos( phase );
					--samps_to_fade;
					*out++ = (*(bulk[active_chan])++ * m1) + (*(bulk[last_chan])++ * m2);
				} 
			}
			else {
				*out++ =  *(bulk[active_chan])++;
			}
		}
	} 
  	else  {
  		while( n-- ) {
			*out++ = 0.0;
		}
  	}
    
	x->samps_to_fade = samps_to_fade;
	return (w + (inlet_count + 4));
}		

void clean_selector_dsp(t_clean_selector *x, t_signal **sp, short *count)
{
	long i;
	t_int **sigvec;
	int pointer_count;
	
	pointer_count = x->inlet_count + 3; // all inlets, 1 outlet, object pointer and vec-samps	
	sigvec  = (t_int **) calloc(pointer_count, sizeof(t_int *));	
	for(i = 0; i < pointer_count; i++){
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	}
	sigvec[0] = (t_int *)x; // first pointer is to the object
	
	sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // last pointer is to vector size (N)

	for(i = 1; i < pointer_count - 1; i++){ // now attach the inlet and all outlets
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
	}
		
	if(x->sr != sp[0]->s_sr){
		x->sr = sp[0]->s_sr;
		x->fadesamps = x->fadetime * x->sr;
		x->samps_to_fade = 0;
	}

#if __MSP__
	dsp_addv(clean_selector_perform, pointer_count, (void **) sigvec); 
#endif
#if __PD__
	dsp_addv(clean_selector_perform, pointer_count, (t_int *) sigvec); 
#endif
	free(sigvec);

#if __MSP__
	 for (i = 0; i < MAX_CHANS; i++) {
		 x->connected_list[i] = count[i];
	 }
#endif
#if __PD__
	 for (i = 0; i < MAX_CHANS; i++) {
		 x->connected_list[i] = 1;
	 }
#endif

			
}

#if __MSP__
void clean_selector_float(t_clean_selector *x, t_float f) // Look at floats at inlets
{
	clean_selector_int(x,(t_int)f);	
}

void clean_selector_int(t_clean_selector *x, t_int i) // Look at int at inlets
{
	int inlet = ((t_pxobject*)x)->z_in;
	
	if (inlet == 0)
	{		
			clean_selector_channel(x,(t_floatarg)i);
	}	

}
#endif

void clean_selector_channel(t_clean_selector *x, t_floatarg i) // Look at int at inlets
{
	int chan = i;
	if(chan < 0 || chan > x->inlet_count - 1){
		post("%s: channel %d out of range",OBJECT_NAME, chan);
		return;
	}	
	if(chan != x->active_chan) {
		
		x->last_chan = x->active_chan;
		x->active_chan = chan;
		x->samps_to_fade = x->fadesamps;
		if( x->active_chan < 0)
			x->active_chan = 0;
		if( x->active_chan > MAX_CHANS - 1) {
			x->active_chan = MAX_CHANS - 1;
		}
		if(! x->connected_list[chan]) {
		// do it anyway - it's user-stupidity
		/*
			post("warning: channel %d not connected",chan);
			x->active_chan = 1; */
		}
		// post("last: %d active %d", x->last_chan, x->active_chan);
	}	
}
