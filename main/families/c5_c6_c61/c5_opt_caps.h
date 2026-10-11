/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 ESPsoup contributors / Peter Holzhauser
 *
 * ESPsoup ESP32-C5 extensions: version, CAPS tokens and LIMITS? keys.
 * Included by receiver.c before burst_limits.h.  Every token names commands
 * that stock clients never send; existing commands and replies are unchanged.
 * See docs/espsoup-c5.md. */
#pragma once
#define FWOPT_VERSION "0.7.2"
#define CAP_MAX_WORDS 32704u   /* BIGCAP: two-half dump, see c5_engine.h */
#define C5_OPT_CAPS " BIGCAP CAPT TEMP FREQK FWOPT DDC SPECDB SWEEP STREAM CONT CSPEC CGRID CPKT TUNEM DEMODFM GFSKPKT DCCAL PARLIO VFMCOLOR"
#define LIMITS_EXTRA ",\"maxSamples\":32704" \
    ",\"stream\":{\"hw\":[8000000,10000000,20000000,4000000],\"maxRate\":500000,\"bits\":[8,16],\"block\":8192,\"gapped\":true,\"cont\":true}" \
    ",\"cspec\":{\"nfft\":[16,256],\"modes\":[\"mean\",\"max\",\"both\"]}" \
    ",\"ddc\":{\"src\":[80000000,40000000,20000000],\"bits\":[8,16]},\"spec\":{\"nfft\":[64,1024]},\"freqStepHz\":1000" \
    ",\"lockMHz\":[[2130,2730],[4790,5990]]"
