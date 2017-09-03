/* 
 cartospher : Max/MSP / Pd object translating 3D cartesian coordinates into
 spherical coordinates (3D equivalent of cartopol object).
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

class cartospher:
	public flext_base
{
	FLEXT_HEADER_S(cartospher,flext_base,setup)
 
public:
	// constructor with no arguments
	cartospher(int argc,t_atom *argv);
	~cartospher();

protected:

	void m_bang();
	void m_setx(double f);
	void m_sety(double f);
	void m_setz(double f);

	
	// variables for coordinates
	long double x_val, y_val, z_val;
	long double r_val, phi_val, theta_val;
	
private:
    
    // added inner_f variable to avoid precision issues   
    double inner_f;
	
	static void setup(t_classid c)
	{
		// --- set up methods (class scope) ---
		
		FLEXT_CADDBANG(c,0,m_bang);
		FLEXT_CADDMETHOD(c,0,m_setx);

		// set up methods for inlets 1 and 2
		// no message tag used
		FLEXT_CADDMETHOD(c,1,m_sety);	// variable arg type recognized automatically
		FLEXT_CADDMETHOD(c,2,m_setz);	// single int or float arg also recognized automatically
		
	}
	
	// for every registered method a callback has to be declared
	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK_F(m_setx)
	FLEXT_CALLBACK_F(m_sety)
	FLEXT_CALLBACK_F(m_setz)

};

// instantiate the class (constructor has a variable argument list)
// let "counter" be an alternative name
// after the colon define the path/name of the help file (a path has a trailing /, a file has not)
FLEXT_NEW_V("cartospher - jl 2012",cartospher)

// _____________________________ CONSTRUCTOR / DESTRUCTOR _______________________________

cartospher::cartospher(int argc,t_atom *argv):
x_val(0.f), y_val(0.f), z_val(0.f), r_val(0.f), phi_val(0.f), theta_val(0.f), inner_f(1000)
{
	// --- define inlets and outlets ---
	
	//AddInFloat("x data");
    AddInAnything("x data");
	AddInFloat("y data");
	AddInFloat("z data");
	
	AddOutFloat("r data");
	AddOutFloat("phi data");
	AddOutFloat("theta data");
}

cartospher::~cartospher()
{

}

void cartospher::m_bang()
{
	r_val = sqrtl(x_val*x_val + y_val*y_val + z_val*z_val);
    // avoid dividing by zero
	phi_val = (r_val == 0) ? 0 : acosl(z_val / r_val);
    if(x_val == 0 && y_val == 0)
    {
        theta_val = 0;
    }
    else
    {
        theta_val = (y_val >= 0)
        ? acosl(x_val / sqrtl(x_val*x_val + y_val*y_val))
        : (2*M_PI - acosl(x_val / sqrtl(x_val*x_val + y_val*y_val)));
    }
	    
	ToOutFloat(2, (double) theta_val);
	ToOutFloat(1, (double) phi_val);
	ToOutFloat(0, (double) (r_val/inner_f));
}

//________________________________ (R,PHI,THETA) DATA IN _________________________________

void cartospher::m_setx(double f)
{
	x_val = (long double)f;
    x_val *= inner_f;
	m_bang();
}

void cartospher::m_sety(double f)
{
	y_val = (long double)f;
    y_val *= inner_f;
}

void cartospher::m_setz(double f)
{
	z_val = (long double)f;
    z_val *= inner_f;
}

//_________________________________________________________________________________
//________________________________ END OF OBJECT __________________________________
//_________________________________________________________________________________