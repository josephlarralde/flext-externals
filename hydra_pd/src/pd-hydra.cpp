//
//  pd-hydra.c
//  pd-hydra
//
//  Created by Joseph Larralde on 12/11/16.
//  Copyright © 2016 IRCAM. All rights reserved.
//

#include <sixense.h>
#include <sixense_math.hpp>
#include <sixense_utils/derivatives.hpp>
#include <sixense_utils/button_states.hpp>
#include <sixense_utils/event_triggers.hpp>
#include <sixense_utils/controller_manager/controller_manager.hpp>

//================== C declarations =====================//

extern "C" {
#include "m_pd.h"

static t_class *hydra_class;
  
typedef struct _hydra {
  t_object x_obj;
  bool inited;
} t_hydra;
};

//================ end C declarations ===================//

void ext_hydra_init(t_hydra *t) {
  
  sixenseAllControllerData acd;
  int base, cont, total_cont = 0;
  std::vector<float> data;

  if (!t->inited)
  {
    sixenseInit();
    sixenseUtils::getTheControllerManager()->setGameType(sixenseUtils::ControllerManager::ONE_PLAYER_TWO_CONTROLLER);
    
    // after these calls the controllers will be tracked consistently in the full electromagnetic spherical field
    // but when sending the first poll message, they need to be above the base, otherwise values will be inverted
    
    for (cont = 0; cont < sixenseGetMaxControllers(); cont++) {
      sixenseAutoEnableHemisphereTracking(cont);
    }
    
    t->inited = true;
    post("hydra initialized");
  }
  
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
          t_atom a;
          a.a_type = A_FLOAT;
          a.a_w.w_float = data[i];
          outData[i] = a;
        }
        
        outlet_list(t->x_obj.ob_outlet,
                    &s_list,
                    static_cast<int>(data.size()),
                    outData);
      }
    }
  }
  
  if (total_cont == 0) {
    // no bases connected
    t_atom outList[1];
    t_atom first;
    first.a_type = A_SYMBOL;
    first.a_w.w_symbol = gensym("hydra error : no base connected");
    outList[0] = first;
    outlet_list(t->x_obj.ob_outlet, &s_list, 1, outList);
  }
}

//================== C declarations =====================//

extern "C" {

void hydra_bang(t_hydra *t) {
  //post("bang bang !");
  ext_hydra_init(t);
}

void *hydra_new(void) {
  t_hydra *x = (t_hydra *)pd_new(hydra_class);
  x->inited = false;
  
  outlet_new(&x->x_obj, &s_list);
  
  return (void *)x;
}

void hydra_setup(void) {
  hydra_class = class_new(gensym("hydra"),
                          (t_newmethod)hydra_new,
                          0, sizeof(t_hydra),
                          CLASS_DEFAULT, A_DEFFLOAT, 0);
  
  class_addbang(hydra_class, hydra_bang);
}

};
