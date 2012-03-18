#include "MSPd.h"

#if __PD__
static t_class *granola_class;
#endif

#if __MSP__
void *granola_class;
#endif

#define OBJECT_NAME "granola~"

typedef struct _granola
{
#if __MSP__
  t_pxobject x_obj;
#endif
#if __PD__
  t_object x_obj;
  float x_f;
#endif
  float *gbuf;
  long grainsamps;
  long buflen ; // length of buffer
  float *grainenv; 
  long gpt1; // grain pointer 1
  long gpt2; // grain pointer 2
  long gpt3; // grain pointer 3
  float phs1; // phase 1
  float phs2; // phase 2
  float phs3; // phase 3
  float incr;
  long curdel;
  short mute_me;
  short iconnect;

} t_granola;



void *granola_new(t_floatarg val);
t_int *offset_perform(t_int *w);
t_int *granola_perform(t_int *w);
void granola_dsp(t_granola *x, t_signal **sp, short *count);
void granola_assist(t_granola *x, void *b, long m, long a, char *s);
void granola_mute(t_granola *x, t_floatarg toggle);
void granola_float(t_granola *x, double f ) ;
void granola_dsp_free(t_granola *x);

#if __MSP__
void main(void)
{
  setup((t_messlist **)&granola_class, (method)granola_new, (method)granola_dsp_free, (short)sizeof(t_granola), 0, A_FLOAT, 0);
  addmess((method)granola_dsp, "dsp", A_CANT, 0);
  addmess((method)granola_assist,"assist",A_CANT,0);
  addmess((method)granola_mute,"mute",A_FLOAT,0);
  addfloat((method)granola_float);
  dsp_initclass();
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

#if __PD__
void granola_tilde_setup(void){
  granola_class = class_new(gensym("granola~"), (t_newmethod)granola_new, 
			    (t_method)granola_dsp_free ,sizeof(t_granola), 0,A_FLOAT,0);
  CLASS_MAINSIGNALIN(granola_class, t_granola, x_f);
  class_addmethod(granola_class,(t_method)granola_dsp,gensym("dsp"),0);
  class_addmethod(granola_class,(t_method)granola_mute,gensym("mute"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif


void granola_float(t_granola *x, double f) {
  x->incr = f; 
} 

void granola_dsp_free(t_granola *x)
{
#if __MSP__
  dsp_free((t_pxobject *)x);
#endif
  free(x->gbuf);
  free(x->grainenv);
}


void granola_mute(t_granola *x, t_floatarg toggle)
{
  x->mute_me = (short)toggle;
}

void granola_assist (t_granola *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:sprintf(dst,"(signal) Input");break;
    case 1:sprintf(dst,"(signal/float) Increment");break;
    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output");
  }
}

void *granola_new(t_floatarg val)
{
  int i;
#if __MSP__
  t_granola *x = (t_granola *)newobject(granola_class);
  dsp_setup((t_pxobject *)x,2);
  outlet_new((t_pxobject *)x, "signal");
#endif
#if __PD__
  t_granola *x = (t_granola *)pd_new(granola_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

  // INITIALIZATIONS
  if( val > 0 ) {
    x->grainsamps = val;
//    post( "grainsize set to %.0f", val );
  } else {
    x->grainsamps = 2048;
    // post( "grainsize defaults to %d, val was %.0f", x->grainsamps, val );

  }
  x->buflen = x->grainsamps * 4;
  x->gbuf = (float *) calloc( x->buflen, sizeof(float) ) ;
  x->grainenv = (float *) calloc( x->grainsamps, sizeof(float) );
  for(i = 0; i < x->grainsamps; i++ ){
    x->grainenv[i] = .5 + (-.5 * cos( TWOPI * ((float)i/(float)x->grainsamps) ) );
  }
  x->gpt1 = 0;
  x->gpt2 = x->grainsamps / 3.;
  x->gpt3 = 2. * x->grainsamps / 3.;
  x->phs1 = 0;
  x->phs2 = x->grainsamps / 3. ;
  x->phs3 = 2. * x->grainsamps / 3. ;
  x->incr = .5 ;
  x->curdel = 0;
  x->mute_me = 0;

  return (x);
}

t_int *granola_perform(t_int *w)
{
	float  outsamp ;
	int iphs_a, iphs_b;
	float frac;

	
	/****/
	t_granola *x = (t_granola *) (w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *increment = (t_float *)(w[3]);
	t_float *out = (t_float *)(w[4]);
	int n = (int)(w[5]);
	int iconnect = x->iconnect;
	
	long gpt1 = x->gpt1;
	long gpt2 = x->gpt2;
	long gpt3 = x->gpt3;
	float phs1 = x->phs1;
	float phs2 = x->phs2;
	float phs3 = x->phs3;
	long curdel = x->curdel;
	long buflen = x->buflen;
	long grainsamps = x->grainsamps;
	float *grainenv = x->grainenv;
	float *gbuf = x->gbuf;
	float incr = x->incr;
	
	if( x->mute_me ) {
		while( n-- ){
			*out++ = 0.0;
		}
		return (w+6);
	} 
	
	while (n--) { 
#if __MSP__
		if(iconnect)
			x->incr = *increment++;
#endif
#if __PD__
		x->incr = *increment++;
#endif
		
		if( x->incr <= 0. ) {
			x->incr = .5 ;
		}
		
		if( curdel >= buflen ){
			curdel = 0 ;
		}    
		gbuf[ curdel ] = *in++;
    	
		// grain 1 
		iphs_a = floor( phs1 );
		iphs_b = iphs_a + 1;
		
		frac = phs1 - iphs_a;
		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp = (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt1++ ];

		if( gpt1 >= grainsamps ) {
			
			gpt1 = 0;
			phs1 = curdel;
		}
		phs1 += incr;
		while( phs1 >= buflen ) {
			phs1 -= buflen;
		}
		
		// now add second grain 
		
		
		iphs_a = floor( phs2 );
		
		iphs_b = iphs_a + 1;
		
		frac = phs2 - iphs_a;

		
		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp += (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt2++ ];
		if( gpt2 >= grainsamps ) {
			gpt2 = 0;
			phs2 = curdel ;
		}
		phs2 += incr ;    
		while( phs2 >= buflen ) {
			phs2 -= buflen ;
		}
		
		// now add third grain 
		
		iphs_a = floor( phs3 );
		iphs_b = iphs_a + 1;
		
		frac = phs3 - iphs_a;

		while( iphs_a >= buflen ) {
			iphs_a -= buflen;
		}
		while( iphs_b >= buflen ) {
			iphs_b -= buflen;
		}
		outsamp += (gbuf[ iphs_a ] + frac * ( gbuf[ iphs_b ] - gbuf[ iphs_a ])) * grainenv[ gpt3++ ];
		if( gpt3 >= grainsamps ) {
			gpt3 = 0;
			phs3 = curdel ;
		}
		phs3 += incr ;    
		while( phs3 >= buflen ) {
			phs3 -= buflen ;
		}
		
		
		++curdel;
		
		*out++ = outsamp; 
		/* output may well need to attenuated */
	}
	x->phs1 = phs1;
	x->phs2 = phs2;
	x->phs3 = phs3;
	x->gpt1 = gpt1;
	x->gpt2 = gpt2;
	x->gpt3 = gpt3;
	x->curdel = curdel;
	return (w+6);
	
}		

void granola_dsp(t_granola *x, t_signal **sp, short *count)
{
#if __MSP__
  x->iconnect = count[1];
#endif
  dsp_add(granola_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,  sp[0]->s_n);
}

