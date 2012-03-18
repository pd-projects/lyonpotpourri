#include "MSPd.h"

/**/

#if __MSP__
void *channel_class;
#endif 

#if __PD__
static t_class *channel_class;
#endif

typedef struct _channel
{
#if __MSP__
	t_pxobject x_obj;
#endif
#if __PD__
	t_object x_obj;
	float x_f;
#endif
	void *float_outlet;
	int channel;

	
} t_channel;

#define OBJECT_NAME "channel~"

void *channel_new(t_symbol *s, int argc, t_atom *argv);

t_int *channel_perform(t_int *w);
void channel_dsp(t_channel *x, t_signal **sp, short *count);
void channel_assist(t_channel *x, void *b, long m, long a, char *s);
void channel_channel(t_channel *x, t_floatarg chan) ;
void channel_int(t_channel *x, long chan) ;

#if __MSP__
void main(void)
{
    setup((t_messlist **)&channel_class, (method)channel_new, (method)dsp_free, 
		  (short)sizeof(t_channel), 0, A_GIMME, 0);
    addmess((method)channel_dsp, "dsp", A_CANT, 0);
    addmess((method)channel_assist,"assist",A_CANT,0);
    addmess((method)channel_channel,"channel",A_FLOAT,0);
    	addinx((method)channel_int, 1);
    dsp_initclass();
    post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

#if __PD__

#define NO_FREE_FUNCTION 0
void channel_tilde_setup(void)
{
	channel_class = class_new(gensym("channel~"), (t_newmethod)channel_new, 
								 NO_FREE_FUNCTION,sizeof(t_channel), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(channel_class, t_channel, x_f);
	class_addmethod(channel_class, (t_method)channel_dsp, gensym("dsp"), 0);
	class_addmethod(channel_class,(t_method)channel_channel,gensym("channel"),A_FLOAT,0);
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
	//  class_addmethod(channel_class, (t_method)channel_bang, gensym("bang"),  0);
}
#endif

void channel_channel(t_channel *x, t_floatarg chan) 
{
	if(chan >= 0)
		x->channel = (int) chan;
}


void channel_int(t_channel *x, long chan) 
{
	if(chan >= 0)
		x->channel = (int) chan;
}

void channel_assist (t_channel *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:sprintf(dst,"(signal) Input");break;
			case 1:sprintf(dst,"(int) Channel Number");break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Channel Value");
	}
}

void *channel_new(t_symbol *s, int argc, t_atom *argv)
{
#if __MSP__
	t_channel *x = (t_channel *)newobject(channel_class);
	intin(x,1);
	dsp_setup((t_pxobject *)x,1);
	outlet_new((t_object *)x, "signal");

#endif
#if __PD__
	t_channel *x = (t_channel *)pd_new(channel_class);
	outlet_new(&x->x_obj, gensym("signal"));	
#endif

	x->channel = (int)atom_getfloatarg(0,argc,argv);
	// x->channel = 0;
	return (x);
}

t_int *channel_perform(t_int *w)
{
	t_channel *x = (t_channel *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	t_float *out_vec = (t_float *)(w[3]);
	t_int n = w[4];
	int channel = x->channel;
	float value;
	
	if(channel < 0 || channel > n){
		return w + 5;
	}
	value = in_vec[channel];
	
	while( n-- ) {
		*out_vec++ = value;
	}
	return w + 5;
}		

void channel_dsp(t_channel *x, t_signal **sp, short *count)
{
    dsp_add(channel_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

