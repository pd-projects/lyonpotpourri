#include "MSPd.h"

#define OBJECT_NAME "impulse~"

#if __MSP__
void *impulse_class;
#endif 

#if __PD__
static t_class *impulse_class;
#endif

typedef struct _impulse
{
#if __MSP__
  t_pxobject x_obj;
#endif
#if __PD__
  t_object x_obj;
  float x_f;
#endif

	int activated;

} t_impulse;

void *impulse_new(t_symbol *s, int argc, t_atom *argv);

t_int *impulse_perform(t_int *w);
void impulse_dsp(t_impulse *x, t_signal **sp, short *count);
void impulse_assist(t_impulse *x, void *b, long m, long a, char *s);
void impulse_bang(t_impulse *x);

void impulse_bang(t_impulse *x) {
	x->activated = 1 ;
}

#if __MSP__
void main(void)
{
    setup((t_messlist **)&impulse_class, (method)impulse_new, (method)dsp_free, 
	(short)sizeof(t_impulse), 0, A_GIMME, 0);
    addmess((method)impulse_dsp, "dsp", A_CANT, 0);
    addmess((method)impulse_assist,"assist",A_CANT,0);
    addmess((method)impulse_bang,"bang",A_CANT,0);
    dsp_initclass();
    post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif
#if __PD__

#define NO_FREE_FUNCTION 0
void impulse_tilde_setup(void)
{
  impulse_class = class_new(gensym("impulse~"), (t_newmethod)impulse_new, 
			 NO_FREE_FUNCTION,sizeof(t_impulse), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(impulse_class, t_impulse, x_f);
  class_addmethod(impulse_class, (t_method)impulse_dsp, gensym("dsp"), 0);
  class_addmethod(impulse_class, (t_method)impulse_bang, gensym("bang"),  0);
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

void impulse_assist (t_impulse *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:sprintf(dst,"(bang) Trigger Impulse");break;
    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output");
  }
}

void *impulse_new(t_symbol *s, int argc, t_atom *argv)
{
#if __MSP__
  t_impulse *x = (t_impulse *)newobject(impulse_class);
  dsp_setup((t_pxobject *)x,0); // No Signal Inlets
  outlet_new((t_pxobject *)x, "signal");
#endif
#if __PD__
  t_impulse *x = (t_impulse *)pd_new(impulse_class);
  outlet_new(&x->x_obj, gensym("signal"));
#endif
  x->activated = 0;

  return (x);
}

t_int *impulse_perform(t_int *w)
{
  t_impulse *x = (t_impulse *) (w[1]);
  t_float *out = (t_float *)(w[2]);
  t_int n = w[3];
  int activated = x->activated;
	while( n-- ) {
		if( activated ) {
			*out++ = 1.0;
			activated = 0;
		}
		else *out++ = 0.0;
	}
	x->activated = activated;
  return (w+4);
}		

void impulse_dsp(t_impulse *x, t_signal **sp, short *count)
{
    dsp_add(impulse_perform, 3, x, sp[0]->s_vec,sp[0]->s_n);
}

