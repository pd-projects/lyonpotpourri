#include "MSPd.h"

/* pointless comment */

#if __PD__
static t_class *distortion_class;
#endif

#if __MSP__
void *distortion_class;
#endif

#define OBJECT_NAME "distortion~"

typedef struct _distortion
{
#if __MSP__
  t_pxobject x_obj;
#endif
#if __PD__
  t_object x_obj;
  float x_f;
#endif	
	float knee;
	float cut;
	float rescale ;
	short mute ;
	short case1;
} t_distortion;

void *distortion_new(t_floatarg knee, t_floatarg cut);

t_int *distortion1_perform(t_int *w);
t_int *distortion2_perform(t_int *w);
t_int *distortion3_perform(t_int *w);
void distortion_dsp(t_distortion *x, t_signal **sp, short *count);
void distortion_assist(t_distortion *x, void *b, long m, long a, char *s);
void distortion_float( t_distortion *x, double f );
void distortion_mute(t_distortion *x, t_floatarg f);


#if __MSP__
void main(void)
{
    setup((t_messlist **)&distortion_class,(method) distortion_new, (method)dsp_free, (short)sizeof(t_distortion),
	 0L, A_DEFFLOAT,A_DEFFLOAT, 0);
    addmess((method)distortion_dsp, "dsp", A_CANT, 0);

    addmess((method)distortion_mute,"mute",A_FLOAT,0);
    addmess((method)distortion_assist,"assist",A_CANT,0);
    addfloat((method)distortion_float);
    dsp_initclass();
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

// no freeing function needed
#if __PD__
void distortion_tilde_setup(void){
  distortion_class = class_new(gensym("distortion~"), (t_newmethod)distortion_new, 
			    0,sizeof(t_distortion), 0,A_DEFFLOAT,A_DEFFLOAT,0);
  CLASS_MAINSIGNALIN(distortion_class, t_distortion, x_f);
  class_addmethod(distortion_class,(t_method)distortion_dsp,gensym("dsp"),0);
  class_addmethod(distortion_class,(t_method)distortion_mute,gensym("mute"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

void distortion_assist (t_distortion *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Knee"); break;
			case 2: sprintf(dst,"(signal/float) Cut"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void *distortion_new(t_floatarg knee, t_floatarg cut)
{
//  	int i;
//	int len1 = 1;
#if __MSP__
    t_distortion *x = (t_distortion *)newobject(distortion_class);
    dsp_setup((t_pxobject *)x,3);
    outlet_new((t_pxobject *)x, "signal");
#endif
#if __PD__
  t_distortion *x = (t_distortion *)pd_new(distortion_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif
	if( knee >= cut || knee <= 0 || cut <= 0 ) {
		// post("setting defaults");
		x->knee = .1;
		x->cut = .3 ;		
	} else {
		x->knee = knee;
		x->cut = cut;		
		// post("User defined values: knee %f cut %f", knee, cut);

	}
	x->rescale = 1.0 / x->cut ;
	x->mute = 0;

    return (x);
}


// use when neither signal is connected

t_int *distortion1_perform(t_int *w)
{

	float rectified_sample, in_sample;
		
	t_distortion *x = (t_distortion *) (w[1]);
	float *in = (t_float *)(w[2]);
//	float *junk1 = (t_float *)(w[3]);
//	float *junk2 = (t_float *)(w[4]);
	float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
//	double fabs();
	
	float knee = x->knee;
	float cut = x->cut;
	float rescale = x->rescale;

	
	if( x->mute ){
		while( n-- ){
			*out++ = 0;
		}
		return (w+7);	
	}
	
	while (n--) { 
		in_sample = *in++;
		rectified_sample = fabs( in_sample );
		if( rectified_sample < knee ){
			*out++ = in_sample;
		} else {
			if( in_sample > 0.0 ){
				*out++ = rescale * (knee + (rectified_sample - knee) * (cut - knee));
			} else {
				*out++ = rescale * (-(knee + (rectified_sample - knee) * (cut - knee)));
			}
		}

	}
	return (w+7);
}

// use when both signals are connected

t_int *distortion2_perform(t_int *w)
{

	float rectified_sample, in_sample;
		
	t_distortion *x = (t_distortion *) (w[1]);
	float *in = (t_float *)(w[2]);
	float *data1 = (t_float *)(w[3]);
	float *data2 = (t_float *)(w[4]);
	float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
//	double fabs();

	float knee = x->knee;
	float cut = x->cut;
	float rescale = x->rescale;

	
	if( x->mute ){
		while( n-- ){
			*out++ = 0;
		}
		return (w+7);	
	}
	
	while (n--) { 
		in_sample = *in++;
		knee = *data1++;
		cut = *data2++;
		if( cut > 0.000001 )
			rescale = 1.0 / cut;
		else 
			rescale = 1.0;
		
		rectified_sample = fabs( in_sample );
		if( rectified_sample < knee ){
			*out++ = in_sample;
		} else {
			if( in_sample > 0.0 ){
				*out++ = rescale * (knee + (rectified_sample - knee) * (cut - knee));
			} else {
				*out++ = rescale * (-(knee + (rectified_sample - knee) * (cut - knee)));
			}
		}

	}
	x->knee = knee;
	x->cut = cut;
	x->rescale = rescale;
	return (w+7);
}

t_int *distortion3_perform(t_int *w)
{

	float rectified_sample, in_sample;
		
	t_distortion *x = (t_distortion *) (w[1]);
	float *in = (t_float *)(w[2]);
	float *data1 = (t_float *)(w[3]);
	float *data2 = (t_float *)(w[4]);
	float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
//	double fabs();

	float knee = x->knee;
	float cut = x->cut;
	float rescale = x->rescale;
	short case1 = x->case1;
	
	if( x->mute ){
		while( n-- ){
			*out++ = 0;
		}
		return (w+7);	
	}
	
	while (n--) { 
	// first case, knee is connected, otherwise cut is connected
		in_sample = *in++;
		if( case1 ){
			knee = *data1++;
		}
		else {
			cut = *data2++;
		}
		if( cut > 0.000001 )
			rescale = 1.0 / cut;
		else 
			rescale = 1.0;
		
		rectified_sample = fabs( in_sample );
		if( rectified_sample < knee ){
			*out++ = in_sample;
		} else {
			if( in_sample > 0.0 ){
				*out++ = rescale * (knee + (rectified_sample - knee) * (cut - knee));
			} else {
				*out++ = rescale * (-(knee + (rectified_sample - knee) * (cut - knee)));
			}
		}

	}
	x->knee = knee;
	x->cut = cut;
	x->rescale = rescale;
	return (w+7);
}
void distortion_mute(t_distortion *x, t_floatarg f) {
	x->mute = f;
}

#if __MSP__
void distortion_float( t_distortion *x, double f )
{
	int inlet = ((t_pxobject*)x)->z_in;
	switch( inlet ){
		case 1: 
			x->knee = (float)f;
			// post("knee: %f",x->knee);
			if( x->knee < .000001 )
				x->knee = .000001 ;

			break;
		case 2:
			x->cut = (float)f;
			// post("cut: %f",x->cut);
			if( x->cut <= 0.0 ) {
				x->cut = .01 ;
			} else if( x->cut > 1.0 ) {
				x->cut = 1.0 ;
			}
			x->rescale = 1.0 / x->cut ;

		break;
	}
}
#endif

void distortion_dsp(t_distortion *x, t_signal **sp, short *count)
{
//	long i;
#if __PD__
			// post("both signals connected");
	dsp_add(distortion2_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,sp[0]->s_n);
#endif
#if __MSP__
		//post("counts: %d %d %d", count[0], count[1], count[2]);
		if( count[0] && ! count[1] && ! count[2] ){
			dsp_add(distortion1_perform, 6, x, 
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
			sp[0]->s_n);
		} else if (count[0] && count[1] && count[2]) {
			// post("both signals connected");
			dsp_add(distortion2_perform, 6, x, 
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
			sp[0]->s_n);
		} else if (count[0] && count[1] && ! count[2]) {
			//post("knee connected");
			x->case1 = 1;
			dsp_add(distortion3_perform, 6, x, 
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
			sp[0]->s_n);
		}
		else if (count[0] && ! count[1] && count[2]) {
			//post("cut connected");
			x->case1 = 0; 
			dsp_add(distortion3_perform, 6, x, 
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
			sp[0]->s_n);
		}
#endif 
}

