/**
	@file
	
	map.param
	
	@ingroup	nogroup	
*/

#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object

////////////////////////// object struct
typedef struct _mapparam 
{
	t_object ob;// the object itself (must be first)
	double value;
	double scale_value;
	double high;
	double low;
	double factor;
	long sine;
	long mode;
	long hot;
	void *outlet_1;
} t_mapparam;

///////////////////////// function prototypes
//// standard set
void *mapparam_new(t_symbol *s, long argc, t_atom *argv);
void mapparam_free(t_mapparam *x);
void mapparam_assist(t_mapparam *x, void *b, long m, long a, char *s);
//void mapparam_int(t_mapparam *x, long n);
void mapparam_float(t_mapparam *x, double f);
void mapparam_value(t_mapparam *x, double f);
void mapparam_high(t_mapparam *x, double f);
void mapparam_low(t_mapparam *x, double f);
void mapparam_factor(t_mapparam *x, double f);
void mapparam_sine(t_mapparam *x, long n);
void mapparam_mode(t_mapparam *x, long n);
void mapparam_hot(t_mapparam *x, long n);
void mapparam_bang(t_mapparam *x);
//void mapparam_list(t_mapparam *x, t_symbol *s, long ac, t_atom *av);


//////////////////////// global class pointer variable
void *mapparam_class;


int main(void)
{	
	// object initialization, OLD STYLE
	// setup((t_messlist **)&mapparam_class, (method)mapparam_new, (method)mapparam_free, (short)sizeof(t_mapparam), 
	//		0L, A_GIMME, 0);
    // addmess((method)mapparam_assist,			"assist",		A_CANT, 0);  
	
	// object initialization, NEW STYLE
	t_class *c;
	
	c = class_new("map.param", (method)mapparam_new, (method)mapparam_free, (long)sizeof(t_mapparam), 
				  0L /* leave NULL!! */, A_GIMME, 0);
	
	/* you CAN'T call this from the patcher */
    class_addmethod(c, (method)mapparam_assist,				"assist",			A_CANT, 0);
	//class_addmethod(c, (method)mapparam_int,					"int",				A_LONG, 0);
	class_addmethod(c, (method)mapparam_float,				"float",			A_FLOAT, 0);
	class_addmethod(c, (method)mapparam_value,				"value",			A_FLOAT, 0);
	class_addmethod(c, (method)mapparam_high,				"high",				A_FLOAT, 0);
	class_addmethod(c, (method)mapparam_low,				"low",				A_FLOAT, 0);
	class_addmethod(c, (method)mapparam_factor,				"factor",			A_FLOAT, 0);
	class_addmethod(c, (method)mapparam_sine,				"sine",				A_LONG, 0);
	class_addmethod(c, (method)mapparam_mode,				"mode",				A_LONG, 0);
	class_addmethod(c, (method)mapparam_hot,				"hot",				A_LONG, 0);
	class_addmethod(c, (method)mapparam_bang,				"bang",				0);
	//class_addmethod(c, (method)mapparam_list,				"list",				A_GIMME, 0);

	//CLASS_ATTR_LONG(c, "sine", 0, t_mapparam, sine);
	//CLASS_ATTR_LONG(c, "hot", 0, t_mapparam, hot);
	
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	mapparam_class = c;

	return 0;
}

void mapparam_assist(t_mapparam *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { // inlet
		sprintf(s, "I am inlet %ld", a);
	} 
	else {	// outlet
		sprintf(s, "I am outlet %ld", a); 			
	}
}

void mapparam_free(t_mapparam *x)
{
	;
}

/*
 A_GIMME signature =
	t_symbol	*s		objectname
	long		argc	num additonal args
	t_atom		*argv	array of t_atom structs
		 type = argv->a_type
		 if (type == A_LONG) ;
		 else if (type == A_FLOAT) ;
		 else if (type == A_SYM) ;
*/
/*
	t_symbol {
		char *s_name;
		t_object *s_thing;
	}
*/

void *mapparam_new(t_symbol *s, long argc, t_atom *argv)
{
	t_mapparam *x = NULL;
    
	// object instantiation, OLD STYLE
	// if (x = (t_mapparam *)newobject(mapparam_class)) {
	// 	;
	// }
	
	// object instantiation, NEW STYLE
	if (x = (t_mapparam *)object_alloc(mapparam_class)) {

		//floatin(x,1);
		x->outlet_1 = floatout((t_object *)x);
		
		/*
		object_post((t_object *)x, "value is %f", atom_getfloat(&x->val));
        object_post((t_object *)x, "a new %s object was instantiated: 0x%X", s->s_name, x);
        object_post((t_object *)x, "it has %ld arguments - youhouuuuuu je suis nouveau", argc);
		*/
		
		x->scale_value = x->value = x->low = x->mode = x->sine = x->hot = 0;
		x->factor = x->high = 1;
		
		/*
		if((atom_gettype(argv)==A_FLOAT || atom_gettype(argv)==A_LONG)
		&& atom_gettype(argv+1)!=A_FLOAT && atom_gettype(argv+1)!=A_LONG) {
			switch (atom_gettype(argv)) {
				case A_FLOAT:
					if(atom_getfloat(argv) < 0)
						x->factor = 0;
					else x->factor = atom_getfloat(argv);
					break;
				case A_LONG:
					if(atom_getlong(argv) < 0)
						x->factor = 0;
					else x->factor = (double)atom_getlong(argv);
					break;
				default:
					break;
			}
		}
		*/
	}
	return (x);
}

//_________________________FUNCTIONS________________________________//

/*
void mapparam_int(t_mapparam *x, long n)
{
	x->val = (double)n;
	mapparam_bang(x);
}
*/

void mapparam_float(t_mapparam *x, double f)
{
	switch(x->mode) {
		case 1:
			x->scale_value = f;
			mapparam_bang(x);
			break;
		case 2:
			outlet_float(x->outlet_1, f);
			break;
		default:
			break;
	}
}

void mapparam_value(t_mapparam *x, double f)
{
	x->value = f;
	if(x->mode == 0) {
		outlet_float(x->outlet_1, x->value);
	}
}

void mapparam_high(t_mapparam *x, double f)
{
	x->high = f;
	if(x->mode == 1 && x->hot == 1) {
		mapparam_bang(x);
	}
}

void mapparam_low(t_mapparam *x, double f)
{
	x->low = f;
	if(x->mode == 1 && x->hot == 1) {
		mapparam_bang(x);
	}
}

void mapparam_factor(t_mapparam *x, double f)
{
	x->factor = f;
	if(x->mode == 1 && x->hot == 1) {
		mapparam_bang(x);
	}
}

void mapparam_sine(t_mapparam *x, long n)
{
	x->sine = (n==0) ? 0 : 1;
	if(x->mode == 1 && x->hot == 1) {
		mapparam_bang(x);
	}
}

void mapparam_mode(t_mapparam *x, long n)
{
	x->mode = (n==1) ? 1 : ((n==2) ? 2 : 0);
	if(x->mode == 0) {
		mapparam_bang(x);
	}
}

void mapparam_hot(t_mapparam *x, long n)
{
	x->hot = (n==0) ? 0 : 1;
}

void mapparam_bang(t_mapparam *x)
{
	//double minout2, maxout2, minin2, maxin2; //intermediary
	double a, b, res = 0;
	
	if(x->mode == 0) {
		outlet_float(x->outlet_1, x->value);
	}
	
	else if(x->mode == 1) {
		//a = (x->maxout-x->minout) / (x->maxin-x->minin); //pente
		//b = x->maxout - (a * x->maxin);
		
		/*
		a = 1 / (x->maxin - x->minin);
		b = 1 - (a * x->maxin);
		res = x->val * a + b;
		res = pow(res, x->factor);
		if(x->sine == 1) {
			a = M_PI;
			b = M_PI / 2 - a;
			res = sin(res * a + b);
			res *= 0.5;
			res += 0.5;
		}
		a = (x->maxout-x->minout);
		b = x->maxout - a;
		res = res * a + b;
		*/
				
		if(x->sine == 1) {
			//a = M_PI / (x->maxin - x->minin);
			//b = M_PI / 2 - (a * x->maxin);
			res = cos((1 - x->scale_value) * M_PI);
			res *= 0.5;
			res += 0.5;
		}
		else {
			//a = 1 / (x->maxin - x->minin);
			//b = 1 - (a * x->maxin);
			res = x->scale_value;// * a + b;
		}
		res = pow(res, x->factor);
		a = (x->high-x->low);
		b = x->high - a;
		res = res * a + b;

		outlet_float(x->outlet_1, res);
	}
	
}
