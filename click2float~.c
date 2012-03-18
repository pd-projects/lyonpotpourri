#include "MSPd.h"

/**/

#if __MSP__
void *click2float_class;
#endif 

#if __PD__
static t_class *click2float_class;
#endif

typedef struct _click2float
{
#if __MSP__
	t_pxobject x_obj;
#endif
#if __PD__
	t_object x_obj;
	float x_f;
#endif
	void *float_outlet;
	void *clock;
	double float_value;
	//	int activated;
	
} t_click2float;

#define OBJECT_NAME "click2float~"

void *click2float_new(t_symbol *s, int argc, t_atom *argv);

t_int *click2float_perform(t_int *w);
void click2float_dsp(t_click2float *x, t_signal **sp, short *count);
void click2float_assist(t_click2float *x, void *b, long m, long a, char *s);
void click2float_tick(t_click2float *x) ;


#if __MSP__
void main(void)
{
    setup((t_messlist **)&click2float_class, (method)click2float_new, (method)dsp_free, 
		  (short)sizeof(t_click2float), 0, A_GIMME, 0);
    addmess((method)click2float_dsp, "dsp", A_CANT, 0);
    addmess((method)click2float_assist,"assist",A_CANT,0);
    dsp_initclass();
    post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

#if __PD__

#define NO_FREE_FUNCTION 0
void click2float_tilde_setup(void)
{
	click2float_class = class_new(gensym("click2float~"), (t_newmethod)click2float_new, 
								 NO_FREE_FUNCTION,sizeof(t_click2float), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(click2float_class, t_click2float, x_f);
	class_addmethod(click2float_class, (t_method)click2float_dsp, gensym("dsp"), 0);
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
	//  class_addmethod(click2float_class, (t_method)click2float_bang, gensym("bang"),  0);
}
#endif

void click2float_tick(t_click2float *x) 
{
  outlet_float(x->float_outlet,x->float_value);
}

void click2float_assist (t_click2float *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:sprintf(dst,"(signal) Click Trigger");break;
		}
	} else if (msg==2) {
		sprintf(dst,"(float) Click Value");
	}
}

void *click2float_new(t_symbol *s, int argc, t_atom *argv)
{
#if __MSP__
	t_click2float *x = (t_click2float *)newobject(click2float_class);
	x->float_outlet = floatout((t_pxobject *)x);
	dsp_setup((t_pxobject *)x,1); // No Signal outlets
#endif
#if __PD__
	t_click2float *x = (t_click2float *)pd_new(click2float_class);
	x->float_outlet = outlet_new(&x->x_obj, gensym("float"));
	
#endif

	x->clock = clock_new(x,(void *)click2float_tick);
	return (x);
}

t_int *click2float_perform(t_int *w)
{
	t_click2float *x = (t_click2float *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	t_int n = w[3];

	while( n-- ) {
		if(*in_vec){
			x->float_value = *in_vec;
			clock_delay(x->clock, 0);
		}
		*in_vec++;
	}
	return (w+4);
}		

void click2float_dsp(t_click2float *x, t_signal **sp, short *count)
{
    dsp_add(click2float_perform, 3, x, sp[0]->s_vec,sp[0]->s_n);
}

