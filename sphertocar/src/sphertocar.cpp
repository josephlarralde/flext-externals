/* 
 sphertocar : Max/MSP / Pd object translating 3D spherical coordinates
 into cartesian coordinates (3D equivalent of poltocar object).
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

#define EPSILON 10e-6

class sphertocar:
	public flext_base
{
	FLEXT_HEADER_S(sphertocar,flext_base,setup)
 
public:

	sphertocar(int argc,t_atom *argv);
	~sphertocar();

protected:

	void m_bang();
	void m_setr(double f);
	void m_setphi(double f);
	void m_settheta(double f);

	
	// variables for coordinates
	long double x_val, y_val, z_val;
	long double r_val, phi_val, theta_val;
	
private:
    
	static void setup(t_classid c);
	
	// for every registered method a callback has to be declared
	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK_F(m_setr)
	FLEXT_CALLBACK_F(m_setphi)
	FLEXT_CALLBACK_F(m_settheta)

};

FLEXT_NEW_V("sphertocar",sphertocar)

// _____________________________ CONSTRUCTOR / DESTRUCTOR _______________________________

sphertocar::sphertocar(int argc,t_atom *argv):
x_val(0.f), y_val(0.f), z_val(0.f), r_val(0.f), phi_val(0.f), theta_val(0.f)
{
	// --- define inlets and outlets ---
	
	//AddInFloat("r data");
    AddInAnything("r data");
	AddInFloat("phi data");
	AddInFloat("theta data");
	
	AddOutFloat("x data");
	AddOutFloat("y data");
	AddOutFloat("z data");
}

sphertocar::~sphertocar()
{

}

void sphertocar::setup(t_classid c)
{
    // --- set up methods (class scope) ---
    
    FLEXT_CADDBANG(c,0,m_bang);
    FLEXT_CADDMETHOD(c,0,m_setr);
    
    // set up methods for inlets 1 and 2
    // no message tag used
    FLEXT_CADDMETHOD(c,1,m_setphi);     // variable arg type recognized automatically
    FLEXT_CADDMETHOD(c,2,m_settheta);	// single int or float arg also recognized automatically
    
}

void sphertocar::m_bang()
{
	x_val = r_val * sinl(phi_val) * cosl(theta_val);
	y_val = r_val * sinl(phi_val) * sinl(theta_val);
	z_val = r_val * cosl(phi_val);

	//x_val = r_val * sin(phi_val) * cos(theta_val) * inner_f;
	//y_val = r_val * sin(phi_val) * sin(theta_val) * inner_f;
	//z_val = r_val * cos(phi_val) * inner_f;
    
    x_val  = (fabs(x_val) > EPSILON) ? x_val : 0.f;
    y_val  = (fabs(y_val) > EPSILON) ? y_val : 0.f;
    z_val  = (fabs(z_val) > EPSILON) ? z_val : 0.f;
	
	ToOutFloat(2, (double) z_val);
	ToOutFloat(1, (double) y_val);
	ToOutFloat(0, (double) x_val);
}

//________________________________ (R,PHI,THETA) DATA IN _________________________________

void sphertocar::m_setr(double f)
{
	r_val = (f<0) ? 0.0 : (long double)f;
	m_bang();
}

void sphertocar::m_setphi(double f)
{
	phi_val = (long double)f;
}

void sphertocar::m_settheta(double f)
{
	theta_val = (long double)f;
}

//_________________________________________________________________________________
//________________________________ END OF OBJECT __________________________________
//_________________________________________________________________________________