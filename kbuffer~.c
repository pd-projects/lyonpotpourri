#include "MSPd.h"

/**/
#define OBJECT_NAME "kbuffer~"

#if __PD__
static t_class *kbuffer_class;
#endif

#if __MSP__
void *kbuffer_class;
#endif

typedef struct _kbuffer
{
#if __MSP__
  t_pxobject x_obj;
#endif
#if __PD__
  t_object x_obj;
  float x_f;
#endif

	float ksrate;
	float srate;
	float si;
	float phase;
	float duration;
	int iphase;
	int lastphase;
	int length;
	float *data;
	float fval;
	float lastval;
	short record_flag;
	short play_flag;
	short dump_flag;
	short loop_flag;
	float sync ;
	float speed ;
	short in_connected;
	int memsize;
} t_kbuffer;

t_int *kbuffer_perform(t_int *w);

void kbuffer_dsp(t_kbuffer *x, t_signal **sp, short *count);
void *kbuffer_new(t_symbol *s, int argc, t_atom *argv);
void kbuffer_assist(t_kbuffer *x, void *b, long m, long a, char *s);
void kbuffer_dsp_free(t_kbuffer *x);
void kbuffer_record(t_kbuffer *x);
void kbuffer_play(t_kbuffer *x);
void kbuffer_loop(t_kbuffer *x);
void kbuffer_info(t_kbuffer *x);
void kbuffer_dump(t_kbuffer *x);
void kbuffer_stop(t_kbuffer *x);
void kbuffer_info(t_kbuffer *x);
void kbuffer_speed(t_kbuffer *x, t_floatarg speed);
void kbuffer_size(t_kbuffer *x, t_floatarg ms);
void kbuffer_ksrate(t_kbuffer *x, t_floatarg ksrate);
void kbuffer_float(t_kbuffer *x, double f);
void kbuffer_int(t_kbuffer *x, int i);
void kbuffer_init(t_kbuffer *x,short initialized);

#if __MSP__
void main(void)
{
	setup((t_messlist **)&kbuffer_class,(method) kbuffer_new, (method)kbuffer_dsp_free, (short)sizeof(t_kbuffer), 0L, A_GIMME, 0);
	addmess((method)kbuffer_dsp, "dsp", A_CANT, 0);
	addmess((method)kbuffer_assist, "assist", A_CANT, 0);
	addmess((method)kbuffer_record, "record", 0);
	addmess((method)kbuffer_play, "play", 0);
	addmess((method)kbuffer_loop, "loop", 0);
	addmess((method)kbuffer_stop, "stop", 0);
	addmess((method)kbuffer_dump, "dump", 0);
	addmess((method)kbuffer_info, "info", 0);
	addmess((method)kbuffer_speed, "speed", A_FLOAT, 0);
	addmess((method)kbuffer_size, "size", A_FLOAT, 0);
	addmess((method)kbuffer_ksrate, "ksrate", A_FLOAT, 0);
	addfloat((method)kbuffer_float);
	addint((method)kbuffer_int);
	dsp_initclass();
	post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

#if __PD__
void kbuffer_tilde_setup(void){
  kbuffer_class = class_new(gensym("kbuffer~"), (t_newmethod)kbuffer_new, 
			    (t_method)kbuffer_dsp_free,sizeof(t_kbuffer), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(kbuffer_class, t_kbuffer, x_f);
  class_addmethod(kbuffer_class,(t_method)kbuffer_dsp,gensym("dsp"),0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_record,gensym("record"),0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_play,gensym("play"),0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_loop,gensym("loop"),0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_stop,gensym("stop"),0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_dump,gensym("dump"),0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_info,gensym("info"),0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_speed,gensym("speed"),A_FLOAT,0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_size,gensym("size"),A_FLOAT,0);
  class_addmethod(kbuffer_class,(t_method)kbuffer_ksrate,gensym("ksrate"),A_FLOAT,0);
  post("%s %s",OBJECT_NAME, LYONPOTPOURRI_MSG);
}
#endif

void kbuffer_speed(t_kbuffer *x, t_floatarg speed) {
	x->speed = speed;
	// post("speed set to %f",speed);	
}

void kbuffer_size(t_kbuffer *x, t_floatarg ms) {
	int i;
	if(ms < 1)
		ms = 1;
	x->duration = ms / 1000.0 ;
	x->memsize = x->ksrate * x->duration * sizeof(float);
    x->length = x->duration * x->ksrate ;
	x->data = (float*) realloc(x->data,x->memsize*sizeof(float));
    for( i = 0; i < x->length; i++){
    	x->data[i] = 0.0;
    }
}

void kbuffer_ksrate(t_kbuffer *x, t_floatarg ksrate) {
	int i;
	if( ksrate < 1 )
		ksrate = 1 ;
	x->ksrate = ksrate ;
	x->memsize = x->ksrate * x->duration * sizeof(float);
    x->length = x->duration * x->ksrate ;
    x->si = x->ksrate / x->srate;
	x->data = (float*) realloc(x->data,x->memsize*sizeof(float));
    for( i = 0; i < x->length; i++){
    	x->data[i] = 0.0;
    }
}

void kbuffer_info(t_kbuffer *x) {
	post("function length is %d samples",x->length);
	post("function sampling rate is %.2f",x->ksrate);
	post("function byte size is %d",x->memsize);
	post("function duration is %.2f seconds",x->duration);
}

void kbuffer_record(t_kbuffer *x) {
	x->record_flag = 1;
	x->play_flag = 0;
	x->dump_flag = 0;
	x->loop_flag = 0;
	x->sync = 0.0;
	x->phase = x->iphase = 0 ;
	x->lastphase = -1 ;
	// post("starting to record");
}
void kbuffer_stop(t_kbuffer *x) {
	x->record_flag = 0;
	x->play_flag = 0;
	x->dump_flag = 0;
	x->loop_flag = 0;
	x->sync = 0.0;
	x->phase = x->iphase = 0 ;
	x->lastphase = -1 ;
}
void kbuffer_dump(t_kbuffer *x) {
	x->record_flag = 0;
	x->play_flag = 0;
	x->loop_flag = 0;
	x->dump_flag = 1;
	x->sync = 0.0;
	x->phase = x->iphase = 0 ;
	x->lastphase = -1 ;
}

void kbuffer_play(t_kbuffer *x) {
	x->record_flag = 0;
	x->play_flag = 1;
	x->dump_flag = 0;
	x->loop_flag = 0;
	x->sync = 0.0;
	x->phase = x->iphase = 0 ;
	x->lastphase = -1 ;
}

void kbuffer_loop(t_kbuffer *x) {
	x->record_flag = 0;
	x->play_flag = 0;
	x->dump_flag = 0;
	x->loop_flag = 1;
	x->sync = 0.0;
	x->phase = x->iphase = 0 ;
	x->lastphase = -1 ;
}

void kbuffer_dsp_free(t_kbuffer *x) {
#if __MSP__
	dsp_free((t_pxobject *) x);
#endif
	free(x->data);
}

t_int *kbuffer_perform(t_int *w)
{
// DSP config
    t_kbuffer *x = (t_kbuffer *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    t_float *sync_out = (t_float *)(w[4]);
    int n = (int) w[5];
//    float frac;
//    int index1, index2;
//    float m1, m2;
    short record_flag = x->record_flag;
    short play_flag = x->play_flag ;
    short dump_flag = x->dump_flag ;
    short loop_flag = x->loop_flag ;
    int length = x->length;
    int iphase = x->iphase;
    int lastphase = x->lastphase;
    float phase = x->phase;
    float *data = x->data;
    float si = x->si;
    float speed = x->speed;
    float sample;
    short in_connected = x->in_connected;
    float fval = x->fval;
/*********************/

while( n-- ){
	if( in_connected ){
		sample = *in++ ;
	} else {
		sample = fval;
	}
	if( record_flag ){
		iphase = phase;
/*		phase += (si * speed); Bug!! */
		phase += si;
		if( iphase >= length ){
			record_flag = 0;
			// post("end of recording at %d samples",length);
		}
		else if( iphase > lastphase ){
			lastphase = iphase ;
			data[ iphase ] = sample ;
		}
		*sync_out++ = phase / (float) length ;
		*out++ = sample ; // mirror input to output
	} else if ( play_flag ){
		iphase = phase;
		phase += (si * speed);
		if( iphase >= length ){
			play_flag = 0;
			*out++ = data[ length - 1 ]; // lock at final value
		} else if (iphase < 0 ) {
			play_flag = 0;
			*out++ = data[ 0 ]; // lock at first value
		}
		else {
			*out++ = data[ iphase ] ;
		}
		*sync_out++ = phase / (float) length ;		
	} 
	else if ( loop_flag ){
		iphase = phase;
		phase += (si * speed);
		if( iphase >= length ){
			phase = iphase = 0;
		} else if (iphase < 0 ) {
			phase = iphase = length - 1;
		}
		*out++ = data[ iphase ] ;
		*sync_out++ = phase / (float) length ;		

	}
	else if ( dump_flag ) {
		iphase = phase ;
		phase += 1.0 ;
		if( iphase >= length ){
			dump_flag = 0;
		} else {
			*out++ = data[ iphase ];
		}

	}
	
	else {
		*sync_out++ = 0.0 ;
		*out++ = 0.0;

	}
	x->phase = phase;
	x->lastphase = lastphase;
	x->record_flag = record_flag;
	x->play_flag = play_flag;

}
// DSP CONFIG
return (w+6);
}

void *kbuffer_new(t_symbol *s, int argc, t_atom *argv)
{
#if __MSP__
	t_kbuffer *x = (t_kbuffer *)newobject(kbuffer_class);
	dsp_setup((t_pxobject *)x,1);
	outlet_new((t_object *)x, "signal");
	outlet_new((t_object *)x, "signal");
#endif
#if __PD__
  t_kbuffer *x = (t_kbuffer *)pd_new(kbuffer_class);
  outlet_new(&x->x_obj, gensym("signal"));
  outlet_new(&x->x_obj, gensym("signal"));
#endif

	x->srate = sys_getsr();
	if( x->srate == 0 ){
		error("zero sampling rate - set to 44100");
		x->srate = 44100;
	}
	x->ksrate = atom_getfloatarg(0,argc,argv);
    x->duration = atom_getfloatarg(1,argc,argv)/1000.0;
	if(x->ksrate <= 0)
		x->ksrate = 128;
	if(x->duration <= 0)
		x->duration = 10.;
	
	kbuffer_init(x,0);   
 	return (x);
}

void kbuffer_init(t_kbuffer *x,short initialized)
{
	if(!initialized){
		x->record_flag = 0;	
		x->play_flag = 0;
		x->dump_flag = 0;
		x->loop_flag = 0;
		x->fval = 0;
		x->speed = 1.0 ;
 	  x->memsize = x->ksrate * x->duration * sizeof(float);
 	  x->length = x->duration * x->ksrate;
 	  x->data = (float *) calloc(x->memsize, sizeof(float));
	} 
    x->si = x->ksrate / x->srate;
}

void kbuffer_dsp(t_kbuffer *x, t_signal **sp, short *count)
{
	// DSP CONFIG
#if __MSP__
	x->in_connected = count[0];
#endif
#if __PD__
	x->in_connected = 1;
#endif

	if(x->srate != sp[0]->s_sr){
	  x->srate = sp[0]->s_sr;
	  kbuffer_init(x,1);
	}
   	dsp_add(kbuffer_perform, 5, x, 
    	sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
  
}



void kbuffer_assist(t_kbuffer *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal/float) Input ");
				break;
		}
	} else if (msg==2) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Output ");
				break;
			case 1:
				sprintf(dst,"(signal) Sync ");
				break;

		}

	}
}
#if __MSP__
void kbuffer_float(t_kbuffer *x, double f) // Look at floats at inlets
{
	int inlet = ((t_pxobject*)x)->z_in;
	
	if (inlet == 0)
	{
		x->fval = f;
		//post("fval set to %f", x->fval);
	}
}

void kbuffer_int(t_kbuffer *x, int i) // Look at floats at inlets
{
	int inlet = ((t_pxobject*)x)->z_in;
	
	if (inlet == 0)
	{
		x->fval = i;
		//post("ival set to %f", x->fval);
	}
}
#endif
