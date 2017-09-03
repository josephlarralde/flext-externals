/* 
 magnetize : Max/MSP / Pd object distorting continuous space by defining
 scale degrees as attractors. The attraction coefficient is a parameter,
 so you can continuously change the look of the transfer function from
 flat to "stairs-like".
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
#include <cmath>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 501)
#error You need at least flext version 0.5.1
#endif

class magnetize:
	public flext_base
{
	FLEXT_HEADER_S(magnetize,flext_base,setup)
 
public:
	// constructor with no arguments
	magnetize(int argc,t_atom *argv);
	~magnetize();

protected:

	void m_bang();
	void m_setvalue(float f);
	void m_setroot(float f);
	void m_setattract(float f);
	void m_setpattern(int argc, const t_atom *argv);

	float outputvalue;
	float root;
	float attraction;
	
	float *pattern;
	long patternlen;
	float patternsize;
	
private:
	
	static void setup(t_classid c)
	{
		// --- set up methods (class scope) ---
		
		FLEXT_CADDBANG(c,0,m_bang);
		FLEXT_CADDMETHOD(c,0,m_setvalue);
		FLEXT_CADDMETHOD_(c,0,"pattern",m_setpattern);
		FLEXT_CADDMETHOD_F(c,0,"root",m_setroot);
		FLEXT_CADDMETHOD_F(c,0,"attraction",m_setattract);
	}
	
	// for every registered method a callback has to be declared
	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK_F(m_setvalue)
	FLEXT_CALLBACK_F(m_setroot)
	FLEXT_CALLBACK_F(m_setattract)
	FLEXT_CALLBACK_V(m_setpattern)

};

FLEXT_NEW_V("magnetize",magnetize)

// _____________________________ CONSTRUCTOR / DESTRUCTOR _______________________________

magnetize::magnetize(int argc,t_atom *argv):
root(60), attraction(0)
{
	/*
	int i;
	pattern = new float[12];
	for(i=0;i<12;i++)
	{
		pattern[i] = 1;
	}
	*/
	pattern = new float[1];
	pattern[0] = 1;
	patternlen = 1;
	patternsize = 1;
	
	// --- define inlets and outlets ---
	
	AddInAnything("float input + config messages");
	
	AddOutFloat("magnetized output");
}

magnetize::~magnetize()
{
	delete pattern;
}

void magnetize::m_bang()
{	
	ToOutFloat(0, outputvalue);
}

//________________________________ MESSAGES _________________________________

void magnetize::m_setvalue(float f)
{
	int i;
	float floating_root = root;
	float normpos;
	
	// set floating root below input value
	while(f<floating_root)
	{
		floating_root -= patternsize;
	}
	while(f>=floating_root+patternsize)
	{
		floating_root += patternsize;
	}
	// look for bounding degrees in pattern
	for(i=0;i<patternlen;i++)
	{
		if(f>=floating_root && f<floating_root+pattern[i])
		{
			// HERE WE ARE
			normpos = (f - floating_root) / pattern[i];
			if(normpos < 0.5)
			{
				if(attraction==1)
				{
					outputvalue = floating_root;
				}
				else
				{
					normpos = pow(normpos*2,1/(1-attraction));
					outputvalue = normpos * pattern[i]/2 + floating_root;
				}
			}
			else
			{
				if(attraction==1)
				{
					outputvalue = floating_root+pattern[i];
				}
				else
				{
					normpos = 1 - pow(1-(normpos-0.5)*2,1/(1-attraction));
					outputvalue = normpos * pattern[i]/2 + floating_root + pattern[i]/2;
				}
			}
			break;

		}
		else
		{
			floating_root+=pattern[i];
		}
	}
		
	m_bang();
}

void magnetize::m_setroot(float f)
{
	root = f;
}

void magnetize::m_setattract(float f)
{
	attraction = f;
}

void magnetize::m_setpattern(int argc, const t_atom *argv)
{
	int i;
	
	if(argc>0)
	{
		for(i=0;i<argc;i++)
		{
			if(!CanbeFloat(argv[i]))
			{
				post("magnetize : pattern message needs only float args");
				return;
			}
		}
		
		pattern = new float[argc];
		patternlen = argc;
		patternsize = 0;
		
		for(i=0;i<argc;i++)
		{
			pattern[i] = GetAFloat(argv[i]);
			patternsize += GetAFloat(argv[i]);
		}
	}
	else
	{
		post("magnetize : pattern message needs only float args");
		return;
	}
}

//_________________________________________________________________________________
//________________________________ END OF OBJECT __________________________________
//_________________________________________________________________________________