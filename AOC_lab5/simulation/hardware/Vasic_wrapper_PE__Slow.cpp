// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vasic_wrapper.h for the primary calling header

#include "Vasic_wrapper__pch.h"
#include "Vasic_wrapper_PE.h"
#include "Vasic_wrapper__Syms.h"

void Vasic_wrapper_PE___ctor_var_reset(Vasic_wrapper_PE* vlSelf);

Vasic_wrapper_PE::Vasic_wrapper_PE(Vasic_wrapper__Syms* symsp, const char* v__name)
    : VerilatedModule{v__name}
    , vlSymsp{symsp}
 {
    // Reset structure values
    Vasic_wrapper_PE___ctor_var_reset(this);
}

void Vasic_wrapper_PE::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Vasic_wrapper_PE::~Vasic_wrapper_PE() {
}
