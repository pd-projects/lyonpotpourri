#include "MSPd.h"

#define OBJECT_NAME "vdb~"

#if __MSP__
void *vdb_class;
#endif
#if __PD__
static t_class *vdb_class;
#endif

typedef struct
{
	float coef;
	float cutoff;
	float x1;
} t_lpf;

typedef struct {
	float *b_samples;
	long b_valid;
	long b_nchans;
	long b_frames;
} t_guffer; // stuff we care about from garrays and buffers


typedef struct _vdb
{
#if __MSP__
    t_pxobject x_obj;
#endif
#if __PD__
  t_object x_obj;
  float x_f;
#endif

	float sr;
	t_lpf lpf;
	short filter;
	//
	float speed;
	float feedback;
	float delay_time;
	float delay_samps;
	float maxdelay; // maximum delay in seconds (cannot be larger than buffer)
	long maxdelay_len; // framelength of usable region of buffer
	long len; // framelength of buffer
	long phs; // current phase
	float tap;
	short *connections;
	short feedback_protect;
	short mute;
	short interpolate;
	short inf_hold;
	short always_update;
	// copy to buffer
	t_symbol *buffername;
	t_guffer *delay_buffer; 
	long b_nchans;
	long b_frames;
	float *b_samples;
	long b_valid;
	// interface
	int inlet_count;
	int outlet_count;
	int delay_inlet;
	int feedback_inlet;
	short redraw_flag; // pd only for gating redraw function
	
} t_vdb;

t_int *vdb_perform(t_int *w);

void vdb_protect(t_vdb *x, t_floatarg state);
void vdb_inf_hold(t_vdb *x, t_floatarg state);
void vdb_always_update(t_vdb *x, t_floatarg state);
void vdb_maxdelay(t_vdb *x, t_floatarg delay);
void vdb_dsp(t_vdb *x, t_signal **sp, short *count);
void *vdb_new(t_symbol *s, int argc, t_atom *argv);
void vdb_assist(t_vdb *x, void *b, long m, long a, char *s);
void vdb_float(t_vdb *x, t_float f);
void vdb_mute(t_vdb *x, t_floatarg t);
void vdb_interpolate(t_vdb *x, t_floatarg t);
void vdb_show(t_vdb *x);
void vdb_update_buffer(t_vdb *x);
void vdb_coef(t_vdb *x, t_floatarg f);
void vdb_filter(t_vdb *x, t_floatarg t);
void vdb_init(t_vdb *x,short initialized);
int vdb_attach_buffer(t_vdb *x);
void vdb_redraw(t_vdb *x);
void vdb_redraw_array(t_vdb *x, t_floatarg t);
void vdb_free(t_vdb *x);

#if __MSP__
void vdb_int(t_vdb *x, long f);

void main(void)
{
	setup((t_messlist **)&vdb_class, (method)vdb_new, (method)vdb_free, 
		  (short)sizeof(t_vdb), 0, A_GIMME, 0);
	addmess((method)vdb_dsp, "dsp", A_CANT, 0);
	addfloat((method)vdb_float);
	addint((method)vdb_int);
	addmess((method)vdb_assist, "assist", A_CANT, 0);
	addmess((method)vdb_protect,"protect",A_FLOAT,0);
	addmess((method)vdb_inf_hold,"inf_hold",A_FLOAT,0);
	addmess((method)vdb_maxdelay,"maxdelay",A_FLOAT,0);
	addmess((method)vdb_always_update,"always_update",A_FLOAT,0);
	addmess((method)vdb_mute, "mute", A_LONG, 0);
	addmess((method)vdb_show, "show", 0);
	addmess((method)vdb_update_buffer, "update_buffer", 0);
	addmess((method)vdb_interpolate, "interpolate", A_LONG, 0);
	
	dsp_initclass();
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

#if __PD__
void vdb_tilde_setup(void)
{
	
	vdb_class = class_new(gensym("vdb~"),(t_newmethod)vdb_new,(t_method)vdb_free, sizeof(t_vdb), 0, A_GIMME,0);
	CLASS_MAINSIGNALIN(vdb_class,t_vdb, x_f );
	class_addmethod(vdb_class,(t_method)vdb_dsp,gensym("dsp"),A_CANT,0);
	class_addmethod(vdb_class,(t_method)vdb_assist,gensym("assist"),A_CANT,0);
	class_addmethod(vdb_class,(t_method)vdb_protect,gensym("protect"),A_FLOAT,0);
	class_addmethod(vdb_class,(t_method)vdb_inf_hold,gensym("inf_hold"),A_FLOAT,0);
	class_addmethod(vdb_class,(t_method)vdb_maxdelay,gensym("maxdelay"),A_FLOAT,0);
	class_addmethod(vdb_class,(t_method)vdb_always_update,gensym("always_update"),A_FLOAT,0);
	class_addmethod(vdb_class,(t_method)vdb_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(vdb_class,(t_method)vdb_show,gensym("show"),0);
//	class_addmethod(vdb_class,(t_method)vdb_update_buffer,gensym("update_buffer"),0);
	class_addmethod(vdb_class,(t_method)vdb_interpolate,gensym("interpolate"),A_FLOAT,0);
	class_addmethod(vdb_class,(t_method)vdb_redraw_array,gensym("redraw_array"),A_FLOAT,0);
	
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif


void vdb_maxdelay(t_vdb *x, t_floatarg delay)
{
	long newlen;
	x->maxdelay = 50.0;
	newlen = delay * .001 * x->sr;
	if(newlen > x->len){
		error("%s: requested a max delay that exceeds buffer size",OBJECT_NAME);
		return;
	}
	x->maxdelay_len = newlen;
	
}

void vdb_update_buffer(t_vdb *x)
{
	vdb_attach_buffer(x);
}

void vdb_mute(t_vdb *x, t_floatarg t)
{
	x->mute = (short)t;
}

void vdb_always_update(t_vdb *x, t_floatarg state)
{
	x->always_update = (short) state;
}

void vdb_redraw_array(t_vdb *x, t_floatarg t)
{
	x->redraw_flag = (short)t;
}


void vdb_inf_hold(t_vdb *x, t_floatarg state)
{
	x->inf_hold = (short) state;
}

void vdb_filter(t_vdb *x, t_floatarg t)
{
	x->filter = (short)t;
}

void vdb_coef(t_vdb *x, t_floatarg f)
{
	x->lpf.coef = (float)f;
}

void vdb_show(t_vdb *x)
{
	post("feedback %f delay %f",x->feedback, x->delay_time);
}

void vdb_interpolate(t_vdb *x, t_floatarg t)
{
	x->interpolate = (short)t;
}

t_int *vdb_perform(t_int *w)
{
	// DSP config
    t_vdb *x = (t_vdb *)(w[1]);
    int n;
	
    float fdelay;
    float insamp; //, insamp2;
    float outsamp;
    float frac;
    float *delay_line = x->b_samples;
	
	int phs = x->phs;
	long maxdelay_len = x->maxdelay_len;
	
	float feedback = x->feedback;
	short *connections = x->connections;
	float sr = x->sr;
	short feedback_protect = x->feedback_protect;
	short interpolate = x->interpolate;
	short inf_hold = x->inf_hold;
	float x1,x2;
	int idelay;
	int dphs,dphs1,dphs2;
	long b_nchans = x->b_nchans;
 	int delay_inlet = x->delay_inlet;
 	int feedback_inlet = x->feedback_inlet;
	t_int i,j;
	t_float *input;
	t_float *output;
	t_float *delay_vec;
	t_float *feedback_vec;
	
	/**********************/
	
    
    
	n = w[b_nchans * 2 + 4];
	
	if(x->always_update){
		vdb_attach_buffer(x);
		maxdelay_len = x->maxdelay_len;
		phs = x->phs;
	}
#if __MSP__
	if( x->mute || x->x_obj.z_disabled) {
#endif
#if __PD__
	if( x->mute ) {
#endif
		for(i = 0; i < b_nchans; i++){
			output = (t_float *) w[4 + b_nchans + i];
			for(j = 0; j < n; j++){
				*output++ = 0.0;
			}
		}
		return (w + b_nchans * 2 + 5);
	}
	
	
	
	if(!x->b_valid){
		for(i = 0; i < b_nchans; i++){
			output = (t_float *) w[4 + b_nchans + i];
			for(j = 0; j < n; j++){
				*output++ = 0.0;
			}
		}
		return (w + b_nchans * 2 + 5);
	}
	
	fdelay = x->delay_time * .001 * sr;
	feedback = x->feedback;
	delay_vec = (t_float *) w[b_nchans + 2];
	feedback_vec = (t_float *) w[b_nchans + 3];
	for(i = 0; i < b_nchans; i++){
		input = (t_float *) w[i+2];
		output = (t_float *) w[4 + b_nchans + i];
		phs = x->phs; // reset for each channel
		for(j = 0; j < n; j++){
			
			//		insamp = input[j];
			
			if ( connections[delay_inlet]) {
				fdelay = delay_vec[j];
				fdelay *= .001 * sr;
				if (fdelay < 1. )
					fdelay = 1.;
				if( fdelay > maxdelay_len - 1 )
					fdelay = maxdelay_len - 1;
				x->delay_time = fdelay;
			}
			if(! inf_hold ){
				if(connections[feedback_inlet]){
					feedback = feedback_vec[j];
					if( feedback_protect ) {
						if( feedback > 0.99)
							feedback = 0.99;
						if( feedback < -0.99 )
							feedback = -0.99;
					}
					x->feedback = feedback;
				}
			} 
			
			idelay = floor(fdelay);
			
			if(phs < 0 || phs >= maxdelay_len){
				error("%s: bad phase %d",OBJECT_NAME,phs);
				phs = 0;
			}
			
			if(interpolate){
				frac = (fdelay - idelay);
				dphs1 = phs - idelay;
				dphs2 = dphs1 - 1;
				
				while(dphs1 >= maxdelay_len){
					dphs1 -= maxdelay_len;
				}
				while(dphs1 < 0){
					dphs1 += maxdelay_len;
				}	
				while(dphs2 >= maxdelay_len){
					dphs2 -= maxdelay_len;
				}
				while(dphs2 < 0){
					dphs2 += maxdelay_len;
				}
				
				x1 = delay_line[dphs1 * b_nchans + i];
				x2 = delay_line[dphs2 * b_nchans + i];
				outsamp = x1 + frac * (x2 - x1);
				
			} else {
				dphs = phs - idelay;
				while(dphs >= maxdelay_len){
					dphs -= maxdelay_len;
				}
				while(dphs < 0){
					dphs += maxdelay_len;
				}
				if(dphs < 0 || dphs >= maxdelay_len){
					error("bad dphase %d",dphs);
					dphs = 0;
				}			
				outsamp = delay_line[dphs * b_nchans + i];
				
				
			}
			output[j] = outsamp;
			if(! inf_hold ){
				insamp = input[j];
				delay_line[phs * b_nchans + i] = insamp + outsamp * feedback;
			}
			++phs;
			while(phs >= maxdelay_len){
				phs -= maxdelay_len;
			}
			while(phs < 0){
				phs += maxdelay_len;
			}	
			
		}
		
	}
	
	
	x->phs = phs;
	if(x->redraw_flag){
		vdb_redraw(x);
	}
	return (w + b_nchans * 2 + 5);
	
	
}

void *vdb_new(t_symbol *s, int argc, t_atom *argv)
{

	int i;
	int user_chans;
#if __MSP__
	t_vdb *x;
	x = (t_vdb *)newobject(vdb_class);
#endif
#if __PD__
	t_vdb *x = (t_vdb *)pd_new(vdb_class);
#endif

	x->sr = sys_getsr();
	if(argc < 2){
		error("%s: you must provide a valid buffer name and channel count",OBJECT_NAME);
		return (void *)NULL;
	}		

	

	
	if(!x->sr){
		error("zero sampling rate - set to 44100");
		x->sr = 44100;
	}
	// DSP CONFIG
	
	
	// SET DEFAULTS
	x->maxdelay = 50.0; // milliseconds
	x->feedback = 0.5;
	x->delay_time  = 0.0;
	
	// args: name channels [max delay, initial delay, feedback, interpolation_flag]

#if __MSP__
    x->buffername = atom_getsymarg(0,argc,argv);
#endif
#if __PD__
    x->buffername = atom_getsymbolarg(0,argc,argv);
#endif

	user_chans = atom_getfloatarg(1,argc,argv);
	x->maxdelay = atom_getfloatarg(2,argc,argv);
	x->delay_time = atom_getfloatarg(3,argc,argv);
	x->feedback = atom_getfloatarg(4,argc,argv);
	x->interpolate = atom_getfloatarg(5,argc,argv);   
	x->b_nchans = user_chans;
	x->redraw_flag = 1;

	/* need data checking here */
	x->inlet_count = x->b_nchans + 2;
	x->outlet_count = x->b_nchans;
	x->delay_inlet = x->b_nchans;
	x->feedback_inlet = x->delay_inlet + 1;
#if __MSP__
	dsp_setup((t_pxobject *)x,x->inlet_count);
	for(i = 0; i < x->outlet_count; i++){
		outlet_new((t_object *)x, "signal");
	}
    x->x_obj.z_misc |= Z_NO_INPLACE;
#endif
#if __PD__
	for(i = 0; i < x->inlet_count - 1; i++){
		inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	}
	outlet_new(&x->x_obj, gensym("signal") );
#endif
	
	
	vdb_init(x,0);
	return (x);
}

void vdb_free(t_vdb *x)
{
#if __MSP__
	dsp_free((t_pxobject *)x);
#endif
	free(x->connections);
}


void vdb_init(t_vdb *x,short initialized)
{
	// int i;
	
	
	if(!initialized){
		if(!x->maxdelay)
			x->maxdelay = 50.0;
		x->maxdelay_len = x->maxdelay * .001 * x->sr;
		x->feedback_protect = 0;
		x->inf_hold = 0;
		x->phs = 0;
		x->mute = 0;
		x->always_update = 0;
		x->connections = (short *) calloc(128, sizeof(short));
	} 
	
}


#if __MSP__
void vdb_int(t_vdb *x, long tog)
{
	vdb_float(x,(float)tog);	
}

void vdb_float(t_vdb *x, t_float df) // Look at floats at inlets
{
	float f = (float)df;
	
	int inlet = x->x_obj.z_in;
	
	if (inlet == x->delay_inlet)
	{
		
		if(f > 0.0 && f < x->maxdelay){
			x->delay_time = f;
			x->delay_samps = f * .001 * x->sr;
			x->tap = 0;
		} else {
			error("%s: delaytime %f out of range [0.0 - %f]",OBJECT_NAME,f,x->maxdelay);
		}
		//		post("delay is now %f",x->delay_time);
		
	}
	else if (inlet == x->feedback_inlet)
	{
		//		post("feedback is now %f",x->feedback);
		x->feedback = f;
		if( x->feedback_protect ) {
			if( x->feedback > 1.0)
				x->feedback = 1.0;
			if( x->feedback < -1.0 )
				x->feedback = -1.0;
		}
	}
	
}
#endif

void vdb_protect(t_vdb *x, t_floatarg state)
{
	x->feedback_protect = state;
}

int vdb_attach_buffer(t_vdb *x)
{
#if __MSP__
	t_buffer *b;
	t_symbol *wavename = x->buffername;
	
	if ((b = (t_buffer *)(wavename->s_thing)) && ob_sym(b) == gensym("buffer~")) {
		
		if(b->b_frames <= 0){
			error("%s: zero length buffer~ %s",OBJECT_NAME, wavename->s_name);
			x->b_valid = 0;
			return(0);
		}	/*
		 
		 if(x->b_frames != b->b_frames){
			 post("updating buffer sIze from %d to %d",x->b_frames, b->b_frames);
		 } */
	    x->delay_buffer = b;
		x->b_nchans = b->b_nchans;
		x->b_frames = b->b_frames;
		x->b_samples = b->b_samples;
		x->b_valid = b->b_valid;		
		x->len = x->b_frames;
		if(x->maxdelay_len > x->len){
			x->maxdelay_len = x->len;
			post("%s: shortened maxdelay to %d frames",OBJECT_NAME,x->maxdelay_len);
		}
		
		
   		return(1);	
	} 
	else {
    	error("%s: no such buffer~ %s",OBJECT_NAME, wavename->s_name);
    	return(0);
	}
#endif
#if __PD__
	t_garray *a;
	t_symbol *wavename = x->buffername;
	int b_frames;
	float *b_samples;
	if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
		if (*wavename->s_name) pd_error(x, "%s: %s: no such array",OBJECT_NAME,wavename->s_name);

		x->b_valid = 0;
		return 0;
    }
	else if (!garray_getfloatarray(a, &b_frames, &b_samples)) {
		pd_error(x, "%s: bad array for %s", wavename->s_name,OBJECT_NAME);
		x->b_valid = 0; 
		return 0;
    }
	else  {
		x->b_nchans = 1;
		x->b_frames = b_frames;
		x->b_samples = b_samples;
		x->b_valid = 1;		
		x->len = x->b_frames;
		if(x->maxdelay_len > x->len){
			x->maxdelay_len = x->len;
			post("%s: shortened maxdelay to %d frames",OBJECT_NAME,x->maxdelay_len);	
		}
		garray_usedindsp(a);
		return(1);	
	}
	
#endif

}

void vdb_assist(t_vdb *x, void *b, long msg, long arg, char *dst)
{
#if __MSP__
	if (msg==1) {
		if(arg < x->b_nchans ){
			sprintf(dst,"(signal) Input %ld",arg + 1);
		} else  if (arg == x->b_nchans) {
			sprintf(dst,"(signal/float) Delay Time");
		} else if (arg == 1+x->b_nchans) {
			sprintf(dst,"(signal/float) Feedback");
		}
		
	} else if (msg==2) {
		sprintf(dst,"(signal) Output %ld", arg + 1);
	}
#endif
}

void vdb_redraw(t_vdb *x)
{
#if __PD__
	t_garray *a;
	t_symbol *wavename = x->buffername;
	if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
		if (*wavename->s_name) pd_error(x, "%s: %s: no such array",OBJECT_NAME, wavename->s_name);
		x->b_valid = 0;
	}
	else  {
		garray_redraw(a);
	}
#endif
}


void vdb_dsp(t_vdb *x, t_signal **sp, short *count)
{
	int i;
	int vector_count;
	t_int **sigvec;
	
	vector_count = x->inlet_count+x->outlet_count + 2;
	
	
	for(i = 0; i < vector_count - 2; i++){
#if __MSP__
		x->connections[i] = count[i];
#endif
#if __PD__
		x->connections[i] = 1;
#endif
	}
	
	vdb_attach_buffer(x);

	sigvec  = (t_int **) calloc(vector_count, sizeof(t_int *));	
	for(i = 0; i < vector_count; i++)
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	
	sigvec[0] = (t_int *)x;
	
	sigvec[vector_count - 1] = (t_int *)sp[0]->s_n;
	
	for(i = 1; i < vector_count - 1; i++){
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
	}


#if __MSP__
	dsp_addv(vdb_perform, vector_count, (void **)sigvec);
#endif
#if __PD__
	dsp_addv(vdb_perform, vector_count, (t_int *)sigvec);
#endif

	free(sigvec);
	
	
}


