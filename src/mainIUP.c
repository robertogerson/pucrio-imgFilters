/**
 *   @file CGT1Img.c Trabalho de processamento de imagem com a IUP e OpenGL.
 *
 *   @author 
 *         - Marcelo Gattass
 *         - Roberto Azevedo
 *
 *   @date
 *         Criado em:           12 de Agosto de 2004
 *         Última Modificação:    01 de Maio de 2011
 *
 */
/*- Bibliotecas padrao usadas: --------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

/*- Inclusao das bibliotecas IUP e CD: ------------------------------------*/
#include <iup.h>                    /* interface das funcoes IUP */
#include <iupgl.h>                  /* interface das funcoes do canvas do OpenGL no IUP */
#include <iupcontrols.h>            /* interface para as funcoes de botoes de controle */ 

#ifdef WIN32
  #include <windows.h>                /* inclui as definicoes do windows para o OpenGL */
  #include <gl/gl.h>                  /* prototypes do OpenGL */
  #include <gl/glu.h>                 /* prototypes do OpenGL */
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif /*WIN32*/

#include "image.h"                  /* TAD para imagem */

/*- Contexto do Programa: -------------------------------------------------*/
Image   *image1, *image2, *undo;          /* imagens dos dois canvas */
Image   *imageTmp;                        /* imagem temporária, enquanto usuário 
                                             escolhe os parametros */
Ihandle *left_canvas, *right_canvas;      /* hadle dos dois canvas */
Ihandle *label;                           /* handle do label para colocar mensagens para usuario */
Ihandle *dialog;                          /* handle para o dialogo principal */

FILE *fpLog;            /* arquivo de log que registra as acoes do usuario */
extern char grupo[]; 
char pbuffer[200];

/*- Funcoes auxiliares ------------*/

/* Dialogo de selecao de arquivo  */
static char * get_file_name( void )
{
  Ihandle* getfile = IupFileDlg();
  char* filename = NULL;

  IupSetAttribute(getfile, IUP_TITLE, "Abertura de arquivo"  );
  IupSetAttribute(getfile, IUP_DIALOGTYPE, IUP_OPEN);
  IupSetAttribute(getfile, IUP_FILTER, "*.bmp;*.pfm");
  IupSetAttribute(getfile, IUP_FILTERINFO, "Arquivo de imagem (*.bmp ou *.hdr)");
  IupPopup(getfile, IUP_CENTER, IUP_CENTER);  /* o posicionamento nao esta sendo respeitado no Windows */

  filename = IupGetAttribute(getfile, IUP_VALUE);
  return filename;
}

static char * get_new_file_name( void )
{
  Ihandle* getfile = IupFileDlg();
  char* filename = NULL;

  IupSetAttribute(getfile, IUP_TITLE, "Salva arquivo"  );
  IupSetAttribute(getfile, IUP_DIALOGTYPE, IUP_SAVE);
  IupSetAttribute(getfile, IUP_FILTER, "*.bmp;*.pfm");
  IupSetAttribute(getfile, IUP_FILTERINFO, "Arquivo de imagem (*.bmp ou *.pfm)");
  IupPopup(getfile, IUP_CENTER, IUP_CENTER);  /* o posicionamento nao esta sendo respeitado no Windows */

  filename = IupGetAttribute(getfile, IUP_VALUE);
  return filename;
}


/* Ajusta o tamanho da janela do dialogo de acordo com o tamanho da imagem exibida */
static void update_dialog_size(void)
{
   if (image1) {
        int w,h;
      char buffer[512];
      w=imgGetWidth(image1);
      h=imgGetHeight(image1);
      sprintf(buffer,"%dx%d",w,h);
      IupSetAttribute(left_canvas, IUP_RASTERSIZE, buffer);
      IupSetAttribute(right_canvas, IUP_RASTERSIZE, buffer);
      IupSetAttribute(dialog, IUP_RASTERSIZE, NULL);
      IupShowXY(dialog, IUP_CENTER, IUP_CENTER);
   }
}
/*------------------------------------------*/
/* Callbacks do IUP.                        */
/*------------------------------------------*/


/* - Callback de mudanca de tamanho no canvas (mesma para ambos os canvas) */
int resize_cb(Ihandle *self, int width, int height)
{
 IupGLMakeCurrent(self);  /* torna o foco do OpenGL para este canvas */

 /* define a area do canvas que deve receber as primitivas do OpenGL */
 glViewport(0,0,width,height);

 /* transformacao de instanciacao dos objetos no sistema de coordenadas da camera */
 glMatrixMode(GL_MODELVIEW);
 glLoadIdentity();           /* identidade,  ou seja nada */

 /* transformacao de projecao */
 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 gluOrtho2D (0.0, (GLsizei)(width), 0.0, (GLsizei)(height));  /* ortografica no plano xy de [0,w]x[0,h] */

 return IUP_DEFAULT; /* retorna o controle para o gerenciador de eventos */
}

/* - Callback de repaint do canvas 1 */
int repaint_cb1(Ihandle *self)
{
  int w = imgGetWidth(image1);
  int h = imgGetHeight(image1);
  int type = imgGetDimColorSpace(image1);
  float* rgbData = imgGetData(image1);

  IupGLMakeCurrent(self);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2d(0.0,0.0);     /* define a origem da imagem */

  if (type==3)
     glDrawPixels (w, h, GL_RGB,GL_FLOAT, rgbData);
  else if (type==1) 
     glDrawPixels (w, h, GL_LUMINANCE,GL_FLOAT, rgbData);
  else 
     printf("Imagem de tipo desconhecida. Nao e' RGB nem Luminancia\n");

  glFlush();               /* forca atualizacao do OpenGL  (que pode ser bufferizado) */ 
  return IUP_DEFAULT; /* retorna o controle para o gerenciador de eventos */
}

/* - Callback de repaint do canvas 2 */
/* - poderia ser a mesma do 1, feita diferente para exemplificar OpenGL */
int repaint_cb2(Ihandle *self)
{
  int w = imgGetWidth(image2);
  int h = imgGetHeight(image2);
  int x,y;

  IupGLMakeCurrent(self);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glBegin(GL_POINTS);
    for (y=0;y<h;y++) {
      for (x=0; x<w; x++) {
         float rgb[3];
         imgGetPixel3fv(image2, x, y, rgb );
         glColor3fv(rgb);
         glVertex2i(x,y);
       }
    }
  glEnd();
  
  glFlush();
  return IUP_DEFAULT; /* retorna o controle para o gerenciador de eventos */
} 

int transf_cb(void)
{
   if (image2) imgDestroy(image2);
   image2=imgCopy(image1);
   repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
   IupSetfAttribute(label, "TITLE", " ");
   fprintf(fpLog,"TRANSF\n");
   return IUP_DEFAULT;
}

int undo_cb(void)
{
  Image* tmp = image2;
  image2 = undo;
  undo = tmp;
  repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  IupSetfAttribute(label, "TITLE", "Undo");
  fprintf(fpLog,"UNDO\n");
  return IUP_DEFAULT;
}


/* - Callback do botao que conta cor */
int conta_cb(Ihandle* self,int but, int pressed, int x, int y, char* status)
{
   int finish_time;
   double duration;
   int start_time; 
   int ncolors1,ncolors2;

   IupSetfAttribute(label, "TITLE", "Contando numero de cores distintas....por favor aguarde...");

   start_time  = clock(); 
   ncolors1 = imgCountColor(image1,1.f/256);
   ncolors2 = imgCountColor(image2,1.f/256);
   finish_time = clock();
   duration = (double)(finish_time - start_time)/CLOCKS_PER_SEC;
   IupSetfAttribute(label, "TITLE", "Numero de cores. Esq: %d  Dir:%d - Tempo: %2.1f", 
                                      ncolors1,ncolors2,duration);
   fprintf(fpLog,"Numero de cores. Esq: %d  Dir:%d - Tempo: %2.1f\n",ncolors1,ncolors2,duration);
   repaint_cb1(left_canvas);  /* redesenha o canvas 2 */
   repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
   return IUP_DEFAULT; /* retorna o controle para o gerenciador de eventos */
}


int gauss_cb(void)
{
   int ncolors2;
   int start_time; 
   int finish_time;
   double duration;
   IupSetfAttribute(label, "TITLE", "Aplicando o filtro de Gauss...por favor aguarde...");

    /* salva a imagem corrente para acao de undo */
    if (undo) imgDestroy(undo);
    undo = imgCopy(image2);

   start_time =  clock();
   imgGauss(image2,undo);
   finish_time = clock();
   
   duration = (double)(finish_time - start_time)/CLOCKS_PER_SEC;
   repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
   ncolors2= imgCountColor(image2,1.f/256);
   IupSetfAttribute(label, "TITLE", "Numero de cores: %d - tempo de processamento: %2.1f segundos", 
                                      ncolors2, duration);
   fprintf(fpLog,"GAUSS Numero de cores: %d - tempo de processamento: %2.1f segundos\n",ncolors2, duration);
   return IUP_DEFAULT;
}


int reduz_cb(void)
{
   int ncolors2;
   int start_time; 
   int finish_time;
   double duration;
   IupSetfAttribute(label, "TITLE", "Reduzindo o numero de cores...por favor aguarde...");

    /* salva a imagem corrente para acao de undo */
    if (undo) imgDestroy(undo);
    undo = imgCopy(image2);

   start_time =  clock();
   image2=imgReduceColors(undo,256);
   finish_time = clock();
   
   duration = (double)(finish_time - start_time)/CLOCKS_PER_SEC;
   repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
   ncolors2= imgCountColor(image2,1.f/256);
   IupSetfAttribute(label, "TITLE", "Numero de cores: %d - tempo de processamento: %2.1f segundos", 
                                      ncolors2, duration);
   fprintf(fpLog,"ReduzCores256 Numero de cores: %d - tempo de processamento: %2.1f segundos\n",ncolors2, duration);
   return IUP_DEFAULT;
}

int mediana_cb(void)
{
   int ncolors2;
   int start_time; 
   int finish_time;
   double duration;
   IupSetfAttribute(label, "TITLE", "Aplicando o filtro de Gauss...por favor aguarde...");

    /* salva a imagem corrente para acao de undo */
    if (undo) imgDestroy(undo);
    undo = imgCopy(image2);

   start_time =  clock();
   imgMedian(image2);
   finish_time = clock();
   
   duration = (double)(finish_time - start_time)/CLOCKS_PER_SEC;
   repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
   ncolors2= imgCountColor(image2,1.f/256);
   IupSetfAttribute(label, "TITLE", "Numero de cores: %d - tempo de processamento: %2.1f segundos", 
                                      ncolors2, duration);
   fprintf(fpLog,"MEDIANA Numero de cores: %d - tempo de processamento: %2.1f segundos\n",ncolors2, duration);
   return IUP_DEFAULT;
}

int arestas_cb(void)
{
   int start_time; 
   int finish_time;
   double duration;
   IupSetfAttribute(label, "TITLE", "Calculando as arestas da imagem...por favor aguarde...");

   /* salva a imagem corrente para acao de undo */
   if (undo) imgDestroy(undo);
   undo = image2;

   start_time =  clock();
   image2=imgEdges(image2);
   finish_time = clock();
   
   duration = (double)(finish_time - start_time)/CLOCKS_PER_SEC;
   repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
   IupSetfAttribute(label, "TITLE", "ARESTAS: tempo de processamento: %2.1f segundos", duration);
   fprintf(fpLog,"ARESTAS tempo de processamento: %2.1f segundos\n",duration);
   return IUP_DEFAULT;
}

int dif_cb(void)
{
  float avg;
  IupSetfAttribute(label, "TITLE", "Comparando imagens....por favor aguarde...");
  avg =  imgDif(image2,image1,2.f);

  IupSetfAttribute(label, "TITLE", "Diferenca corrigida com gama de 0.5. Valor medio dos pixels = %.3f", avg);
  fprintf(fpLog,"DIF Valor medio dos pixels = %.3f\n", avg);
  repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  return IUP_DEFAULT;
}

int gamma_cb(void)
{
  int w, h, i, x, y;
	float rgb[3], gamma=10.00;
  /* salva a imagem corrente para acao de undo */
  w = imgGetWidth(image1);
	h = imgGetHeight(image1);
  float rGamma[256], gGamma[256], bGamma[256];
  for(i=0; i<256; i++)
	{
	  rGamma[i]=pow(i/255.00, 1.0/gamma);
		if( rGamma[i] > 1.0) rGamma[i] = 1.0;
		if( gGamma[i] > 1.0) gGamma[i] = 1.0;
		if( bGamma[i] > 1.0) bGamma[i] = 1.0; 
	}

	for(y=0; y<h; y++)
	  for(x=0; x<w; x++)
		{
      imgGetPixel3fv(image2, x, y, rgb);
      rgb[0] = rGamma[(int)(floor(255*rgb[0]))];
      rgb[1] = gGamma[(int)(floor(255*rgb[1]))];
      rgb[2] = bGamma[(int)(floor(255*rgb[2]))];
      imgSetPixel3fv(image2, x, y, rgb);
		}

  repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  return IUP_DEFAULT;  
}

/* Callback used when the user change the twirl parameters */
int twirl_param_cb(Ihandle* dialog, int n, void* value)
{
  float xc, yc, angle, r;
	Ihandle* param = (Ihandle*)IupGetAttribute(dialog, "PARAM0");
	xc = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
	yc = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM2");
	angle = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM3");
	r = IupGetFloat(param, IUP_VALUE);

	angle = (2.0*M_PI)*(angle/360.0);

  if(image2) imgDestroy(image2);
  image2 = imgTwirl(imageTmp, xc, yc, angle, r);

	repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  return IUP_DEFAULT;
}

int twirl_cb(void)
{
  int w = imgGetWidth(image2);
	int h = imgGetHeight(image2);
  float xc, yc, angle, rad;

  xc = (double)w/2.0;
  yc = (double)h/2.0;
  angle = 70.0; //43 graus
  rad = sqrt((xc*xc)+(yc*yc));

  int n = sprintf (pbuffer, 
				     "Bt %%u[, Cancel]\n"
	           "xc: %%r[0,%d]\n"
						 "yc: %%r[0,%d]\n"
	           "angle: %%r[0,%d]\n"
						 "radius: %%r[0,%.2f]\n",
						 w, h, 360, sqrt(w*w+h*h) );

  imageTmp = imgCopy(image2);

  if (!IupGetParam("Twirl parameters", twirl_param_cb, 0,
     pbuffer, 
     &xc, &yc, &angle, &rad, 
     NULL))
  {
	    if(image2) imgDestroy(image2);

		  image2 = imageTmp;
  }
	else 
	{
    if (undo) imgDestroy(undo);
    
		undo = imageTmp;
		
		if(image2) imgDestroy(image2);

	  angle = (2.0*M_PI)*(angle/360.0); /* convert to radians */
    image2 = imgTwirl(imageTmp, xc, yc, angle, rad);
  }

	repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  return IUP_DEFAULT;
}

/* Callback used when the user change the sphere parameters */
int sphere_param_cb(Ihandle* dialog, int n, void* value)
{
  float xc, yc, rmax, refr;
	Ihandle* param = (Ihandle*)IupGetAttribute(dialog, "PARAM0");
	xc = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
	yc = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM2");
	rmax = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM3");
	refr = IupGetFloat(param, IUP_VALUE);

  if(image2) imgDestroy(image2);
  image2 = imgSphere(imageTmp, xc, yc, rmax, refr);

	repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  return IUP_DEFAULT;
}

int sphere_cb(void)
{
  int w = imgGetWidth(image1);
  int h = imgGetHeight(image1);
  float xc, yc, rmax, refr;

  xc = (float)w/2.0;
  yc = (float)h/2.0;
  rmax = xc/2.0;
  refr = 1.5;

  int n = sprintf (pbuffer, 
				     "Bt %%u[, Cancel]\n"
	           "xc: %%r[0,%d]\n"
						 "yc: %%r[0,%d]\n"
	           "rmax: %%r[0,%d]\n"
						 "refr: %%r[1.0,%.2f]\n",
						 w, h, (int)sqrt(w*w+h*h), 3.0);

  imageTmp = imgCopy(image2);

  if (!IupGetParam("Sphere parameters", sphere_param_cb, 0,
     pbuffer, 
     &xc, &yc, &rmax, &refr, 
     NULL))
  {
	    if(image2) imgDestroy(image2);

		  image2 = imageTmp;
  }
	else 
	{
    if (undo) imgDestroy(undo);
    
		undo = imageTmp;
		
		if(image2) imgDestroy(image2);

    image2 = imgSphere(imageTmp, xc, yc, rmax, refr);
  }

  repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  return IUP_DEFAULT;

}


/* Callback used when the user change the perspective parameters */
int perspective_param_cb(Ihandle* dialog, int n, void* value)
{
  float x0, y0, x1, y1, x2, y2, x3, y3;
	int ewa_filter = 0;
	Ihandle* param = (Ihandle*)IupGetAttribute(dialog, "PARAM0");
	x0 = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
	y0 = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM2");
	x1 = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM3");
	y1 = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM4");
	x2 = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM5");
	y2 = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM6");
	x3 = IupGetFloat(param, IUP_VALUE);
  param = (Ihandle*)IupGetAttribute(dialog, "PARAM7");
	y3 = IupGetFloat(param, IUP_VALUE);

  param = (Ihandle*)IupGetAttribute(dialog, "PARAM8");
  ewa_filter = IupGetFloat(param, IUP_VALUE);

  if(image2) imgDestroy(image2);

	if(ewa_filter){
    image2 = imgPerspective( imageTmp, x0, y0, x1, y1, x2, y2, x3, y3, 
	                           IMG_EWA_FILTER);
  }
	else
	{
    image2 = imgPerspective(imageTmp, x0, y0, x1, y1, x2, y2, x3, y3, 
	                        IMG_NO_FILTER);
	}

	repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  return IUP_DEFAULT;
}


int perspective_cb(void)
{ 
	int w = imgGetWidth(image1);
	int h = imgGetHeight(image1);
  float x0 = 0.0, y0 = 0.0;
  float x1 = w, y1 = 0.0;
  float x2 = 3*w/4, y2 = 2*h/3;
  float x3 = w/4, y3 = 2*h/3;
	int use_ewa = 0;

  int n = sprintf (pbuffer, 
				     "Bt %%u[, Cancel]\n"
	           "x0: %%r[0,%d]\n"
						 "y0: %%r[0,%d]\n"
	           "x1: %%r[0,%d]\n"
						 "y1: %%r[0,%d]\n" 
	           "x2: %%r[0,%d]\n"
						 "y2: %%r[0,%d]\n"
	           "x3: %%r[0,%d]\n"
						 "y3: %%r[0,%d]\n"
						 "Use EWA: %%b[No,Yes]\n",
						 w, h, w, h, w, h, w, h);

  imageTmp = imgCopy(image2);

  if (!IupGetParam("Perspective points", perspective_param_cb, 0,
     pbuffer, 
     &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3, &use_ewa,
     NULL))
  {
	    if(image2) imgDestroy(image2);

		  image2 = imageTmp;
  }
	else 
	{
    if (undo) imgDestroy(undo);
    
		undo = imageTmp;
		
		if(image2) imgDestroy(image2);
    if (use_ewa)
		{
      image2 = imgPerspective(imageTmp, x0, y0, x1, y1, x2, y2, x3, y3,
		                        IMG_EWA_FILTER);
		}
		else
		{
      image2 = imgPerspective(imageTmp, x0, y0, x1, y1, x2, y2, x3, y3,
		                        IMG_NO_FILTER);
    }
  }
  
	repaint_cb2(right_canvas);  /* redesenha o canvas 2 */
  return IUP_DEFAULT;
}

int save_cb(Ihandle *self)
{
  char* filename = get_new_file_name( );  /* chama o dialogo de abertura de arquivo */
  if (filename==NULL) return 0;
 if (strstr(filename,".bmp")||strstr(filename,".BMP"))
   imgWriteBMP(filename,image2);
 else 
   imgWritePFM(filename,image2);

  fprintf(fpLog,"SAVE %s\n",filename);
  return IUP_DEFAULT;
}


int load_cb(void) {
  char* filename = get_file_name();  /* chama o dialogo de abertura de arquivo */

  if (filename==NULL) return 0;

  if (strstr(filename,".bmp")||strstr(filename,".BMP")){
      /* le nova imagem */
      if (image1) imgDestroy(image1);
      image1 = imgReadBMP(filename);
      if (image2) imgDestroy(image2);
      image2 = imgCopy(image1);
      }
  else if (strstr(filename,".pfm")||strstr(filename,".PFM")){
       /* le nova imagem */
      if (image1) imgDestroy(image1);
      image1 = imgReadPFM(filename);
      if (image2) imgDestroy(image2);
      image2 = imgCopy(image1);
      }
  else
      IupMessage("Aviso","Formato do arquivo de imagem não reconhecido\n");
 

  IupSetFunction("repaint_cb1", (Icallback) repaint_cb1);
  IupSetFunction("repaint_cb2", (Icallback) repaint_cb2 );

  update_dialog_size( );
  repaint_cb1(left_canvas);  /* redesenha o canvas 2 */
  repaint_cb2(right_canvas);  /* redesenha o canvas 2 */

  IupSetfAttribute( label, "TITLE", "%3dx%3d", imgGetWidth(image1), 
	                  imgGetHeight(image1));

  IupSetFunction("transf_cb", (Icallback)transf_cb);
  IupSetFunction("undo_cb", (Icallback)undo_cb);
  IupSetFunction("conta_cb", (Icallback)conta_cb); 
  IupSetFunction("gauss_cb", (Icallback)gauss_cb);
  IupSetFunction("reduz_cb", (Icallback)reduz_cb);
  IupSetFunction("mediana_cb", (Icallback)mediana_cb);
  IupSetFunction("arestas_cb", (Icallback)arestas_cb);
  IupSetFunction("dif_cb", (Icallback)dif_cb);
  IupSetFunction("gamma_cb", (Icallback)gamma_cb);

  //geometric transform
  IupSetFunction("twirl_cb", (Icallback)twirl_cb);
  IupSetFunction("sphere_cb", (Icallback)sphere_cb);
  IupSetFunction("perspective_cb", (Icallback)perspective_cb);

  IupSetFunction("save_cb", (Icallback)save_cb);
  fprintf(fpLog,"OPEN %s\n",filename);

  return IUP_DEFAULT;

}


/*-------------------------------------------------------------------------*/
/* Incializa os controles geometricos do                                   */
/*-------------------------------------------------------------------------*/
Ihandle *init_geometric_controls(void)
{
  Ihandle *frm_geom;
  Ihandle *twirl, *sphere, *perspective, *water;
	Ihandle *interpolation, *none, *bilinear, *linear, *nearest_neighbor;

  //Geometric operations
  twirl = IupButton("Twirl", "twirl_cb");
  sphere = IupButton("Sphere", "sphere_cb");
  water = IupButton("Water", "water_cb");
  perspective = IupButton("Perspective", "perspective_cb");

  none = IupToggle ("None", "");
  linear = IupToggle ("Linear", "");
  bilinear = IupToggle ("Bilinear", "");
  nearest_neighbor = IupToggle ("Nearest Neighbor", "");

	interpolation = IupRadio (
	  IupVbox(
		  none,
		  linear,
			bilinear,
			nearest_neighbor
    )
	);

	frm_geom = IupFrame(
	  IupHbox(
		    twirl,
			  sphere,
				perspective,
			NULL)
  );
	IupSetAttribute(frm_geom, "TITLE", "Geometric");

	return frm_geom;

}

Ihandle *init_filter_controls()
{
  Ihandle *frm_filters;
  Ihandle *conta, *reduz, *gauss, *mediana, *arestas, *dif, *gamma;

  conta = IupButton("nCores", "conta_cb");
  reduz = IupButton("Reduz","reduz_cb");
  gauss = IupButton("Gauss","gauss_cb");
  mediana = IupButton("Mediana","mediana_cb");
  arestas = IupButton("Arestas", "arestas_cb");
  dif = IupButton("Dif.", "dif_cb");
  gamma = IupButton("Gamma", "gamma_cb");

  frm_filters = IupFrame(
	  IupHbox(
		  conta,
			reduz,
			gauss,
			mediana,
			arestas,
			dif,
			gamma,
			NULL
		)
	);
  IupSetAttribute(frm_filters, "TITLE", "Filters");

  return frm_filters;
}

/*-------------------------------------------------------------------------*/
/* Incializa o programa.                                                   */
/*-------------------------------------------------------------------------*/
int init(void)
{
  Ihandle *toolbar, *statusbar, *box;
  Ihandle *load, *transf, *undo, *save;

  time_t now = time(NULL);  /* obtem a data corrente para registro no arquivo de log */

  /* creates the toolbar and its buttons */
  load = IupButton("", "load_cb");
  IupSetAttribute(load,"TIP","Carrega uma imagem.");
  IupSetAttribute(load,"IMAGE","IUP_FileOpen");
  IupSetFunction("load_cb", (Icallback)load_cb);
 
  transf = IupButton("", "transf_cb");
  IupSetAttribute(transf,"IMAGE","IUP_ArrowRight");
  IupSetAttribute(transf,"TIP","Transfere para o outro canvas.");

  undo = IupButton("","undo_cb"),
  IupSetAttribute(undo,"TIP","Desfaz processamento (qdo possivel).");
  IupSetAttribute(undo,"IMAGE","IUP_EditUndo");

  save = IupButton("", "save_cb");
  IupSetAttribute(save,"TIP","Salva imagem processada.");
  IupSetAttribute(save,"IMAGE","IUP_FileSave");

  toolbar = IupHbox(
      load, 
      transf,
      IupFill(),
	  	init_filter_controls(),
			init_geometric_controls(),
      IupFill(),
      undo,
      save,
      NULL);

  IupSetAttribute(toolbar, "FONT", "HELVETICA_NORMAL_8");
  IupSetAttribute(toolbar, "ALIGNMENT", "ACENTER");
 

  /* cria dois canvas */
  left_canvas = IupGLCanvas("repaint_cb1"); 
  IupSetAttribute(left_canvas,IUP_RASTERSIZE,"320x240");
  IupSetAttribute(left_canvas, "RESIZE_CB", "resize_cb");

  right_canvas = IupGLCanvas("repaint_cb2"); 
  IupSetAttribute(right_canvas,IUP_RASTERSIZE,"320x240");
  IupSetAttribute(right_canvas, "RESIZE_CB", "resize_cb");

  /* associa o evento de repaint a funccao repaint_cb */
  IupSetFunction("repaint_cb1", NULL);
  IupSetFunction("repaint_cb2", NULL );
  IupSetFunction("resize_cb", (Icallback) resize_cb);

  /* the status bar is just a label to put some usefull information in run time */
  label = IupLabel("status");
  IupSetAttribute(label, "EXPAND", "HORIZONTAL");
  IupSetAttribute(label, "FONT", "HELVETICA_NORMAL_8");
  statusbar = IupSetAttributes(IupHbox(
                IupFrame(IupHbox(label, NULL)), 
                NULL), "MARGIN=5x5");

  /* this is the most external box that puts together
     the toolbar, the two canvas and the status bar */
  box = IupVbox(
          toolbar,
          IupSetAttributes(IupHbox(
            left_canvas, 
            right_canvas, 
            NULL), "GAP=5"),
          statusbar, 
          NULL);

  /* create the dialog and set its attributes */
  dialog = IupDialog(box);
  IupSetAttribute(dialog, "CLOSE_CB", "app_exit_cb");
  IupSetAttribute(dialog, "TITLE", "FCG:Trabalho Processamento de Imagens");

  IupShowXY(dialog, IUP_CENTER, IUP_CENTER);

  fpLog = fopen("inf1761T1.log","at");
  fprintf(fpLog,"****************************************************************\n");
  fprintf(fpLog,"%s\n",grupo);
  fprintf(fpLog,"Data: %s\n",ctime(&now));
  
  return 1;
}

/*-------------------------------------------------------------------------*/
/* Rotina principal.                                                       */
/*-------------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    IupOpen(&argc,&argv);
    IupGLCanvasOpen();
    IupImageLibOpen(); 
    if ( init() ) 
      IupMainLoop();
    IupClose();
}
