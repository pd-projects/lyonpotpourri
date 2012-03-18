#include "MSPd.h"

#if __PD__
static t_class *chopper_class;
#endif

#if __MSP__

void *chopper_class;

#endif

#define MAXSTORE 1024
#define OBJECT_NAME "chopper~"
	
typedef struct _chopper
{
#if __MSP__
  t_pxobject x_obj;
  t_buffer *l_buf;
#endif
#if __PD__
  t_object x_obj;
  float x_f;
#endif 
  t_symbol *l_sym;
  long l_chan;
  float increment;
  double fbindex;
  float buffer_duration;
  float minseg;
  float maxseg;
  float segdur;
  float minincr;
  float maxincr;
  int loop_samps;
  int samps_to_go ;
  int loop_start;
  int bindex ;
  int taper_samps;
  int loop_min_samps;
  int loop_max_samps;
  float R;
  float ldev;
  float st_dev ;
  int lock_loop;
  int force_new_loop;
  int framesize;
  short mute;
  short disabled;
  int setup_chans;
  int *stored_starts;
  int *stored_samps;   
  float *stored_increments;
  short preempt;
  short loop_engaged;
  short data_recalled;
  short initialize_loop;
  short fixed_increment_on;
  float fixed_increment;
  float retro_odds;
  float fade_level;
  int transp_loop_samps;
  float taper_duration;
  short lock_terminated;
  int preempt_samps;
  int preempt_count;
  short recalling_loop;
  float jitter_factor;
  float rdur_factor;
  float rinc_factor;
  short increment_adjusts_loop ;
  short loop_adjust_inverse;
  long b_frames;
  long b_nchans;
  float *b_samples;
} t_chopper;


t_int *chopper_perform_stereo(t_int *w);
t_int *choppermono_perform(t_int *w);
t_int *chopper_perform_stereo_nointerpol(t_int *w);
t_int *chopper_perform_mono(t_int *w);
t_int *chopper_pd_perform(t_int *w);
void chopper_dsp(t_chopper *x, t_signal **sp);
void chopper_set(t_chopper *x, t_symbol *s);
void chopper_mute(t_chopper *x, t_floatarg toggle);
void chopper_increment_adjust(t_chopper *x, t_floatarg toggle);
void chopper_adjust_inverse(t_chopper *x, t_floatarg toggle);
void *chopper_new(t_symbol *msg, short argc, t_atom *argv);
void chopper_in1(t_chopper *x, long n);
void chopper_set_minincr(t_chopper *x, t_floatarg n);
void chopper_set_maxincr(t_chopper *x, t_floatarg n);
void chopper_set_minseg(t_chopper *x, t_floatarg n);
void chopper_set_maxseg(t_chopper *x, t_floatarg n);
void chopper_taper(t_chopper *x, t_floatarg f);
void chopper_fixed_increment(t_chopper *x, t_floatarg f);
void chopper_lockme(t_chopper *x, t_floatarg n);
void chopper_force_new(t_chopper *x);
float chopper_boundrand(float min, float max);
void chopper_assist(t_chopper *x, void *b, long m, long a, char *s);
void chopper_dblclick(t_chopper *x);
void chopper_show_loop(t_chopper *x);
void chopper_set_loop(t_chopper *x, t_symbol *msg, short argc, t_atom *argv);
void chopper_randloop( t_chopper *x);
void chopper_store_loop(t_chopper *x, t_floatarg loop_bindex);
void chopper_recall_loop(t_chopper *x,  t_floatarg loop_bindex);
void chopper_free(t_chopper *x) ;
void chopper_retro_odds(t_chopper *x, t_floatarg f);
void chopper_jitter(t_chopper *x, t_floatarg f);
void chopper_jitterme(t_chopper *x);
void chopper_rdur(t_chopper *x, t_floatarg f);
void chopper_rdurme(t_chopper *x);
void chopper_rinc(t_chopper *x, t_floatarg f);
void chopper_rincme(t_chopper *x);
void chopper_adjust_inverse(t_chopper *x, t_floatarg toggle);
t_int *chopper_performtest(t_int *w);
void chopper_init(t_chopper *x, short initialized);
void chopper_seed(t_chopper *x, t_floatarg seed);
void chopper_testrand(t_chopper *x);

t_symbol *ps_buffer;

#if __MSP__
void main(void)
{
  setup((t_messlist **)&chopper_class, (method)chopper_new, (method)chopper_free, (short)sizeof(t_chopper), 0, 
	A_GIMME, 0);
  addmess((method)chopper_dsp, "dsp", A_CANT, 0);
  addftx((method)chopper_set_minincr, 1);
  addftx((method)chopper_set_maxincr, 2);
  addftx((method)chopper_set_minseg, 3);
  addftx((method)chopper_set_maxseg, 4);
  addftx((method)chopper_lockme, 5);
  addbang((method)chopper_force_new);
  addmess((method)chopper_assist, "assist", A_CANT, 0);
  addmess((method)chopper_dblclick, "dblclick", A_CANT, 0);
  addmess((method)chopper_taper, "taper", A_FLOAT, 0);
  addmess((method)chopper_fixed_increment, "fixed_increment", A_FLOAT, 0);
  addmess((method)chopper_retro_odds, "retro_odds", A_FLOAT, 0);
  addmess((method)chopper_show_loop, "show_loop", 0);
  addmess((method)chopper_set_loop, "set_loop", A_GIMME, 0);
  addmess((method)chopper_store_loop, "store_loop", A_FLOAT, 0);
  addmess((method)chopper_recall_loop, "recall_loop", A_FLOAT, 0);
  addmess((method)chopper_increment_adjust, "increment_adjust", A_FLOAT, 0);
  addmess((method)chopper_adjust_inverse, "adjust_inverse", A_FLOAT, 0);
  addmess((method)chopper_jitter, "jitter", A_FLOAT, 0);
  addmess((method)chopper_rdur, "rdur", A_FLOAT, 0);
  addmess((method)chopper_rinc, "rinc", A_FLOAT, 0);
  addmess((method)chopper_mute, "mute", A_FLOAT, 0);
  dsp_initclass();
  ps_buffer = gensym("buffer~");
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}

void chopper_dblclick(t_chopper *x)
{
#if __MSP__
  t_buffer *b;
  t_symbol *wavename = x->l_sym;
  if ((b = (t_buffer *)(wavename->s_thing)) && ob_sym(b) == gensym("buffer~"))
    mess0((t_object *)b,gensym("dblclick"));
#endif
}

void chopper_assist(t_chopper *x, void *b, long msg, long arg, char *dst)
{
  if (msg==1) {
    switch (arg) {
    case 0:
      sprintf(dst,"(bang) Force New Loop ");
      break;
    case 1:
      sprintf(dst,"(float) Minimum Increment ");
      break;
    case 2:
      sprintf(dst,"(float) Maximum Increment ");
      break;
    case 3:
      sprintf(dst,"(float) Minimum Segdur ");
      break;
    case 4:
      sprintf(dst,"(float) Maximum Segdur ");
      break;
    case 5:
      sprintf(dst,"(int) Non-Zero Locks Loop ");
      break;

    }
  } else if (msg==2) {
    sprintf(dst,"(signal) Output");
  }

}
#endif

void chopper_testrand(t_chopper *x)
{
	float rval = chopper_boundrand(0.0, 1.0);
	post("random btwn 0.0 1.0: %f",rval);
}

void chopper_mute(t_chopper *x, t_floatarg toggle)
{
	x->mute = (short) toggle;
}

void chopper_seed(t_chopper *x, t_floatarg seed)
{
#if __PD__
	srandom((long)seed);
#endif
#if __MSP__
	srand((long)seed);
#endif
}

#if __PD__
void chopper_tilde_setup(void){
  chopper_class = class_new(gensym("chopper~"), (t_newmethod)chopper_new, 
      (t_method)chopper_free ,sizeof(t_chopper), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(chopper_class, t_chopper, x_f);
  class_addmethod(chopper_class,(t_method)chopper_dsp,gensym("dsp"),0);
  class_addmethod(chopper_class,(t_method)chopper_mute,gensym("mute"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_taper,gensym("taper"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_fixed_increment,gensym("fixed_increment"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_retro_odds,gensym("retro_odds"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_show_loop,gensym("show_loop"),0);
  class_addmethod(chopper_class,(t_method)chopper_set_loop,gensym("set_loop"),A_GIMME,0);
  class_addmethod(chopper_class,(t_method)chopper_store_loop,gensym("store_loop"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_recall_loop,gensym("recall_loop"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_increment_adjust,gensym("increment_adjust"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_adjust_inverse,gensym("adjust_inverse"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_jitter,gensym("jitter"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_rdur,gensym("rdur"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_rinc,gensym("rinc"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_set_minincr,gensym("set_minincr"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_set_maxincr,gensym("set_maxincr"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_set_minseg,gensym("set_minseg"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_set_maxseg,gensym("set_maxseg"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_lockme,gensym("lockme"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_force_new,gensym("force_new"),0);
  class_addmethod(chopper_class,(t_method)chopper_seed,gensym("seed"),A_FLOAT,0);
  class_addmethod(chopper_class,(t_method)chopper_testrand,gensym("testrand"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

void chopper_increment_adjust(t_chopper *x, t_floatarg toggle)
{
  x->increment_adjusts_loop = (short)toggle;
  
}

void chopper_adjust_inverse(t_chopper *x, t_floatarg toggle)
{
  x->loop_adjust_inverse = (short)toggle;
}

void chopper_fixed_increment(t_chopper *x, t_floatarg f)
{
  float new_samps = 0;
  float rectf;

  x->fixed_increment = f;
  if( f ){
    x->fixed_increment_on = 1;
  } else {
    x->fixed_increment_on = 0;
  }
  
  rectf = fabs(f);
  
  if( x->lock_loop && rectf > 0.0 ){

    if( x->loop_adjust_inverse ){
      new_samps = (float) x->loop_samps * rectf ;
    } else {
      new_samps = (float) x->loop_samps / rectf ;
    }
    if( f > 0 ){
      if( x->loop_start + new_samps >= x->framesize ){
		return;
      } else {
		x->increment = x->fixed_increment;
      }
    } else {
      if( x->loop_start - new_samps < 0) {
		return;
      } else {
		x->increment = x->fixed_increment;
      }
    }
		
  }
  
  if( x->increment_adjusts_loop ){
    x->transp_loop_samps = new_samps;
  }
  

}

void chopper_jitter(t_chopper *x, t_floatarg f)
{
  f *= 0.1; // scale down a bit
  if( f >= 0. && f <= 1.0 )
    x->jitter_factor = f;
}

void chopper_rdur(t_chopper *x, t_floatarg f)
{

  if( f >= 0. && f <= 1.0 )
    x->rdur_factor = f;
}

void chopper_rinc(t_chopper *x, t_floatarg f)
{
  // f *= 0.1; // scale down a bit

  if( f >= 0. && f <= 1.0 )
    x->rinc_factor = f;
}


void chopper_retro_odds(t_chopper *x, t_floatarg f)
{

  if( f < 0 )
    f = 0;
  if( f > 1 )
    f = 1;
		
  x->retro_odds = f;

}

void chopper_show_loop(t_chopper *x)
{
  post("start: %d, samps: %d, increment: %f", x->loop_start, x->transp_loop_samps, x->increment);
  post("minloop %f, maxloop %f", x->minseg, x->maxseg);
}

void chopper_store_loop(t_chopper *x, t_floatarg f)
{
  int loop_bindex = (int) f;

  if( loop_bindex < 0 || loop_bindex >= MAXSTORE ){
    error("bindex %d out of range", loop_bindex);
    return;
  }
	
  x->stored_starts[ loop_bindex ] = x->loop_start;
  x->stored_samps[ loop_bindex ] = x->transp_loop_samps;
  x->stored_increments[ loop_bindex ] = x->increment;

  post("storing loop %d: %d %d %f",loop_bindex, 
       x->stored_starts[ loop_bindex ],x->stored_samps[ loop_bindex ],  x->stored_increments[ loop_bindex ] );

  // post("loop stored at position %d", loop_bindex );
}

void chopper_recall_loop(t_chopper *x, t_floatarg f)
{
  // bug warning: recall preceding store will crash program
  // need to add warning
int loop_bindex = (int) f;

  if( loop_bindex < 0 || loop_bindex >= MAXSTORE ){
    error("bindex %d out of range", loop_bindex);
    return;
  }
	
  if( ! x->stored_samps[ loop_bindex ] ){
    error("no loop stored at position %d!", loop_bindex);
    return;
  }

	
  x->loop_start = x->stored_starts[ loop_bindex ];
  x->samps_to_go = x->transp_loop_samps = x->stored_samps[ loop_bindex ];
	 
  if( x->loop_min_samps > x->transp_loop_samps )
    x->loop_min_samps = x->transp_loop_samps ;
  if( x->loop_max_samps < x->transp_loop_samps )
    x->loop_max_samps = x->transp_loop_samps ;
  x->increment = x->stored_increments[ loop_bindex ];
  /*
    post("restoring loop %d: %d %d %f",loop_bindex, 
    x->loop_start,x->transp_loop_samps,  x->increment );
    post("min %d max %d",x->loop_min_samps, x->loop_max_samps );
  */
  x->preempt_count = x->preempt_samps;
  // post("preempt samps:%d", x->preempt_count);
  x->recalling_loop = 1;
  //  x->data_recalled = 1;
}

void chopper_set_loop(t_chopper *x, t_symbol *msg, short argc, t_atom *argv)
{
  if( argc < 3 ){
    error("format: start samples increment");
    return;
  }
  x->loop_start = atom_getintarg(0,argc,argv);
  x->loop_samps = atom_getintarg(1,argc,argv);
  x->increment = atom_getfloatarg(2,argc,argv);
  x->data_recalled = 1;

  x->samps_to_go = x->loop_samps;
  x->fbindex = x->bindex = x->loop_start;
//  post("loop set to: st %d samps %d incr %f", x->loop_start, x->loop_samps,x->increment);
}

void chopper_taper(t_chopper *x, t_floatarg f)
{
  f /= 1000.0;
	
  if( f > 0 ){
    x->taper_samps = (float) x->R * f ;
  }
  if( x->taper_samps < 2 )
    x->taper_samps = 2;
 // post("taper samps set to %d",x->taper_samps);
}

#if __MSP__
void *chopper_new(t_symbol *msg, short argc, t_atom *argv)
{
  t_chopper *x = (t_chopper *)newobject(chopper_class);
//  t_buffer *b = x->l_buf;

  x->setup_chans = 2;

  x->l_sym = atom_getsymarg(0,argc,argv);
  if( argc > 1 )
    x->setup_chans = atom_getintarg(1,argc,argv);
  if( argc > 2)
    x->taper_duration = atom_getfloatarg(2,argc,argv);	
  if( x->setup_chans != 1 && x->setup_chans != 2){
    error("chopper~ only supports mono or stereo");
  }		
  dsp_setup((t_pxobject *)x,0);
  floatin((t_object *)x,5);
  floatin((t_object *)x,4);
  floatin((t_object *)x,3);
  floatin((t_object *)x,2);
  floatin((t_object *)x,1);
  outlet_new((t_object *)x, "signal");
  if(x->setup_chans == 2)
    outlet_new((t_object *)x, "signal");	
  x->R = (float) sys_getsr();
  chopper_init(x,0);
  return (x);
}
#endif

#if __PD__
void *chopper_new(t_symbol *msg, short argc, t_atom *argv)
{
  t_chopper *x = (t_chopper *)pd_new(chopper_class);
  outlet_new(&x->x_obj, gensym("signal"));
  x->R = sys_getsr();
  x->l_sym = atom_getsymbolarg(0,argc,argv);
  chopper_init(x,0);
  return (x);
}
#endif

void chopper_init(t_chopper *x, short initialized) 
{
	if(!initialized){
	  srand(time(0)); 
#if __PD__
		srandom(time(0)); // codewarrior lacks random()/srandom(), only supplies dirtbag rand()
#endif 	
	  if(!x->R) {
		error("zero sampling rate - set to 44100");
		x->R = 44100;
	  }
	  x->minseg = 0.1;
	  x->maxseg = 0.8 ;
	  x->minincr = 0.5 ;
	  x->maxincr = 2.0 ;
	  x->data_recalled = 0;		
	  x->segdur = 0;
	  x->bindex = 0 ;
	  x->taper_duration /= 1000.0;
	  if( x->taper_duration < .0001 || x->taper_duration > 10.0 )
		x->taper_duration = .0001;
		x->increment_adjusts_loop = 0;
	  x->taper_samps = x->R * x->taper_duration;
	  if(x->taper_samps < 2)
		x->taper_samps = 2;
		
	  x->preempt_samps = 5;
	  x->loop_adjust_inverse = 0;
	  x->preempt = 1;
	  x->recalling_loop = 0;	
	  x->ldev = 0;
	  x->lock_loop = 0;
	  x->buffer_duration = 0.0 ;
	  x->st_dev = 0.0;
	  x->framesize = 0;
	  x->force_new_loop = 0;
	  x->mute = 0;
	  x->disabled = 1;
	  x->initialize_loop = 1;
	  x->loop_engaged = 0;
	  x->fixed_increment_on = 0;
	  x->retro_odds = 0.5;
	  x->fade_level = 1.0;
	  x->lock_terminated = 0;
		
	  x->stored_starts = calloc(MAXSTORE, sizeof(int));
	  x->stored_samps = calloc(MAXSTORE, sizeof(int));
	  x->stored_increments = calloc(MAXSTORE, sizeof(int));
		
	} else {
		x->taper_samps = x->R * x->taper_duration;
		if(x->taper_samps < 2)
		  x->taper_samps = 2;
	}
}

void chopper_free(t_chopper *x) 
{
#if __MSP__
  dsp_free((t_pxobject *) x);
#endif
  free(x->stored_increments);
  free(x->stored_samps);
  free(x->stored_starts);
}

void chopper_jitterme(t_chopper *x)
{
  float new_start;
  float jitter_factor = x->jitter_factor;
  new_start = (1.0 + chopper_boundrand(-jitter_factor, jitter_factor) ) * (float) x->loop_start ;
	
  if( new_start < 0 ){
//    error("jitter loop %d out of range", new_start);
    new_start = 0;

  }
  else if( new_start + x->transp_loop_samps >= x->framesize ){
//    error("jitter loop %d out of range", new_start);
    new_start = x->framesize - x->transp_loop_samps ;
  }
  if( new_start >= 0 )
    x->loop_start = new_start;
}

void chopper_rdurme(t_chopper *x)
{
  float new_dur;
  float rdur_factor = x->rdur_factor;
	
  new_dur = (1.0 + chopper_boundrand( -rdur_factor, rdur_factor)) * (float) x->transp_loop_samps;
  if( new_dur > x->loop_max_samps )
    new_dur = x->loop_max_samps;
  if( new_dur < x->loop_min_samps )
    new_dur = x->loop_min_samps;

  x->transp_loop_samps = new_dur;
}

void chopper_rincme(t_chopper *x )
{
  float new_inc = 0;
//  int count = 0;
  int new_samps;
  float rinc_factor = x->rinc_factor;
	
  /* test generate a new increment */
  new_inc = (1.0 + chopper_boundrand( 0.0, rinc_factor)) ;
  if( chopper_boundrand(0.0,1.0) > 0.5 ){
    new_inc = 1.0 / new_inc;
  }
	
  // test for transgression
	
//	post("increment adjust:%d",x->increment_adjusts_loop);
	
  if( fabs(new_inc * x->increment) < x->minincr ) {
    new_inc = x->minincr / fabs(x->increment) ; // now when we multiply - increment is set to minincr
  }
  else if ( fabs(new_inc * x->increment) > x->maxincr ){
    new_inc = x->maxincr / fabs(x->increment) ; // now when we multiply - increment is set to maxincr
  }

 if(x->increment_adjusts_loop){
 	 new_samps = (float) x->transp_loop_samps / new_inc ; 
  } else {
  	new_samps = x->transp_loop_samps;
  }
    
  new_inc *= x->increment ;
  if( x->increment > 0 ){
    if( x->loop_start + new_samps >= x->framesize ){
      new_samps = (x->framesize - 1) - x->loop_start ;
    }
  } else {
    if( x->loop_start - new_samps < 0) {
      new_samps = x->loop_start + 1;
    }
  }
  x->transp_loop_samps = new_samps;
  x->increment = new_inc;		
}

void chopper_randloop( t_chopper *x )
{
  int framesize = x->b_frames;//test
//  long bindex = x->fbindex;
  float segdur = x->segdur;
  int loop_start = x->loop_start;
  int loop_samps = x->loop_samps;
  int transp_loop_samps = x->transp_loop_samps;
  int samps_to_go = x->samps_to_go;
  float increment = x->increment;
//  int taper_samps = x->taper_samps ;
//  float taper_duration = x->taper_duration;
  float minincr = x->minincr;
  float maxincr = x->maxincr;
  float minseg = x->minseg;
  float maxseg = x->maxseg;
  float buffer_duration = x->buffer_duration;
  float R = x->R;
  float fixed_increment = x->fixed_increment;

  short fixed_increment_on = x->fixed_increment_on;
  float retro_odds = x->retro_odds;
	
  if(fixed_increment_on){
    increment = fixed_increment;
  } else {
    increment = chopper_boundrand(minincr,maxincr);
  }
  segdur = chopper_boundrand( minseg, maxseg );
  loop_samps = segdur * R * increment; // total samples in segment
//  post("rand: segdur %f R %f increment %f lsamps %d",segdur,R,increment,loop_samps);
  transp_loop_samps = segdur * R ; // actual count of samples to play back
  samps_to_go = transp_loop_samps;  
  if( loop_samps >= framesize ){
    loop_samps = framesize - 1;
    loop_start = 0;
  } else {
//    post("rand: bufdur %f segdur %f",buffer_duration, segdur);
    loop_start = R * chopper_boundrand( 0.0, buffer_duration - segdur );
    if( loop_start + loop_samps >= framesize ){
      loop_start = framesize - loop_samps;
      if( loop_start < 0 ){
				loop_start = 0;
				error("negative starttime");
      }
    }
  }
  if( chopper_boundrand(0.0,1.0) < retro_odds ){
    increment *= -1.0 ;
    loop_start += (loop_samps - 1);
  }

//	post("randset: lstart %d lsamps %d incr %f segdur %f",loop_start,loop_samps,increment,segdur);
  x->samps_to_go = samps_to_go;
  x->fbindex = x->bindex = loop_start;
  x->loop_start = loop_start;
  x->loop_samps = loop_samps;
  x->transp_loop_samps = transp_loop_samps;
  x->increment = increment;
  x->segdur = segdur;
}

t_int *chopper_pd_perform(t_int *w)
{
  int bindex, bindex2;
  float sample1, m1, m2;	
  t_chopper *x = (t_chopper *)(w[1]);
  t_float *out1 = (t_float *)(w[2]);
  t_int n = w[3];
  

  /*********************************************/
  float *tab = x->b_samples;
//  long chan = 1;
  long b_frames = x->b_frames;
  long nc = x->b_nchans;

  float segdur = x->segdur;
  int taper_samps = x->taper_samps ;
  float taper_duration = x->taper_duration;
//  float minincr = x->minincr;
//  float maxincr = x->maxincr;
  float minseg = x->minseg;
  float maxseg = x->maxseg;
//  float ldev = x->ldev;
  int lock_loop = x->lock_loop;
//  float st_dev = x->st_dev;
  int force_new_loop = x->force_new_loop;
  float R = x->R;
  short initialize_loop = x->initialize_loop;
//  short fixed_increment_on = x->fixed_increment_on;
//  float retro_odds = x->retro_odds;
  float fade_level = x->fade_level;
  short preempt = x->preempt;

  int preempt_count = x->preempt_count;
  int preempt_samps = x->preempt_samps;
  short recalling_loop = x->recalling_loop;
  float preempt_gain;
	
  float jitter_factor = x->jitter_factor;
  float rdur_factor = x->rdur_factor;
  float rinc_factor = x->rinc_factor;

  if(x->mute){
		while(n--) *out1++ = 0.0; return (w+4);
  }
	
  /* SAFETY CHECKS */
  if( b_frames <= 0 || nc != 1) {
    x->disabled = 1;
  }
	
  if(x->mute || x->disabled){
    while(n--){
      *out1++ = 0.0;
    }
    return (w+4);
  }
	
  if(x->framesize != b_frames) {
    x->framesize = b_frames ;
    x->buffer_duration = (float)  b_frames / R ;
    initialize_loop = 1;
  }	
  else if(x->buffer_duration <= 0.0) { /* THIS WILL HAPPEN THE FIRST TIME */
    x->framesize = b_frames ;
    x->buffer_duration = (float)  b_frames / R ;
    initialize_loop = 1;
//	post("initializing from perform method");
  }
  if(maxseg > x->buffer_duration){
    maxseg = x->buffer_duration ;
  }
	
  if(minseg < 2. * taper_duration)
    minseg = taper_duration;

  /* SET INITIAL SEGMENT */
	
  bindex = x->fbindex;
  
  if(initialize_loop){ /* FIRST TIME ONLY */
    chopper_randloop(x);
    bindex = x->fbindex;
    initialize_loop = 0;
  }

  while(n--){
    if( lock_loop )  {
      if ( recalling_loop ) { 
        bindex = x->fbindex ;
		x->fbindex += x->increment;
		--preempt_count;
		preempt_gain = fade_level  * ((float) preempt_count / (float) preempt_samps);
		*out1++ = tab[bindex] * preempt_gain;
		if( preempt_count <= 0) {
		  bindex = x->fbindex = x->loop_start;
		  recalling_loop = 0;
		}
      }
		
      else if(force_new_loop){

		if( bindex < 0 || bindex >= b_frames ){
		  x->fbindex = bindex = b_frames/2; // start in the middle
		}
		// should switch to <
		if( preempt && preempt_samps > x->samps_to_go ){ /* PREEMPT FADE */

		  --preempt_count;
		  preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
		  bindex = x->fbindex ;
		  x->fbindex += x->increment;
			 
						
		  *out1++ = tab[ bindex ] * preempt_gain;
		  if(! preempt_count) {
			chopper_randloop(x);
			bindex = x->fbindex;
			force_new_loop = 0;
		  }
		} 
		else { 
		  /* IMMEDIATE FORCE NEW LOOP AFTER PREEMPT FADE */
		  chopper_randloop(x);
		  
		  force_new_loop = 0;
		  bindex = x->fbindex ;
		  bindex2 = bindex << 1;	
		  x->fbindex += x->increment;
		  
		  --(x->samps_to_go);
		  if( x->samps_to_go <= 0 ){
			x->fbindex = x->loop_start;
			bindex = x->fbindex;
			x->samps_to_go = x->transp_loop_samps;
		  }
		  if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
			fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
			*out1++ = tab[bindex] * fade_level;
		  } else if( x->samps_to_go < taper_samps ) {
			fade_level = (float)(x->samps_to_go)/(float)taper_samps;
			*out1++ = tab[bindex] * fade_level;
		  } else {	
			fade_level = 1.0;
			*out1++ = tab[bindex];
		  }
		}
				
      } 
      /* REGULAR PLAYBACK */
      else { 

		if( bindex < 0 || bindex >= b_frames ){
		  error("lock_loop: bindex %d is out of range", bindex);
		  x->fbindex = bindex = b_frames / 2;
		}
		bindex = floor( (double) x->fbindex );
		m2 = x->fbindex - bindex ;
		m1 = 1.0 - m2;
		
	//	bindex2 = bindex << 1;
		x->fbindex += x->increment;
		
		--(x->samps_to_go);
		if( x->samps_to_go <= 0 ){
		  if( rdur_factor ){
			chopper_rdurme( x );
		  }
		  if( jitter_factor ){
			chopper_jitterme( x );
		  }
		  if( rinc_factor ) {
			chopper_rincme( x );
		  }
		  x->fbindex = x->loop_start;
		  // x->fbindex -= x->transp_loop_samps;
		  bindex = x->fbindex;
		  x->samps_to_go = x->transp_loop_samps;
		}
		
		if( bindex >= b_frames ){
		  sample1 = tab[bindex];
		} else {
		  sample1 = m1 * tab[bindex] + m2 * tab[bindex + 1];// optimize last ;)
		}
		if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
		  fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
		  *out1++ = sample1 * fade_level;
		} 
		else if( x->samps_to_go < taper_samps ) {
		  fade_level = (float)(x->samps_to_go)/(float)taper_samps;
		  *out1++ = sample1 * fade_level;
		} 
		else {	
		  fade_level = 1.0;
		  *out1++ = sample1;
		}
      }
    } /* END OF LOCK LOOP */
    /* RECALL STORED LOOP */
    
    else if (recalling_loop) { 
      bindex = x->fbindex ;
      x->fbindex += x->increment;
      --preempt_count;
      preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
					
      *out1++ = tab[bindex] * preempt_gain;
      if( preempt_count <= 0) {
		x->fbindex = x->loop_start;
		bindex = x->fbindex;
		recalling_loop = 0;
      }
    }
    
    else {
      if( force_new_loop ){
	/* FORCE LOOP CODE : MUST PREEMPT */
		force_new_loop = 0;
	/* NEED CODE HERE*/
      } 
      else {  /* NORMAL OPERATION */
		fade_level = 1.0; /* default level */

		if( bindex < 0 || bindex >= b_frames ){
		  error("force loop: bindex %d is out of range", bindex);
		  post("frames:%d start:%d, samps2go:%d, tloopsamps:%d, increment:%f", 
			   x->framesize, bindex, x->samps_to_go, x->transp_loop_samps, x->increment);
		  chopper_randloop(x);
		  bindex = x->fbindex;
		}
		bindex = x->fbindex ;
		x->fbindex += x->increment;
		
		if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
		  fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
		  *out1++ = tab[bindex] * fade_level;
		} 
		else if(x->samps_to_go < taper_samps) {
		  fade_level = (float)(x->samps_to_go)/(float)taper_samps;
		  *out1++ = tab[bindex] * fade_level;
		} 
		else {	
		  fade_level = 1.0;
		  *out1++ = tab[bindex];
		}
		--(x->samps_to_go);
		if( x->samps_to_go <= 0 ){
		  chopper_randloop( x );
		  bindex = x->fbindex;
		}
      }
    }
  }

  x->recalling_loop = recalling_loop;
  x->fade_level = fade_level;
  x->initialize_loop = initialize_loop;
  x->maxseg = maxseg;
  x->minseg = minseg;	
  x->segdur = segdur;
  x->force_new_loop = force_new_loop;
  return (w+4);
  
}

#if __MSP__
t_int *chopper_perform_stereo(t_int *w)
{

  // ADDED INTERPOLATION

  int bindex, bindex2;
  float sample1, sample2, frac;	
  float tmp1, tmp2;
  t_chopper *x = (t_chopper *)(w[1]);
  t_float *out1 = (t_float *)(w[2]);
  t_float *out2 = (t_float *)(w[3]);
  t_int n = w[4];
  t_buffer *b = x->l_buf;

  /*********************************************/
  float *tab = x->b_samples;
//  long chan = x->l_chan;
  long frames = x->b_frames;
  long nc = x->b_nchans;

  float segdur = x->segdur;
  int taper_samps = x->taper_samps ;
  float taper_duration = x->taper_duration;
//  float minincr = x->minincr;
//  float maxincr = x->maxincr;
  float minseg = x->minseg;
  float maxseg = x->maxseg;
//  float ldev = x->ldev;
  int lock_loop = x->lock_loop;
//  float st_dev = x->st_dev;
  int force_new_loop = x->force_new_loop;
//  float R = x->R;
  short initialize_loop = x->initialize_loop;
//  short fixed_increment_on = x->fixed_increment_on;
 // float retro_odds = x->retro_odds;
  float fade_level = x->fade_level;
  short preempt = x->preempt;

  int preempt_count = x->preempt_count;
  int preempt_samps = x->preempt_samps;
  short recalling_loop = x->recalling_loop;
  float preempt_gain;
	
  float jitter_factor = x->jitter_factor;
  float rdur_factor = x->rdur_factor;
  float rinc_factor = x->rinc_factor;
  long b_valid = b->b_valid;
  float fbindex = x->fbindex;
	float increment = x->increment;
	int transp_loop_samps = x->transp_loop_samps;
	int framesize = x->framesize;
	int loop_start = x->loop_start;
	int loop_samps = x->loop_samps;
	int samps_to_go = x->samps_to_go;
//	int tcase;

  /* SAFETY CHECKS */
  if( frames <= 0 || nc != 2) {
    x->disabled = 1;
  }
	
  if(x->mute || x->disabled || ! b_valid){
    while(n--){
      *out1++ = 0.0;
      *out2++ = 0.0;
    }
    return (w+5);
  }

  
  
  if(maxseg > x->buffer_duration){
    maxseg = x->buffer_duration ;
  }
	
  if(minseg < 2. * taper_duration)
    minseg = taper_duration;

  /* SET INITIAL SEGMENT */
	
  bindex = fbindex = x->fbindex;
  

  while(n--) {
	
    if(lock_loop)  {

      if (recalling_loop) { 
				
				bindex = floor((double)fbindex);
				bindex2 = bindex << 1;
				frac = fbindex - bindex ;
				fbindex += increment;				  
				--preempt_count;
				preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );

				if(bindex < frames - 1){
					tmp1 = tab[bindex2];
				  tmp2 = tab[bindex2 + 2];
				  sample1 = (tmp1 + frac * (tmp2-tmp1)) * preempt_gain;
					tmp1 = tab[bindex2 + 1];
				  tmp2 = tab[bindex2 + 3];
				  sample2 = (tmp1 + frac * (tmp2-tmp1)) * preempt_gain;
				} else {
					sample1 = tab[bindex2] * preempt_gain;
					sample2 = tab[bindex2 + 1] * preempt_gain;
				}
				*out1++ = sample1;
				*out2++ = sample2;
				if(preempt_count <= 0) {
				  bindex = fbindex = loop_start;
				  recalling_loop = 0;
				}
      }
		
      else if(force_new_loop){
				if( bindex < 0 || bindex >= frames ){
				  fbindex = bindex = frames / 2;
				}
				// should switch to <
				if( preempt && preempt_samps > x->samps_to_go ){ /* PREEMPT FADE */

				  --preempt_count;
				  preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
				  bindex = fbindex ;
				  bindex2 = bindex << 1;
				  frac = fbindex - bindex ;
				  fbindex += increment;
					if(bindex < frames - 1){
						tmp1 = tab[bindex2];
					  tmp2 = tab[bindex2 + 2];
					  sample1 = (tmp1 + frac * (tmp2-tmp1)) * preempt_gain;
						tmp1 = tab[bindex2 + 1];
					  tmp2 = tab[bindex2 + 3];
					  sample2 = (tmp1 + frac * (tmp2-tmp1)) * preempt_gain;
					} else {
						sample1 = tab[bindex2] * preempt_gain;
						sample2 = tab[bindex2 + 1] * preempt_gain;
					}		 	
				  *out1++ = sample1;
				  *out2++ = sample2;
				  
				  if(! preempt_count) {
//				  	post("inside forced new loop");
				    chopper_randloop(x);
				    bindex = fbindex = x->fbindex;
				    samps_to_go = x->samps_to_go;
				    loop_start = x->loop_start;
				    loop_samps = x->loop_samps;
				    transp_loop_samps = x->transp_loop_samps;
				    increment = x->increment;
				    segdur = x->segdur;
				    force_new_loop = 0;
				  }
				} 
				else { 
				  /* IMMEDIATE FORCE NEW LOOP AFTER PREEMPT FADE */
//				  post("forced after preempt");
				  chopper_randloop(x);
			    bindex = fbindex = x->fbindex;
			    samps_to_go = x->samps_to_go;
			    loop_start = x->loop_start;
			    loop_samps = x->loop_samps;
			    transp_loop_samps = x->transp_loop_samps;
			    increment = x->increment;
			    segdur = x->segdur;
				  force_new_loop = 0;
				  bindex2 = bindex << 1;	
				  fbindex += increment;
				  
				  --samps_to_go;
				  if(samps_to_go <= 0 ){
				    bindex = fbindex = loop_start;
				    samps_to_go = transp_loop_samps;
				  }
				  bindex = fbindex ;
				  bindex2 = bindex << 1;
				  frac = fbindex - bindex ;
				  fbindex += increment;


					if(bindex < frames - 1){
						tmp1 = tab[bindex2];
					  tmp2 = tab[bindex2 + 2];
					  sample1 = (tmp1 + frac * (tmp2-tmp1));
						tmp1 = tab[bindex2 + 1];
					  tmp2 = tab[bindex2 + 3];
					  sample2 = (tmp1 + frac * (tmp2-tmp1));
					} else {
						sample1 = tab[bindex2];
						sample2 = tab[bindex2 + 1];
					}		 	
	

				  if(samps_to_go > transp_loop_samps - taper_samps ){
				    fade_level =  (float)(transp_loop_samps - samps_to_go)/(float)taper_samps ;
				    *out1++ = sample1 * fade_level;
				    *out2++ = sample2 * fade_level;
				  } else if(samps_to_go < taper_samps ) {
				    fade_level = (float)(samps_to_go)/(float)taper_samps;
				    *out1++ = sample1 * fade_level;
				    *out2++ = sample2 * fade_level;
				  } else {	
				    fade_level = 1.0;
				    *out1++ = sample1;
				    *out2++ = sample2;
				  }
				}
						
      } 
      /* REGULAR PLAYBACK */
      else { 

				if( bindex < 0 || bindex >= frames ){
				  error("lock_loop: bindex %d is out of range", bindex);
				  fbindex = bindex = frames / 2;// go to middle
				}
				bindex = floor( (double) fbindex );
				frac = fbindex - bindex ;
				
				bindex2 = bindex << 1;
				fbindex += increment;
				
				--samps_to_go;
				if(samps_to_go <= 0){
				  if( rdur_factor ){
				    chopper_rdurme(x);
				  }
				  if( jitter_factor ){
				    chopper_jitterme(x);
				  }
				  if( rinc_factor ) {
				    chopper_rincme(x);
				  }
				  bindex = fbindex = loop_start = x->loop_start;
				  samps_to_go = transp_loop_samps = x->transp_loop_samps;
				  increment = x->increment;
				  
				}

				if(bindex < frames - 1){
					tmp1 = tab[bindex2];
				  tmp2 = tab[bindex2 + 2];
				  sample1 = (tmp1 + frac * (tmp2-tmp1));
					tmp1 = tab[bindex2 + 1];
				  tmp2 = tab[bindex2 + 3];
				  sample2 = (tmp1 + frac * (tmp2-tmp1));
				} else {
					sample1 = tab[bindex2];
					sample2 = tab[bindex2 + 1];
				}		 	
				
				if(samps_to_go > transp_loop_samps - taper_samps ){
				  fade_level =  (float)(transp_loop_samps - samps_to_go)/(float)taper_samps ;
				  *out1++ = sample1 * fade_level;
				  *out2++ = sample2 * fade_level;
				} 
				
				else if(samps_to_go < taper_samps) {
				  fade_level = (float)(samps_to_go)/(float)taper_samps;
				  *out1++ = sample1 * fade_level;
				  *out2++ = sample2 * fade_level;
				} 
				else {	
				  fade_level = 1.0;
				  *out1++ = sample1;
				  *out2++ = sample2;
				}
				if(fade_level > 1){
/*				post("fadelevel %f samps2go %d transloopsamps %d tapersamps %d tcase %d",
					fade_level,samps_to_go,transp_loop_samps,taper_samps, tcase);*/
				}
      }
    } /* END OF LOCK LOOP */
    /* RECALL STORED LOOP */
    
    else if (recalling_loop) { 
      bindex = fbindex;
      bindex2 = bindex << 1;
      frac = fbindex - bindex;
      fbindex += increment;
      
			if(bindex < frames - 1){
				tmp1 = tab[bindex2];
			  tmp2 = tab[bindex2 + 2];
			  sample1 = (tmp1 + frac * (tmp2-tmp1));
				tmp1 = tab[bindex2 + 1];
			  tmp2 = tab[bindex2 + 3];
			  sample2 = (tmp1 + frac * (tmp2-tmp1));
			} else {
				sample1 = tab[bindex2];
				sample2 = tab[bindex2 + 1];
			}		 	
      
	  
      --preempt_count;
      preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
					
      *out1++ = sample1 * preempt_gain;
      *out2++ = sample2 * preempt_gain;
      if( preempt_count <= 0) {
				fbindex = loop_start;
				bindex = fbindex;
				recalling_loop = 0;
      }
    }
    
    else {
      if( force_new_loop ){
				/* FORCE LOOP CODE : MUST PREEMPT */
				force_new_loop = 0;
				/* NEED CODE HERE*/
      } 
      else {  /* NORMAL OPERATION */
				fade_level = 1.0; /* default level */

				if( bindex < 0 || bindex >= frames ){
				  error("force loop: bindex %d is out of range", bindex);
				  post("total:%d start:%d, samps2go:%d, tloopsamps:%d, increment:%f", 
				       framesize, bindex, samps_to_go, transp_loop_samps, increment);
				  chopper_randloop(x);
			    bindex = fbindex = x->fbindex;
			    samps_to_go = x->samps_to_go;
			    loop_start = x->loop_start;
			    loop_samps = x->loop_samps;
			    transp_loop_samps = x->transp_loop_samps;
			    increment = x->increment;
			    segdur = x->segdur;
				}
	      bindex = fbindex;
	      bindex2 = bindex << 1;
	      frac = fbindex - bindex;
	      fbindex += increment;

				if(bindex < frames - 1){
					tmp1 = tab[bindex2];
				  tmp2 = tab[bindex2 + 2];
				  sample1 = (tmp1 + frac * (tmp2-tmp1));
					tmp1 = tab[bindex2 + 1];
				  tmp2 = tab[bindex2 + 3];
				  sample2 = (tmp1 + frac * (tmp2-tmp1));
				} else {
					sample1 = tab[bindex2];
					sample2 = tab[bindex2 + 1];
				}		 	
	      
					
				
				if(samps_to_go > transp_loop_samps - taper_samps ){
				  fade_level =  (float)(transp_loop_samps - samps_to_go)/(float)taper_samps ;
				  *out1++ = sample1 * fade_level;
				  *out2++ = sample2 * fade_level;
				} 
				else if(samps_to_go < taper_samps ) {
				  fade_level = (float)(samps_to_go)/(float)taper_samps;
				  *out1++ = sample1 * fade_level;
				  *out2++ = sample2 * fade_level;
				} 
				else {	
				  fade_level = 1.0;
				  *out1++ = sample1;
				  *out2++ = sample2;
				}
				--samps_to_go;
				if(samps_to_go <= 0){
//				post("new loop normal operation");
				  chopper_randloop(x);
			    bindex = fbindex = x->fbindex;
			    samps_to_go = x->samps_to_go;
			    loop_start = x->loop_start;
			    loop_samps = x->loop_samps;
			    transp_loop_samps = x->transp_loop_samps;
			    increment = x->increment;
			    segdur = x->segdur;
				}
      }
    }
  }

  x->recalling_loop = recalling_loop;
  x->fade_level = fade_level;
  x->initialize_loop = initialize_loop;
  x->maxseg = maxseg;
  x->minseg = minseg;	
  x->segdur = segdur;
  x->force_new_loop = force_new_loop;
  x->fbindex = fbindex;
  x->samps_to_go = samps_to_go;
  x->fbindex = fbindex;
  x->loop_start = loop_start;
  x->loop_samps = loop_samps;
  x->increment = increment;
  return (w+5);
}

// this is the official mono performer
t_int *chopper_perform_mono(t_int *w)
{

  // ADDED INTERPOLATION

  int bindex, bindex2;
  float sample1, sample2, m1, m2, frac;	
  t_chopper *x = (t_chopper *)(w[1]);
  t_float *out1 = (t_float *)(w[2]);
  t_int n = w[3];
  t_buffer *b = x->l_buf;

  /*********************************************/
  float *tab = x->b_samples;
//  long chan = x->l_chan;
  long frames = x->b_frames;
  long nc = x->b_nchans;

  float segdur = x->segdur;
  int taper_samps = x->taper_samps ;
  float taper_duration = x->taper_duration;
//  float minincr = x->minincr;
//  float maxincr = x->maxincr;
  float minseg = x->minseg;
  float maxseg = x->maxseg;
//  float ldev = x->ldev;
  int lock_loop = x->lock_loop;
//  float st_dev = x->st_dev;
  int force_new_loop = x->force_new_loop;
//  float R = x->R;
  short initialize_loop = x->initialize_loop;
//  short fixed_increment_on = x->fixed_increment_on;
//  float retro_odds = x->retro_odds;
  float fade_level = x->fade_level;
  short preempt = x->preempt;

  int preempt_count = x->preempt_count;
  int preempt_samps = x->preempt_samps;
  short recalling_loop = x->recalling_loop;
  float preempt_gain;
	
  float jitter_factor = x->jitter_factor;
  float rdur_factor = x->rdur_factor;
  float rinc_factor = x->rinc_factor;
  long b_valid = b->b_valid;
  float fbindex = x->fbindex;
	float increment = x->increment;
	int transp_loop_samps = x->transp_loop_samps;
	int framesize = x->framesize;
	int loop_start = x->loop_start;
	int loop_samps = x->loop_samps;
	int samps_to_go = x->samps_to_go;
//	int tcase;

  /* SAFETY CHECKS */
  if( frames <= 0 || nc != 2) {
    x->disabled = 1;
  }
	
  if(x->mute || x->disabled || ! b_valid){
    while(n--){
      *out1++ = 0.0;
    }
    return (w+4);
  }

  
  
  if(maxseg > x->buffer_duration){
    maxseg = x->buffer_duration ;
  }
	
  if(minseg < 2. * taper_duration)
    minseg = taper_duration;

  /* SET INITIAL SEGMENT */
	
  bindex = fbindex = x->fbindex;
  

  while(n--) {
	
    if(lock_loop)  {

      if (recalling_loop) { 
				
				bindex = floor((double)fbindex);
				bindex2 = bindex << 1;
				frac = fbindex - bindex ;
				fbindex += increment;				  
				--preempt_count;
				preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );

				if(bindex < frames - 1){
					sample1 = tab[bindex] * preempt_gain;
				  sample2 = tab[bindex + 1] * preempt_gain;
					sample1 += frac * (sample2-sample1);
				} else {
					sample1 = tab[bindex] * preempt_gain;
				}
				*out1++ = sample1;
				if(preempt_count <= 0) {
				  bindex = fbindex = loop_start;
				  recalling_loop = 0;
				}
      }
		
      else if(force_new_loop){
				if( bindex < 0 || bindex >= frames ){
				  fbindex = bindex = frames / 2;
				}
				// should switch to <
				if( preempt && preempt_samps > x->samps_to_go ){ /* PREEMPT FADE */

				  --preempt_count;
				  preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
				  bindex = fbindex ;
				  frac = fbindex - bindex ;
				  fbindex += increment;
	
					if(bindex < frames - 1){
						sample1 = tab[bindex] * preempt_gain;
					  sample2 = tab[bindex + 1] * preempt_gain;
						sample1 += frac * (sample2-sample1);
					} else {
						sample1 = tab[bindex] * preempt_gain;
					}			 	
				  *out1++ = sample1;
				  
				  if(! preempt_count) {
//				  	post("inside forced new loop");
				    chopper_randloop(x);
				    bindex = fbindex = x->fbindex;
				    samps_to_go = x->samps_to_go;
				    loop_start = x->loop_start;
				    loop_samps = x->loop_samps;
				    transp_loop_samps = x->transp_loop_samps;
				    increment = x->increment;
				    segdur = x->segdur;
				    force_new_loop = 0;
				  }
				} 
				else { 
				  /* IMMEDIATE FORCE NEW LOOP AFTER PREEMPT FADE */
//				  post("forced after preempt");
				  chopper_randloop(x);
			    bindex = fbindex = x->fbindex;
			    samps_to_go = x->samps_to_go;
			    loop_start = x->loop_start;
			    loop_samps = x->loop_samps;
			    transp_loop_samps = x->transp_loop_samps;
			    increment = x->increment;
			    segdur = x->segdur;
				  force_new_loop = 0;
				  bindex2 = bindex << 1;	
				  fbindex += increment;
				  
				  --samps_to_go;
				  if(samps_to_go <= 0 ){
				    bindex = fbindex = loop_start;
				    samps_to_go = transp_loop_samps;
				  }
				  bindex = fbindex ;
				  bindex2 = bindex << 1;
				  frac = fbindex - bindex ;
				  fbindex += increment;
	
					if(bindex < frames - 1){
						sample1 = tab[bindex];
					  sample2 = tab[bindex + 1];
						sample1 += frac * (sample2-sample1);

					} else {
						sample1 = tab[bindex];
					}			 	

				  if(samps_to_go > transp_loop_samps - taper_samps ){
				    fade_level =  (float)(transp_loop_samps - samps_to_go)/(float)taper_samps ;
				    *out1++ = sample1 * fade_level;
				  } else if(samps_to_go < taper_samps ) {
				    fade_level = (float)(samps_to_go)/(float)taper_samps;
				    *out1++ = sample1 * fade_level;
				  } else {	
				    fade_level = 1.0;
				    *out1++ = sample1;
				  }
				}				
      } 
      /* REGULAR PLAYBACK */
      else { 

				if( bindex < 0 || bindex >= frames ){
				  error("lock_loop: bindex %d is out of range", bindex);
				  fbindex = bindex = frames / 2;// go to middle
				}
				bindex = floor( (double) fbindex );
				m2 = fbindex - bindex ;
				m1 = 1.0 - m2;
				
				bindex2 = bindex << 1;
				fbindex += increment;
				
				--samps_to_go;
				if(samps_to_go <= 0){
				  if( rdur_factor ){
				    chopper_rdurme(x);
				  }
				  if( jitter_factor ){
				    chopper_jitterme(x);
				  }
				  if( rinc_factor ) {
				    chopper_rincme(x);
				  }
				  bindex = fbindex = loop_start = x->loop_start;
				  samps_to_go = transp_loop_samps = x->transp_loop_samps;
				  increment = x->increment;
				  
				}
				
				if( bindex >= frames-1 ){
				  sample1 = tab[bindex];
				} else {
				  sample1 = m1 * tab[bindex] + m2 * tab[bindex + 1];
				}
				if(samps_to_go > transp_loop_samps - taper_samps ){
				  fade_level =  (float)(transp_loop_samps - samps_to_go)/(float)taper_samps ;
				  *out1++ = sample1 * fade_level;
				} 
				else if(samps_to_go < taper_samps) {
				  fade_level = (float)(samps_to_go)/(float)taper_samps;
				  *out1++ = sample1 * fade_level;
				} 
				else {	
				  fade_level = 1.0;
				  *out1++ = sample1;
				}
				if(fade_level > 1){
/*				post("fadelevel %f samps2go %d transloopsamps %d tapersamps %d tcase %d",
					fade_level,samps_to_go,transp_loop_samps,taper_samps, tcase);*/
				}
      }
    } /* END OF LOCK LOOP */
    /* RECALL STORED LOOP */
    
    else if (recalling_loop) { 
      bindex = fbindex;
//      bindex2 = bindex << 1;
      frac = fbindex - bindex;
      fbindex += increment;
      
			if(bindex < frames - 1){
				sample1 = tab[bindex];
			  sample2 = tab[bindex + 1];
				sample1 += frac * (sample2-sample1);
			} else {
				sample1 = tab[bindex];
				sample2 = tab[bindex + 1];
			}		      
	  
      --preempt_count;
      preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
					
      *out1++ = sample1 * preempt_gain;
      if( preempt_count <= 0) {
				fbindex = loop_start;
				bindex = fbindex;
				recalling_loop = 0;
      }
    }
    
    else {
      if( force_new_loop ){
				/* FORCE LOOP CODE : MUST PREEMPT */
				force_new_loop = 0;
				/* NEED CODE HERE*/
      } 
      else {  /* NORMAL OPERATION */
				fade_level = 1.0; /* default level */

				if( bindex < 0 || bindex >= frames ){
				  error("force loop: bindex %d is out of range", bindex);
				  post("total:%d start:%d, samps2go:%d, tloopsamps:%d, increment:%f", 
				       framesize, bindex, samps_to_go, transp_loop_samps, increment);
				  chopper_randloop(x);
			    bindex = fbindex = x->fbindex;
			    samps_to_go = x->samps_to_go;
			    loop_start = x->loop_start;
			    loop_samps = x->loop_samps;
			    transp_loop_samps = x->transp_loop_samps;
			    increment = x->increment;
			    segdur = x->segdur;
				}
	      bindex = fbindex;
	      frac = fbindex - bindex;
	      fbindex += increment;

				if(bindex < frames - 1){
					sample1 = tab[bindex];
				  sample2 = tab[bindex + 1];
					sample1 += frac * (sample2-sample1);

				} else {
					sample1 = tab[bindex];
				}					
				
				if(samps_to_go > transp_loop_samps - taper_samps ){
				  fade_level =  (float)(transp_loop_samps - samps_to_go)/(float)taper_samps ;
				  *out1++ = sample1 * fade_level;
				} 
				else if(samps_to_go < taper_samps ) {
				  fade_level = (float)(samps_to_go)/(float)taper_samps;
				  *out1++ = sample1 * fade_level;
				} 
				else {	
				  fade_level = 1.0;
				  *out1++ = sample1;
				}
				--samps_to_go;
				if(samps_to_go <= 0){
//				post("new loop normal operation");
				  chopper_randloop(x);
			    bindex = fbindex = x->fbindex;
			    samps_to_go = x->samps_to_go;
			    loop_start = x->loop_start;
			    loop_samps = x->loop_samps;
			    transp_loop_samps = x->transp_loop_samps;
			    increment = x->increment;
			    segdur = x->segdur;
				}
      }
    }
  }

  x->recalling_loop = recalling_loop;
  x->fade_level = fade_level;
  x->initialize_loop = initialize_loop;
  x->maxseg = maxseg;
  x->minseg = minseg;	
  x->segdur = segdur;
  x->force_new_loop = force_new_loop;
  x->fbindex = fbindex;
  x->samps_to_go = samps_to_go;
  x->fbindex = fbindex;
  x->loop_start = loop_start;
  x->loop_samps = loop_samps;
  x->increment = increment;
  return (w+5);
}


t_int *chopper_perform_stereo_nointerpol(t_int *w)
{

  // ADDED INTERPOLATION

  int bindex, bindex2;
  float sample1, sample2, m1, m2;	
  t_chopper *x = (t_chopper *)(w[1]);
  t_float *out1 = (t_float *)(w[2]);
  t_float *out2 = (t_float *)(w[3]);
  t_int n = w[4];
  t_buffer *b = x->l_buf;

  /*********************************************/
  float *tab = x->b_samples;
//  long chan = x->l_chan;
  long frames = x->b_frames;
  long nc = x->b_nchans;

  float segdur = x->segdur;
  int taper_samps = x->taper_samps ;
  float taper_duration = x->taper_duration;
//  float minincr = x->minincr;
//  float maxincr = x->maxincr;
  float minseg = x->minseg;
  float maxseg = x->maxseg;
//  float ldev = x->ldev;
  int lock_loop = x->lock_loop;
//  float st_dev = x->st_dev;
  int force_new_loop = x->force_new_loop;
  float R = x->R;
  short initialize_loop = x->initialize_loop;
//  short fixed_increment_on = x->fixed_increment_on;
//  float retro_odds = x->retro_odds;
  float fade_level = x->fade_level;
  short preempt = x->preempt;

  int preempt_count = x->preempt_count;
  int preempt_samps = x->preempt_samps;
  short recalling_loop = x->recalling_loop;
  float preempt_gain;
	
  float jitter_factor = x->jitter_factor;
  float rdur_factor = x->rdur_factor;
  float rinc_factor = x->rinc_factor;
  long b_valid = b->b_valid;
	
  /* SAFETY CHECKS */
  if( frames <= 0 || nc != 2) {
    x->disabled = 1;
  }
	
  if(x->mute || x->disabled || ! b_valid){
    while(n--){
      *out1++ = 0.0;
      *out2++ = 0.0;
    }
    return (w+5);
  }
	
  if(x->framesize != b->b_frames) {
    x->framesize = b->b_frames ;
    x->buffer_duration = (float)  b->b_frames / R ;
    initialize_loop = 1;
  }	
  else if(x->buffer_duration <= 0.0) { /* THIS WILL HAPPEN THE FIRST TIME */
    x->framesize = b->b_frames ;
    x->buffer_duration = (float)  b->b_frames / R ;
    initialize_loop = 1;
  }
  if(maxseg > x->buffer_duration){
    maxseg = x->buffer_duration ;
  }
	
  if(minseg < 2. * taper_duration)
    minseg = taper_duration;

  /* SET INITIAL SEGMENT */
	
  bindex = x->fbindex;
  
  if( initialize_loop ){ /* FIRST TIME ONLY */
    chopper_randloop(x);

    bindex = x->fbindex;
    initialize_loop = 0;

  }
  /* MAIN SYNTHESIS LOOPS */	
  /* PUT EVERYTHING IN ONE LOOP*/

  while(n--) {
	
    if( lock_loop )  {

      if ( recalling_loop ) { 
	        bindex = x->fbindex ;
					bindex2 = bindex << 1;
					x->fbindex += x->increment;
						
					  
					--preempt_count;
					preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );

									
					*out1++ = tab[bindex2] * preempt_gain;
					*out2++ = tab[bindex2 + 1] * preempt_gain;
					if( preempt_count <= 0) {
					  bindex = x->fbindex = x->loop_start;
					  recalling_loop = 0;
					}
      }
		
      else if(force_new_loop){
				if( bindex < 0 || bindex >= frames ){
				  x->fbindex = bindex = frames / 2;
				}
				// should switch to <
				if( preempt && preempt_samps > x->samps_to_go ){ /* PREEMPT FADE */

				  --preempt_count;
				  preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
				  bindex = x->fbindex ;
				  bindex2 = bindex << 1;
				  x->fbindex += x->increment;
				 	 								
				  *out1++ = tab[ bindex2 ] * preempt_gain;
				  *out2++ = tab[ bindex2 + 1] * preempt_gain;
				  if( ! preempt_count ) {
				    chopper_randloop( x );
				    bindex = x->fbindex;
				    force_new_loop = 0;
				  }
				} 
				else { 
				  /* IMMEDIATE FORCE NEW LOOP AFTER PREEMPT FADE */
				  chopper_randloop( x );
				  
				  force_new_loop = 0;
				  bindex = x->fbindex ;
				  bindex2 = bindex << 1;	
				  x->fbindex += x->increment;
				  
				  --(x->samps_to_go);
				  if( x->samps_to_go <= 0 ){
				    x->fbindex = x->loop_start;
				    bindex = x->fbindex;
				    x->samps_to_go = x->transp_loop_samps;
				  }
				  if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
				    fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
				    *out1++ = tab[ bindex2 ] * fade_level;
				    *out2++ = tab[ bindex2 + 1] * fade_level;
				  } else if( x->samps_to_go < taper_samps ) {
				    fade_level = (float)(x->samps_to_go)/(float)taper_samps;
				    *out1++ = tab[ bindex2 ] * fade_level;
				    *out2++ = tab[ bindex2 + 1 ] * fade_level;
				  } else {	
				    fade_level = 1.0;
				    *out1++ = tab[ bindex2 ];
				    *out2++ = tab[ bindex2 + 1];
				  }
				}
						
      } 
      /* REGULAR PLAYBACK */
      else { 

	if( bindex < 0 || bindex >= frames ){
	  error("lock_loop: bindex %d is out of range", bindex);
	  x->fbindex = bindex = frames / 2;
	}
	bindex = floor( (double) x->fbindex );
	m2 = x->fbindex - bindex ;
	m1 = 1.0 - m2;
	
	bindex2 = bindex << 1;
	x->fbindex += x->increment;
	
	--(x->samps_to_go);
	if( x->samps_to_go <= 0 ){
	  if( rdur_factor ){
	    chopper_rdurme( x );
	  }
	  if( jitter_factor ){
	    chopper_jitterme( x );
	  }
	  if( rinc_factor ) {
	    chopper_rincme( x );
	  }
	  x->fbindex = x->loop_start;
	  // x->fbindex -= x->transp_loop_samps;
	  bindex = x->fbindex;
	  x->samps_to_go = x->transp_loop_samps;
	}
	
	if( bindex >= frames ){
	  sample1 = tab[ bindex2 ];
	  sample2 = tab[ bindex2 + 1 ];
	} else {
	  sample1 = m1 * tab[ bindex2 ] + m2 * tab[ bindex2 + 2 ];
	  sample2 = m1 * tab[ bindex2 + 1] + m2 * tab[ bindex2 + 3 ];
	}
	if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
	  fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
	  *out1++ = sample1 * fade_level;
	  *out2++ = sample2 * fade_level;
	} 
	else if( x->samps_to_go < taper_samps ) {
	  fade_level = (float)(x->samps_to_go)/(float)taper_samps;
	  *out1++ = sample1 * fade_level;
	  *out2++ = sample2 * fade_level;
	} 
	else {	
	  fade_level = 1.0;
	  *out1++ = sample1;
	  *out2++ = sample2;
	}
      }
    } /* END OF LOCK LOOP */
    /* RECALL STORED LOOP */
    
    else if ( recalling_loop ) { 
      bindex = x->fbindex ;
      bindex2 = bindex << 1;
      x->fbindex += x->increment;
      
	  
      --preempt_count;
      preempt_gain = fade_level  * ( (float) preempt_count / (float) preempt_samps );
					
      *out1++ = tab[ bindex2 ] * preempt_gain;
      *out2++ = tab[ bindex2 + 1] * preempt_gain;
      if( preempt_count <= 0) {
	x->fbindex = x->loop_start;
	bindex = x->fbindex;
	recalling_loop = 0;
      }
    }
    
    else {
      if( force_new_loop ){
	/* FORCE LOOP CODE : MUST PREEMPT */
	force_new_loop = 0;
	/* NEED CODE HERE*/
      } 
      else {  /* NORMAL OPERATION */
				fade_level = 1.0; /* default level */

				if( bindex < 0 || bindex >= frames ){
				  error("force loop: bindex %d is out of range", bindex);
				  post("total:%d start:%d, samps2go:%d, tloopsamps:%d, increment:%f", 
				       x->framesize, bindex, x->samps_to_go, x->transp_loop_samps, x->increment);
				  chopper_randloop( x );
				  bindex = x->fbindex;
				}
				bindex = x->fbindex ;
				bindex2 = bindex << 1;
				x->fbindex += x->increment;
				
				if( x->samps_to_go > x->transp_loop_samps - taper_samps ){
				  fade_level =  (float)(x->transp_loop_samps - x->samps_to_go)/(float)taper_samps ;
				  *out1++ = tab[ bindex2 ] * fade_level;
				  *out2++ = tab[ bindex2 + 1] * fade_level;
				} 
				else if( x->samps_to_go < taper_samps ) {
				  fade_level = (float)(x->samps_to_go)/(float)taper_samps;
				  *out1++ = tab[ bindex2 ] * fade_level;
				  *out2++ = tab[ bindex2 + 1 ] * fade_level;
				} 
				else {	
				  fade_level = 1.0;
				  *out1++ = tab[ bindex2 ];
				  *out2++ = tab[ bindex2 + 1];
				}
				
				
				--(x->samps_to_go);
				if( x->samps_to_go <= 0 ){
				  chopper_randloop( x );
				  bindex = x->fbindex;
				}
      }
    }
  }

  x->recalling_loop = recalling_loop;
  x->fade_level = fade_level;
  x->initialize_loop = initialize_loop;
  x->maxseg = maxseg;
  x->minseg = minseg;	
  x->segdur = segdur;
  x->force_new_loop = force_new_loop;
  return (w+5);
}

t_int *choppermono_perform(t_int *w)
{

  float fbindex;
  float skipin;
//  long bindex2;
  float testseg;
	
  t_chopper *x = (t_chopper *)(w[1]);
  t_float *out1 = (t_float *)(w[2]);
  int n = (int)(w[3]);
  t_buffer *b = x->l_buf;

  /*********************************************/
  float *tab = b->b_samples;
//  long chan = x->l_chan;
  long frames = b->b_frames;
  long nc = b->b_nchans;

  long bindex = fbindex = x->fbindex;
  float segdur = x->segdur;
  int loop_start = x->loop_start;
  int loop_samps = x->loop_samps;
  int samps_to_go = x->samps_to_go;
  float increment = x->increment;
  int taper_samps = x->taper_samps ;
  float minincr = x->minincr;
  float maxincr = x->maxincr;
  float minseg = x->minseg;
  float maxseg = x->maxseg;
	
//  float ldev = x->ldev;
  int lock_loop = x->lock_loop;
 // float st_dev = x->st_dev;
  int force_new_loop = x->force_new_loop;

  if( frames <= 0 || nc != 1) {
    x->disabled = 1;
  }
  if( x->mute || x->disabled ){
    while( n-- ){
      *out1++ = 0.0;
    }
    return (w+4);
  }
	
  if( x->framesize != b->b_frames ) {
    x->framesize = b->b_frames ;
    x->buffer_duration = (float)  b->b_frames / (float) x->R ;
    force_new_loop = 1;
  }	
  else if( x->buffer_duration <= 0.0 ) {
    x->framesize = b->b_frames ;
    x->buffer_duration = (float)  b->b_frames / (float) x->R ;
    force_new_loop = 1;
  }
  if( x->maxseg > x->buffer_duration ){
    x->maxseg = x->buffer_duration ;
  }

  if( force_new_loop ) {
    // NEW LOOP  - THIS NOW IS A SOURCE OF CLICKS
    if( ! lock_loop ){
      increment = chopper_boundrand(minincr,maxincr);
      segdur = chopper_boundrand( minseg, maxseg );
      testseg = segdur * increment ;			
      skipin = chopper_boundrand(0.0, x->buffer_duration - testseg);
      loop_start = skipin * (float) x->R ;
      if( chopper_boundrand(0.0,1.0) < 0.5 ){
	increment *= -1. ;
	loop_start += samps_to_go;
      }
    } else {
    }
    // Must Finish Last Loop First To AVOID A CLICK!!
    loop_samps = samps_to_go = segdur * (float) x->R;
    bindex = fbindex = loop_start;
    force_new_loop = 0;
  }
  while( n-- ) {
    if( bindex < 0 ){
      force_new_loop = 1;
      fbindex = bindex = 0;
    } else if( bindex >= frames ) {
      force_new_loop = 1;
      fbindex = bindex = 0;
    }
    if( samps_to_go > loop_samps - taper_samps ){
      //			bindex2 = bindex * 2 ;
      *out1++ = tab[ bindex ] * ( (float)(loop_samps - samps_to_go)/(float)taper_samps );
      //			*out2++ = tab[ bindex2 + 1] * ( (float)(loop_samps - samps_to_go)/(float)taper_samps );
			
    } else if( samps_to_go < taper_samps ) {
      //			bindex2 = bindex * 2 ;
      *out1++ = tab[ bindex ] * ( (float)(samps_to_go)/(float)taper_samps );
      //			*out2++ = tab[ bindex2 + 1 ] * ( (float)(samps_to_go)/(float)taper_samps );

    } else {
      //			bindex2 = bindex * 2;
      *out1++ = tab[ bindex ];
      //			*out2++ = tab[ bindex2 + 1];

    }
    fbindex += increment;
    bindex = fbindex ;
    if( ! --samps_to_go || force_new_loop){
      // NEW LOOP
      if( ! lock_loop ){
	increment = chopper_boundrand(minincr,maxincr);
	segdur = chopper_boundrand( minseg, maxseg );
	testseg = segdur * increment ;
	skipin = chopper_boundrand(0.0, x->buffer_duration - testseg);
	loop_start = skipin * (float) x->R ;
	if(chopper_boundrand(0.0,1.0) < 0.5 ){
	  increment *= -1. ;
	  // work backwards
	  loop_start += samps_to_go;
	}
      }
      loop_samps = samps_to_go = segdur * (float) x->R;
      bindex = fbindex = loop_start;
      force_new_loop = 0;
    }
  }
  x->fbindex = fbindex;
  x->segdur = segdur;
  x->loop_start = loop_start;
  x->loop_samps = loop_samps;
  x->samps_to_go = samps_to_go;
  x->increment = increment;
  x->force_new_loop = force_new_loop;
  return (w+4);
}
#endif


void chopper_force_new(t_chopper *x)
{
  x->preempt_count = x->preempt_samps;
  x->force_new_loop = 1;

}

void chopper_lockme(t_chopper *x, t_floatarg n)
{
	x->lock_loop = (short) n;
//	post("lock loop set to %d from %f",x->lock_loop,n);
}

//set min time for loop
void chopper_set_minincr(t_chopper *x, t_floatarg n)
{
//  post("set minincr to %f", n);
	
  if( n < .005 ){
    n = .005;
  }
  //x->minincr = n ;
}

// set deviation factor
void chopper_set_maxseg(t_chopper *x, t_floatarg n)
{

  n /= 1000.0 ; // convert to seconds
  if( n > 120. )
    n = 120.;
  //post("set maxseg to %f", n);
  x->maxseg = n;
  x->loop_max_samps = x->maxseg * x->R;
}

void chopper_set_minseg(t_chopper *x, t_floatarg n)
{

  n /= 1000.0 ; // convert to seconds
	
  if( n < 0.03 )
    n = 0.03;
  //post("set minseg to %f", n);
  x->minseg = n;
  x->loop_min_samps = x->minseg * x->R;
}

// set max time for loop
void chopper_set_maxincr(t_chopper *x, t_floatarg n)
{
  if( n > 4 ){
    n = 4;
  }
  //post("set maxincr to %f", n);
  x->maxincr =  n ;
}



#if __MSP__
void chopper_set(t_chopper *x, t_symbol *s)
{
  t_buffer *b;
  x->l_sym = s;
  if ((b = (t_buffer *)(s->s_thing)) && ob_sym(b) == ps_buffer) {
    x->l_buf = b;
    x->disabled = 0;
    if( b->b_nchans != x->setup_chans ){
      error("channel mismatch between chopper~ and buffer~ contents");
    }
  } else {
    error("chopper~: no such buffer %s", s->s_name);
    x->disabled = 1;
    x->l_buf = 0;
  }
  x->b_frames = b->b_frames;
  x->b_nchans = b->b_nchans;
  x->b_samples = b->b_samples;
  x->buffer_duration = (float)  b->b_frames / x->R;
  x->framesize = b->b_frames;// redundant but needed (for now)
//  post("buffer duration %f",x->buffer_duration);
  chopper_randloop(x);
}
#endif

#if __PD__
void chopper_set(t_chopper *x, t_symbol *wavename)
{
  int frames;
  t_garray *a;
  x->disabled = 0;

  x->b_frames = 0;
  x->b_nchans = 1;
  if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
      if (*wavename->s_name) pd_error(x, "chopper~: %s: no such array",
				      wavename->s_name);
      x->b_samples = 0;
      x->disabled = 1;
    }
  else if (!garray_getfloatarray(a, &frames, &x->b_samples)) {
      pd_error(x, "%s: bad template for chopper~", wavename->s_name);
      x->b_samples = 0;
      x->disabled = 1;
    }
  else  {
    x->b_frames = frames;
//    post("%d frames in buffer",x->b_frames);
    garray_usedindsp(a);
  }
}
#endif



void chopper_dsp(t_chopper *x, t_signal **sp)
{
  chopper_set(x,x->l_sym);
  if(x->R != sp[0]->s_sr){
    x->R = sp[0]->s_sr;
  	chopper_init(x,1);
  }
  if(x->disabled){
    return;
  }
#if __PD__
	dsp_add(chopper_pd_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
#endif

#if __MSP__
  if(x->setup_chans == 2)
    dsp_add(chopper_perform_stereo, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
  else if(x->setup_chans == 1) {
    dsp_add(chopper_perform_mono, 3, x, sp[0]->s_vec, sp[0]->s_n);
  } 
#endif
}


float chopper_boundrand(float min, float max)
{
   return min + (max-min) * ((float) (rand() % RAND_MAX)/(float)RAND_MAX);

}
