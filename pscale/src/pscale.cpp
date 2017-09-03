/* 
 pscale : same as max/msp scale object with integrated transfer function.
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
#include <cmath>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 501)
#error You need at least flext version 0.5.1
#endif

class pscale:
	public flext_base
{
	FLEXT_HEADER_S(pscale,flext_base,setup)
 
public:

	pscale(int argc,t_atom *argv);
	~pscale();

protected:

	void m_bang();
	void m_float(double f);
	void m_setminin(double f);
	void m_setmaxin(double f);
	void m_setminout(double f);
	void m_setmaxout(double f);
	void m_setfactor(double f);
	void m_setsigmoid(int i);

	// variables for coordinates
	double val, minin, maxin, minout, maxout, pow, res;
    int sine;
	
private:
    
	static void setup(t_classid c);
	
	// for every registered method a callback has to be declared
	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK_F(m_float)
	FLEXT_CALLBACK_F(m_setminin)
	FLEXT_CALLBACK_F(m_setmaxin)
	FLEXT_CALLBACK_F(m_setminout)
	FLEXT_CALLBACK_F(m_setmaxout)
	FLEXT_CALLBACK_F(m_setfactor)
	FLEXT_CALLBACK_F(m_setsigmoid)

};

FLEXT_NEW_V("pscale",pscale)

// _____________________________ CONSTRUCTOR / DESTRUCTOR _______________________________

pscale::pscale(int argc,t_atom *argv):
val(0.f), minin(0.f), maxin(1.f), minout(0.f), maxout(1.f), pow(1.f), res(0.f),
sine(0)
{
    switch(argc) {
        case 1:
            if(CanbeFloat(argv[0])) {
                pow = GetAFloat(argv[0]);
            }
            break;
            
        case 2:
            if(CanbeFloat(argv[0])) {
                pow = GetAFloat(argv[0]);
            }
            if(CanbeInt(argv[1])) {
                sine = (GetAInt(argv[1]) != 0) ? 1 : 0;
            }
            break;
            
        default:
            if(argc >= 4) {
                if(CanbeFloat(argv[0])) {
                    minin = GetAFloat(argv[0]);
                }
                if(CanbeFloat(argv[1])) {
                    maxin = GetAFloat(argv[1]);
                }
                if(CanbeFloat(argv[2])) {
                    minout = GetAFloat(argv[2]);
                }
                if(CanbeFloat(argv[3])) {
                    maxout = GetAFloat(argv[3]);
                }
                
                if(argc >= 5) {
                    if(CanbeFloat(argv[4])) {
                        pow = GetAFloat(argv[4]);
                    }
                    
                    if(argc >= 6) {
                        if(CanbeInt(argv[5])) {
                            sine = (GetAInt(argv[5]) != 0) ? 1 : 0;
                        }                        
                    }                   
                }
            }
            break;
    }
    
                
    // --- define inlets and outlets ---
	
	//AddInFloat("r data");
    AddInAnything("value to scale, sigmoid message");
	AddInFloat("min input bound");
	AddInFloat("max input bound");
	AddInFloat("min output bound");
	AddInFloat("max output bound");
	AddInFloat("reshaping (pow) factor");
	
	AddOutFloat("scaled value");
}

pscale::~pscale()
{

}

void pscale::setup(t_classid c)
{
    // --- set up methods (class scope) ---
    
    FLEXT_CADDBANG(c,0,m_bang);
    FLEXT_CADDMETHOD(c,0,m_float);
    FLEXT_CADDMETHOD_F(c,0,"sigmoid",m_setsigmoid);
    
    // set up methods for inlets 1 to 5
    // no message tag used
    FLEXT_CADDMETHOD(c,1,m_setminin);
    FLEXT_CADDMETHOD(c,2,m_setmaxin);    
    FLEXT_CADDMETHOD(c,3,m_setminout);
    FLEXT_CADDMETHOD(c,4,m_setmaxout);    
    FLEXT_CADDMETHOD(c,5,m_setfactor);
}

void pscale::m_bang()
{
	long double a, b;
	
    res = 0;
    if(maxin - minin != 0) {
            
        if(sine == 1)
        {
            a = M_PI / (maxin - minin);
            b = M_PI / 2 - (a * maxin);
            res = sin(val * a + b);
            res *= 0.5;
            res += 0.5;
        }
        else
        {
            a = 1 / (maxin - minin);
            b = 1 - (a * maxin);
            res = val * a + b;
        }
            
        res = powf(res, pow);
        a = (maxout - minout);
        b = maxout - a;
        res = res * a + b;
    }
    
	ToOutFloat(0, (double) res);
}

void pscale::m_float(double f)
{
	val = f;
	m_bang();
}

void pscale::m_setminin(double f)
{
	minin = f;
}

void pscale::m_setmaxin(double f)
{
	maxin = f;
}

void pscale::m_setminout(double f)
{
	minout = f;
}

void pscale::m_setmaxout(double f)
{
	maxout = f;
}

void pscale::m_setfactor(double f)
{
	pow = f;
}

void pscale::m_setsigmoid(int i)
{
	sine = (i!=0) ? 1 : 0;
}

//_________________________________________________________________________________
//________________________________ END OF OBJECT __________________________________
//_________________________________________________________________________________