#include "MSPd.h"

/* comment */

static t_class *function_class;
#define OBJECT_NAME "function~"

typedef struct _function
{
    t_object x_obj;
    float x_f;
    t_symbol *wavename; // name of waveform buffer
    
    long b_frames;
    long b_nchans;
    float *b_samples;
    short normalize;
} t_function;

void function_setbuf(t_function *x, t_symbol *wavename);
void *function_new(t_symbol *msg, short argc, t_atom *argv);
void function_dsp(t_function *x, t_signal **sp);
void function_redraw(t_function *x);
void function_clear(t_function *x);
void function_addsyn(t_function *x, t_symbol *msg, short argc, t_atom *argv);
void function_aenv(t_function *x, t_symbol *msg, short argc, t_atom *argv);
void function_adenv(t_function *x, t_symbol *msg, short argc, t_atom *argv);
void function_normalize(t_function *x, t_floatarg f);
void function_adrenv(t_function *x, t_symbol *msg, short argc, t_atom *argv);
void function_rcos(t_function *x);
void function_gaussian(t_function *x);


void function_tilde_setup(void)
{
    function_class = class_new(gensym("function~"), (t_newmethod)function_new,
                               NO_FREE_FUNCTION,sizeof(t_function), 0,A_GIMME,0);
    CLASS_MAINSIGNALIN(function_class, t_function, x_f);
    class_addmethod(function_class,(t_method)function_dsp,gensym("dsp"),0);
    class_addmethod(function_class,(t_method)function_addsyn,gensym("addsyn"),A_GIMME,0);
    class_addmethod(function_class,(t_method)function_aenv,gensym("aenv"),A_GIMME,0);
    class_addmethod(function_class,(t_method)function_adenv,gensym("adenv"),A_GIMME,0);
    class_addmethod(function_class,(t_method)function_adrenv,gensym("adrenv"),A_GIMME,0);
    class_addmethod(function_class,(t_method)function_clear,gensym("clear"),0);
    class_addmethod(function_class,(t_method)function_normalize,gensym("normalize"),A_FLOAT,0);
    class_addmethod(function_class,(t_method)function_rcos,gensym("rcos"),0);
    class_addmethod(function_class,(t_method)function_gaussian,gensym("gaussian"),0);
    potpourri_announce(OBJECT_NAME);
}

void function_rcos(t_function *x)
{
    int i;
    long b_frames = x->b_frames;
    float *b_samples = x->b_samples;
    function_setbuf(x, x->wavename);
    for(i=0;i<b_frames;i++){
        b_samples[i] = 0.5 - 0.5 * cos(TWOPI * (float)i/(float)b_frames);
    }
    function_redraw(x);
}

void function_gaussian(t_function *x)
{
    int i;
    long b_frames = x->b_frames;
    float *b_samples = x->b_samples;
    float arg, xarg,in;
    
    if(!b_frames){
        post("* zero length function!");
        return;
    }
    function_setbuf(x, x->wavename);
    arg = 12.0 / (float)b_frames;
    xarg = 1.0;
    in = -6.0;
    
    for(i=0;i<b_frames;i++){
        b_samples[i] = xarg * pow(2.71828, -(in*in)/2.0);
        in += arg;
    }
    function_redraw(x);
}

void function_normalize(t_function *x, t_floatarg f)
{
    x->normalize = (short)f;
}

void function_clear(t_function *x)
{
    int i;
    long b_frames = x->b_frames;
    float *b_samples = x->b_samples;
    
    function_setbuf(x, x->wavename);
    b_frames = x->b_frames;
    b_samples = x->b_samples;
    for(i=0;i<b_frames;i++)
        b_samples[i] = 0.0;
    function_redraw(x);
}

void function_adrenv(t_function *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i,j;
    int al, dl, sl, rl;
    long b_frames = x->b_frames;
    float *b_samples = x->b_samples;
    float downgain = 0.33;
    
    function_setbuf(x, x->wavename);
    al = (float) b_frames * atom_getfloatarg(0,argc,argv);
    dl = (float) b_frames * atom_getfloatarg(1,argc,argv);
    rl = (float) b_frames * atom_getfloatarg(2,argc,argv);
    downgain = atom_getfloatarg(3,argc,argv);
    if(downgain <= 0)
        downgain = 0.333;
    if(al+dl+rl >= b_frames){
        post("atk and dk and release are too long");
        return;
    }
    sl = b_frames - (al+dl+rl);
    
    for(i=0;i<al;i++){
        b_samples[i] = (float)i/(float)al;
    }
    for(i=al, j=dl;i<al+dl;i++,j--){
        b_samples[i] = downgain + (1.-downgain)*(float)j/(float)dl;
    }
    for(i=al+dl;i<al+dl+sl;i++){
        b_samples[i] = downgain;
    }
    for(i=al+dl+sl,j=rl;i<b_frames;i++,j--){
        b_samples[i] = downgain * (float)j/(float)rl;
    }
    function_redraw(x);
}

void function_adenv(t_function *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i,j;
    int al, dl, rl;
    long b_frames = x->b_frames;
    float *b_samples = x->b_samples;
    float downgain = 0.33;
    
    function_setbuf(x, x->wavename);
    al = (float) b_frames * atom_getfloatarg(0,argc,argv);
    dl = (float) b_frames * atom_getfloatarg(1,argc,argv);
    downgain = atom_getfloatarg(2,argc,argv);
    if(downgain <= 0)
        downgain = 0.333;
    if(al+dl >= b_frames){
        post("atk and dk are too long");
        return;
    }
    rl = b_frames - (al+dl);
    
    for(i=0;i<al;i++){
        b_samples[i] = (float)i/(float)al;
    }
    for(i=al, j=dl;i<al+dl;i++,j--){
        b_samples[i] = downgain + (1.-downgain)*(float)j/(float)dl;
    }
    for(i=al+dl,j=rl;i<b_frames;i++,j--){
        b_samples[i] = downgain * (float)j/(float)rl;
    }
    function_redraw(x);
}

void function_aenv(t_function *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i,j;
    int al, dl;
    long b_frames = x->b_frames;
    float *b_samples = x->b_samples;
    float frac;
    frac = atom_getfloatarg(0,argc,argv);
    
    function_setbuf(x, x->wavename);
    if(frac <= 0 || frac >= 1){
        post("* attack time must range from 0.0 - 1.0, rather than %f",frac);
    }
    
    al = b_frames * frac;
    
    dl = b_frames - al;
    for(i=0;i<al;i++){
        b_samples[i] = (float)i/(float)al;
    }
    for(i=al, j=dl;i<b_frames;i++,j--){
        b_samples[i] = (float)j/(float)dl;
    }
    function_redraw(x);
}

void function_addsyn(t_function *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i,j;
    long b_frames = x->b_frames;
    float *b_samples = x->b_samples;
    float amp;
    float maxamp, rescale;
    
    function_setbuf(x, x->wavename);
    amp = atom_getfloatarg(0,argc,argv);
    for(i=0;i<b_frames;i++){
        b_samples[i] = amp;
    }
    for(j=1;j<argc;j++){
        amp = atom_getfloatarg(j,argc,argv);
        if(amp){
            for(i=0;i<b_frames;i++){
                b_samples[i] += amp * sin(TWOPI * (float) j * (float)i/(float)b_frames);
            }
        }
    }
    if(x->normalize){
        maxamp = 0;
        for(i=0;i<b_frames;i++){
            if(maxamp < fabs(b_samples[i]))
                maxamp = fabs(b_samples[i]);
        }
        if(!maxamp){
            post("* zero maxamp!");
            return;
        }
        rescale = 1.0 / maxamp;
        for(i=0;i<b_frames;i++){
            b_samples[i] *= rescale;
        }
    }
    function_redraw(x);
}


void *function_new(t_symbol *msg, short argc, t_atom *argv)
{
    
    t_function *x = (t_function *)pd_new(function_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->wavename = atom_getsymbolarg(0,argc,argv);
    x->normalize = 1;
    
    return x;
}

void function_setbuf(t_function *x, t_symbol *wavename)
{
    int frames;
    t_garray *a;
    
    x->b_frames = 0;
    x->b_nchans = 1;
    if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
        if (*wavename->s_name) pd_error(x, "function~: %s: no such array", wavename->s_name);
    }
    else if (!garray_getfloatarray(a, &frames, &x->b_samples)) {
        pd_error(x, "%s: bad template for function~", wavename->s_name);
    }
    else  {
        x->b_frames = frames;
        garray_usedindsp(a);
    }
}

void function_redraw(t_function *x)
{
    t_garray *a;
    if (!(a = (t_garray *)pd_findbyclass(x->wavename, garray_class))) {
        if (*x->wavename->s_name) pd_error(x, "function~: %s: no such array", x->wavename->s_name);
    }
    else  {
        garray_redraw(a);
    }
}



void function_dsp(t_function *x, t_signal **sp)
{
}

