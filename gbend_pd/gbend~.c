/*
 *  gbend~.c
 *  gbend~ object works with table object.
 *  It reads audio files and provides various utilities defined as control
 *  parameters such as : integrated fade in / out to avoid clics, reverse,
 *  loop, begin point, end point, etc.
 *
 *  Created by joseph larralde on 25/08/11.
 *  Copyright 2011 Joseph Larralde - All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#include "m_pd.h"
#include "math.h"

static t_class *gbend_tilde_class;

typedef struct _gbend_tilde
{
	//*********** AS IN TABREAD4 *********//
    t_object x_obj;
    int x_npoints;
	float arrayms;
    float *x_vec;
    t_symbol *x_arrayname;
    float x_f;
	
	//************** ADDED ***************//
	//MY VARS :
	float pitch;
	float cpitch;
	
	long rvs;
	long true_rvs;
	long loop;
	long true_loop;
	
	float fadi;
	float sfadi;
	float fado;
	float sfado;
	float interrupt;
	float sinterrupt;
	
	// FUNCTIONNAL FLAGS :
	long interrupting;
	long playing;
	long playing2;
	long stopping;
	long silence;
	long firstvector;
    long changetable;
	
	float interruptindex;
	float lastlastindex;
	
	float *sindexarray;
	
	long w_begin;
	long w_len;
	long w_stoplen;
	float w_start;
	float w_end;
	
	float w_msr;
	long w_blk;
	//*********** END ADDED *************//
	
} t_gbend_tilde;

//************** MY FUNCTIONS ***************//

void gbend_tilde_computeparams(t_gbend_tilde *x)
{
	float len, tmp;
		
	x->pitch = x->cpitch;
	
	len = (x->x_npoints - 3);
		
	if((x->w_end < x->w_start && x->rvs == 0) || (x->w_start < x->w_end && x->rvs == 1))
	{
		x->true_rvs = 1;
	}
	else
	{
		x->true_rvs = 0;
	}

	if(x->w_end < x->w_start)
	{
		tmp = x->w_end;
	}
	else
	{
		tmp = x->w_start;
	}
	
	//post("total sample size : %f", (float)len);
	
	//_____MINIMUM_SELECTION_1_MS_____
	x->w_begin = (long) (tmp * x->w_msr);
	x->w_begin = (long) ((x->w_begin < 0) ? 0 : ((x->w_begin > len - 2 * x->w_blk) ? (len - 2 * x->w_blk) : x->w_begin));

	x->w_len = (long) (fabs(x->w_end - x->w_start) * x->w_msr);
	x->w_len = (x->w_len > 2 * x->w_blk) ? x->w_len : 2 * x->w_blk;
    x->w_len = (x->w_begin + x->w_len <= len) ? x->w_len : (len - x->w_begin);
	x->w_stoplen = x->w_len;
		
	//************ now we have pitch, begin, len and true_rvs set up ***********//
	
	x->sfadi = (long) (x->fadi * x->w_msr);
	x->sfado = (long) (x->fado * x->w_msr);
	x->sinterrupt = (long) (x->interrupt * x->w_msr);
	
	//if selection too short, scale fades
    /*
	while(x->sfadi + x->sfado >= x->w_len)
	{
		if(x->sfadi < x->sfado)
        {
			x->sfadi--;
		}
		else
        {
			x->sfado--;
		}
	}
    */
    //faster fades scaling :
	if(x->sfadi + x->sfado >= x->w_len)
    {
        if(x->sfadi < x->w_len / 2) {
            x->sfado = x->w_len - x->sfadi;
        }
        else if(x->sfado < x->w_len / 2) {
            x->sfadi = x->w_len - x->sfado;
        }
        else {
            x->sfadi = x->sfado = x->w_len / 2;
        }
    }
    
	//if interrupt too long, shorten it
	if(x->sinterrupt > x->w_len - x->sfadi)
	{
		x->sinterrupt = x->w_len - x->sfadi;
	}
}

void gbend_tilde_set(t_gbend_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    
    if(x->playing)
    {
        x->changetable = 1;
        return;
    }
    
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name)
            pd_error(x, "gbend~: %s: no such array", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_npoints, &x->x_vec)) 
        // HERE WE GET LENGTH OF ARRAY
        // LET'S CALCULATE IN MILLISECONDS ... AND RETURN IT THRU RIGHT OUTLET.
    {
        pd_error(x, "%s: bad template for gbend~", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else
	{ 
		garray_usedindsp(a);
		//post("using array %s", x->x_arrayname->s_name);
		//x->arrayms = 
	}
}

void gbend_tilde_bang(t_gbend_tilde *x)
{
    if(x->x_arrayname == gensym(""))
    {
        //post("gbend~ : can't play - no associated table");
        return;
    }
    
    //post("table size : %i",x->x_npoints);
    if(x->x_npoints <= 100)
    {
        //post("gbend~ : can't play - table is empty");
        return;
    }
    
	//post("bangbang !!!");
	if(!x->interrupting)
	{
		x->interrupting = 1;
		x->interruptindex = 0;
		x->playing2 = 1;
		//x->stopping = 0;
		
		if(!x->playing)
		{
			x->playing = 1;
			x->silence = 1;
			 
			if(x->loop == 1) {
				x->true_loop = 1;
			}
		}
	}
}

void gbend_tilde_stop(t_gbend_tilde *x)
{
	if(!x->stopping)
	{
		x->stopping = 1;
		x->true_loop = 0;
	}
}

void gbend_tilde_pitch(t_gbend_tilde *x, t_floatarg f)
{
	x->cpitch = (f < -120) ? -120 : ((f>120) ? 120 : f);
}

void gbend_tilde_fade(t_gbend_tilde *x, t_floatarg f)
{
	x->fadi = x->fado = (f<1) ? 1 : f;
}

void gbend_tilde_fadi(t_gbend_tilde *x, t_floatarg f)
{
	x->fadi = (f<1) ? 1 : f;
}

void gbend_tilde_fado(t_gbend_tilde *x, t_floatarg f)
{
	x->fado = (f<1) ? 1 : f;
}

void gbend_tilde_interrupt(t_gbend_tilde *x, t_floatarg f)
{
	x->interrupt = (f<1) ? 1 : f;
}

void gbend_tilde_beg(t_gbend_tilde *x, t_floatarg f)
{
	x->w_start = f;
	//post("beg : %f", x->w_start);
}

void gbend_tilde_end(t_gbend_tilde *x, t_floatarg f)
{
	x->w_end = f;
	//post("end : %f", x->w_end);
}

void gbend_tilde_loop(t_gbend_tilde *x, t_floatarg f)
{
	x->loop = x->true_loop = (f==0) ? 0 : 1;
}

void gbend_tilde_rvs(t_gbend_tilde *x, t_floatarg f)
{
	x->rvs = (f==0) ? 0 : 1;
}

void gbend_tilde_setsr(t_gbend_tilde *x, t_floatarg f)
{
	x->w_msr = f / 1000;
}

// ****************************** END OF MY FUNCTIONS ****************************** //

void *gbend_tilde_new(t_symbol *s)
{
	int i;
	
    t_gbend_tilde *x = (t_gbend_tilde *)pd_new(gbend_tilde_class);
    x->x_arrayname = s;
    //if(x->x_arrayname == gensym("")) post("no table affected");
    
    x->x_vec = 0;
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
	x->w_msr = sys_getsr() / 1000;
	x->w_blk = 64;
	
	x->w_start = 0;
	x->w_end = 0;
	x->w_begin = 0;
	x->w_len = 0;

	x->pitch = x->cpitch = 0;
	x->fadi = 5;
	x->fado = 5;
	x->interrupt = 5;
	x->rvs = 0;
	x->loop = x->true_loop = 0;
	x->silence = 1;
	x->playing2 = 0;
	x->interrupting = x->playing = x->stopping = x->changetable = 0;
	x->interruptindex = 0;
	//x->interruptindex = x->playindex = x->stopindex = 0;
	
	x->sindexarray = (float *)getbytes(4096 * sizeof(float));
	for(i=0; i<4096; i++) {
		*(x->sindexarray + i) = 0;
	}
	
    gbend_tilde_set(x, s);
    
    return (x);
}

void gbend_tilde_free(t_gbend_tilde *x)
{
	freebytes(x->sindexarray, 4096 * sizeof(float));
}

// ____________________________ PERFORM FUNCTION ____________________________ //

t_int *gbend_tilde_perform(t_int *w)
{
    t_gbend_tilde *x = (t_gbend_tilde *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    int n = x->w_blk = (int)(w[4]); // VECTOR SIZE
	int ntmp = n;

    float *buf = x->x_vec, *fp;
    //int i = 0;
	long index;
	long maxindex = x->x_npoints - 3;
	float rel_index, ival, fval;
    float frac,  a,  b,  c,  d, cminusb;

	float *ref_sindexarray = x->sindexarray;
	
    if (!buf) goto zero;
/*
#if 0       //test for spam -- I'm not ready to deal with this
    for (i = 0,  xmax = 0, xmin = maxindex,  fp = in1; i < n; i++,  fp++)
    {
        float f = *in1;
        if (f < xmin) xmin = f;
        else if (f > xmax) xmax = f;
    }
    if (xmax < xmin + x->c_maxextent) xmax = xmin + x->c_maxextent;
    for (i = 0, splitlo = xmin+ x->c_maxextent, splithi = xmax - x->c_maxextent,
        fp = in1; i < n; i++,  fp++)
    {
        float f = *in1;
        if (f > splitlo && f < splithi) goto zero;
    }
#endif
*/
	
	if(x->stopping) {
		x->w_stoplen = (*(x->sindexarray) + x->sfado < x->w_stoplen) ? (*(x->sindexarray) + x->sfado) : x->w_stoplen;
		x->stopping = 0;
	}
	
if(x->playing)
{
    //for (i = 0; i < n; i++)
	while(ntmp--)
    {
		
		*(x->sindexarray) = exp((*in++ + x->pitch) * 0.057762265) + ((ntmp == n-1) ? x->lastlastindex : *((x->sindexarray) - 1));
		
		if(x->firstvector) {
			*(x->sindexarray) = x->lastlastindex = 0;
			x->firstvector = 0;
		}
		
		if(ntmp == 0) {
			x->lastlastindex = *(x->sindexarray);
		}
		
		// ___________ CHECK IF REVERSE ____________________________________ //
		if(x->playing) {
			if(x->true_rvs == 0) {
				rel_index = *(x->sindexarray);
			}
			else {
				rel_index = x->w_stoplen - 1 - *(x->sindexarray);
			}
		}
		else {
			rel_index = 0;
		}
		
		// ___________ SET INTERRUPT FADE VALUE ____________________________ //
		if(x->interrupting) {
			if(x->interruptindex < x->sinterrupt) {
				ival = x->silence ? 0 : (float) (1 - ((x->interruptindex + 1) / x->sinterrupt));
				x->interruptindex++;
			}
			else {
				ival = 0;
			}
		}
		else ival = 1;
		
		// ____________ CHECK IF PLAYING TOO FAR ___________________________ //
		if(x->playing && !x->interrupting && (rel_index > x->w_stoplen || rel_index < 0)) {
			if(x->true_loop == 1) {
				gbend_tilde_computeparams(x);
				x->firstvector = 1;
			}
			else {
				x->playing = 0;
				ival = 0;
			}
		}
		
		// ____________ SET INDEX & FRAC VALUES ____________________________ //
		index = floor(x->w_begin + rel_index);
		
		if(index < 1) {
			index = 1;
			frac = 0;
		}
		else if(index > maxindex) {
			index = maxindex, frac = 1;
		}
		else {
			frac = (x->w_begin + rel_index) - index;
		}
		
		fp = buf + index;
		
		a = (index==0) ? 0 : fp[-1];
		b = fp[0];
		c = fp[1];
		d = fp[2];
		
		fval = (rel_index < x->sfadi) ?
		((rel_index < 0) ? 0 : (rel_index / x->sfadi)) :
		((rel_index > x->w_stoplen - x->sfado) ?
		 ((rel_index > x->w_stoplen) ? 0 : ((x->w_stoplen - rel_index) / x->sfado)) :
		 1);
		
		cminusb = c - b;
		
		/************************ BILINEAR INTERPOLATION **************************/
		*out++ = (index < x->x_npoints) ? (float)
		(
		 (
		  b + frac * (
					  cminusb - 0.1666667f * (1.-frac) *
					  ((d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b))
					  )
		  )
		 * ival * fval
		 ) : 0;
		
		x->sindexarray++;
		/********************** END OF BILINEAR INTERPOLATION *********************/
		
		//in case of end playing and retrig during same signal vector
		//we use the variable playing2 set up to 1 in the bang() function
		if(!x->interrupting && x->playing2)
		{
			gbend_tilde_computeparams(x);
			x->playing = 1;
			x->playing2 = 0;
		}
		
		//most general cae :
		if(x->interrupting && (x->interruptindex == x->sinterrupt)) { //interrupt just ended
			x->interrupting = 0;
			x->interruptindex = 0;
			x->silence = 0;
			x->firstvector = 1;
			
			if(x->playing) {
				gbend_tilde_computeparams(x);
				x->playing = 1;
			}
		}
    } // end of while
	
	x->sindexarray = ref_sindexarray;
}//end of if x->playing
else
{
	// NOT PLAYING
	while (n--) *out++ = 0;
    
    if(x->changetable)
    {
        gbend_tilde_set(x, x->x_arrayname);
        x->changetable = 0;
    }
    
}
    return (w+5);

zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void gbend_tilde_dsp(t_gbend_tilde *x, t_signal **sp)
{
    gbend_tilde_set(x, x->x_arrayname);
    
    dsp_add(gbend_tilde_perform, 4, x,
            sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
    
}

void gbend_tilde_setup(void)
{
    gbend_tilde_class = class_new(gensym("gbend~"),
								  (t_newmethod)gbend_tilde_new, (t_method)gbend_tilde_free,
								  sizeof(t_gbend_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(gbend_tilde_class, t_gbend_tilde, x_f);
	class_addbang(gbend_tilde_class, (t_method)gbend_tilde_bang);
    class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_dsp,
					gensym("dsp"), 0);
    class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_set,
					gensym("set"), A_SYMBOL, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_stop,
					gensym("stop"), 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_pitch,
					gensym("pitch"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_fade,
					gensym("fade"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_fadi,
					gensym("fadi"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_fado,
					gensym("fado"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_interrupt,
					gensym("interrupt"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_beg,
					gensym("beg"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_end,
					gensym("end"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_rvs,
					gensym("rvs"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_loop,
					gensym("loop"), A_DEFFLOAT, 0);
	class_addmethod(gbend_tilde_class, (t_method)gbend_tilde_setsr,
					gensym("setsr"), A_DEFFLOAT, 0);
}
