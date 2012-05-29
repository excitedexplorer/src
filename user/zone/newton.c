/*
 *  newton.c
 *  Program_1
 *
 *  Created by Yanadet Sripanich on 1/24/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */
#include <stdio.h>
#include <math.h>
#include <rsf.h>
#include "newton.h"

#ifndef _newton_h

typedef float (*func1)(float);
/* function pointer for float -> float */
/*^*/

#endif


float newton(func1 function /*f(x)*/,
			 func1 derivative /*f'(x)*/,
			 float x0 /*initial value*/,	
          	 int niter /*maximum number of iterations*/,
			 float tol /*tolerance (maximum |f(x)|)*/)
/*<run Newton's method for solving f(x)=0>*/
{
	float x;  /*current value*/
	float f;  /*function value*/
	float fp; /*function derivative value*/
	
	float x_back; /*value of the previous step*/
	float xtem; /*Temporary value of x*/
	float h; /*increment value*/
	float htem; /*Temporary value of h*/
	int iter; /*iteration counter*/
	int check; /*checking counter*/
	int n;
	const int nmax = 100;
	
	
	x=x_back=x0;
	h = function(x0)/derivative(x0);
	
	for (iter = 0; iter < niter; iter++) {
		f = function(x);
		fp = derivative(x);
		
		if (fabsf(f) < tol) { /*Break operation if the tolerance level is reached*/
			sf_warning("Tolerance level is reached.");
			break;
		}
		
		for (n=0; n<nmax && fabsf((f/fp)/h)>1; n++) { /* If the increment f/fp diverges, decrease the increment size by half*/
			h = 0.5*h;
			x = x_back - h;
			f = function(x);
			fp = derivative(x);
			sf_warning("Increment diverges-->decrease x to %g, f(x)=%g, fp(x)=%g",x,f,fp);
			
		}

		if (fabsf(f) < tol) { /*Break operation if the tolerance level is reached (Check again)*/
			sf_warning("Tolerance level is reached.");
			break;
		}
		
		h=f/fp;
		
		if (isnan(fp) != 0 || isinf(fp) != 0 || isnan(f) != 0 || isinf(f) != 0) { /*If the derivative and/or function values are '0' or 'infinity', we will reduce the size of increment*/
			xtem = x_back; /*Go back to the previous step*/
			f = function(xtem); /*Generate f of the previous step again*/
			fp = derivative(xtem); /*Generate fp of the previous step again*/
			h = f/fp; /*Generate the increment value of the previous step*/
			htem = sqrt(-1); /*Set the initial value of htem to get into the for loop*/
			for (check = 0; isnan(htem) != 0 || isinf(htem)!= 0;check++) { 
				if (check == 10) break;
				h *= 0.5; /*Reduce the size of increment by half each loop*/
				xtem -= h; /*Calculate x with the new increment*/
				htem = function(xtem)/derivative(xtem);
			}
			x = x_back; /*Take the value of x back to the previous step*/
			sf_warning("The derivative and/or function values is undefined."); 
			sf_warning("The number of times the increment size is halved = %d and the new increment = %g",check,h);
		}
		
 		x_back = x;
		
		/*Newton step*/
		x -= h;
		
		/*Print out output*/
		sf_warning("x=%8.3g x_back=%8.3g f(x_back)=%8.3g f'(x_back)=%8.3g ",x,x_back,f,fp);
	}
	
	return x;
}
