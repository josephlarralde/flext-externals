//
//  hydra.cpp
//  hydra
//
//  Created by Joseph Larralde on 05/12/16.
//
//

/**
 *
 * @file hydra.cpp
 * @author joseph.larralde@ircam.fr
 *
 * @brief max interface object for the Sixense SDK (access to hydra / stem systems)
 *
 * Copyright (C) 2016 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 You should have received a copy of the GNU Lesser General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */

#include <sixense.h>
#include <sixense_math.hpp>
#include <sixense_utils/derivatives.hpp>
#include <sixense_utils/button_states.hpp>
#include <sixense_utils/event_triggers.hpp>
#include <sixense_utils/controller_manager/controller_manager.hpp>

#include "ext.h"
#include "ext_obex.h"
#include "ext_systhread.h"
#ifdef WIN32
#include <sys/select.h>
#endif
#include <stdio.h>
#include <queue>
#include <map>

#define DEFAULT_POLL_INTERVAL 20

// Global vars prevent other objects to interfere with devices currently in use.
// First object to start on a specific port gets the exclusive connection
// until it releases it.
bool hydra_busy;

/**
 * @todo add a method to control buffer queues sizes
 */

typedef struct _hydra {
  t_object p_ob;

  void     *m_poll;
  double   poll_interval;
  void     *p_outlet;
  
  bool     connected;
} t_hydra;

void hydra_connect(t_hydra *x, t_symbol *s, long argc, t_atom *argv);
void hydra_reset(t_hydra *x);
void hydra_disconnect(t_hydra *x);

void hydra_bang(t_hydra *x);
void hydra_clock(t_hydra *x);
void hydra_start(t_hydra *x, t_symbol *s, long argc, t_atom *argv);
void hydra_stop(t_hydra *x);
void hydra_poll(t_hydra *x, t_symbol *s, long argc, t_atom *argv);
//void hydra_poll(t_hydra *x);
void hydra_nopoll(t_hydra *x);
void hydra_assist(t_hydra *x, void *b, long m, long a, char *s);
void *hydra_new(t_symbol *s, long argc, t_atom *argv);
void hydra_free(t_hydra *x);
//================================ ATTRIBUTE GETTERS / SETTERS :
//t_max_err hydra_get_automatic(t_hydra *x, t_object *attr,
//                                 long *argc, t_atom **argv);
//t_max_err hydra_set_automatic(t_hydra *x, t_object *attr,
//                                 long argc, t_atom *argv);

t_symbol *ps_list;
t_class *hydra_class;


//--------------------------------------------------------------------------

// main method called only once in a Max session
int C74_EXPORT main(void)
{
  t_class *c;
  
  c = class_new("hydra", (method)hydra_new, (method)hydra_free,
                sizeof(t_hydra), 0L, A_GIMME, 0);
  
  // (optional) assistance method needs to be declared like this
  class_addmethod(c, (method)hydra_assist,      "assist",     A_CANT,   0);
  class_addmethod(c, (method)hydra_connect,     "connect",    A_GIMME,  0);
  class_addmethod(c, (method)hydra_reset,       "reset",                0);
  class_addmethod(c, (method)hydra_disconnect,  "disconnect",           0);
  class_addmethod(c, (method)hydra_bang,        "bang",                 0);
  class_addmethod(c, (method)hydra_poll,        "poll",       A_GIMME,  0);
  class_addmethod(c, (method)hydra_nopoll,      "nopoll",               0);
  
//  CLASS_ATTR_CHAR       (c, "automatic",    0, t_bitalino, automatic);
//  CLASS_ATTR_STYLE_LABEL(c, "automatic",    0, "onoff",
//                         "automatic frames polling");
//  CLASS_ATTR_ACCESSORS  (c, "automatic", bitalino_get_automatic, bitalino_set_automatic);
  
//  CLASS_ATTR_CHAR       (c, "continuous", 0, t_bitalino, continuous);
//  CLASS_ATTR_STYLE_LABEL(c, "continuous", 0, "onoff",
//                         "continuous output of values (if automatic enabled)");
  
//  CLASS_ATTR_DOUBLE     (c, "interval",   0, t_bitalino, poll_interval);
  
  class_register(CLASS_BOX, c);
  hydra_class = c;
  
  ps_list = gensym("list");

  post("hydra object loaded");
  hydra_busy = false;
  return 0;
}


//--------------------------------------------------------------------------

void *hydra_new(t_symbol *s, long argc, t_atom *argv)
{
  t_hydra *x;
  
  x = (t_hydra *)object_alloc(hydra_class);

  x->p_outlet = outlet_new(x, NULL);
  
  x->poll_interval = DEFAULT_POLL_INTERVAL;
  x->m_poll = clock_new((t_object *)x, (method)hydra_clock);
  
  x->connected = false;
  
  // if we have any atttributes :
  // attr_args_process(x, argc, argv);
  
  return(x);
}

void hydra_free(t_hydra *x)
{
//  // stop thread
//  hydra_stop(x);
//  
//  if (x->qelem)
//    qelem_free(x->qelem);
  
  // free out mutex
//  if (x->mutex)
//    systhread_mutex_free(x->mutex);
  
  object_free(x->m_poll);
}

//------------------------------------------------------------------------------

void hydra_assist(t_hydra *x, void *b, long m, long a, char *s)
{
  if (m == ASSIST_OUTLET) {
    sprintf(s,"list : <#base> <#controller> <positionX> <positionY> <positionZ> \
            <rotMatrix1> <rotMatrix2> <rotMatrix3> <rotMatrix4> <rotMatrix5> \
            <rotMatrix6> <rotMatrix7> <rotMatrix8> <rotMatrix9> \
            <quaternion1> <quaternion2> <quaternion3> <quaternion4> \
            <joystickX> <joystickY> <trigger> <buttonsState>");
  } else {
    switch (a) {
      case 0:
        sprintf(s,"connect / disconnect, poll <optional_ms_interval> / nopoll");
        break;
    }
  }
}

//------------------------------------------------------------------------------

void hydra_clock(t_hydra *x)
{
  clock_fdelay(x->m_poll, x->poll_interval);
  hydra_bang(x);
}

void hydra_bang(t_hydra *x)
{
  sixenseAllControllerData acd;
  int base, cont, total_cont = 0;
  std::vector<float> data;

  for (base = 0; base < sixenseGetMaxBases(); ++base) {
    sixenseSetActiveBase(base);
    sixenseGetAllNewestData(&acd);
    sixenseUtils::getTheControllerManager()->update(&acd);
    
    for (cont = 0; cont < sixenseGetMaxControllers(); ++cont) {
      
      if (sixenseIsControllerEnabled(cont)) {
        total_cont++;
        
        //LEFT OUTLET (position and buttons)
        data.clear();
        
        // base and controller identifiers
        data.push_back(base);
        data.push_back(cont);
        
        // position data
        data.push_back(acd.controllers[cont].pos[0]);
        data.push_back(acd.controllers[cont].pos[1]);
        data.push_back(acd.controllers[cont].pos[2]);
        
        // rotation data
        data.push_back(acd.controllers[cont].rot_mat[0][0]);
        data.push_back(acd.controllers[cont].rot_mat[0][1]);
        data.push_back(acd.controllers[cont].rot_mat[0][2]);
        data.push_back(acd.controllers[cont].rot_mat[1][0]);
        data.push_back(acd.controllers[cont].rot_mat[1][1]);
        data.push_back(acd.controllers[cont].rot_mat[1][2]);
        data.push_back(acd.controllers[cont].rot_mat[2][0]);
        data.push_back(acd.controllers[cont].rot_mat[2][1]);
        data.push_back(acd.controllers[cont].rot_mat[2][2]);
        
        // rotation data in quaternion
        data.push_back(acd.controllers[cont].rot_quat[0]);
        data.push_back(acd.controllers[cont].rot_quat[1]);
        data.push_back(acd.controllers[cont].rot_quat[2]);
        data.push_back(acd.controllers[cont].rot_quat[3]);
        
        // josystick
        data.push_back(acd.controllers[cont].joystick_x);
        data.push_back(acd.controllers[cont].joystick_y);
        
        // trigger
        data.push_back(acd.controllers[cont].trigger);
        
        // buttons
        data.push_back(acd.controllers[cont].buttons);
        
        // output list of controller states :
        t_atom outData[data.size()];
        
        for (int i = 0; i < data.size(); ++i) {
          /*
          t_atom a;
          a.a_type = A_FLOAT;
          a.a_w.w_float = data[i];
          outData[i] = a;
          //*/
          atom_setfloat(outData + i, data[i]);
        }
        
        outlet_list(x->p_ob.o_outlet,ps_list,data.size(),outData);
      }
    }
  }
  
  if (total_cont == 0) {
    // no bases connected
//    t_atom outList[1];
//    t_atom first;
//    first.a_type = A_SYMBOL;
//    first.a_w.w_symbol = gensym("hydra error : no base connected");
//    outList[0] = first;
//    outlet_list(t->x_obj.ob_outlet, &s_list, 1, outList);
  }

}

void on_hydra_connected(sixenseUtils::ControllerManager::setup_step step)
{
  
  if(sixenseUtils::getTheControllerManager()->isMenuVisible()) {
    post("menu visible");
    hydra_busy = true;
    post("hydra connected");
  }
}
  
void hydra_connect(t_hydra *x, t_symbol *s, long argc, t_atom *argv)
{
  if (!hydra_busy && !x->connected) {
    sixenseInit();
    sixenseUtils::getTheControllerManager()->setGameType(sixenseUtils::ControllerManager::ONE_PLAYER_TWO_CONTROLLER);
    //sixenseUtils::getTheControllerManager()->registerSetupCallback(on_hydra_connected);
    
    // after these calls the controllers will be tracked consistently in the full electromagnetic spherical field
    // but when sending the first poll message, they need to be above the base, otherwise values will be inverted
    
    for (int cont = 0; cont < sixenseGetMaxControllers(); ++cont) {
      sixenseAutoEnableHemisphereTracking(cont);
    }
    
    //x->connected = true;
    post("hydra initialized");
  }
}

void hydra_reset(t_hydra *x)
{
  for (int cont = 0; cont < sixenseGetMaxControllers(); ++cont) {
    sixenseAutoEnableHemisphereTracking(cont);
  }
}

void hydra_disconnect(t_hydra *x)
{
  hydra_nopoll(x);
  if (hydra_busy) {
    sixenseExit();
    hydra_busy = false;
  }
  x->connected = false;
}

void hydra_poll(t_hydra *x, t_symbol *s, long argc, t_atom *argv)
{
  if (argc > 0) {
    double tmpInterval = x->poll_interval;
    if (argv[0].a_type == A_FLOAT) {
      tmpInterval = argv[0].a_w.w_float;
    } else if (argv[0].a_type == A_LONG) {
      tmpInterval = argv[0].a_w.w_long;
    }
    
    if (tmpInterval <= 1) {
      hydra_nopoll(x);
      return;
    }
    
    x->poll_interval = tmpInterval;
  }

  clock_fdelay(x->m_poll, x->poll_interval);
  post("start polling hydra at interval : %f ms", x->poll_interval);
}

void hydra_nopoll(t_hydra *x)
{
  clock_unset(x->m_poll);
  post("stop polling hydra\n");
}

//======================= attribute getters / setters ========================//

/*
t_max_err bitalino_get_automatic(t_bitalino *x, t_object *attr,
                                 long *argc, t_atom **argv)
{
  if (argc && argv) {
    char alloc;
    if (atom_alloc(argc, argv, &alloc)) {
      return MAX_ERR_GENERIC;
    }
    atom_setchar_array(*argc, *argv, 1, &x->automatic);
    //post("automatic getter called");
  }
  return MAX_ERR_NONE;
}

t_max_err bitalino_set_automatic(t_bitalino *x, t_object *attr,
                                 long argc, t_atom *argv)
{
  if(argc && argv) {
    unsigned char prev = x->automatic;
    atom_getchar_array(argc, argv, 1, &x->automatic);
    if (x->automatic != prev) {
      if (x->automatic == 0) {
        //        bitalino_poll(x);
        //        post("polling enabled");
      } else {
        //        bitalino_nopoll(x);
        //        post("polling disabled");
      }
    }
    //post("automatic setter called  with value %ld", x->automatic);
  }
  return MAX_ERR_NONE;
}
//*/

