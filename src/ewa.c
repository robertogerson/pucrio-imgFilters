#include "ewa.h"


void ewaJacobianCoeficients( double x, double y, double *H, 
                             double *Ux, double *Uy, double *Vx, double *Vy)
{
  /* 
	 Considering H = [a d g]
	                 [b e h]
								   [c f i]
  
	 Calculating the parcial derivates, for Jacobian we should have as result:
     dU/dx = Ux = ( y(ah-bg) + ai - gc ) / (gx+hy+i)^2
     dU/dy = Uy = ( x(bg-ah) + bi - hc ) / (gx+hy+i)^2
     dV/dx = Vx = ( y(dh-eg) + di - gf ) / (gx+hy+i)^2
     dV/dx = Vy = ( y(eg-dh) + ei - hf ) / (gx+hy+i)^2

		 By simplication:
		 DENOM = (gx + hy + i)^2;
  */
	double DENOM = H[6]*x + H[7] * y + H[8];
  DENOM *= DENOM;

  /*
	  and A = ah-bg, and B = dh-eg
	*/
	double A = H[0]*H[7] - H[1]*H[6];
	double B = H[3]*H[7] - H[4]*H[6];

	/* resulting: */ 
	*Ux = (    A*y + (H[0]*H[8] - H[6]*H[2]) ) / DENOM;
	*Uy = ( (-A)*x + (H[1]*H[8] - H[7]*H[2]) ) / DENOM;
	*Vx = (    B*y + (H[3]*H[8] - H[6]*H[5]) ) / DENOM;
	*Vy = ( (-B)*y + (H[4]*H[8] - H[7]*H[5]) ) / DENOM;
}


void ewaEllipseCoeficients(double Ux, double Uy, double Vx, double Vy,
												  double *A, double *B, double *C, double *F)
{

  *A = Vx*Vx + Vy*Vy;
	*B = (-2)*(Ux*Vx + Uy*Vy);
  *C = Ux*Ux + Uy*Uy;
	*F = pow((Ux*Vy - Uy*Vx), 2);

	//TODO: Scale A, B, C, and F equally so THAT F = WTAB length
}

/*
void ewaEllipseCenter()
{

}

void ewaEllipseBoundingBox()
{

}

void ewaCalcColor()
{

}

Image* ewaApplyFilter(Image *img)
{
 return 0;
}
*/
