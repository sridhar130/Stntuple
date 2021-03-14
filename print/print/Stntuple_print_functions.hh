//////////////////////////////////////////////////////////////////////////////
#ifndef __Stntuple_print_functions_hh__
#define __Stntuple_print_functions_hh__

#ifndef __CINT__

#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#else

namespace art {
  class Event;
}

#endif

void     print_combo_hit_coll  (const char* ComboHitCollTag, const char* StrawHitCollTag = "makeSH");
void     print_combo_hit_coll_2(const char* ComboHitCollTag, const char* StrawHitCollTag = "makeSH");

void     print_sd_colls        ();
void     print_sdmc_colls      ();
void     print_kalrep_colls    ();
void     print_kalseed_colls   ();
void     print_helix_seed_colls();

#endif