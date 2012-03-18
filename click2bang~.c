#include "MSPd.h"

#define OBJECT_NAME "click2bang~"

#if __MSP__
void *click2bang_class;
#endif 

#if __PD__
static t_class *click2bang_class;
#endif

typedef struct _click2bang
{
#if __MSP__
	t_pxobject x_obj;
#endif
#if __PD__
	t_object x_obj;
	float x_f;
#endif
	void *bang;
	void *clock;
	//	int activated;
	
} t_click2bang;

void *click2bang_new(t_symbol *s, int argc, t_atom *argv);

t_int *click2bang_perform(t_int *w);
void click2bang_dsp(t_click2bang *x, t_signal **sp, short *count);
void click2bang_assist(t_click2bang *x, void *b, long m, long a, char *s);
void click2bang_tick(t_click2bang *x) ;

/* void click2bang_bang(t_click2bang *x);

void click2bang_bang(t_click2bang *x) {
	x->activated = 1 ;
}*/

#if __MSP__
void main(void)
{
    setup((t_messlist **)&click2bang_class, (method)click2bang_new, (method)dsp_free, 
		  (short)sizeof(t_click2bang), 0, A_GIMME, 0);
    addmess((method)click2bang_dsp, "dsp", A_CANT, 0);
    addmess((method)click2bang_assist,"assist",A_CANT,0);
	//    addmess((method)click2bang_bang,"bang",A_CANT,0);
    dsp_initclass();
    post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif
#if __PD__



#define NO_FREE_FUNCTION 0
void click2bang_tilde_setup(void)
{
	click2bang_class = class_new(gensym("click2bang~"), (t_newmethod)click2bang_new, 
								 NO_FREE_FUNCTION,sizeof(t_click2bang), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(click2bang_class, t_click2bang, x_f);
	class_addmethod(click2bang_class, (t_method)click2bang_dsp, gensym("dsp"), 0);
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

void click2bang_tick(t_click2bang *x) 
{
  outlet_bang(x->bang);
}

void click2bang_assist (t_click2bang *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:sprintf(dst,"(bang) Trigger click2bang");break;
		}
	} else if (msg==2) {
		sprintf(dst,"(bang) Output");
	}
}

void *click2bang_new(t_symbol *s, int argc, t_atom *argv)
{
#if __MSP__
	t_click2bang *x = (t_click2bang *)newobject(click2bang_class);
	dsp_setup((t_pxobject *)x,1); // No Signal Inlets
								  // need a bang here
								  //  outlet_new((t_pxobject *)x, "signal");
#endif
#if __PD__
	t_click2bang *x = (t_click2bang *)pd_new(click2bang_class);
	x->bang = outlet_new(&x->x_obj, gensym("bang"));
	x->clock = clock_new(x,(void *)click2bang_tick);
#endif
	
	return (x);
}

t_int *click2bang_perform(t_int *w)
{
	t_click2bang *x = (t_click2bang *) (w[1]);
	t_float *in_vec = (t_float *)(w[2]);
	t_int n = w[3];

	while( n-- ) {
		if(*in_vec++)
			clock_delay(x->clock, 0);
	}
	return (w+4);
}		

void click2bang_dsp(t_click2bang *x, t_signal **sp, short *count)
{
    dsp_add(click2bang_perform, 3, x, sp[0]->s_vec,sp[0]->s_n);
}

