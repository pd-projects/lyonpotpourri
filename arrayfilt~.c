#include "MSPd.h"

static t_class *arrayfilt_class;

/* Pd version of arrayfilt~ */

#define OBJECT_NAME "arrayfilt~"
typedef struct _arrayfilt
{
	t_object x_obj;
    t_float x_f;
    t_float *a_samples;
    int a_frames;
} t_arrayfilt;

void *arrayfilt_new(t_symbol *msg, short argc, t_atom *argv);
void arrayfilt_dsp(t_arrayfilt *x, t_signal **sp);
void arrayfilt_setarray(t_arrayfilt *x, t_symbol *arrayname);

void arrayfilt_tilde_setup(void){
    arrayfilt_class = class_new(gensym("arrayfilt~"), (t_newmethod)arrayfilt_new,
                              0, sizeof(t_arrayfilt),0,A_GIMME,0);
	CLASS_MAINSIGNALIN(arrayfilt_class, t_arrayfilt, x_f);
    class_addmethod(arrayfilt_class, (t_method)arrayfilt_dsp, gensym("dsp"),0);
    potpourri_announce(OBJECT_NAME);
}

void *arrayfilt_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_arrayfilt *x = (t_arrayfilt *)pd_new(arrayfilt_class);
    t_symbol *arrayname;
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    arrayname = atom_getsymbolarg(0, argc, argv);
    arrayfilt_setarray(x,arrayname);
	return x;
}

void arrayfilt_setarray(t_arrayfilt *x, t_symbol *arrayname)
{
    t_garray *a;
	if (!(a = (t_garray *)pd_findbyclass(arrayname, garray_class))) {
		if (*arrayname->s_name) pd_error(x, "player~: %s: no such array", arrayname->s_name);
    }
	else  {
		garray_usedindsp(a);
        if (!garray_getfloatarray(a, &x->a_frames, &x->a_samples))
        {
            pd_error(x, "%s: bad template for player~", arrayname->s_name);
        }
	}
}

t_int *arrayfilt_perform(t_int *w)
{
    int i;
    t_arrayfilt *x = (t_arrayfilt *) w[1];
    t_float *mag_in = (t_float *) w[2];
    t_float *phase_in = (t_float *) w[3];
    t_float *mag_out = (t_float *) w[4];
    t_float *phase_out = (t_float *) w[5];
    t_float mag, phase;
    t_float *a_samples = x->a_samples;
    t_int a_frames = x->a_frames;
    int n = w[6];
    int N2 = n / 2;
    if(a_frames < N2+1) {
        goto exit;
    }
    for(i = 0; i < N2 + 1; i++){
        mag = mag_in[i];
        phase = phase_in[i];
        mag_out[i] = mag * a_samples[i];
        phase_out[i] = phase;
    }
exit:
    return (w + 7);
}

void arrayfilt_dsp(t_arrayfilt *x, t_signal **sp)
{
    dsp_add(arrayfilt_perform,6, x,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}
