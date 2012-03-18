#include "MSPd.h"

#if __MSP__
void *rtrig_class;
#endif 

#if __PD__
static t_class *rtrig_class;
#endif

#define OBJECT_NAME "rtrig~"

#define MAXFLAMS (16)
#define MAXATTACKS (128)
#define STOPGAIN (.001)

// CW's RAND_MAX is wrong
#define RAND_MAX2 (0x7fffffff)

typedef struct
{
	int attack_count; // number of triggers per flam event
	float *attack_times; // trigger times in seconds
	int *attack_points; // trigger times in samples
	int fdex; // current flam
	float gainatten; // attenuation factor
	float amp; // current amp
	int atks;// number of attacks per flam
	long counter; // internal clock
	short active; // flag that flam is turned on


} t_flam;

typedef struct _rtrig
{
#if __MSP__
	t_pxobject x_obj;
#endif
#if __PD__
	t_object x_obj;
	float x_f;
#endif
  short mute;
  float min;
  float max;
  float odds;

} t_rtrig;


void *rtrig_new(t_symbol *s, int argc, t_atom *argv);
t_int *rtrig_perform(t_int *w);
void rtrig_dsp(t_rtrig *x, t_signal **sp, short *count);
void rtrig_free(t_rtrig *x);
void rtrig_assist(t_rtrig *x, void *b, long msg, long arg, char *dst);
void rtrig_mute(t_rtrig *x, t_floatarg tog);
void rtrig_min(t_rtrig *x, t_floatarg v);
void rtrig_max(t_rtrig *x, t_floatarg v);
void rtrig_odds(t_rtrig *x, t_floatarg v);

#if __MSP__
void main(void)
{
    setup((t_messlist **)&rtrig_class, (method)rtrig_new, (method)rtrig_free, (short)sizeof(t_rtrig), 0, A_GIMME, 0);
    addmess((method)rtrig_dsp, "dsp", A_CANT, 0);
    addmess((method)rtrig_assist, "assist", A_CANT, 0);
    addmess((method)rtrig_mute, "mute", A_FLOAT, 0);
    addmess((method)rtrig_min, "min", A_FLOAT, 0);
    addmess((method)rtrig_max, "max", A_FLOAT, 0);
    addmess((method)rtrig_odds, "odds", A_FLOAT, 0);
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);

    dsp_initclass();
}
#endif
#if __PD__
void rtrig_tilde_setup(void)
{
rtrig_class = class_new(gensym("rtrig~"),(t_newmethod)rtrig_new,(t_method)rtrig_free, sizeof(t_rtrig), 0, A_GIMME,0);
CLASS_MAINSIGNALIN(rtrig_class,t_rtrig, x_f );

class_addmethod(rtrig_class,(t_method)rtrig_dsp,gensym("dsp"),A_CANT,0);
class_addmethod(rtrig_class,(t_method)rtrig_assist,gensym("assist"),A_CANT,0);
class_addmethod(rtrig_class,(t_method)rtrig_mute,gensym("mute"),A_FLOAT,0);
class_addmethod(rtrig_class,(t_method)rtrig_min,gensym("min"),A_FLOAT,0);
class_addmethod(rtrig_class,(t_method)rtrig_max,gensym("max"),A_FLOAT,0);
class_addmethod(rtrig_class,(t_method)rtrig_odds,gensym("odds"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

void rtrig_mute(t_rtrig *x, t_floatarg tog)
{
	x->mute = (short) tog;
}

void rtrig_min(t_rtrig *x, t_floatarg v)
{
	x->min = (float) v;
}

void rtrig_max(t_rtrig *x, t_floatarg v)
{
	x->max = (float) v;
}

void rtrig_odds(t_rtrig *x, t_floatarg v)
{
	x->odds = (float) v;
}


void rtrig_assist(t_rtrig *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    	case 0: sprintf(dst,"(signal) Unused"); break;
    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Triggers");
  }

}

void *rtrig_new(t_symbol *s, int argc, t_atom *argv)
{
#if __MSP__
    t_rtrig *x = (t_rtrig *)newobject(rtrig_class);
    dsp_setup((t_pxobject *)x,1);	
    outlet_new((t_pxobject *)x, "signal");
#endif
#if __PD__
  t_rtrig *x = (t_rtrig *)pd_new(rtrig_class);
  outlet_new(&x->x_obj, gensym("signal"));
#endif

    x->mute = 0;
		x->min = atom_getfloatarg(0,argc,argv);
		x->max = atom_getfloatarg(1,argc,argv);
		x->odds = atom_getfloatarg(2,argc,argv);

		srand(time(0));
    return (x);
}



void rtrig_free(t_rtrig *x)
{
//int i;

#if __MSP__
  dsp_free((t_pxobject *) x);
#endif

}

t_int *rtrig_perform(t_int *w)
{

	t_rtrig *x = (t_rtrig *) (w[1]);
	float *out_vec = (t_float *)(w[2]);
	int n = w[3];
	
	float rval;
	float min = x->min;
	float max = x->max;
	float odds = x->odds;

	if(x->mute){
		memset( (void *)out_vec, 0, n * sizeof(float) );
		return (w+4);
	}
		
	while( n-- ){
		rval = (float) rand() / (float) RAND_MAX2;
		if(rval < odds){
			rval = min + (max-min) * ((float) rand() / (float) RAND_MAX2);
			*out_vec++ = rval;
		} else {
			*out_vec++ = 0.0;
		}
	}
	

	return (w+4);
}

void rtrig_dsp(t_rtrig *x, t_signal **sp, short *count)
{

	dsp_add(rtrig_perform, 3, x, 
//			sp[0]->s_vec, 
			sp[1]->s_vec,
			sp[0]->s_n
	);
}

