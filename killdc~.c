
#include "MSPd.h"

/*
IMPLEMENT ELLIPTICAL FILTERS FROM CMIX
*/


#define OBJECT_NAME "killdc~"
#if __MSP__
void *killdc_class;
#endif
#if __PD__
static t_class *killdc_class;
#endif

#define FUNC_LEN (512)

#define MAXSECTS 20

typedef struct {
  float ps[4][MAXSECTS];
  float c[4][MAXSECTS];
  int nsects ;
  float xnorm;
} COEFS ;

typedef struct _killdc
{
#if __MSP__
    t_pxobject x_obj;
#endif
#if __PD__
  t_object x_obj;
  float x_f;
#endif

    COEFS fdata;
} t_killdc;

void *killdc_new(t_symbol *s);
t_int *offset_perform(t_int *w);
t_int *killdc_perform(t_int *w);
void killdc_dsp(t_killdc *x, t_signal **sp, short *count);
void killdc_assist(t_killdc *x, void *b, long m, long a, char *s);

#define DEFAULT_DSP_FREE 0
#if __MSP__
void main(void)
{

    setup((t_messlist **)&killdc_class, (method)killdc_new, (method)dsp_free, (short)sizeof(t_killdc), 0L, A_DEFSYM, 0);
    addmess((method)killdc_dsp, "dsp", A_CANT, 0);
    addmess((method)killdc_assist,"assist",A_CANT,0);
  	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
    dsp_initclass();
    
}
#endif
#if __PD__
void killdc_tilde_setup(void)
{
killdc_class = class_new(gensym("killdc~"),(t_newmethod)killdc_new,DEFAULT_DSP_FREE, 
sizeof(t_killdc), 0, A_DEFSYMBOL,0);
CLASS_MAINSIGNALIN(killdc_class,t_killdc, x_f );


class_addmethod(killdc_class,(t_method)killdc_dsp,gensym("dsp"),A_CANT,0);
class_addmethod(killdc_class,(t_method)killdc_assist,gensym("assist"),A_CANT,0);
  	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
    
}
#endif

void killdc_assist (t_killdc *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input");
				break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void *killdc_new(t_symbol *s)
{
 	int i, j;
	   


#if __MSP__
	t_killdc *x = (t_killdc *)newobject(killdc_class);
    dsp_setup((t_pxobject *)x,1);
    outlet_new((t_pxobject *)x, "signal");
 #endif  
 
#if __PD__
   t_killdc *x = (t_killdc *)pd_new(killdc_class);
  outlet_new(&x->x_obj, gensym("signal"));
 #endif
 
/**************/


	x->fdata.nsects = 3;
	x->fdata.c[0][0] = -1.9999995 ;
	x->fdata.c[1][0] = -1.9997407 ;
	x->fdata.c[2][0] =  1.0000000  ;
	x->fdata.c[3][0] =  0.99974253 ;
	x->fdata.c[0][1] = -1.9999997  ;
	x->fdata.c[1][1] = -1.9988353 ;
	x->fdata.c[2][1] =  1.0000000  ;
	x->fdata.c[3][1] =  0.99883796 ;
	x->fdata.c[0][2] = -2.0000000   ;
	x->fdata.c[1][2] = -1.9959218 ;
	x->fdata.c[2][2] =  1.0000000  ;
	x->fdata.c[3][2] =  0.99592773 ;
	
    for(i=0; i< x->fdata.nsects; i++) {
    	for(j=0;j<4;j++)  {    
      		x->fdata.ps[j][i] = 0.0; 
    	}
  	}

	x->fdata.xnorm = 0.99725327e+00 ;




// INITIALIZATIONS


    return (x);
}
// method from Paul Lansky's cmix implementation
t_int *killdc_perform(t_int *w)
{
	t_float *in1,*out;
	float sample ;
	int n;
	int m;
	float op;
//	int lcount = 0;
/********/
	t_killdc *x = (t_killdc *) (w[1]);
	in1 = (t_float *)(w[2]);
	out = (t_float *)(w[3]);
	n = (int)(w[4]);
	
	while (n--) { 
		sample = *in1++;
    	for(m=0; m< x->fdata.nsects;m++) {
   			 op = sample + x->fdata.c[0][m] * x->fdata.ps[0][m] + 
     		 x->fdata.c[2][m] * x->fdata.ps[1][m]
    		 - x->fdata.c[1][m] * x->fdata.ps[2][m] 
      		 - x->fdata.c[3][m] * x->fdata.ps[3][m];

    		x->fdata.ps[1][m] = x->fdata.ps[0][m];
   			x->fdata.ps[0][m] = sample;
    		x->fdata.ps[3][m] = x->fdata.ps[2][m];
    		x->fdata.ps[2][m] = op;
    		sample = op;
  		}
 		*out++ = sample * x->fdata.xnorm ; 
	}
	return (w+5);
}		

void killdc_dsp(t_killdc *x, t_signal **sp, short *count)
{
//	long i;

		dsp_add(killdc_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec,  sp[0]->s_n);
}

