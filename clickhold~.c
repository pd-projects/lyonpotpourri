#include "MSPd.h"

/**/

#if __MSP__
void *clickhold_class;
#endif 

#if __PD__
static t_class *clickhold_class;
#endif

#define OBJECT_NAME "clickhold~"

typedef struct _clickhold
{
#if __MSP__
	t_pxobject x_obj;
#endif
#if __PD__
	t_object x_obj;
	float x_f;
#endif
	float hold_value;
	
} t_clickhold;

void *clickhold_new(t_symbol *s, int argc, t_atom *argv);
t_int *clickhold_perform(t_int *w);
void clickhold_dsp(t_clickhold *x, t_signal **sp, short *count);
void clickhold_assist(t_clickhold *x, void *b, long m, long a, char *s);


/* void clickhold_bang(t_clickhold *x);

void clickhold_bang(t_clickhold *x) {
	x->activated = 1 ;
}*/

#if __MSP__
void main(void)
{
    setup((t_messlist **)&clickhold_class, (method)clickhold_new, (method)dsp_free, 
		  (short)sizeof(t_clickhold), 0, A_GIMME, 0);
    addmess((method)clickhold_dsp, "dsp", A_CANT, 0);
    addmess((method)clickhold_assist,"assist",A_CANT,0);
	//    addmess((method)clickhold_bang,"bang",A_CANT,0);
    dsp_initclass();
    post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif
#if __PD__



#define NO_FREE_FUNCTION 0
void clickhold_tilde_setup(void)
{
	clickhold_class = class_new(gensym("clickhold~"), (t_newmethod)clickhold_new, 
								 NO_FREE_FUNCTION,sizeof(t_clickhold), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(clickhold_class, t_clickhold, x_f);
	class_addmethod(clickhold_class, (t_method)clickhold_dsp, gensym("dsp"), 0);
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif


void clickhold_assist (t_clickhold *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:sprintf(dst,"(signal) Non-Zero Trigger Value");break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Sample and Hold Output");
	}
}

void *clickhold_new(t_symbol *s, int argc, t_atom *argv)
{
#if __MSP__
	t_clickhold *x = (t_clickhold *)newobject(clickhold_class);
	dsp_setup((t_pxobject *)x,1); 
	outlet_new((t_pxobject *)x, "signal");
#endif
#if __PD__
	t_clickhold *x = (t_clickhold *)pd_new(clickhold_class);
	outlet_new(&x->x_obj, gensym("signal"));
#endif
	x->hold_value = 0;
	return (x);
}

t_int *clickhold_perform(t_int *w)
{
	t_clickhold *x = (t_clickhold *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	t_float *out_vec = (t_float *)(w[3]);
	t_int n = w[4];

	float hold_value = x->hold_value;
	
	while( n-- ) {
		if(*in_vec){
			hold_value = *in_vec;
		}
		*in_vec++;
		*out_vec++ = hold_value;

	}
	x->hold_value = hold_value;
	return (w+5);
}		

void clickhold_dsp(t_clickhold *x, t_signal **sp, short *count)
{
    dsp_add(clickhold_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

