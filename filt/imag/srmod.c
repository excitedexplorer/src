/* 3-D shot-record modeling using extended split-step */
/*
  Copyright (C) 2004 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <math.h>

#include <rsf.h>
/*^*/

#include "srmod.h"
#include "taper.h"
#include "slowref.h"
#include "ssr.h"

#include "slice.h"
/*^*/

#define LOOP(a) for( iy=0; iy< ay.n; iy++){ \
                for( ix=0; ix< ax.n; ix++){ {a} }}
#define SOOP(a) for(ily=0;ily<aly.n;ily++){ \
                for(ilx=0;ilx<alx.n;ilx++){ {a} }}

static axa az,aw,alx,aly,ax,ay,ae;
static bool verb;
static float eps;

static float         **rr;
static float complex **ww_s;
static float complex **ww_r;

static float         **ss; /* slowness */
static float         **so; /* slowness */

static int            *nr_s, *nr_r, *nr; /* number of refs */
static float         **sm_s,**sm_r,**sm; /* ref slo squared */
static fslice        slow_s,slow_r,slow; /* slowness slice */
static fslice        wtmp;

void srmod_init(bool verb_,
		float eps_,
		float dtmax,
		axa ae_        /* experiments (e.g. shots) */,
		axa az_        /* depth */,
		axa aw_        /* frequency */,
		axa ax_        /* i-line (data) */,
		axa ay_        /* x-line (data) */,
		axa alx_       /* i-line (slowness/image) */,
		axa aly_       /* x-line (slowness/image) */,
		int tx, int ty /* taper size */,
		int px, int py /* padding in the k domain */
    )
/*< initialize S/R modeling >*/
{
    float ds;

    verb=verb_;
    eps = eps_;
    
    az = az_;
    aw = aw_;
    ax = ax_;
    ay = ay_;
    alx= alx_;
    aly= aly_;
    ae = ae_;

    /* from hertz to radian */
    aw.d *= 2.*SF_PI; 
    aw.o *= 2.*SF_PI;

    ds  = dtmax/az.d;

    /* SSR */
    ssr_init(az_ ,
	     ax_ ,ay_,
	     alx_,aly_,
	     px  ,py,
	     tx  ,ty,
	     ds);

    /* precompute taper */
    taper2_init(ay.n,
		ax.n,
		SF_MIN(ty,ay.n-1),
		SF_MIN(tx,ax.n-1),
		true,
		true);

    /* compute reference slowness */
    ss = sf_floatalloc2(alx.n,aly.n); /* slowness */
    so = sf_floatalloc2(alx.n,aly.n); /* slowness */

    /* allocate wavefield storage */
    ww_s = sf_complexalloc2(ax.n,ay.n);
    ww_r = sf_complexalloc2(ax.n,ay.n);

    /*< allocate reflectivity storage >*/
    rr = sf_floatalloc2(ax.n,ay.n);

    wtmp  = fslice_init( ax.n* ay.n, az.n,sizeof(float complex));
}

void srmod_pw_init( float dtmax,
		    int   nrmax      /* maximum number of references */,
		    fslice slow_)
/*< initialize P-wave S/R modeling >*/
{
    int   iz, jj;
    float ds;

    ds  = dtmax/az.d;

    sm= sf_floatalloc2 (nrmax,az.n); /* ref slowness squared*/
    nr= sf_intalloc          (az.n); /* nr of ref slownesses */
    slow= slow_;
    for (iz=0; iz<az.n; iz++) {
	fslice_get(slow,iz,ss[0]);
	
	nr[iz] = slowref(nrmax,ds,alx.n*aly.n,ss[0],sm[iz]);
	if (verb) sf_warning("nr[%d]=%d",iz,nr[iz]);
    }
    for (iz=0; iz<az.n-1; iz++) {
	for (jj=0; jj<nr[iz]; jj++) {
	    sm[iz][jj] = 0.5*(sm[iz][jj]+sm[iz+1][jj]);
	}
    }
}

void srmod_cw_init( float dtmax,
		    int   nrmax      /* maximum number of references */,
		    fslice slow_s_,
		    fslice slow_r_)
/*< initialize C-wave S/R modeling >*/
{
    int   iz, jj;
    float ds;

    ds  = dtmax/az.d;

    /*------------------------------------------------------------*/
    /* slowness: downgoing wavefield */
    sm_s= sf_floatalloc2 (nrmax,az.n); /* ref slowness squared*/
    nr_s= sf_intalloc          (az.n); /* nr of ref slownesses */
    slow_s= slow_s_;
    for (iz=0; iz<az.n; iz++) {
	fslice_get(slow_s,iz,ss[0]);
	
	nr_s[iz] = slowref(nrmax,ds,alx.n*aly.n,ss[0],sm_s[iz]);
	if (verb) sf_warning("nr_s[%d]=%d",iz,nr_s[iz]);
    }
    for (iz=0; iz<az.n-1; iz++) {
	for (jj=0; jj<nr_s[iz]; jj++) {
	    sm_s[iz][jj] = 0.5*(sm_s[iz][jj]+sm_s[iz+1][jj]);
	}
    }
    /*------------------------------------------------------------*/
    /* slowness: up-going wavefield */
    sm_r= sf_floatalloc2 (nrmax,az.n); /* ref slowness squared*/
    nr_r= sf_intalloc          (az.n); /* nr of ref slownesses */
    slow_r= slow_r_;
    for (iz=0; iz<az.n; iz++) {
	fslice_get(slow_r,iz,ss[0]);
	
	nr_r[iz] = slowref(nrmax,ds,alx.n*aly.n,ss[0],sm_r[iz]);
	if (verb) sf_warning("nr_r[%d]=%d",iz,nr_r[iz]);
    }
    for (iz=0; iz<az.n-1; iz++) {
	for (jj=0; jj<nr_r[iz]; jj++) {
	    sm_r[iz][jj] = 0.5*(sm_r[iz][jj]+sm_r[iz+1][jj]);
	}
    }
    /*------------------------------------------------------------*/
}


/*------------------------------------------------------------*/
void srmod_pw_close(void)
/*< free P-wave slowness storage >*/
{
    free( *sm); free( sm);
    ;           free( nr);
}

void srmod_cw_close(void)
/*< free C-wave slowness storage >*/
{
    free( *sm_s); free( sm_s);
    ;             free( nr_s);
    free( *sm_r); free( sm_r);
    ;             free( nr_r);
}

void srmod_close(void)
/*< free allocated storage >*/
{
    ssr_close();
    
    free( *ww_s); free( ww_s);
    free( *ww_r); free( ww_r);

    free( *ss); free( ss);
    free( *so); free( so);

    free( *rr); free( rr);

    fslice_close(wtmp);
}

/*------------------------------------------------------------*/
void srmod_pw(fslice dwfl /* source   data [nw][ny][nx] */,
	      fslice uwfl /* receiver data [nw][ny][nx] */,
	      fslice refl
    )
/*< Apply P-wave S/R modeling >*/
{
    int iz,iw,ix,iy,ilx,ily;
    float complex w;
    
    for (iw=0; iw<aw.n; iw++) {
	if(verb) sf_warning("iw=%3d of %3d",iw+1,aw.n);
	w = eps*aw.d + I*(aw.o+iw*aw.d);

	/* downgoing wavefield */
	fslice_get(dwfl,iw,ww_s[0]); taper2(ww_s);
	fslice_put(wtmp, 0,ww_s[0]);

	fslice_get(slow,0,so[0]);
	for (iz=0; iz<az.n-1; iz++) {
	    fslice_get(slow,iz+1,ss[0]);

	    ssr_ssf(w,ww_s,so,ss,nr[iz],sm[iz]);
	    SOOP( so[ily][ilx] = ss[ily][ilx]; );

	    fslice_put(wtmp,iz+1,ww_s[0]);
	}
	
	/* upgoing wavefield */
	LOOP( ww_r[iy][ix] = 0; );

	fslice_get(slow,az.n-1,so[0]);
	for (iz=az.n-1; iz>0; iz--) {
	    fslice_get(slow,iz-1,ss[0]);

	    fslice_get(wtmp,iz,ww_s[0]); 
	    fslice_get(refl,iz,rr[0]); /* reflectivity */

	    LOOP( ww_s[iy][ix] *= rr  [iy][ix]; );
	    LOOP( ww_r[iy][ix] += ww_s[iy][ix]; );

	    ssr_ssf(w,ww_r,so,ss,nr[iz],sm[iz]);
	    SOOP( so[ily][ilx] = ss[ily][ilx]; );
	}
	fslice_put(uwfl,iw,ww_r[0]);
    } /* iw */
}

/*------------------------------------------------------------*/

void srmod_cw(fslice dwfl /* source   data [nw][ny][nx] */,
	      fslice uwfl /* receiver data [nw][ny][nx] */,
	      fslice refl
    )
/*< Apply C-wave S/R modeling >*/
{
    int iz,iw,ix,iy,ilx,ily;
    float complex w;
    
    for (iw=0; iw<aw.n; iw++) {
	if(verb) sf_warning("iw=%3d of %3d",iw+1,aw.n);
	w = eps*aw.d + I*(aw.o+iw*aw.d);

	/* downgoing wavefield */
	fslice_get(dwfl,iw,ww_s[0]); taper2(ww_s);

	fslice_put(wtmp,0,ww_s[0]);

	fslice_get(slow_s,0,so[0]);
	for (iz=0; iz<az.n-1; iz++) {
	    fslice_get(slow_s,iz+1,ss[0]);

	    ssr_ssf(w,ww_s,so,ss,nr_s[iz],sm_s[iz]);
	    SOOP( so[ily][ilx] = ss[ily][ilx]; );

	    fslice_put(wtmp,iz+1,ww_s[0]);
	}
	
	/* upgoing wavefield */
	LOOP( ww_r[iy][ix] = 0; );

	fslice_get(slow_r,az.n-1,so[0]);
	for (iz=az.n-1; iz>0; iz--) {
	    fslice_get(slow_r,iz-1,ss[0]);

	    fslice_get(wtmp,iz,ww_s[0]); 
	    fslice_get(refl,iz,rr[0]); /* reflectivity */
	    LOOP( ww_s[iy][ix] *= rr  [iy][ix]; );
	    LOOP( ww_r[iy][ix] += ww_s[iy][ix]; );

	    ssr_ssf(w,ww_r,so,ss,nr_r[iz],sm_r[iz]);
	    SOOP( so[ily][ilx] = ss[ily][ilx]; );
	}
	fslice_put(uwfl,iw,ww_r[0]);
    } /* iw */
}
