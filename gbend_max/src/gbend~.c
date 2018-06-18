//
//  gbend~.c
//  gbend~
//
//  Created by Joseph Larralde on 03/11/14.
//
//

#include "ext.h"
#include "z_dsp.h"
#include "math.h"
#include "buffer.h"
#include "ext_atomic.h"
#include "ext_obex.h"

typedef struct _gbend
{
  t_pxobject w_obj;
  t_buffer_ref *w_buf;
  t_symbol *w_name;
  short w_connected[2];
  t_bool w_buffer_modified;
    void *endloop; // outlet to notify of selection play ending
      
    t_symbol *w_nextname; // added to prevent changing buf while playing
    t_bool w_changebuf;   // flag to tell if buffer has to be changed to w_nextname on next trig
    
    long a_channel_offset;
    char a_autoselectwholebuf;
    char a_changeplayingbuf;
  
  long rvs;
  long true_rvs;

  // set to 0 if receive "stop" message while loop playing to keep loop flag true
  // set to 1 automatically on trig if loop == true
  long loop;
  long true_loop;
  
    // "s" prefix is for "sample" (values in samples computed from duration and samplerate)
  double fadi;
  double sfadi;
  double fado;
  double sfado;
  double interrupt;
  double sinterrupt;
  double pitch;
  double cpitch;
  
  // FLAGS:
  long interrupting;
  long playing;
  long playing2;
  long stopping;
    long true_stopping; // if selection ends playing without a stop message
  
  double playindex;
  //double stopindex;
  double interruptindex;

  // as the names say
    double *sindexarray;
  //double *sindexarrayL;
    //double *sindexarrayR;
  
  // remember last pitch sample value from last signal input vector
  // to compute first sample index
  double lastlastindex;
  
  // boolean
  long firstvector; 
  
  float w_start;
  float w_end;

  long w_begin;
  // this one is used if the sound isn't stopped before its end
  long w_len;
  // the real length param to take into account (updated when stop msg received)
  long w_stoplen;

  float w_msr;
  long w_nchans;
  //long w_bframes;
  //long w_bmodtime;
  long blksize;
  //short connected;
  short silence;
    
} t_gbend;


// __________ new style dsp object interface :
t_max_err gbend_notify(t_gbend *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void gbend_assist(t_gbend *x, void *b, long m, long a, char *s);
void gbend_dblclick(t_gbend *x);
void gbend_set(t_gbend *x, t_symbol *s, long ac, t_atom *av);
void gbend_perform64(t_gbend *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void gbend_dsp64(t_gbend *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

// __________ object messages :
void gbend_bang(t_gbend *x);
void gbend_stop(t_gbend *x);
void gbend_pitch(t_gbend *x, double f);

// fade-in and fade-out values are defined in ms for a playing speed of 1 (pitch == 0)
// they are bound to the pitch signal input value
// they are like mixed to selection before pitch bending

// sets same value for fade-in and fade-out
void gbend_fade(t_gbend *x, double f);
// sets fade-in value
void gbend_fadi(t_gbend *x, double f);
// sets fade-out value
void gbend_fado(t_gbend *x, double f);
// sets interrupt value in ms (this one is independent from pitch value)
void gbend_interrupt(t_gbend *x, double f);
void gbend_beg(t_gbend *x, double f);
void gbend_end(t_gbend *x, double f);
void gbend_loop(t_gbend *x, long n);
void gbend_rvs(t_gbend *x, long n);

// __________ private functions :
void gbend_computeparams(t_gbend *x);
void gbend_selectwholebuffer(t_gbend *x);
void *gbend_new(t_symbol *s,  long argc, t_atom *argv);
void gbend_free(t_gbend *x);

static t_symbol *ps_buffer_modified;
static t_class *s_gbend_class;

int C74_EXPORT main(void)
{
  t_class *c = class_new("gbend~", (method)gbend_new, (method)gbend_free, sizeof(t_gbend), NULL, A_GIMME, 0);
  
    class_addmethod(c, (method)gbend_dsp64,   "dsp64",        A_CANT, 0);
    class_addmethod(c, (method)gbend_set,   "set",      A_GIMME, 0);
    class_addmethod(c, (method)gbend_notify,  "notify",       A_CANT, 0);
    class_addmethod(c, (method)gbend_assist,  "assist",   A_CANT, 0);
    class_addmethod(c, (method)gbend_dblclick,  "dblclick",   A_CANT, 0);
  class_addmethod(c, (method)gbend_bang,    "bang",     0);
  class_addmethod(c, (method)gbend_stop,    "stop",     0);
    class_addmethod(c, (method)gbend_pitch,   "pitch",    A_FLOAT, 0);
    class_addmethod(c, (method)gbend_fade,    "fade",     A_FLOAT, 0);
    class_addmethod(c, (method)gbend_fadi,    "fadi",     A_FLOAT, 0);
    class_addmethod(c, (method)gbend_fado,    "fado",     A_FLOAT, 0);
    class_addmethod(c, (method)gbend_interrupt, "interrupt",  A_FLOAT, 0);
    class_addmethod(c, (method)gbend_beg,   "beg",      A_FLOAT, 0);
    class_addmethod(c, (method)gbend_end,   "end",      A_FLOAT, 0);
    class_addmethod(c, (method)gbend_loop,    "loop",     A_LONG, 0);
    class_addmethod(c, (method)gbend_rvs,   "rvs",      A_LONG, 0);

    CLASS_ATTR_LONG(c, "channel_offset", 0, t_gbend, a_channel_offset);
    //CLASS_ATTR_FILTER_CLIP(c, "channel_offset", 0, 1023);
    CLASS_ATTR_CHAR(c, "auto_select_whole_buffer", 0, t_gbend, a_autoselectwholebuf);
    CLASS_ATTR_CHAR(c, "change_buffer_while_playing", 0, t_gbend, a_changeplayingbuf);
    CLASS_ATTR_STYLE(c, "auto_select_whole_buffer", 0, "onoff");
    CLASS_ATTR_STYLE(c, "change_buffer_while_playing", 0, "onoff");

    class_dspinit(c);
  class_register(CLASS_BOX, c);
  s_gbend_class = c;
  
  ps_buffer_modified = gensym("buffer_modified");
    post("gbend~ - a versatile audio sample player - joseph larralde, 2014", 0);
  return 0;
}

void *gbend_new(t_symbol *s, long argc, t_atom *argv)
{
  t_gbend *x = (t_gbend *)object_alloc(s_gbend_class);
  t_symbol *buf = gensym("");
    int chans = 1;
  float start=0., end=0.; 
  dsp_setup((t_pxobject *)x,1); // <=> 1 SIGNAL INLET
  // x->w_phase = 0;
    
    // with this one can specifiy nb of channels and / or buffer name in any order (args types will be detected)
    switch (atom_gettype(argv)) {
    
        case A_SYM:
            buf = atom_getsym(argv);
            if(argc > 1) {
                switch(atom_gettype(argv+1)) {
                    case A_LONG:
                        chans = (int) atom_getlong(argv+1);
                        break;
                    case A_FLOAT:
                        chans = (int) atom_getfloat(argv+1);
                        break;
                    default:
                        break;
                }
            }
            break;
            
        case A_LONG:
            chans = (int) atom_getlong(argv);
            if(argc > 1) {
                switch(atom_gettype(argv+1)) {
                    case A_SYM:
                        buf = atom_getsym(argv+1);
                        break;
                    default:
                        break;
                }
            }
            break;
            
        case A_FLOAT:
            chans = (int) atom_getfloat(argv);
            if(argc > 1) {            
                switch(atom_gettype(argv+1)) {
                    case A_SYM:
                        buf = atom_getsym(argv+1);
                        break;
                    default:
                        break;
                }
            }
            break;
            
        default:
            break;
    }
    
  x->w_name = buf;
  x->w_buf = buffer_ref_new((t_object *)x, x->w_name);
    
    x->w_nextname = gensym("");
    x->w_changebuf = false;
    
  x->w_msr = sys_getsr() * 0.001;
  x->w_start = start;
  x->w_end = end;
  x->w_begin = start * x->w_msr;
  x->w_len = (end - start) * x->w_msr;
  x->w_nchans = (chans < 1) ? 1 : ((chans > 32) ? 32 : chans); // clip between 1 and 32 outlets
    // (if loaded file has less channels than the object has audio outlets, last channel will be
    // duplicated in lasting outlets)
    if(argc == 0) {
        x->w_nchans = 1;
    }


    x->a_channel_offset = 0;
    x->a_autoselectwholebuf = 0;
    x->a_changeplayingbuf = 1;
  
  x->pitch = 0;
  x->fadi = 3;
  x->fado = 3;
  x->interrupt = 3;
  x->rvs = 0;
  x->loop = 0;
    x->true_loop = 0;
    
  //x->silence = 1;
  //x->playing2 = 0;
    
  x->interrupting = 0;
    x->playing = 0;
    x->stopping = 0;
    x->true_stopping = 0;
    
  //x->interruptindex = x->playindex = x->stopindex = 0;

  x->blksize = sys_getblksize();
  
  x->sindexarray = (double *)sysmem_newptr(/*x->blksize*/ 4096 * sizeof(double));
    
    // notification outlet <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    x->endloop = intout((t_object *)x);

    for(int i=0; i<x->w_nchans; i++) {
        // audio outlets <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        outlet_new((t_object *)x, "signal");
    }
    
    attr_args_process(x, argc, argv);
    /*
    if(x->a_channel_offset >= x->w_nchans) {
                x->a_channel_offset = x->w_nchans - 1;
    }
    */

  return (x);
}

void gbend_free(t_gbend *x)
{
  dsp_free((t_pxobject*)x);
  
  // must free our buffer reference when we will no longer use it
  object_free(x->w_buf);
}

void gbend_set(t_gbend *x, t_symbol *s, long ac, t_atom *av)
{
  t_symbol *name;
  //double start, end;
  
  name = (ac) ? atom_getsym(av) : gensym("");

    if(x->a_changeplayingbuf) {
        buffer_ref_set(x->w_buf, name); // change the buffer used by our buffer reference
        gbend_computeparams(x);
    } else {
        x->w_nextname = name;
        x->w_changebuf = true;
    }
}

// A notify method is required for our buffer reference
// This handles notifications when the buffer appears, disappears, or is modified.
t_max_err gbend_notify(t_gbend *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
  if (msg == ps_buffer_modified)
    x->w_buffer_modified = true;
  return buffer_ref_notify(x->w_buf, s, msg, sender, data);
}

void gbend_assist(t_gbend *x, void *b, long m, long a, char *s)
{ 
  if (m == ASSIST_INLET) { // inlets
    switch (a) {
      case 0: snprintf_zero(s, 256, "(signal) Real-time detune in semitones");  break;
    }
  } 
  else { // outlet
        if(a >= x->w_nchans) {
            snprintf_zero(s, 256, "(int) 0 : natural stop, 1 : manual stop, 2 : juste looped");
        } else {
            snprintf_zero(s, 256, "(signal) Output %ld", a+1);
        }
  }
}

void gbend_dblclick(t_gbend *x)
{
  buffer_view(buffer_ref_getobject(x->w_buf));
}

void gbend_computeparams(t_gbend *x)
{

    ///////////////////////////////////// NEW GBEND CODE !!!

  t_buffer_obj *b = buffer_ref_getobject(x->w_buf); // get the actual buffer object from our reference
  
  if (b) {
    // number of floats long the buffer is for a single channel
        t_atom_long framecount   = buffer_getframecount(b);
        
    // sample rate of the buffer in samples per millisecond
        double    msr      = buffer_getmillisamplerate(b);
    
    if((x->w_end < x->w_start && x->rvs == 0) || (x->w_start < x->w_end && x->rvs == 1)) {
      x->true_rvs = 1;
    }
    else {
      x->true_rvs = 0;
    }
  
        double tmp;
    if(x->w_end < x->w_start) {
      tmp = x->w_end;
    }
    else {
      tmp = x->w_start;
    }

    //_____MINIMUM_SELECTION_1_MS______________
    
    x->w_begin = (long) (tmp * msr);        
    x->w_begin = (long) ((x->w_begin < 0) ? 0
                            : ((x->w_begin > framecount - x->blksize) ? (framecount - x->blksize)
                                : x->w_begin)
                            );

    x->w_len = (long) (fabs(x->w_end - x->w_start) * msr);
    x->w_len = (x->w_len > x->blksize) ? x->w_len : x->blksize;
    x->w_stoplen = x->w_len;
    
    x->sfadi = (long) (x->fadi * x->w_msr);
    x->sfado = (long) (x->fado * x->w_msr);
    x->sinterrupt = (long) (x->interrupt * x->w_msr);
    
    while(x->sfadi + x->sfado >= x->w_len) {
      if(x->sfadi > x->sfado) {
        x->sfadi--;
      }
      else {
        x->sfado--;
      }
      
    }
    
    if(x->sinterrupt > x->w_len - x->sfadi) {
      x->sinterrupt = x->w_len - x->sfadi;
    }
    
    x->pitch = x->cpitch;
    
    }
}

void gbend_selectwholebuffer(t_gbend *x) {

  t_buffer_obj *b = buffer_ref_getobject(x->w_buf); // get the actual buffer object from our reference
  
  if (b) {
    // number of floats long the buffer is for a single channel
        t_atom_long framecount   = buffer_getframecount(b);
        
    // sample rate of the buffer in samples per millisecond
        double    msr      = buffer_getmillisamplerate(b);
        
        x->w_start = 0;
        x->w_end = framecount * msr;
    }
}

// ___________________________ MESSAGES TO GBEND~ OBJECT ______________________________ //

void gbend_bang(t_gbend *x)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) { // trig sample portion play
    
        /*
        t_buffer_obj *b = buffer_ref_getobject(x->w_buf);
        if (!b) {
            x->interrupting = x->playing2 = 0;
            //x->playing2 = 0; // avoid playing when selecting a full buffer after trying to play an empty buffer
            //return;
        }
        //*/
    
    if(!x->interrupting) {
      x->interrupting = 1;
      x->interruptindex = 0;
      x->playing2 = 1;
      //x->stopping = 0;
        
      if(!x->playing) {
        //post("set playing on");
        x->playing = 1;
        //x->playindex = 0;
        x->silence = 1;
        
        // if true_loop has been disabled by stop msg, we set it again to 1 :
        if(x->loop == 1) {
          x->true_loop = 1;
        }
      }       
    }
  //}
}
  
void gbend_stop(t_gbend *x)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) { // stop playing using fado duration
    if(!x->stopping) {
            x->stopping = 1;
            x->true_stopping = 1;
      x->true_loop = 0;
    }
  //}
}
  
void gbend_pitch(t_gbend *x, double f)
{
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->cpitch = (f < -120) ? -120 : ((f>120) ? 120 : f);
  //}
}
  
void gbend_fade(t_gbend *x, double f)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->fadi = x->fado = (f<1) ? 1 : f;
  //}
}
  
void gbend_fadi(t_gbend *x, double f)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->fadi = (f<1) ? 1 : f;
  //}
}
  
void gbend_fado(t_gbend *x, double f)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->fado = (f<1) ? 1 : f;
  //}
}
  
void gbend_interrupt(t_gbend *x, double f)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->interrupt = (f<1) ? 1 : f;
  //}
}
  
void gbend_beg(t_gbend *x, double f)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->w_start = f;
  //}
}
  
void gbend_end(t_gbend *x, double f)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->w_end = f;
  //}
}
  
void gbend_loop(t_gbend *x, long n)
{
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->loop = x->true_loop = (n==0) ? 0 : 1;
  //}
}
  
void gbend_rvs(t_gbend *x, long n)
{ 
  //if (proxy_getinlet((t_object *)x) == 0) {
    x->rvs = (n==0) ? 0 : 1;
  //}
}

/////////////////////////////// A U D I O // S T U F F /////////////////////////////////

void gbend_perform64(t_gbend *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    t_double    *in = ins[0];

    int       n = sampleframes;
  int             ntmp = n;
    float     *buf, *fp;

    t_buffer_obj  *buffer = buffer_ref_getobject(x->w_buf);
  t_atom_long   channelcount;
    t_atom_long     framecount;
    
    long index;
  long maxindex;// = x->w_bframes - 3;
  double rel_index, ival, fval; // ival for interrupting, fval for fade value
  double frac, a, b, c, d, /*va, vb, vc, vd,*/ cminusb; //, vcminusvb;
  
  double *ref_sindexarray = x->sindexarray;

  
  buf = buffer_locksamples(buffer);
  if (!buf) {
        //x->playing = 0; // avoid playing when selecting a full buffer after trying to play an empty buffer
    goto zero;
    }
  
  channelcount = buffer_getchannelcount(buffer);
    framecount = buffer_getframecount(buffer);
    maxindex = framecount - 3;
    
    
    
  //buf = x->w_buf->b_samples;
  //framesize = x->w_buf->b_nchans;
  /*
  if ((x->w_bframes != x->w_buf->b_frames) || (x->w_bmodtime != x->w_buf->b_modtime)) {   // if buffer has changed  
    gbend_setup(x,x->w_name,sys_getsr(),x->blksize);
    gbend_computeparams(x);
    //post("buffer has changed");   
  }
    //*/
  
  if(x->stopping) {
    x->w_stoplen = (*(x->sindexarray) + x->sfado < x->w_stoplen) ? (*(x->sindexarray) + x->sfado) : x->w_stoplen;
    x->stopping = 0;
  }
    
        
    if(x->playing) {
        
        // ________ TRANSLATION FROM PITCH SIGNAL ARRAY (OF SIZE BLKSIZE) TO INDEX SIGNAL ARRAY (OF SAME SIZE) ________ //
        // ________________________________ INITIAL CONDITIONS + AUDIO LOOP 1 _________________________________________ //
        
        // _____________________________________ INITIAL CONDITIONS ___________________________________________________ //
        
        // ___________________________________________ AUDIO LOOP 1 ___________________________________________________ //

        while (ntmp--) {
            
            *(x->sindexarray) = (x->w_connected[0])
            ? (exp((*in++ + x->pitch) * 0.057762265) + ((ntmp == n-1) ? x->lastlastindex : *((x->sindexarray) - 1)))
            : (exp(x->pitch * 0.057762265) + ((ntmp == n-1) ? x->lastlastindex : *((x->sindexarray) - 1)));

            if(x->firstvector) { // THIS ONE IS SET TO 1 WHEN x->playing IS SET TO 1 AFTER BEING AT 0
                
                // INDEX SIGNAL GETS 1 SAMPLE DELAYED FOR CONTINUITY
                *(x->sindexarray) = x->lastlastindex= 0;      
                x->firstvector = 0;
            }

            if(ntmp == 0) { // LAST SIG VECT VAL
                x->lastlastindex = *(x->sindexarray);
            }
            
            // READ SAMPLES AND FADES ACCORDING TO THE SIGNAL INDEX ARRAY + BILINEAR INTERPOLATION

            //////////////////////////////////////////
            //                                      //
            //           CRITICAL SECTION           //
            //                                      //
            //////////////////////////////////////////
            
            // ______ CHECK IF REVERSE _______________________________________________________________//
            if(x->playing) {
                if(x->true_rvs == 0) {
                    rel_index = *(x->sindexarray); // the index according to the beginning of selection
                }
                else {
                    rel_index = x->w_stoplen - 1 - *(x->sindexarray); // the same in reverse
                }
            }
            else {
                rel_index = 0;
            }
            
            // ____ FIRST SET INTERRUPT FADE VALUE ___________________________________________________//
            if(x->interrupting) {
                
                //post("interrupt");
                //post("%f", rel_index);

                if(x->interruptindex < x->sinterrupt) {
                    ival = x->silence ? 0 : (double) (1 - ((x->interruptindex + 1) / x->sinterrupt));
                    x->interruptindex++;
                }
                else {
                    ival = 0;
                }
            }
            else ival = 1;
            

            // ___ HERE WE KNOW IF WE'LL BE READING TOO FAR : _________________________________________//
            
            if(x->playing && !x->interrupting && (rel_index > x->w_stoplen || rel_index < 0)) { // we're reading too far ...
                
                //post("%f : <<<< rel_index", rel_index);
                
                if(x->true_loop == 1) { // ... so we loop (never after stop msg received)
                
                    gbend_computeparams(x);
                    //post("looping");
                    x->firstvector = 1;
                    outlet_int(x->endloop, 2); // --------> send value 2 out rightmost outlet (we just looped)
                }
                else {
                    x->playing = 0; // ... so we stop
                    ival = 0;

                    if(x->true_stopping == 0) {
                        outlet_int(x->endloop, 1);
                    }
                    else {
                        outlet_int(x->endloop, 0);
                        x->true_stopping = 0;
                    }
                }
            }
            
            index = floor(x->w_begin + rel_index); // rounded down by conversion from double to long
            //rel_index = (long) rel_index;
            
            if (index < 1) {
                index = 1;
                frac = 0;
            }
            else if (index > maxindex) {
                index = maxindex, frac = 1;
            }
            else {
                frac = (x->w_begin + rel_index) - index;
            }
            
            ////////////////////////////////////////////////////////// compute each channel's audio vector
            
            for(int ch=0; ch<x->w_nchans; ch++) {

                if(ch >= channelcount) {
    
                    outs[ch][n - ntmp - 1] = outs[ch - channelcount][n - ntmp - 1];

                } else {
        
                    //t_double *out = outs[ch];

                    fp = buf + ((ch + x->a_channel_offset) % channelcount) + index * channelcount; // here is the channel offset offset (yes, double !)
            
                    // GET NEIGHBOUR VALUES FOR 4-POINT INTERPOLATION
                    a = (index==0) ? 0 : fp[-1 * channelcount];
                    b = fp[0];
                    c = fp[1 * channelcount];
                    d = fp[2 * channelcount];
                        
            
                    // LET'S FIND ANOTHER WAY TO COMPUTE THE OUTPUT VOLUME ACCORDING TO FADES :
                    // (INTERPOLATING OUTPUT VOLUME CAUSES AUDIO CLICS BECAUSE IT DOESN'T REACH ZERO ANY TIME)
            
                    fval = (rel_index < x->sfadi)
                            ? ((rel_index < 0)
                                ? 0
                                : (rel_index / x->sfadi))
                            : ((rel_index > x->w_stoplen - x->sfado)
                                ? ((rel_index > x->w_stoplen)
                                    ? 0
                                    : ((x->w_stoplen - rel_index) / x->sfado))
                                : 1
                            );
                                 
                    cminusb = c - b; // 8-)
            
                    /// ********************* BILINEAR INTERPOLATION ****************************/
             
                    //*out++ = (index < framecount * channelcount) ? (float)
                    outs[ch][n - ntmp -1] = (index < framecount) ? (float)
                        // GET INTERPOLATED SAMPLE VALUE
                        (
                            (
                                b + frac * (
                                            cminusb - 0.1666667f * (1.-frac) *
                                            ((d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b))
                                            )
                            )
                        // THEN MULTIPLY IT BY THE INTERPOLATED VOLUME VALUE
                        /*
                            *
                            (
                                vb + frac * (
                                            vcminusvb - 0.1666667f * (1.-frac) *
                                            ((vd - va - 3.0f * vcminusvb) * frac + (vd + 2.0f*va - 3.0f*vb))
                                            )
                            )
                        */
                    
                        // THEN BY THE INTERRUPT AND FADE VALUES
                        * ival * fval
                    
                    ) : 0; // zero padding
                }
            } // end of FOR loop iterating to fill all audio outputs
             
            x->sindexarray++;

            /// ********************** END OF BILINEAR INTERPOLATION ********************/

            
            //__________________THIS_IS_THE_ONLY_GOOD_PLACE_TO_TRIG_SAMPLE_PLAYING______________
            
            // AN EXCEPTION :
            //in case of end playing and retrig during the same signal vector
            //we use the variable playing2 set up to 1 in the bang() function
            
            if(!x->interrupting && x->playing2) {
                if(x->w_changebuf) {
                  buffer_ref_set(x->w_buf, x->w_nextname);  // change the buffer used by our buffer reference
                    x->w_changebuf = false;
                }
                if(x->a_autoselectwholebuf) {
                    gbend_selectwholebuffer(x);
                }
                gbend_computeparams(x);
                x->playing = 1;
                //x->playindex = 0;
                x->playing2 = 0;
            }
            
            // MOST GENERAL CASE :
            
            if(x->interrupting && (x->interruptindex == x->sinterrupt)) { //interrupt is just finished
                x->interrupting = 0;
                x->interruptindex = 0;
                x->silence = 0;
                x->firstvector = 1;
            
                //post("trig");
                if(x->w_changebuf) {
                  buffer_ref_set(x->w_buf, x->w_nextname);  // change the buffer used by our buffer reference
                    x->w_changebuf = false;
                    if(x->a_autoselectwholebuf) {
                        gbend_selectwholebuffer(x);
                    }
                    if(x->playing == 0) {
                        gbend_computeparams(x);
                    }
                }
                
                if(x->playing) {
                    gbend_computeparams(x);
                    x->playing = 1;
                    //x->playindex = 0;
                }
            }
                        
        } //______________________________________ END OF AUDIO LOOP 2 _____________________________________// (while(ntmp--))
    
        x->sindexarray = ref_sindexarray;
    }
    
    else { // NOT PLAYING
        for(int ch=0; ch<x->w_nchans; ch++) {
    
            t_double *out = outs[ch];
            ntmp = n;
    
            while (ntmp--) {
                *out++ = 0.;
            }
        }
        
        if(x->w_changebuf) {
            buffer_ref_set(x->w_buf, x->w_nextname);  // change the buffer used by our buffer reference
            x->w_changebuf = false;
            if(x->a_autoselectwholebuf) {
                gbend_selectwholebuffer(x);
            }
            gbend_computeparams(x);
        }
    }
    
  buffer_unlocksamples(buffer);
  return;
    
    
zero:
    for(int ch=0; ch<x->w_nchans; ch++) {
        
        t_double *out = outs[ch];
        ntmp = n;
        
        while (ntmp--) {
            *out++ = 0.;
        }
    }
    
    //*
    if(x->w_changebuf) {
        buffer_ref_set(x->w_buf, x->w_nextname);  // change the buffer used by our buffer reference
        x->w_changebuf = false;
        if(x->a_autoselectwholebuf) {
            gbend_selectwholebuffer(x);
        }
        gbend_computeparams(x);
    }
    //*/
}

////////////////////////////////////////// [ D S P ] /// [ 6 4 ] /////////////////////////////////////////////

void gbend_dsp64(t_gbend *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
  x->w_connected[0] = count[1];
  x->w_connected[1] = count[2];
  object_method(dsp64, gensym("dsp_add64"), x, gbend_perform64, 0, NULL);
}