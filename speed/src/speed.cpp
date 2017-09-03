/*
 speed : Max/MSP / Pd object doing real-time speed estimation on a
 variable number of dimensions.
 Copyright (C) 2012 Joseph Larralde
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.  
 */

// include flext header
#include <flext.h>
#include <new>
#include <math.h>

#define MAX_MAX_WSIZE 120
#define DEFAULT_MAX_WSIZE 20
#define DEFAULT_WSIZE 12
#define MAX_NDIM 9
#define DEFAULT_NDIM 1
#define DEFAULT_FACTOR 1.

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 501)
#error You need at least flext version 0.5.1
#endif

class speed:
	public flext_base
{
	FLEXT_HEADER_S(speed,flext_base,setup)
 
public:
	
	speed(int argc,t_atom *argv);
	~speed();

    bool CbMethodResort(int inlet,const t_symbol *s,int argc,const t_atom *argv); 

protected:

	void m_timer(void *);
	void m_bang();
	void m_dump();
	void m_nopoll();
	void m_poll(int i);
	void m_list(int argc, const t_atom *argv);	
	void m_wsize(int i);	
	void m_factor(double f);
	
	Timer tmr;
	int ndim, interval, running, wsize, maxwsize;
	
    double *vals;
    double *prev_vals;
    
    double factor;
    
    double **windows;
    double *sp_window;

private:
	
	float smooth(double factor, int wsize, double *data);
	
	static void setup(t_classid c)
	{
		FLEXT_CADDBANG(c,0,m_bang);
		FLEXT_CADDMETHOD(c,0,m_list);
		
		FLEXT_CADDMETHOD_(c,0,"dump",m_dump);
		FLEXT_CADDMETHOD_(c,0,"nopoll",m_nopoll);
		FLEXT_CADDMETHOD_I(c,0,"poll",m_poll);
		FLEXT_CADDMETHOD_I(c,0,"wsize",m_wsize);
		FLEXT_CADDMETHOD_F(c,0,"factor",m_factor);
	}
	
	FLEXT_CALLBACK_T(m_timer)

	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK(m_dump)
	FLEXT_CALLBACK(m_nopoll)
	FLEXT_CALLBACK_I(m_poll)
	FLEXT_CALLBACK_I(m_wsize)
	FLEXT_CALLBACK_F(m_factor)
	FLEXT_CALLBACK_V(m_list)
};

// instantiate the class (constructor has a variable argument list)
// let "counter" be an alternative name
// after the colon define the path/name of the help file (a path has a trailing /, a file has not)
FLEXT_NEW_V("speed, jl 2012",speed)

// _____________________________ CONSTRUCTOR / DESTRUCTOR _______________________________

speed::speed(int argc,t_atom *argv):
tmr(false), running(0), interval(0)
{
    int i,j;
    
    // arguments must be specified in this order
    ndim = DEFAULT_NDIM;
    wsize = DEFAULT_WSIZE;
    factor = DEFAULT_FACTOR;
    maxwsize = DEFAULT_MAX_WSIZE;
    
    if(argc>=1 && CanbeInt(argv[0]))
    {
        ndim = GetAInt(argv[0]);
        ndim = (ndim<1) ? 1 : ((ndim>MAX_NDIM) ? MAX_NDIM : ndim);
        
        if(argc>=2 && CanbeInt(argv[1]))
        {
            wsize = GetAInt(argv[1]);
            wsize = (wsize>MAX_MAX_WSIZE) ? MAX_MAX_WSIZE : wsize;
            maxwsize = (maxwsize<wsize) ? wsize : maxwsize;
            
            if(argc>=3 && CanbeFloat(argv[2]))
            {
                factor = GetAFloat(argv[2]);
                factor = (factor<0.) ? 0. : factor;
                
                if(argc>=4 && CanbeInt(argv[3]))
                {
                    if(GetAInt(argv[3])>maxwsize)
                    {
                        maxwsize = GetAInt(argv[3]);
                        maxwsize = (maxwsize>MAX_MAX_WSIZE) ? MAX_MAX_WSIZE : maxwsize;
                    }
                }

            }
        }
    }
    
	// --- define inlets and outlets ---
	
	AddInAnything("msgs, float (dim1 data stream), or list (listlength == ndim)");    
    for(i=0; i<ndim-1; i++) AddInFloat("dim speed data input stream");
    for(i=0; i<ndim; i++) AddOutFloat("dim speed data output stream");
	AddOutAnything("speed vector norm data stream, dumpout");
	
	FLEXT_ADDTIMER(tmr, m_timer);
	
    vals = new double[ndim];
    prev_vals = new double[ndim];
    sp_window = new double[maxwsize];
    for(i=0; i<maxwsize; i++) { sp_window[i] = 0.; }
    
    windows = new double *[ndim];
    for(i=0; i<ndim; i++) {
        windows[i] = new double[maxwsize];
        for(j=0; j<maxwsize; j++) { windows[i][j] = 0.; }
        vals[i] = 0.;
        prev_vals[i] = 0.;
    }
}

speed::~speed()
{
    int i;
    delete vals;
    delete prev_vals;
    delete sp_window;
    
    for(i=0; i<ndim; i++) { delete windows[i]; }
	delete windows;
}

//____________________________________ CALLBACK ____________________________________

void speed::m_timer(void *)
{
	//_____ " tick " function _____________ :
	m_bang();
}

void speed::m_bang()
{
	// ROLL !!!
	int i,j;
    double squares = 0.;
    
    for(i=0; i<ndim; i++)
    {
        for(j=maxwsize-1; j>0; j--)
        {
            windows[i][j] = windows[i][j-1];
        }
        
        windows[i][0] = vals[i] - prev_vals[i];
        prev_vals[i] = vals[i];
        squares += windows[i][0] * windows[i][0];
    }
    
    for(j=maxwsize-1; j>0; j--)
    {
        sp_window[j] = sp_window[j-1];
    }
    sp_window[0] = sqrt(squares);
	
    // output speed data from right to left
    ToOutFloat(ndim,smooth(factor, wsize, sp_window));
    for(i=ndim-1; i>=0; i--)
    {
        ToOutFloat(i,smooth(factor, wsize, windows[i]));
    }
}

//______________________________ SMOOTHING FUNCTION ______________________________

float speed::smooth(double f, int w, double *data)
{
	double res = 0.0; // Žchantillon lissŽ
	double sum=0.0; // somme des coefficients de pondŽration
	double coeff=0.0; // coefficient de pondŽration
	
	int i; // variable de dŽcrŽmentation pour le dŽplacement dans la fentre
	
	for (i=(w-1); i>=0; i--)
	{   
		coeff = pow((double)(w-i)/(double)w, f); // calcul du coefficient pour l'Žchantillon i
		res+= *(data+i) * coeff ; // pondŽration de l'Žchantillon i par "coeff"
		sum+= coeff; // mise ˆ jour de la somme des coeffs de pondŽration
	}
	
	return res/sum; // division de l'ensemble par la somme des coeffs de pondŽration
}	

//________________________________ CONFIG ETC ____________________________________

void speed::m_dump()
{
    post("TODO");
    /*
	post("--");
	post("data_wsize : %i samples", data_wsize);
	post("data_factor : %f", data_factor);
	post("----------------------");
	post("speed_wsize : %i samples", speed_wsize);
	post("speed_factor : %f", speed_factor);
	post("----------------------");
	post("accel_wsize : %i samples", accel_wsize);
	post("accel_factor : %f", accel_factor);
	post("----------------------");
	post("polling interval : %i ms", interval);
	post("--");
    */
}

void speed::m_nopoll()
{
	tmr.Reset();
	running = 0;
	interval = 0;
	post("nopoll");
}

void speed::m_poll(int i)
{
	int j = (i>=1) ? i : 1;
	tmr.Periodic(j*0.001);
	running = 1;
	interval = j;
	post("polling interval : %i ms", j);
}

// _____________________________ SET WINDOWS SIZE _______________________________

void speed::m_wsize(int i)
{
	int wsize = (i>maxwsize) ? maxwsize : ((i<1) ? 1 : i);
}

// ____________________________ SET PONDERATION FACTOR ___________________________

void speed::m_factor(double f)
{
	factor = f<0 ? 0 : f;
}

//___________________________________ DATA IN ____________________________________

void speed::m_list(int argc, const t_atom *argv)
{
    int i,cnt;
    const int dims = (const int) ndim;
    
	if(argc == 1) {
		if(CanbeFloat(argv[0])) {
			vals[0] = GetAFloat(argv[0]);
		} else {
			post("1 arg must be a float");
        }
    }
    else if(argc == ndim) {
        for(i=0,cnt=0; i<ndim; i++)
        {
           if(CanbeFloat(argv[i])) { cnt++; }
        }
        if(cnt==ndim)
        {
            for(i=0; i<ndim; i++)
            {
                vals[i] = GetAFloat(argv[i]);
            }                
        }
        else
        {
            post("%i args must be %i floats", ndim);
        }
    }
    else {
        post("only 1 float or %i floats list recognized", ndim);
	}
}

bool speed::CbMethodResort(int inlet,const t_symbol *s,int argc,const t_atom *argv)
{
    if(inlet>0) { vals[inlet] = GetAFloat(argv[0]); }
    /*
    post("inlet %ld used with %ld arg(s)", inlet, argc);
    if(argc == 1 && CanbeFloat(argv[0]))
        post("arg is : %f", GetAFloat(argv[0]));
    */
    return(argc == 1);
}

//_________________________________________________________________________________
//________________________________ END OF OBJECT __________________________________
//_________________________________________________________________________________